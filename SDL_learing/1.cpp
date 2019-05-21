#include "/usr/include/SDL/SDL.h"

int main( int argc, char* args[] )
{

        //声明表面
    SDL_Surface* hello = NULL;
    SDL_Surface* screen = NULL; //一个SDL_Surface是一张图像


    SDL_Init( SDL_INIT_EVERYTHING );//调用这个函数能初始化SDL的所有子系统
 
    //设置窗口
    //建立一个640像素宽、480像素高的32位（位/像素）窗口
    //其中最后一个参数SDL_SWSURFACE指定将表面存放在软件内存中。 该函数执行完毕后，返回了一个指向新建的窗口表面的指针，随后我们可以使用它。
    screen = SDL_SetVideoMode( 576, 1024, 32, SDL_SWSURFACE );
 
    //加载图像,///应该是加载到内存
    hello = SDL_LoadBMP( "Demo-4.bmp" );
     
    //将图像应用到窗口上 ，把加载好的图像应用到窗口上。 
    //中第一个参数是源表面，第三个参数是目的表面，它的功能是将源表面粘贴到目的表面上。
    SDL_BlitSurface( hello, NULL, screen, NULL );

    //更新窗口
    SDL_Flip( screen );
 
    //暂停
    SDL_Delay( 2000 );

        //释放已加载的图像
    SDL_FreeSurface( hello );
 
    //退出SDL
    SDL_Quit();
 
    return 0;

}