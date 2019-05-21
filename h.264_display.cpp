#include <stdio.h>

#include "/usr/include/SDL2/SDL.h"
 
#define REFRESH_EVENT  (SDL_USEREVENT + 1)
#define BREAK_EVENT  (SDL_USEREVENT + 2)
 
#define BPP			12
#define SCREEN_W	500
#define SCREEN_H 	500
#define PIXEL_W	 	320
#define	PIXEL_H		180
 
const int bpp	=BPP;
int screen_w	=SCREEN_W;
int screen_h	=SCREEN_H;
const int pixel_w=PIXEL_W;
const int pixel_h=PIXEL_H;
unsigned char buffer[PIXEL_W*PIXEL_H*BPP/8];
 
int thread_exit=0;

int refresh_video(void *opaque)
{
	thread_exit=0;
	while (thread_exit==0) {
		SDL_Event event;
		event.type = REFRESH_EVENT;
        /* 发送一个事件 */
		SDL_PushEvent(&event);
		SDL_Delay(40);
	}
	thread_exit=0;
	//Break
	SDL_Event event;
	event.type = BREAK_EVENT;
    /* 发送一个事件*/
	SDL_PushEvent(&event);
	return 0;
}
 
int main(int argc, char* argv[])
{
	Uint32 pixformat=0;
	FILE *fp=NULL;	
	SDL_Rect sdlRect;
	SDL_Window *screen; 
	SDL_Event event;
 
    /* 初始化SDL系统 */
	if(SDL_Init(SDL_INIT_VIDEO)) {  
		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
		return -1;
	} 
 
 
	/* 创建窗口SDL_Window */
	screen = SDL_CreateWindow("Simplest Video Play SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_w, screen_h,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
	if(!screen) {  
		printf("SDL: could not create window - exiting:%s\n",SDL_GetError());  
		return -1;
	}
	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(screen, -1, 0);  
 
	
	//YV12: Y + V + U  (3 planes)
	pixformat= SDL_PIXELFORMAT_IYUV;  
    /* 创建渲染器SDL_Renderer */
	SDL_Texture* sdlTexture = SDL_CreateTexture(sdlRenderer,pixformat, SDL_TEXTUREACCESS_STREAMING,pixel_w,pixel_h);
 
	
	fp=fopen("in.yuv","rb+");
 
	if(fp==NULL){
		printf("cannot open this file\n");
		return -1;
	}
 
    /* 创建一个线程 */
	SDL_Thread *refresh_thread = SDL_CreateThread(refresh_video,NULL,NULL);
	
	while(1){
		/* 等待一个事件 */
		SDL_WaitEvent(&event);
		if(event.type==REFRESH_EVENT){
			if (fread(buffer, 1, pixel_w*pixel_h*bpp/8, fp) != pixel_w*pixel_h*bpp/8){
				fseek(fp, 0, SEEK_SET);
				fread(buffer, 1, pixel_w*pixel_h*bpp/8, fp);
			}
 
            /* 设置纹理的数据 */
			SDL_UpdateTexture( sdlTexture, NULL, buffer, pixel_w);  
 
			//FIX: If window is resize
			sdlRect.x = 0;  
			sdlRect.y = 0;  
			sdlRect.w = screen_w;  
			sdlRect.h = screen_h;  
			
			SDL_RenderClear( sdlRenderer );   
            /* 将纹理的数据拷贝给渲染器 */
			SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, &sdlRect);  
            /* 显示 */
            SDL_RenderPresent( sdlRenderer );  
			
		}else if(event.type==SDL_WINDOWEVENT){
			/* 查询显示窗口大小 */
			SDL_GetWindowSize(screen,&screen_w,&screen_h);
		}else if(event.type==SDL_QUIT){
			thread_exit=1;
		}else if(event.type==BREAK_EVENT){
			break;
		}
	}
	SDL_Quit();
	return 0;
}