/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <iostream>

using namespace std;

extern "C"
{


#include <SDL2/SDL.h>
#include <libavutil/imgutils.h>

#include </home/resource_sjk/ffmpeg/include/libavcodec/avcodec.h>
#include </home/resource_sjk/ffmpeg/include/libavformat/avformat.h>
#include </home/resource_sjk/ffmpeg/include/libavutil/time.h>
#include </home/resource_sjk/ffmpeg/include/libavutil/pixfmt.h>
#include </home/resource_sjk/ffmpeg/include/libswscale/swscale.h>
#include </home/resource_sjk/ffmpeg/include/libswresample/swresample.h>

#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_types.h>
#include <SDL2/SDL_name.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_config.h>

};

typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

#define VIDEO_PICTURE_QUEUE_SIZE 1
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

#define MAX_AUDIO_SIZE (25 * 16 * 1024)
#define MAX_VIDEO_SIZE (25 * 256 * 1024)

class VideoPlayer; //前置声明

typedef struct VideoState {
    AVFormatContext *ic;
    AVFrame *audio_frame;// 解码音频过程中的使用缓存
    PacketQueue audioq;
    AVStream *audio_st; //音频流
    unsigned int audio_buf_size;
    unsigned int audio_buf_index;
    AVPacket audio_pkt;
    uint8_t *audio_pkt_data;
    int audio_pkt_size;
    uint8_t *audio_buf;
    DECLARE_ALIGNED(16,uint8_t,audio_buf2) [AVCODEC_MAX_AUDIO_FRAME_SIZE * 4];
    enum AVSampleFormat audio_src_fmt;
    enum AVSampleFormat audio_tgt_fmt;
    int audio_src_channels;
    int audio_tgt_channels;
    int64_t audio_src_channel_layout;
    int64_t audio_tgt_channel_layout;
    int audio_src_freq;
    int audio_tgt_freq;
    struct SwrContext *swr_ctx; //用于解码后的音频格式转换
    int audio_hw_buf_size;

    double audio_clock; ///音频时钟
    double video_clock; ///<pts of last decoded frame / predicted pts of next decoded frame

    AVStream *video_st;
    PacketQueue videoq;


    SDL_Thread *video_tid;  //视频线程id
    //SDL_AudioDeviceID audioID; //该变量出现does no name a type 问题，
                                //这个问题出现的原因是包含了没有经过定义的数据结构，
                                //但是在此得不到体现，故直接按定义代替注释掉 --sjk
    Uint32 audioID; //--sjk


    VideoPlayer *player; //记录下这个类的指针  主要用于在线程里面调用激发信号的函数

} VideoState;

class VideoPlayer 
{
    

public:

    char *inp_file;

    void play();

    VideoState mVideoState;

};

#endif // VIDEOPLAYER_H
