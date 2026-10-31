/* Prospero Hi-Fi native AGC SDL2 renderer.
 * Copyright (C) 2026 Anime0t4ku
 * GPL-3.0-or-later. AGC setup adapted from Rufidj/ps5link-sdk (GPL-3.0),
 * SvenGDK/SharpProspero (GPL-3.0) and Mihawk-99/PS5_Vulkan (GPL-3.0-or-later).
 * This is an altered, application-specific addition to the PS5 SDL port.
 * GPU draws the complete SDL interface; no CPU framebuffer raster/upload path.
 */
#ifdef PROSPERO_AGC_HOST_TEST
#include "agc_test_sdl.h"
#else
#include "../../SDL_internal.h"
#endif
#ifdef SDL_VIDEO_DRIVER_PS5
#ifndef PROSPERO_AGC_HOST_TEST
#include "../SDL_sysrender.h"
#include "SDL_timer.h"
#include "../../video/ps5/SDL_ps5video.h"
#endif
#include <string.h>
#include "mesh_vs_sb.h"
#include "textured_ps_sb.h"
#include <stdint.h>
#include <stddef.h>

extern int sceKernelUsleep(unsigned);
extern int sceKernelMunmap(void *,size_t);
extern int sceAgcInit(void *,unsigned);
extern void *sceAgcGetRegisterDefaults(void);
extern int sceAgcCreateShader(void **,void *,void *);
extern int sceAgcLinkShaders(void *,void *,void *,void *,void *,unsigned);
extern void *sceAgcDcbSetCxRegistersIndirect(void *,const void *,unsigned);
extern void *sceAgcDcbSetShRegistersIndirect(void *,const void *,unsigned);
extern void *sceAgcDcbSetUcRegistersIndirect(void *,const void *,unsigned);
extern void *sceAgcCbSetShRegisterRangeDirect(void *,unsigned,const unsigned *,unsigned);
extern void *sceAgcDcbDrawIndexAuto(void *,unsigned,uint64_t);
extern void *sceAgcDcbSetFlip(void *,unsigned,int,unsigned,int64_t);
extern int sceAgcDriverSubmitDcb(void *);
extern int sceAgcSuspendPoint(void);
extern unsigned sceAgcDriverGetWaitRenderingPacketSizeInDwords(void);
extern unsigned sceAgcDriverWaitUntilSafeForRendering(unsigned **,unsigned,unsigned,unsigned,int);
extern void *sceAgcCbReleaseMem(void *,uint8_t,int16_t,uint64_t,int8_t,void *,unsigned,uint64_t,uint16_t,uint16_t,int8_t,int);
extern SDL_RenderDriver PS5AGC_RenderDriver;

typedef struct { Uint16 offset,pad;Uint32 value; } Reg;
typedef struct { Uint32 *bottom,*top,*up,*down;uintptr_t callback;void *user;Uint32 reserved,pad; } Dcb;
typedef struct { void *ptr;intptr_t physical;size_t bytes; } Memory;
typedef struct { float x,y,z,u,v,nz,unused_u,unused_v;Uint32 color; } Vertex;
typedef struct { Memory memory;Uint32 desc[8];int pitch;SDL_ScaleMode filter; } Texture;
#define VERTEX_CAPACITY 262144u
#define COMMAND_WORDS 131072u
#define TABLE_BYTES 0x100000u
#define MAX_STATE_REGS 256
#define SH_CX(h) (*(Reg **)((Uint8 *)(h)+24))
#define SH_SH(h) (*(Reg **)((Uint8 *)(h)+32))
#define SH_CX_COUNT(h) (((Uint8 *)(h))[91])
#define SH_SH_COUNT(h) (((Uint8 *)(h))[92])
typedef struct {
    Memory words,vertices,tables,marker;
    Dcb command;
    size_t vertex_count,table_used;
    Uint32 sequence;
    SDL_bool submitted,started,pending;
} Frame;
typedef struct {
    PS5_DeviceData *video;
    Frame frames[2];unsigned slot;Uint32 sequence;
    int width,height;Uint64 agc_state;
    Memory shader_mem[4];void *vs,*ps;
    Memory constants,white;
    Uint32 white_desc[8];
    Reg state[MAX_STATE_REGS],sh[64],primitive[3];unsigned state_count,sh_count;
    int cb_slot,vb_slot;SDL_Rect viewport,clip;SDL_bool clipping;
    SDL_bool failed;
} Renderer;

