/* Native AGC backend command/lifetime regression, using a mocked system API.
 * This proves host-side submission structure, not console pixels or speed. */
#define PROSPERO_AGC_HOST_TEST 1
#include "../third_party/SDL-ps5/src/render/ps5agc/SDL_render_ps5agc.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static Memory allocations[4096];static unsigned allocation_count,draws,submits,flips,waits,completions;
static void *queued_vertices;static volatile Uint32 *deferred_marker[4096];static Uint32 deferred_value[4096];static unsigned deferred_count;
static volatile Uint32 *next_marker;static Uint32 next_value;static PS5_DeviceData mock_video;static SDL_VideoDevice mock_device;
static unsigned last_wait_slot;static void *bound_vertices;static unsigned bound_count;
static int in_direct_memory(const void *p){unsigned i;for(i=1;i<4096;i++)if(allocations[i].ptr&&(const Uint8 *)p>=(Uint8 *)allocations[i].ptr&&(const Uint8 *)p<(Uint8 *)allocations[i].ptr+allocations[i].bytes)return 1;return 0;}
SDL_VideoDevice *SDL_GetVideoDevice(void){return &mock_device;}
void *SDL_AllocateRenderVertices(SDL_Renderer *r,size_t bytes,size_t alignment,size_t *first){(void)r;(void)alignment;free(queued_vertices);queued_vertices=malloc(bytes);*first=0;return queued_vertices;}
int sceKernelAllocateMainDirectMemory(size_t bytes,size_t align,int type,intptr_t *physical){unsigned i;assert(type==12&&align>=0x4000);for(i=1;i<4096;i++)if(!allocations[i].ptr){assert(posix_memalign(&allocations[i].ptr,align,bytes)==0);allocations[i].bytes=bytes;*physical=i;allocation_count++;return 0;}return -1;}
int sceKernelMapDirectMemory(void **p,size_t bytes,int protection,int flags,intptr_t physical,size_t align){assert(bytes==allocations[physical].bytes&&protection==0x33);(void)flags;(void)align;*p=allocations[physical].ptr;return 0;}
int sceKernelMunmap(void *p,size_t bytes){assert(in_direct_memory(p));(void)bytes;return 0;}
int sceKernelReleaseDirectMemory(intptr_t physical,size_t bytes){assert(physical>0&&allocations[physical].ptr&&bytes==allocations[physical].bytes);free(allocations[physical].ptr);allocations[physical].ptr=NULL;allocation_count--;return 0;}
int sceKernelUsleep(unsigned us){unsigned i;(void)us;for(i=0;i<deferred_count;i++){*deferred_marker[i]=deferred_value[i];completions++;}deferred_count=0;return 0;}
int sceAgcInit(void *state,unsigned revision){assert(state&&revision==8);return 0;}
void *sceAgcGetRegisterDefaults(void){static Uint8 data[64];static Reg dummy={0x205,0,0x100};static Reg *blocks[]={&dummy};Reg **ptr=blocks;Uint32 count=1;memcpy(data,&ptr,8);memcpy(data+0x20,&count,4);return data;}
int sceAgcCreateShader(void **handle,void *header,void *code){static Uint8 shader_data[2][128],user_data[2][64];static Uint16 ro[2]={0,0},cb=4,sampler=8;static unsigned index;unsigned k=index++%2;Uint8 *ud=user_data[k];Uint16 *ptr;assert(in_direct_memory(header)&&in_direct_memory(code));memset(shader_data[k],0,128);memset(ud,0,64);
 memcpy(shader_data[k]+8,&ud,8);ptr=&ro[k];memcpy(ud+8,&ptr,8);((Uint16 *)(ud+46))[0]=1;
 if(k==0){ptr=&cb;memcpy(ud+32,&ptr,8);((Uint16 *)(ud+46))[3]=1;}else{ptr=&sampler;memcpy(ud+24,&ptr,8);((Uint16 *)(ud+46))[2]=1;}
 *handle=shader_data[k];return 0;}
