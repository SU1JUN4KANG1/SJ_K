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



void apply_surface( int x, int y, SDL_Surface* source, SDL_Surface* destination );
SDL_Surface *load_image( std::string filename );
void clean_up();
bool load_files();
bool init();

//表面
SDL_Surface *image = NULL;
SDL_Surface *screen = NULL;
//将要用到的事件结构体
SDL_Event event;

int main (int argc ,char * argv[])
{

    //确保程序一直等待quit
    bool quit = false;

        //初始化
    if( init() == false )
    {
        return 1;    
    }
    
    //加载文件
    if( load_files() == false )
    {
        return 1;    
    }

    //将image表面应用到窗口上
    apply_surface( 0, 0, image, screen );
    
    //更新窗口
    if( SDL_Flip( screen ) == -1 )
    {
        return 1;    
    }


    while(quit ==false)
    {
        while( SDL_PollEvent( &event ) )
        {
            if( event.type == SDL_QUIT )
            {
                //退出程序
                quit = true;
            }    
        }
    }
     

    clean_up();
        
    return 0; 

}

bool init()
{
    //初始化SDL的所有子系统
    if( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )
    {
        return false;    
    }
    
    //设置窗口
    screen = SDL_SetVideoMode( SCREEN_WIDTH*2, SCREEN_HEIGHT*2, SCREEN_BPP, SDL_SWSURFACE );
    
    //如果设置出错
    if( screen == NULL )
    {
        return false;    
    }
    
    //设置窗口标题
    SDL_WM_SetCaption( "Event test", NULL );
    
    //如果所有初始化操作都成功
    return true;
}

SDL_Surface *load_image( std::string filename ) 
{
    //存储加载好的图像
    SDL_Surface* loadedImage = NULL;
    
    //存储优化后的图像
    SDL_Surface* optimizedImage = NULL;
    
    //加载图像
    loadedImage = IMG_Load( filename.c_str() );
    
    //如果加载成功
    if( loadedImage != NULL )
    {
        //创建优化的图像
        optimizedImage = SDL_DisplayFormat( loadedImage );
        
        //释放原先加载的图像
        SDL_FreeSurface( loadedImage );
    }
    
    //返回优化的图像
    return optimizedImage;
}
 
void apply_surface( int x, int y, SDL_Surface* source, SDL_Surface* destination )
{
    //存储偏移量的临时矩形
    SDL_Rect offset;
    
    //存入偏移量
    offset.x = x;
    offset.y = y;
    
    //执行blit操作
    SDL_BlitSurface( source, NULL, destination, &offset );//NULL 表示为全图像，有参数则为坐标
}

bool load_files()
{
    //加载图像
    image = load_image( "Demo-2.jpg" );
    
    //如果加载出错
    if( image == NULL )
    {
        return false;    
    }
    
    //如果所有图片加载正常
    return true;    
}

void clean_up()
{
    //释放图像
    SDL_FreeSurface( image );
    
    //退出SDL
    SDL_Quit();    
}