static int allocation(Memory *m,size_t bytes){
    m->bytes=(bytes+0x3fff)&~(size_t)0x3fff;m->physical=-1;m->ptr=NULL;
    if(sceKernelAllocateMainDirectMemory(m->bytes,0x4000,12,&m->physical)!=0)return SDL_SetError("AGC direct allocation failed (%lu bytes)",(unsigned long)m->bytes);
    if(sceKernelMapDirectMemory(&m->ptr,m->bytes,0x33,0,m->physical,0x4000)!=0){sceKernelReleaseDirectMemory(m->physical,m->bytes);m->physical=-1;return SDL_SetError("AGC direct mapping failed");}
    SDL_memset(m->ptr,0,m->bytes);return 0;
}
static void release(Memory *m){if(m->ptr){sceKernelMunmap(m->ptr,m->bytes);sceKernelReleaseDirectMemory(m->physical,m->bytes);m->ptr=NULL;}}
static Uint8 overflow(Dcb *d,unsigned size,void *user){(void)d;(void)size;(void)user;return 0;}
static void reset(Frame *f){f->command=(Dcb){f->words.ptr,(Uint32 *)f->words.ptr+COMMAND_WORDS,f->words.ptr,(Uint32 *)f->words.ptr+COMMAND_WORDS,(uintptr_t)overflow,NULL,0,0};f->vertex_count=f->table_used=0;f->started=f->pending=SDL_FALSE;}
static int wait_frame(Renderer *d,Frame *f){
    Uint64 deadline=SDL_GetTicks64()+3000;
    if(d->failed)return -1;
    if(f->submitted){while(*(volatile Uint32 *)f->marker.ptr!=f->sequence){if(SDL_GetTicks64()>deadline){d->failed=SDL_TRUE;return SDL_SetError("AGC completion timeout; GPU resources retained");}sceKernelUsleep(50);}f->submitted=SDL_FALSE;}
    return 0;
}
static void *table(Frame *f,size_t bytes){void *p;if(bytes>TABLE_BYTES-f->table_used)return NULL;p=(Uint8 *)f->tables.ptr+f->table_used;f->table_used=(f->table_used+bytes+15)&~(size_t)15;return p;}
static Uint32 float_word(float v){Uint32 n;SDL_memcpy(&n,&v,4);return n;}
static void add(Reg *out,unsigned *n,unsigned off,Uint32 value){out[(*n)++]=(Reg){(Uint16)off,0,value};}
static void texture_descriptor(Uint32 *w,void *pixels,int width,int height){
    uint64_t a=(uint64_t)(uintptr_t)pixels;
    SDL_memset(w,0,32);w[0]=(Uint32)(a>>8);w[1]=(Uint32)(a>>40)|56u<<20|((Uint32)(width-1)&3)<<30;
    w[2]=(Uint32)(width-1)>>2|(Uint32)(height-1)<<14;
    w[3]=4|5<<3|6<<6|7<<9|9u<<28; /* RGBA8 UNORM, linear, one mip, 2D. Pitch aligned to 256 bytes. */
}
static void buffer_descriptor(Uint32 *w,void *pixels,unsigned stride,unsigned count,Uint32 format){uint64_t a=(uint64_t)(uintptr_t)pixels;w[0]=(Uint32)a;w[1]=((Uint32)(a>>32)&0xffffu)|(stride<<16);w[2]=count;w[3]=format;}
static int resource_slot(void *h,unsigned kind){Uint8 *ud=*(Uint8 **)((Uint8 *)h+8);Uint16 *slots;if(!ud||!((Uint16 *)(ud+46))[kind])return -1;slots=*(Uint16 **)(ud+8+kind*sizeof(void *));return slots[0]&0x7fff;}
static Uint32 rd32(const Uint8 *p){Uint32 n;SDL_memcpy(&n,p,4);return n;}
static uint64_t rd64(const Uint8 *p){uint64_t n;SDL_memcpy(&n,p,8);return n;}
static int shader(Renderer *d,unsigned index,const Uint8 *bytes,size_t length,void **handle){
    uint64_t off;Uint16 es,num,str;const Uint8 *header=NULL,*code=NULL;size_t hs=0,cs=0;unsigned i;
    if(length<64||SDL_memcmp(bytes,"\177ELF",4))return SDL_SetError("Invalid AGC shader ELF");
    off=rd64(bytes+40);SDL_memcpy(&es,bytes+58,2);SDL_memcpy(&num,bytes+60,2);SDL_memcpy(&str,bytes+62,2);
    if(es<64||str>=num||off>length||(uint64_t)es*num>length-off)return SDL_SetError("Invalid AGC shader sections");
    {const Uint8 *rec=bytes+off+es*str;uint64_t strings=rd64(rec+24),sn=rd64(rec+32);if(strings>length||sn>length-strings)return SDL_SetError("Invalid AGC strings");
    for(i=0;i<num;i++){const Uint8 *s=bytes+off+es*i;uint64_t begin=rd64(s+24),size=rd64(s+32);Uint32 name=rd32(s);if(begin>length||size>length-begin||name>=sn)continue;
       if(!memchr(bytes+strings+name,0,sn-name))continue;
       if(!SDL_strcmp((const char *)bytes+strings+name,".shader_header")){header=bytes+begin;hs=size;}
       if(!SDL_strcmp((const char *)bytes+strings+name,".shader_text")){code=bytes+begin;cs=size;}
    }}
    if(!header||!code||hs<96||allocation(&d->shader_mem[index],hs)||allocation(&d->shader_mem[index+1],cs))return SDL_SetError("AGC shader allocation failed");
    SDL_memcpy(d->shader_mem[index].ptr,header,hs);SDL_memcpy(d->shader_mem[index+1].ptr,code,cs);
    if(sceAgcCreateShader(handle,d->shader_mem[index].ptr,d->shader_mem[index+1].ptr)!=0)return SDL_SetError("AGC shader creation failed");
    return 0;
}
static int submit(Renderer *d,SDL_bool present){
    Frame *f=&d->frames[d->slot];struct {void *words;Uint32 count;Uint8 flags,pad[3];} desc;
    if(!f->started&&!present)return 0;
    if(!f->pending&&!present&&f->command.up==f->command.bottom)return 0;
    if(present)sceAgcDcbSetFlip(&f->command,(unsigned)d->video->handle,d->slot,1,d->sequence);
    f->sequence=++d->sequence;
    sceAgcCbReleaseMem(&f->command,45,12,1,0,NULL,0,0,0,1,0,0);
    sceAgcCbReleaseMem(&f->command,40,0x30c,0,0,f->marker.ptr,1,f->sequence,0,0,0,0);
    desc.words=f->command.bottom;desc.count=(Uint32)(f->command.up-f->command.bottom);desc.flags=0;
    if(desc.count>COMMAND_WORDS||sceAgcDriverSubmitDcb(&desc)!=0){d->failed=SDL_TRUE;return SDL_SetError("AGC submission failed");}
    f->submitted=SDL_TRUE;f->pending=SDL_FALSE;return 0;
}
static int drain(Renderer *d){
    unsigned i;Frame *f=&d->frames[d->slot];
    if(d->failed||submit(d,SDL_FALSE))return -1;
    for(i=0;i<2;i++)if(wait_frame(d,&d->frames[i]))return -1;
    /* Each later submission rebinds its complete pipeline; never rely on GPU state surviving a submission boundary. */
    reset(f);return 0;
}
static int begin(Renderer *d){
    Frame *f=&d->frames[d->slot];Reg *cx,*sh,*uc;unsigned count=d->state_count,i;uint64_t a=(uintptr_t)d->video->vbuf[d->slot].data;
    if(d->failed||wait_frame(d,f))return -1;
    if(f->started)return 0;
    reset(f);
    {unsigned words=sceAgcDriverGetWaitRenderingPacketSizeInDwords();if(!words||words>128)return SDL_SetError("Invalid AGC wait packet size");
    if(sceAgcDriverWaitUntilSafeForRendering(&f->command.up,words,0,d->video->handle,d->slot)!=0)return SDL_SetError("AGC target wait failed");}
    cx=table(f,count*sizeof(Reg));sh=table(f,d->sh_count*sizeof(Reg));uc=table(f,3*sizeof(Reg));
    if(!cx||!sh||!uc)return SDL_OutOfMemory();SDL_memcpy(cx,d->state,count*sizeof(Reg));
    for(i=0;i<count;i++){if(cx[i].offset==0x318)cx[i].value=(Uint32)(a>>8);if(cx[i].offset==0x390)cx[i].value=(cx[i].value&0xffffff00u)|((Uint32)(a>>40)&255u);}
    SDL_memcpy(sh,d->sh,d->sh_count*sizeof(Reg));SDL_memcpy(uc,d->primitive,3*sizeof(Reg));
    sceAgcDcbSetCxRegistersIndirect(&f->command,cx,count);sceAgcDcbSetShRegistersIndirect(&f->command,sh,d->sh_count);sceAgcDcbSetUcRegistersIndirect(&f->command,uc,3);
    f->started=SDL_TRUE;return 0;
}
static Uint32 blend_word(SDL_BlendMode mode){
    if(mode==SDL_BLENDMODE_BLEND)return 1u<<30|1u<<29|4u|5u<<8|1u<<16|5u<<24;
    if(mode==SDL_BLENDMODE_ADD)return 1u<<30|1u<<29|4u|1u<<8|1u<<24;
    return 0;
}
static int draw_range(Renderer *d,size_t first,unsigned count,Texture *texture,SDL_BlendMode blend,SDL_Rect clip){
    Frame *f=&d->frames[d->slot];Uint32 cb[4],vb[4],sampler[4]={0x92,0,0,0};Reg *regs;unsigned n=0;
    if(!count||clip.w<=0||clip.h<=0)return 0;
    if((size_t)(f->command.top-f->command.up)<256||f->table_used+256>TABLE_BYTES)return SDL_SetError("AGC frame command capacity exceeded");
    regs=table(f,5*sizeof(Reg));if(!regs)return SDL_OutOfMemory();
    add(regs,&n,0x090,0x80000000u|(Uint32)clip.x|(Uint32)clip.y<<16);add(regs,&n,0x091,(Uint32)(clip.x+clip.w)|(Uint32)(clip.y+clip.h)<<16);
    add(regs,&n,0x1e0,blend_word(blend));add(regs,&n,0x202,0x00cc0011);add(regs,&n,0x200,0);
    sceAgcDcbSetCxRegistersIndirect(&f->command,regs,n);
    buffer_descriptor(cb,d->constants.ptr,16,8,0xfacu|77u<<12);
    buffer_descriptor(vb,(Vertex *)f->vertices.ptr+first,36,count,0x204u|5u<<12);
    sceAgcCbSetShRegisterRangeDirect(&f->command,0x8c+d->cb_slot,cb,4);sceAgcCbSetShRegisterRangeDirect(&f->command,0x8c+d->vb_slot,vb,4);
    if(texture&&texture->filter==SDL_ScaleModeLinear)sampler[2]=1u<<20|1u<<22;
    sceAgcCbSetShRegisterRangeDirect(&f->command,0x0c,texture?texture->desc:d->white_desc,8);sceAgcCbSetShRegisterRangeDirect(&f->command,0x14,sampler,4);
    sceAgcDcbDrawIndexAuto(&f->command,count,0);f->pending=SDL_TRUE;return 0;
}
static void vertex(Vertex *v,float x,float y,float u,float t,Uint32 color,Renderer *d){*v=(Vertex){x*2/d->width-1,1-y*2/d->height,0,u,t,0,0,0,color};}
static Uint32 rgba(SDL_Color c){return (Uint32)c.a<<24|(Uint32)c.r<<16|(Uint32)c.g<<8|c.b;}
static int noop(SDL_Renderer *r,SDL_RenderCommand *cmd){(void)r;(void)cmd;return 0;}
static SDL_bool supports(SDL_Renderer *r,SDL_BlendMode mode){(void)r;return mode==SDL_BLENDMODE_NONE||mode==SDL_BLENDMODE_BLEND||mode==SDL_BLENDMODE_ADD;}
static int queue_geometry(SDL_Renderer *r,SDL_RenderCommand *cmd,SDL_Texture *tex,const float *xy,int xs,const SDL_Color *colors,int cs,const float *uv,int us,int nv,const void *indices,int ni,int is,float sx,float sy){
    size_t count=indices?(size_t)ni:(size_t)nv;Vertex *v;size_t i;(void)tex;
    if(count>VERTEX_CAPACITY)return SDL_SetError("AGC geometry too large");
    v=SDL_AllocateRenderVertices(r,count*sizeof(Vertex),4,&cmd->data.draw.first);if(!v)return -1;cmd->data.draw.count=count;
    for(i=0;i<count;i++){
        unsigned j=!indices?(unsigned)i:is==1?((const Uint8 *)indices)[i]:is==2?((const Uint16 *)indices)[i]:((const Uint32 *)indices)[i];
        const float *pos=(const float *)((const Uint8 *)xy+j*xs);const SDL_Color *color=(const SDL_Color *)((const Uint8 *)colors+j*cs);
        const float *coord=uv?(const float *)((const Uint8 *)uv+j*us):NULL;
        v[i]=(Vertex){pos[0]*sx,pos[1]*sy,0,coord?coord[0]:0,coord?coord[1]:0,0,0,0,rgba(*color)};
    }return 0;
}
static int queue_points(SDL_Renderer *r,SDL_RenderCommand *cmd,const SDL_FPoint *points,int count){
    SDL_FPoint *v=SDL_AllocateRenderVertices(r,(size_t)count*sizeof(*v),4,&cmd->data.draw.first);if(!v)return -1;SDL_memcpy(v,points,(size_t)count*sizeof(*v));cmd->data.draw.count=count;return 0;
}
static SDL_Rect scissor(Renderer *d){SDL_Rect bounds={0,0,d->width,d->height},clip=d->viewport;if(d->clipping){SDL_Rect c=d->clip;c.x+=d->viewport.x;c.y+=d->viewport.y;SDL_IntersectRect(&clip,&c,&clip);}SDL_IntersectRect(&clip,&bounds,&clip);return clip;}
static int run_queue(SDL_Renderer *r,SDL_RenderCommand *cmd,void *vertices,size_t bytes){
    Renderer *d=r->driverdata;Frame *f=&d->frames[d->slot];size_t first=0;unsigned count=0;Texture *texture=NULL;SDL_BlendMode blend=SDL_BLENDMODE_NONE;SDL_Rect clip={0,0,0,0};
    (void)bytes;if(begin(d))return -1;
    for(;cmd;cmd=cmd->next){
        if(cmd->command==SDL_RENDERCMD_SETDRAWCOLOR||cmd->command==SDL_RENDERCMD_NO_OP)continue;
        if(cmd->command==SDL_RENDERCMD_GEOMETRY){
            Texture *t=cmd->data.draw.texture?cmd->data.draw.texture->driverdata:NULL;SDL_Rect c=scissor(d);unsigned n=(unsigned)cmd->data.draw.count,i;Vertex *src=(Vertex *)((Uint8 *)vertices+cmd->data.draw.first),*dst;
            if(count&&(t!=texture||cmd->data.draw.blend!=blend||SDL_memcmp(&clip,&c,sizeof(c)))){if(draw_range(d,first,count,texture,blend,clip))return -1;count=0;}
            if(n>VERTEX_CAPACITY-f->vertex_count)return SDL_SetError("AGC frame vertex capacity exceeded");
            if(!count){first=f->vertex_count;texture=t;blend=cmd->data.draw.blend;clip=c;}
            dst=(Vertex *)f->vertices.ptr+f->vertex_count;for(i=0;i<n;i++)vertex(&dst[i],src[i].x+d->viewport.x,src[i].y+d->viewport.y,src[i].u,src[i].v,src[i].color,d);
            f->vertex_count+=n;count+=n;continue;
        }
        if(count){if(draw_range(d,first,count,texture,blend,clip))return -1;count=0;}
        if(cmd->command==SDL_RENDERCMD_SETVIEWPORT)d->viewport=cmd->data.viewport.rect;
        else if(cmd->command==SDL_RENDERCMD_SETCLIPRECT){d->clip=cmd->data.cliprect.rect;d->clipping=cmd->data.cliprect.enabled;}
        else if(cmd->command==SDL_RENDERCMD_CLEAR){
            Vertex *v;SDL_Rect c={0,0,d->width,d->height};Uint32 color=rgba((SDL_Color){cmd->data.color.r,cmd->data.color.g,cmd->data.color.b,cmd->data.color.a});
            if(f->vertex_count+6>VERTEX_CAPACITY)return SDL_OutOfMemory();first=f->vertex_count;v=(Vertex *)f->vertices.ptr+first;
            vertex(v,0,0,0,0,color,d);vertex(v+1,d->width,0,0,0,color,d);vertex(v+2,d->width,d->height,0,0,color,d);v[3]=v[0];v[4]=v[2];vertex(v+5,0,d->height,0,0,color,d);f->vertex_count+=6;
            if(draw_range(d,first,6,NULL,SDL_BLENDMODE_NONE,c))return -1;
        }else if(cmd->command==SDL_RENDERCMD_DRAW_POINTS||cmd->command==SDL_RENDERCMD_DRAW_LINES){
            unsigned i,n=(unsigned)cmd->data.draw.count;SDL_FPoint *p=(SDL_FPoint *)((Uint8 *)vertices+cmd->data.draw.first);SDL_Rect c=scissor(d);Uint32 color=rgba((SDL_Color){cmd->data.draw.r,cmd->data.draw.g,cmd->data.draw.b,cmd->data.draw.a});
            for(i=0;i<(cmd->command==SDL_RENDERCMD_DRAW_POINTS?n:n>0?n-1:0);i++){
                Vertex *v;float ax=p[i].x+d->viewport.x,ay=p[i].y+d->viewport.y,bx=ax,by=ay,nx=.5f,ny=.5f;
                if(cmd->command==SDL_RENDERCMD_DRAW_LINES){float dx=p[i+1].x-p[i].x,dy=p[i+1].y-p[i].y,len=SDL_sqrtf(dx*dx+dy*dy);bx=ax+dx;by=ay+dy;if(len>.01f){nx=-dy/len*.5f;ny=dx/len*.5f;}}
                else{bx+=1;ay+=.5f;by=ay;nx=0;ny=.5f;}
                if(f->vertex_count+6>VERTEX_CAPACITY)return SDL_OutOfMemory();first=f->vertex_count;v=(Vertex *)f->vertices.ptr+first;
                vertex(v,ax+nx,ay+ny,0,0,color,d);vertex(v+1,bx+nx,by+ny,0,0,color,d);vertex(v+2,bx-nx,by-ny,0,0,color,d);v[3]=v[0];v[4]=v[2];vertex(v+5,ax-nx,ay-ny,0,0,color,d);f->vertex_count+=6;
                if(draw_range(d,first,6,NULL,cmd->data.draw.blend,c))return -1;
            }
        }
    }
    return count?draw_range(d,first,count,texture,blend,clip):0;
}
static int create_texture(SDL_Renderer *r,SDL_Texture *t){Texture *tex=SDL_calloc(1,sizeof(*tex));(void)r;if(!tex)return SDL_OutOfMemory();tex->pitch=(t->w+63)&~63;tex->pitch*=4;tex->filter=t->scaleMode;
    if(t->access==SDL_TEXTUREACCESS_TARGET){SDL_free(tex);return SDL_SetError("AGC target textures are not advertised");}
    if(allocation(&tex->memory,(size_t)tex->pitch*t->h)){SDL_free(tex);return -1;}texture_descriptor(tex->desc,tex->memory.ptr,t->w,t->h);t->driverdata=tex;return 0;
}
static int lock_texture(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *rect,void **pixels,int *pitch){Texture *tex=t->driverdata;if(drain(r->driverdata))return -1;*pixels=(Uint8 *)tex->memory.ptr+rect->y*tex->pitch+rect->x*4;*pitch=tex->pitch;return 0;}
static void unlock_texture(SDL_Renderer *r,SDL_Texture *t){(void)r;(void)t;__sync_synchronize();}
static int update_texture(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *rect,const void *pixels,int pitch){Texture *tex=t->driverdata;Uint8 *dst;int row;if(drain(r->driverdata))return -1;dst=(Uint8 *)tex->memory.ptr+rect->y*tex->pitch+rect->x*4;for(row=0;row<rect->h;row++)SDL_memcpy(dst+row*tex->pitch,(const Uint8 *)pixels+row*pitch,rect->w*4);__sync_synchronize();return 0;}
static void scale_mode(SDL_Renderer *r,SDL_Texture *t,SDL_ScaleMode mode){(void)r;((Texture *)t->driverdata)->filter=mode;}
static int target(SDL_Renderer *r,SDL_Texture *t){(void)r;return t?SDL_Unsupported():0;}
static int read_pixels(SDL_Renderer *r,const SDL_Rect *rect,Uint32 format,void *pixels,int pitch){(void)r;(void)rect;(void)format;(void)pixels;(void)pitch;return SDL_Unsupported();}
static int output_size(SDL_Renderer *r,int *w,int *h){Renderer *d=r->driverdata;*w=d->width;*h=d->height;return 0;}
static int present(SDL_Renderer *r){Renderer *d=r->driverdata;if(begin(d)||submit(d,SDL_TRUE))return -1;sceAgcSuspendPoint();d->frames[d->slot].started=SDL_FALSE;d->slot^=1;return 0;}
static int vsync(SDL_Renderer *r,int enabled){(void)r;return enabled?0:SDL_SetError("AGC renderer requires vsync");}
static void destroy_texture(SDL_Renderer *r,SDL_Texture *t){Texture *tex=t->driverdata;if(tex){if(!drain(r->driverdata)){release(&tex->memory);SDL_free(tex);}t->driverdata=NULL;}}
static void destroy_renderer(SDL_Renderer *r){Renderer *d=r->driverdata;unsigned i;if(d){if(!drain(d)){for(i=0;i<2;i++){release(&d->frames[i].words);release(&d->frames[i].vertices);release(&d->frames[i].tables);release(&d->frames[i].marker);}for(i=0;i<4;i++)release(&d->shader_mem[i]);release(&d->constants);release(&d->white);}SDL_free(d);}SDL_free(r);}
static SDL_Renderer *create(SDL_Window *window,Uint32 flags){
    SDL_Renderer *r=SDL_calloc(1,sizeof(*r));Renderer *d=SDL_calloc(1,sizeof(*d));unsigned i,n=0;Reg *defaults;unsigned default_count;Reg linkage[34];Uint32 rt[16];static const Uint16 offsets[]={0x318,0x31b,0x31c,0x31d,0x31e,0x31f,0x321,0x323,0x324,0x325,0x390,0x398,0x3a0,0x3a8,0x3b0,0x3b8};
    (void)flags;if(!r||!d){SDL_free(r);SDL_free(d);SDL_OutOfMemory();return NULL;}r->driverdata=d;
    d->video=SDL_GetVideoDevice()->driverdata;SDL_GetWindowSizeInPixels(window,&d->width,&d->height);d->viewport=(SDL_Rect){0,0,d->width,d->height};
    if(sceAgcInit(&d->agc_state,8)!=0){SDL_SetError("sceAgcInit failed");goto fail;}
    if(shader(d,0,mesh_vs_sb,mesh_vs_sb_len,&d->vs)||shader(d,2,textured_ps_sb,textured_ps_sb_len,&d->ps))goto fail;
    if(SH_CX_COUNT(d->vs)+SH_CX_COUNT(d->ps)>128||SH_SH_COUNT(d->vs)+SH_SH_COUNT(d->ps)>64){SDL_SetError("AGC shader register limits exceeded");goto fail;}
    if(sceAgcLinkShaders(linkage,d->primitive,NULL,d->vs,d->ps,4)!=0){SDL_SetError("AGC shader link failed");goto fail;}
    d->cb_slot=resource_slot(d->vs,3);d->vb_slot=resource_slot(d->vs,0);if(d->cb_slot<0||d->vb_slot<0||resource_slot(d->ps,0)!=0||resource_slot(d->ps,2)!=8){SDL_SetError("AGC shader resource layout mismatch");goto fail;}
    if(allocation(&d->constants,128)||allocation(&d->white,256))goto fail;
    {float *matrix=d->constants.ptr;for(i=0;i<32;i++)matrix[i]=i%16%5==0?1:0;*(Uint32 *)d->white.ptr=0xffffffffu;texture_descriptor(d->white_desc,d->white.ptr,1,1);}
    for(i=0;i<2;i++){Frame *f=&d->frames[i];if(allocation(&f->words,COMMAND_WORDS*4)||allocation(&f->vertices,VERTEX_CAPACITY*sizeof(Vertex))||allocation(&f->tables,TABLE_BYTES)||allocation(&f->marker,8))goto fail;reset(f);}
    {Uint8 *a=sceAgcGetRegisterDefaults();Reg **blocks=*(Reg ***)a;default_count=*(Uint32 *)(a+0x20);defaults=blocks?blocks[0]:NULL;if(!defaults||default_count>65536){SDL_SetError("AGC register defaults missing");goto fail;}}
    for(i=0;i<16;i++){unsigned j;rt[i]=0;for(j=0;j<default_count;j++)if(defaults[j].offset==offsets[i]){rt[i]=defaults[j].value;break;}}
    rt[0]=0;rt[1]&=~0x03ffe000u;rt[2]=(rt[2]&~0x1005df7cu)|0x8828u;rt[3]&=~0x1f000u;
    rt[4]=(rt[4]&~0x00180200u)|0x48u;rt[5]=rt[6]=rt[9]=0;rt[10]&=0xffffff00u;rt[11]&=0xffffff00u;rt[12]&=0xffffff00u;rt[13]&=0xffffff00u;
    rt[14]=(Uint32)(d->height-1)|((Uint32)(d->width-1)<<14);rt[15]=(rt[15]&~0x4707dfffu)|0x4506c000u;
    for(i=0;i<16;i++)add(d->state,&n,offsets[i],rt[i]);
    add(d->state,&n,0x10f,float_word(d->width*.5f));add(d->state,&n,0x110,float_word(d->width*.5f));add(d->state,&n,0x111,float_word(-d->height*.5f));add(d->state,&n,0x112,float_word(d->height*.5f));
    add(d->state,&n,0x113,float_word(1));add(d->state,&n,0x114,0);add(d->state,&n,0xb4,0);add(d->state,&n,0xb5,float_word(1));
    for(i=0;i<4;i++)add(d->state,&n,0x2fa+i,float_word(8));
    add(d->state,&n,0x8e,0xf);
    {Uint32 mode=0;for(i=0;i<default_count;i++)if(defaults[i].offset==0x205){mode=defaults[i].value;break;}add(d->state,&n,0x205,mode&~7u);}add(d->state,&n,0x200,0);
    for(i=0;i<34;i++)d->state[n++]=linkage[i];
    for(i=0;i<SH_CX_COUNT(d->vs);i++)d->state[n++]=SH_CX(d->vs)[i];for(i=0;i<SH_CX_COUNT(d->ps);i++)d->state[n++]=SH_CX(d->ps)[i];d->state_count=n;
    for(i=0;i<SH_SH_COUNT(d->vs);i++)d->sh[d->sh_count++]=SH_SH(d->vs)[i];for(i=0;i<SH_SH_COUNT(d->ps);i++)d->sh[d->sh_count++]=SH_SH(d->ps)[i];
    r->GetOutputSize=output_size;r->SupportsBlendMode=supports;r->CreateTexture=create_texture;r->UpdateTexture=update_texture;r->LockTexture=lock_texture;r->UnlockTexture=unlock_texture;r->SetTextureScaleMode=scale_mode;r->SetRenderTarget=target;
    r->QueueSetViewport=noop;r->QueueSetDrawColor=noop;r->QueueDrawPoints=queue_points;r->QueueDrawLines=queue_points;r->QueueGeometry=queue_geometry;r->RunCommandQueue=run_queue;r->RenderReadPixels=read_pixels;r->RenderPresent=present;r->DestroyTexture=destroy_texture;r->DestroyRenderer=destroy_renderer;r->SetVSync=vsync;r->info=PS5AGC_RenderDriver.info;r->window=window;
    SDL_Log("Prospero renderer: PS5 AGC GPU, %dx%d, vsync, batched textured geometry",d->width,d->height);return r;
fail:
    /* No GPU command was submitted: partial initialization is safe to release. */
    for(i=0;i<2;i++){release(&d->frames[i].words);release(&d->frames[i].vertices);release(&d->frames[i].tables);release(&d->frames[i].marker);}for(i=0;i<4;i++)release(&d->shader_mem[i]);release(&d->constants);release(&d->white);SDL_free(d);SDL_free(r);return NULL;
}
SDL_RenderDriver PS5AGC_RenderDriver={.CreateRenderer=create,.info={.name="ps5agc",.flags=SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC,.num_texture_formats=1,.texture_formats={SDL_PIXELFORMAT_ABGR8888},.max_texture_width=8192,.max_texture_height=8192}};
#endif
