#include "/usr/include/SDL/SDL.h"
#include <iostream>
#include <string>
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"
#include "SDL/SDL_mixer.h"




using namespace std;

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SCREEN_BPP = 32;

SDL_Surface *load_image( char *inp_file )
{
    SDL_Surface *load_image =NULL;
    SDL_Surface *optimizedImage =NULL;

    //加载图像
    load_image = SDL_LoadBMP(inp_file);
    if( load_image != NULL )
    {
        //创建一个优化了的图像，创建一个与窗口拥有同样格式的新版本的图像 比如图像是24位的转为32位的
        optimizedImage = SDL_DisplayFormat( load_image );
        
        //释放临时的图像
        SDL_FreeSurface( load_image );
    }

    return optimizedImage;
}

void apply_surface( int x, int y, SDL_Surface* source, SDL_Surface* destination )
{
    //新建一个临时的矩形来保存偏移量
    SDL_Rect offset;
    
    //将传入的偏移量保存到矩形中
    offset.x = x;
    offset.y = y;

    //执行表面的Blit
    SDL_BlitSurface( source, NULL, destination, &offset );


}

int main (int argc ,char * argv[])
{

    SDL_Surface *message = NULL;
    SDL_Surface *background = NULL;
    SDL_Surface *screen = NULL; //表面

    if( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )
    {
        return 1;    
    }

    screen = SDL_SetVideoMode(SCREEN_WIDTH*2, SCREEN_HEIGHT*2, SCREEN_BPP, SDL_SWSURFACE);
    if( screen == NULL )
    {
        return 1;    
    }

    //设置窗口标题
    SDL_WM_SetCaption( "Hello World", NULL );

    message =IMG_Load(argv[1]);
    background = IMG_Load(argv[2]);


    SDL_Rect offset;    
    //将传入的偏移量保存到矩形中
    offset.x = 0;
    offset.y = 0;
    SDL_BlitSurface( background, NULL, screen, &offset );
    offset.x = SCREEN_WIDTH;
    SDL_BlitSurface( background, NULL, screen, &offset );
    if( SDL_Flip( screen ) == -1 )//更新窗口
    {
        return 1;    
    }

    apply_surface( 50, 50, message, screen );
    if( SDL_Flip( screen ) == -1 )//更新窗口
    {
        return 1;    
    }
    SDL_Delay( 2000 );

    SDL_FreeSurface( message );
    SDL_FreeSurface( background );

    //退出SDL
    SDL_Quit();
    
    //main函数返回
    return 0;
}