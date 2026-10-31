/* Minimal SDL private adapter for deterministic native renderer command tests.
 * This is only a host test shim, never compiled into the console renderer. */
#pragma once
#include <SDL.h>
#define SDL_VIDEO_DRIVER_PS5 1
struct SDL_Texture {int w,h,access;Uint32 format;SDL_ScaleMode scaleMode;void *driverdata;};
typedef enum {SDL_RENDERCMD_NO_OP,SDL_RENDERCMD_SETVIEWPORT,SDL_RENDERCMD_SETCLIPRECT,SDL_RENDERCMD_SETDRAWCOLOR,SDL_RENDERCMD_CLEAR,SDL_RENDERCMD_DRAW_POINTS,SDL_RENDERCMD_DRAW_LINES,SDL_RENDERCMD_FILL_RECTS,SDL_RENDERCMD_COPY,SDL_RENDERCMD_COPY_EX,SDL_RENDERCMD_GEOMETRY} SDL_RenderCommandType;
typedef struct SDL_RenderCommand {SDL_RenderCommandType command;union {
 struct {size_t first;SDL_Rect rect;} viewport;struct {SDL_bool enabled;SDL_Rect rect;} cliprect;
 struct {size_t first,count;Uint8 r,g,b,a;SDL_BlendMode blend;SDL_Texture *texture;} draw;
 struct {size_t first;Uint8 r,g,b,a;} color;} data;struct SDL_RenderCommand *next;} SDL_RenderCommand;
struct SDL_Renderer {
 void *driverdata;SDL_RendererInfo info;SDL_Window *window;
 int (*GetOutputSize)(SDL_Renderer *,int *,int *);SDL_bool (*SupportsBlendMode)(SDL_Renderer *,SDL_BlendMode);
 int (*CreateTexture)(SDL_Renderer *,SDL_Texture *);int (*UpdateTexture)(SDL_Renderer *,SDL_Texture *,const SDL_Rect *,const void *,int);
 int (*LockTexture)(SDL_Renderer *,SDL_Texture *,const SDL_Rect *,void **,int *);void (*UnlockTexture)(SDL_Renderer *,SDL_Texture *);
 void (*SetTextureScaleMode)(SDL_Renderer *,SDL_Texture *,SDL_ScaleMode);int (*SetRenderTarget)(SDL_Renderer *,SDL_Texture *);
 int (*QueueSetViewport)(SDL_Renderer *,SDL_RenderCommand *);int (*QueueSetDrawColor)(SDL_Renderer *,SDL_RenderCommand *);
 int (*QueueDrawPoints)(SDL_Renderer *,SDL_RenderCommand *,const SDL_FPoint *,int);int (*QueueDrawLines)(SDL_Renderer *,SDL_RenderCommand *,const SDL_FPoint *,int);
 int (*QueueGeometry)(SDL_Renderer *,SDL_RenderCommand *,SDL_Texture *,const float *,int,const SDL_Color *,int,const float *,int,int,const void *,int,int,float,float);
 int (*RunCommandQueue)(SDL_Renderer *,SDL_RenderCommand *,void *,size_t);int (*RenderReadPixels)(SDL_Renderer *,const SDL_Rect *,Uint32,void *,int);
 int (*RenderPresent)(SDL_Renderer *);void (*DestroyTexture)(SDL_Renderer *,SDL_Texture *);void (*DestroyRenderer)(SDL_Renderer *);int (*SetVSync)(SDL_Renderer *,int);
};
typedef struct {SDL_Renderer *(*CreateRenderer)(SDL_Window *,Uint32);SDL_RendererInfo info;} SDL_RenderDriver;
typedef struct {int handle;struct {void *data;} vbuf[2];} PS5_DeviceData;
typedef struct {void *driverdata;} SDL_VideoDevice;
SDL_VideoDevice *SDL_GetVideoDevice(void);
void *SDL_AllocateRenderVertices(SDL_Renderer *,size_t,size_t,size_t *);
int sceKernelAllocateMainDirectMemory(size_t,size_t,int,intptr_t *);
int sceKernelMapDirectMemory(void **,size_t,int,int,intptr_t,size_t);
int sceKernelReleaseDirectMemory(intptr_t,size_t);
