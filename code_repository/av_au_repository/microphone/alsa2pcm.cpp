

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
}

#include <iostream>
#include <unistd.h>
#include <sys/time.h>

using namespace std;

#define recording_time 2

//g++ alsa2pcm.cpp -o alsa2pcm  -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale

int main ()
{

    av_register_all();
    avformat_network_init();
    avdevice_register_all();

    AVFormatContext	*pFormatCtx;
    int				i, audioindex;
    AVCodecContext	*aCodecCtx;
    AVFrame	*aFrame;
    uint8_t *out_buffer;
    bool m_isRun;
    bool m_pause;
    int ret, got_frame;

    AVCodec	*aCodec = NULL;

    FILE *pcmFp = fopen("out.pcm","wb");

    pFormatCtx = avformat_alloc_context();

    AVInputFormat *ifmt = av_find_input_format("alsa");

    if(avformat_open_input(&pFormatCtx,NULL,ifmt,NULL)!=0){
        fprintf(stderr,"Couldn't open input stream audio.\n");
        return -1;
    }

    audioindex = -1;
    aCodecCtx = NULL;


    for(i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO)
        {
            audioindex=i;
            break;
        }
    if(audioindex==-1)
    {
        printf("Didn't find a video stream.\n");
        return -1;
    }

    aCodecCtx = pFormatCtx->streams[audioindex]->codec;
    aCodec = avcodec_find_decoder(aCodecCtx->codec_id);

    if(aCodec == NULL)
    {
        printf("audio Codec not found.\n");
        return -1;
    }

    if(avcodec_open2(aCodecCtx, aCodec,NULL)<0)
    {
        printf("Could not open video codec.\n");
        return -1;
    }

    aFrame = av_frame_alloc();

    AVPacket *packet=(AVPacket *)av_malloc(sizeof(AVPacket));

    struct tm nowtime;
    int buf =0;
    //struct timeval tv;
    //unsigned char time_now[128];
    time_t time_seconds = time(0);
    //gettimeofday(&tv, NULL);
    localtime_r(&time_seconds,&nowtime);
    buf =nowtime.tm_sec;
    cout <<nowtime.tm_sec <<endl;

    while(1)
    {
        time_t time_seconds1 = time(0);
        struct tm nowtime1;
        localtime_r(&time_seconds1,&nowtime1);
        if(nowtime1.tm_sec ==buf +recording_time || nowtime1.tm_sec +60 ==buf +recording_time)
        {
            cout <<"time out " <<endl;
            break;
        }
        //else
        //{
         //   cout <<nowtime1.tm_sec <<"sss"<<endl;
        //}
        

        if (av_read_frame(pFormatCtx, packet)<0)
        {
            cout <<"read failed!" <<endl;
            sleep(1);
            continue;
        }

        if(packet->stream_index == audioindex)
        {

            ret = avcodec_decode_audio4(aCodecCtx, aFrame, &got_frame, packet);

            if(ret < 0)
            {
                fprintf(stderr,"video Audio Error.\n");
                return -1;
            }

            if (got_frame)
            {


                int framSize = av_samples_get_buffer_size(NULL,aCodecCtx->channels, aFrame->nb_samples,aCodecCtx->sample_fmt, 1);

                uint8_t * audio_buf = (uint8_t *)malloc(framSize);
                memcpy(audio_buf, aFrame->data[0], framSize);
                

                fwrite(audio_buf,1,framSize,pcmFp);
            }
        }
        else
        {
            cout <<"other"<<packet->stream_index <<endl;
        }

    }
    
    //cout <<"ending" <<endl;
//释放内存时，出现段错误和莫名卡死，现全注释
    if (0)
    {
        av_free(out_buffer);
        out_buffer = NULL;
    }

    if (0)
    {
        av_free(aFrame);
        aFrame = NULL;
    }

    //avformat_close_input(&pFo  rmatCtx);

    //avformat_free_context(pFormatCtx);
    fclose(pcmFp);

    cout <<"ending" <<endl;

    return 0;

}