int sceAgcLinkShaders(void *cx,void *uc,void *unused,void *vs,void *ps,unsigned primitive){assert(vs&&ps&&primitive==4);(void)unused;memset(cx,0,34*sizeof(Reg));memset(uc,0,3*sizeof(Reg));return 0;}
static void packet(Dcb *d,unsigned words){assert(d->up+words<=d->top);memset(d->up,0,words*4);d->up+=words;}
void *sceAgcDcbSetCxRegistersIndirect(void *d,const void *p,unsigned count){assert(in_direct_memory(p));packet(d,count+2);return p?(void *)p:NULL;}
void *sceAgcDcbSetShRegistersIndirect(void *d,const void *p,unsigned count){assert(in_direct_memory(p));packet(d,count+2);return (void *)p;}
void *sceAgcDcbSetUcRegistersIndirect(void *d,const void *p,unsigned count){assert(in_direct_memory(p));packet(d,count+2);return (void *)p;}
void *sceAgcCbSetShRegisterRangeDirect(void *d,unsigned offset,const unsigned *words,unsigned count){if(offset==0x8c){uint64_t a=words[0]|(uint64_t)(words[1]&0xffffu)<<32;bound_vertices=(void *)(uintptr_t)a;bound_count=words[2];assert(in_direct_memory(bound_vertices));}packet(d,count+2);return NULL;}
void *sceAgcDcbDrawIndexAuto(void *d,unsigned count,uint64_t modifier){assert(count>0&&count%3==0&&count==bound_count&&bound_vertices);(void)modifier;draws++;packet(d,4);return NULL;}
void *sceAgcDcbSetFlip(void *d,unsigned video,int slot,unsigned mode,int64_t frame){assert(video==7&&slot>=0&&slot<2&&mode==1&&frame>=0);flips++;packet(d,8);return NULL;}
void *sceAgcCbReleaseMem(void *d,uint8_t event,int16_t control,uint64_t a,int8_t b,void *marker,unsigned select,uint64_t value,uint16_t c,uint16_t e,int8_t f,int g){(void)a;(void)b;(void)c;(void)e;(void)f;(void)g;if(event==40){assert(control==0x30c&&select==1&&in_direct_memory(marker));next_marker=marker;next_value=(Uint32)value;}else assert(event==45&&control==12);packet(d,8);return NULL;}
int sceAgcDriverSubmitDcb(void *p){struct {void *words;Uint32 count;Uint8 flag;} *desc=p;assert(in_direct_memory(desc->words)&&desc->count>0&&desc->count<=COMMAND_WORDS&&next_marker);deferred_marker[deferred_count]=next_marker;deferred_value[deferred_count++]=next_value;next_marker=NULL;submits++;return 0;}
int sceAgcSuspendPoint(void){return 0;}
unsigned sceAgcDriverGetWaitRenderingPacketSizeInDwords(void){return 32;}
unsigned sceAgcDriverWaitUntilSafeForRendering(unsigned **up,unsigned words,unsigned reserved,unsigned video,int slot){assert(words==32&&reserved==0&&video==7&&slot>=0&&slot<2);last_wait_slot=slot;memset(*up,0,words*4);*up+=words;waits++;return 0;}
int main(void){
 SDL_Window *window;SDL_Renderer *renderer;Renderer *data;Vertex source[6];SDL_RenderCommand cmd[40];unsigned i,frame;Texture *texture;SDL_Texture tex={0};SDL_Rect rect={0,0,13,7};Uint32 pixels[13*7];unsigned start;
 SDL_setenv("SDL_VIDEODRIVER","dummy",1);assert(SDL_Init(SDL_INIT_VIDEO)==0);window=SDL_CreateWindow("AGC host test",0,0,1920,1080,0);assert(window);
 mock_video.handle=7;mock_video.vbuf[0].data=(void *)0x100000000ull;mock_video.vbuf[1].data=(void *)0x104000000ull;mock_device.driverdata=&mock_video;
 renderer=create(window,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);if(!renderer)fprintf(stderr,"%s\n",SDL_GetError());assert(renderer);data=renderer->driverdata;
 assert(renderer->info.flags&SDL_RENDERER_ACCELERATED);assert(!renderer->info.flags||!(renderer->info.flags&SDL_RENDERER_TARGETTEXTURE));
 tex.w=13;tex.h=7;tex.format=SDL_PIXELFORMAT_ABGR8888;tex.scaleMode=SDL_ScaleModeLinear;assert(create_texture(renderer,&tex)==0);texture=tex.driverdata;assert(texture->pitch==256&&texture->desc[0]==(Uint32)((uintptr_t)texture->memory.ptr>>8));
 for(i=0;i<13*7;i++)pixels[i]=0xff443322;assert(update_texture(renderer,&tex,&rect,pixels,13*4)==0);for(i=0;i<7;i++)assert(*(Uint32 *)((Uint8 *)texture->memory.ptr+i*256)==pixels[0]);
 for(i=0;i<6;i++)source[i]=(Vertex){100+i*20.f,200+i*5.f,0,.3f,.7f,0,0,0,0x80ff0080};
 memset(cmd,0,sizeof(cmd));for(i=0;i<40;i++){cmd[i].command=SDL_RENDERCMD_GEOMETRY;cmd[i].data.draw.count=6;cmd[i].data.draw.blend=SDL_BLENDMODE_BLEND;cmd[i].next=i<39?&cmd[i+1]:NULL;}
 start=draws;assert(run_queue(renderer,cmd,source,sizeof(source))==0);assert(draws==start+1); // 40 separate shapes -> one GPU draw.
 assert(((Vertex *)data->frames[0].vertices.ptr)[0].x==source[0].x*2/1920-1);
 assert(present(renderer)==0);assert(flips==1&&last_wait_slot==0);
 for(frame=1;frame<12;frame++){assert(run_queue(renderer,cmd,source,sizeof(source))==0);assert(last_wait_slot==(frame&1));assert(present(renderer)==0);}
 assert(flips==12&&waits==12&&completions>=10);
 // Mutating/freeing a texture must drain outstanding GPU work first.
 assert(update_texture(renderer,&tex,&rect,pixels,13*4)==0);assert(deferred_count==0);
 cmd[0].data.draw.texture=&tex;cmd[0].next=NULL;assert(run_queue(renderer,cmd,source,sizeof(source))==0);start=submits;destroy_texture(renderer,&tex);assert(submits>start&&deferred_count==0&&!tex.driverdata);
 // Reject unsupported capabilities and capacity overruns explicitly.
 assert(target(renderer,(SDL_Texture *)1)<0);assert(vsync(renderer,0)<0);
 cmd[0].data.draw.texture=NULL;cmd[0].data.draw.count=VERTEX_CAPACITY+1;assert(run_queue(renderer,cmd,source,sizeof(source))<0);
 destroy_renderer(renderer);assert(allocation_count==0);free(queued_vertices);SDL_DestroyWindow(window);SDL_Quit();
 printf("Native AGC command tests passed: shader/container setup, batching, scanout alternation, completion fences, texture uploads/lifetime, capacities and cleanup. Hardware pixels/FPS remain untested.\n");return 0;
}
