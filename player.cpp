#include <stdio.h>
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

#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIO_SIZE (25 * 16 * 1024)
#define MAX_VIDEO_SIZE (25 * 256 * 1024)
#define sjk_debug 1

SDL_Event event;

SDL_Window *screen; 
SDL_Renderer* sdlRenderer;
SDL_Texture* sdlTexture;
SDL_Rect sdlRect;

typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;


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


    //VideoPlayer *player; //记录下这个类的指针  主要用于在线程里面调用激发信号的函数

} VideoState;



int audio_stream_component_open(VideoState *is, int stream_index);
void packet_queue_init(PacketQueue *q);
int video_thread(void *arg);
int packet_queue_put(PacketQueue *q, AVPacket *pkt);

int main(int argc, char* argv[])
{

    VideoState is;

	char *filename =argv[1];

    av_register_all(); 


    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {  
		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
		return -1;
	} 

    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;

    AVCodecContext *aCodecCtx;
    AVCodec *aCodec;

    int audioStream ,videoStream, i;

    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
        printf("can't open the file. \n");
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        printf("Could't find stream infomation.\n");
        return -1;
    }

    videoStream = -1;
    audioStream = -1;

    ///循环查找视频中包含的流信息，
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = i;
        }
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO  && audioStream < 0)
        {
            audioStream = i;
        }
    }

    if (videoStream == -1) {
        printf("Didn't find a video stream.\n");
        return 0;
    }

    if (audioStream == -1) {
        printf("Didn't find a audio stream.\n");
        return 0;
    }

    is.ic = pFormatCtx;

    if (audioStream >= 0) {
        /* 所有设置SDL音频流信息的步骤都在这个函数里完成 */
        //sjk 设置了SDL参数，callback函数等等
        audio_stream_component_open(&is, audioStream);
    }

    ///查找音频解码器
    aCodecCtx = pFormatCtx->streams[audioStream]->codec;
    aCodec = avcodec_find_decoder(aCodecCtx->codec_id);

    if (aCodec == NULL) {
        printf("ACodec not found.\n");
        return 0;
    }

    ///打开音频解码器
    if (avcodec_open2(aCodecCtx, aCodec, NULL) < 0) {
        printf("Could not open audio codec.\n");
        return 0; 
    }

    is.audio_st = pFormatCtx->streams[audioStream];

    ///查找视频解码器
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    if(pCodecCtx ==NULL)
    {
        printf("pCodecCtx ERROR\N");
        return 0;
    }
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

    if (pCodec == NULL) {
        printf("PCodec not found.\n");
        return 0;
    }

   
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open video codec.\n");
        return 0;
    }

    is.video_st = pFormatCtx->streams[videoStream];
    packet_queue_init(&is.videoq);


    ///创建一个线程专门用来解码视频
    is.video_tid = SDL_CreateThread(video_thread, "video_thread", &is);

    AVPacket *packet = (AVPacket *) malloc(sizeof(AVPacket)); //分配一个packet 用来存放读取的视频

    av_dump_format(pFormatCtx, 0, filename, 0); //输出视频信息

     while (1)
    {

        //If there's an event to handle
        if( SDL_PollEvent( &event ) )
        {
            if( event.type == SDL_QUIT )break;
        }

        if(sjk_debug)cout <<"test_sjk" <<endl;
        //-------------------------- -->sjk
                    //qDebug("%d ",sjk_nu);
                    //sjk_nu++;

        //这里做了个限制  当队列里面的数据超过某个大小的时候 就暂停读取  防止一下子就把视频读完了，导致的空间分配不足
        /* 这里audioq.size是指队列中的所有数据包带的音频数据的总量或者视频数据总量，并不是包的数量 */
        //这个值可以稍微写大一些
        if (is.audioq.size > MAX_AUDIO_SIZE || is.videoq.size > MAX_VIDEO_SIZE) {
            SDL_Delay(10);
            continue;
            if(sjk_debug)cout <<"delay" <<endl;
        }

        if (av_read_frame(pFormatCtx, packet) < 0)
        {
            cout <<"over_read" <<endl;
            break; //这里认为视频读取完了
        }

        if (packet->stream_index == videoStream)
        {
            packet_queue_put(&is.videoq, packet);
            //这里我们将数据存入队列 因此不调用 av_free_packet 释放
        }
        else if( packet->stream_index == audioStream )
        {
            packet_queue_put(&is.audioq, packet);
            //这里我们将数据存入队列 因此不调用 av_free_packet 释放
        }
        else
        {
            // Free the packet that was allocated by av_read_frame
            av_free_packet(packet);
        }

        //SDL_Delay(2000);
    }



    return 0;
    
}


void packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
    q->size = 0;
    q->nb_packets = 0;
    q->first_pkt = NULL;
    q->last_pkt = NULL;
}


int packet_queue_put(PacketQueue *q, AVPacket *pkt) {

    AVPacketList *pkt1;
    if (av_dup_packet(pkt) < 0) {
        return -1;
    }
    pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;

    SDL_LockMutex(q->mutex);

    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size;
    SDL_CondSignal(q->cond);

    SDL_UnlockMutex(q->mutex);
    return 0;
}

static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block) {
    AVPacketList *pkt1;
    int ret;

    SDL_LockMutex(q->mutex);

    for (;;) {

        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            SDL_CondWait(q->cond, q->mutex);
        }

    }

    SDL_UnlockMutex(q->mutex);
    return ret;
}

static int audio_decode_frame(VideoState *is, double *pts_ptr)
{
    int len1, len2, decoded_data_size;
    AVPacket *pkt = &is->audio_pkt;
    int got_frame = 0;
    int64_t dec_channel_layout;
    int wanted_nb_samples, resampled_data_size, n;



    double pts;

    for (;;) {

        while (is->audio_pkt_size > 0) {

//            if (is->isPause == true)
//            {
//                SDL_Delay(10);
//                continue;
//            }

            if (!is->audio_frame) {
                if (!(is->audio_frame = av_frame_alloc())) {
                    return AVERROR(ENOMEM);
                }
            } else
                av_frame_unref(is->audio_frame);


            len1 = avcodec_decode_audio4(is->audio_st->codec, is->audio_frame,
                    &got_frame, pkt);
            if (len1 < 0) {
                // error, skip the frame
    //-------------------------- -->sjk
                
                is->audio_pkt_size = 0;


                break;
            }

            is->audio_pkt_data += len1;
            is->audio_pkt_size -= len1;

            if (!got_frame)
                continue;

            /* 计算解码出来的桢需要的缓冲大小 */
            decoded_data_size = av_samples_get_buffer_size(NULL,
                    is->audio_frame->channels, is->audio_frame->nb_samples,
                    (AVSampleFormat)is->audio_frame->format, 1);

            dec_channel_layout =
                    (is->audio_frame->channel_layout
                            && is->audio_frame->channels
                                    == av_get_channel_layout_nb_channels(
                                            is->audio_frame->channel_layout)) ?
                            is->audio_frame->channel_layout :
                            av_get_default_channel_layout(
                                    is->audio_frame->channels);

            wanted_nb_samples = is->audio_frame->nb_samples;

            if (is->audio_frame->format != is->audio_src_fmt
                    || dec_channel_layout != is->audio_src_channel_layout
                    || is->audio_frame->sample_rate != is->audio_src_freq
                    || (wanted_nb_samples != is->audio_frame->nb_samples
                            && !is->swr_ctx)) {
                if (is->swr_ctx)
                    swr_free(&is->swr_ctx);
                is->swr_ctx = swr_alloc_set_opts(NULL,
                        is->audio_tgt_channel_layout, (AVSampleFormat)is->audio_tgt_fmt,
                        is->audio_tgt_freq, dec_channel_layout,
                        (AVSampleFormat)is->audio_frame->format, is->audio_frame->sample_rate,
                        0, NULL);
                if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
                    //fprintf(stderr,"swr_init() failed\n");
                    break;
                }
                is->audio_src_channel_layout = dec_channel_layout;
                is->audio_src_channels = is->audio_st->codec->channels;
                is->audio_src_freq = is->audio_st->codec->sample_rate;
                is->audio_src_fmt = is->audio_st->codec->sample_fmt;
            }

            /* 这里我们可以对采样数进行调整，增加或者减少，一般可以用来做声画同步 */
            if (is->swr_ctx) {
                const uint8_t **in =
                        (const uint8_t **) is->audio_frame->extended_data;
                uint8_t *out[] = { is->audio_buf2 };
                if (wanted_nb_samples != is->audio_frame->nb_samples) {
                    if (swr_set_compensation(is->swr_ctx,
                            (wanted_nb_samples - is->audio_frame->nb_samples)
                                    * is->audio_tgt_freq
                                    / is->audio_frame->sample_rate,
                            wanted_nb_samples * is->audio_tgt_freq
                                    / is->audio_frame->sample_rate) < 0) {
                        //fprintf(stderr,"swr_set_compensation() failed\n");
                        break;
                    }
                }

                len2 = swr_convert(is->swr_ctx, out,
                        sizeof(is->audio_buf2) / is->audio_tgt_channels
                                / av_get_bytes_per_sample(is->audio_tgt_fmt),
                        in, is->audio_frame->nb_samples);
                if (len2 < 0) {
                    //fprintf(stderr,"swr_convert() failed\n");
                    break;
                }
                if (len2
                        == sizeof(is->audio_buf2) / is->audio_tgt_channels
                                / av_get_bytes_per_sample(is->audio_tgt_fmt)) {
                    //fprintf(stderr,"warning: audio buffer is probably too small\n");
                    swr_init(is->swr_ctx);
                }
                is->audio_buf = is->audio_buf2;
                resampled_data_size = len2 * is->audio_tgt_channels
                        * av_get_bytes_per_sample(is->audio_tgt_fmt);
            } else {
                resampled_data_size = decoded_data_size;
                is->audio_buf = is->audio_frame->data[0];
            }

            pts = is->audio_clock;
            *pts_ptr = pts;
            n = 2 * is->audio_st->codec->channels;
            is->audio_clock += (double) resampled_data_size
                    / (double) (n * is->audio_st->codec->sample_rate);

            // We have data, return it and come back for more later
            return resampled_data_size;
        }

//        if (is->isPause == true)
//        {
//            SDL_Delay(10);
//            continue;
//        }

        if (pkt->data)
            av_free_packet(pkt);
        memset(pkt, 0, sizeof(*pkt));
//        if (is->quit)
//            return -1;
        if (packet_queue_get(&is->audioq, pkt, 0) <= 0)
            return -1;

//        if(pkt->data == is->flush_pkt.data) {
////fprintf(stderr,"avcodec_flush_buffers(is->audio...\n");
//        avcodec_flush_buffers(is->audio_st->codec);
////        fprintf(stderr,"avcodec_flush_buffers(is->audio 222\n");

//        continue;

//        }

        is->audio_pkt_data = pkt->data;
        is->audio_pkt_size = pkt->size;

        /* if update, update the audio clock w/pts */
        if (pkt->pts != AV_NOPTS_VALUE) {
            is->audio_clock = av_q2d(is->audio_st->time_base) * pkt->pts;
        }
    }

    return 0;
}

static void audio_callback(void *userdata, Uint8 *stream, int len) {
    VideoState *is = (VideoState *) userdata;

    if(sjk_debug)cout <<"audio_callback" <<endl;
    //SDL_Delay(1000);

    int len1, audio_data_size;

    double pts;

    /*   len是由SDL传入的SDL缓冲区的大小，如果这个缓冲未满，我们就一直往里填充数据 */
    while (len > 0) {
        /*  audio_buf_index 和 audio_buf_size 标示我们自己用来放置解码出来的数据的缓冲区，*/
        /*   这些数据待copy到SDL缓冲区， 当audio_buf_index >= audio_buf_size的时候意味着我*/
        /*   们的缓冲为空，没有数据可供copy，这时候需要调用audio_decode_frame来解码出更
         /*   多的桢数据 */

        if (is->audio_buf_index >= is->audio_buf_size) {
            audio_data_size = audio_decode_frame(is, &pts);
            /* audio_data_size < 0 标示没能解码出数据，我们默认播放静音 */
            if (audio_data_size < 0) {
                /* silence */
                is->audio_buf_size = 1024;
                /* 清零，静音 */
                memset(is->audio_buf, 0, is->audio_buf_size);
            } else {
                is->audio_buf_size = audio_data_size;
            }
            is->audio_buf_index = 0;
        }


        /*  查看stream可用空间，决定一次copy多少数据，剩下的下次继续copy */
        len1 = is->audio_buf_size - is->audio_buf_index;
        if (len1 > len) {
            len1 = len;
        }

        memcpy(stream, (uint8_t *) is->audio_buf + is->audio_buf_index, len1);//(target,resource,size)
//        SDL_MixAudio(stream, (uint8_t * )is->audio_buf + is->audio_buf_index, len1, 50);

//        SDL_MixAudioFormat(stream, (uint8_t * )is->audio_buf + is->audio_buf_index, AUDIO_S16SYS, len1, 50);


        len -= len1;
        stream += len1;
        is->audio_buf_index += len1;
    }
}


int audio_stream_component_open(VideoState *is, int stream_index)
{
    AVFormatContext *ic = is->ic;
    AVCodecContext *codecCtx;
    AVCodec *codec;
    SDL_AudioSpec wanted_spec, spec;
    int64_t wanted_channel_layout = 0;
    int wanted_nb_channels;
    /*  SDL支持的声道数为 1, 2, 4, 6 */
    /*  后面我们会使用这个数组来纠正不支持的声道数目 */
    const int next_nb_channels[] = { 0, 0, 1, 6, 2, 6, 4, 6 };

    if (stream_index < 0 || stream_index >= ic->nb_streams) {
        return -1;
    }
        // sjk_ 不懂
    codecCtx = ic->streams[stream_index]->codec;
    wanted_nb_channels = codecCtx->channels;
    if (!wanted_channel_layout
            || wanted_nb_channels
                    != av_get_channel_layout_nb_channels(
                            wanted_channel_layout)) {
        wanted_channel_layout = av_get_default_channel_layout(
                wanted_nb_channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }

    wanted_spec.channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
    wanted_spec.freq = codecCtx->sample_rate;
    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
        fprintf(stderr,"Invalid sample rate or channel count!\n");
        return -1;
    }
    wanted_spec.format = AUDIO_S16SYS; // 具体含义请查看“SDL宏定义”部分
    wanted_spec.silence = 0;            // 0指示静音
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;  // 自定义SDL缓冲区大小
    wanted_spec.callback = audio_callback;        // 音频解码的关键回调函数
    wanted_spec.userdata = is;                    // 传给上面回调函数的外带数据


    /*  打开音频设备，这里使用一个while来循环尝试打开不同的声道数(由上面 */
    /*  next_nb_channels数组指定）直到成功打开，或者全部失败 */
//    while (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
    do {

        is->audioID = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0,0),0,&wanted_spec, &spec,0);

;

//        if (audioID >= 1) break;

        fprintf(stderr,"SDL_OpenAudio (%d channels): %s\n",wanted_spec.channels, SDL_GetError());
        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
        if (!wanted_spec.channels) {
            fprintf(stderr,"No more channel combinations to tyu, audio open failed\n");
//            return -1;
            break;
        }
        wanted_channel_layout = av_get_default_channel_layout(
                wanted_spec.channels);
    }while(is->audioID == 0);

    /* 检查实际使用的配置（保存在spec,由SDL_OpenAudio()填充） */
    if (spec.format != AUDIO_S16SYS) {
        fprintf(stderr,"SDL advised audio format %d is not supported!\n",spec.format);
        return -1;
    }

    if (spec.channels != wanted_spec.channels) {
        wanted_channel_layout = av_get_default_channel_layout(spec.channels);
        if (!wanted_channel_layout) {
            fprintf(stderr,"SDL advised channel count %d is not supported!\n",spec.channels);
            return -1;
        }
    }

    is->audio_hw_buf_size = spec.size;

    /* 把设置好的参数保存到大结构中 */
    is->audio_src_fmt = is->audio_tgt_fmt = AV_SAMPLE_FMT_S16;
    is->audio_src_freq = is->audio_tgt_freq = spec.freq;
    is->audio_src_channel_layout = is->audio_tgt_channel_layout =
            wanted_channel_layout;
    is->audio_src_channels = is->audio_tgt_channels = spec.channels;

    codec = avcodec_find_decoder(codecCtx->codec_id);
    if (!codec || (avcodec_open2(codecCtx, codec, NULL) < 0)) {
        fprintf(stderr,"Unsupported codec!\n");
        return -1;
    }
    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    switch (codecCtx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
//        is->audioStream = stream_index;
        is->audio_st = ic->streams[stream_index];
        is->audio_buf_size = 0;
        is->audio_buf_index = 0;
        memset(&is->audio_pkt, 0, sizeof(is->audio_pkt));
        packet_queue_init(&is->audioq);
//        SDL_PauseAudio(0); // 开始播放静音
        SDL_PauseAudioDevice(is->audioID,0);
        break;
    default:
        break;
    }

    return 0;
}


static double synchronize_video(VideoState *is, AVFrame *src_frame, double pts) {

    double frame_delay;

    if (pts != 0) {
        /* if we have pts, set video clock to it */
        is->video_clock = pts;
    } else {
        /* if we aren't given a pts, set it to the clock */
        pts = is->video_clock;
    }
    /* update the video clock */
    frame_delay = av_q2d(is->video_st->codec->time_base);
    /* if we are repeating a frame, adjust clock accordingly */
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
    is->video_clock += frame_delay;
    return pts;
}

int video_thread(void *arg)
{



    VideoState *is = (VideoState *) arg;
    AVCodecContext *pCodecCtx = is->video_st->codec; //视频解码器
    AVPacket pkt1, *packet = &pkt1;

    int ret, got_picture, numBytes;

    double video_pts = 0; //当前视频的pts
    double audio_pts = 0; //音频pts

//--------------------------------SDL
    int screen_w=0,screen_h=0;


        screen_w = pCodecCtx->width;
        screen_h = pCodecCtx->height;
        cout <<"w and h------" <<screen_w <<" "<<screen_h <<endl;
        //SDL 2.0 Support for multiple windows
        screen = SDL_CreateWindow("Simplest ffmpeg player's Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            screen_w, screen_h,
            SDL_WINDOW_OPENGL);

        if(!screen) {
            printf("SDL: could not create window - exiting:%s\n",SDL_GetError());
            return -1;
        }

        sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
        //IYUV: Y + U + V  (3 planes)
        //YV12: Y + V + U  (3 planes)
        sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,pCodecCtx->width,pCodecCtx->height);

        sdlRect.x=0;
        sdlRect.y=0;
        sdlRect.w=screen_w;
        sdlRect.h=screen_h;

//--------------------------------SDL
    ///解码视频相关
    AVFrame *pFrame, *pFrameYUV;
    uint8_t *out_buffer_rgb; //解码后的rgb数据
    struct SwsContext *img_convert_ctx;  //用于解码后的视频格式转换



    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();


    ///这里我们改成了 将解码后的YUV数据转换成RGB32
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
            pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
            AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);



    numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width,pCodecCtx->height);


    out_buffer_rgb = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    avpicture_fill((AVPicture *) pFrameYUV, out_buffer_rgb, AV_PIX_FMT_YUV420P,
            pCodecCtx->width, pCodecCtx->height);



    while(1)
    {



        if (packet_queue_get(&is->videoq, packet, 1) <= 0)
        {

            //qDebug("-------------------vedio_over\n");
            cout<<"-------------------vedio_over" <<endl;
            break;//队列里面没有数据了  读取完毕了
        }


        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,packet);

       if (ret < 0) {
           //printf("decode error.\n");

           //-------------------------- -->sjk
                      
            cout<<"-------------------vedio_error" <<endl;

            return -2;
       }


//设置视频pts，具体怎么运作不知 -------》sjk
        if (packet->dts == AV_NOPTS_VALUE && pFrame->opaque&& *(uint64_t*) pFrame->opaque != AV_NOPTS_VALUE)
        {
            video_pts = *(uint64_t *) pFrame->opaque;
        }
        else if (packet->dts != AV_NOPTS_VALUE)
        {
            video_pts = packet->dts;
        }
        else
        {
            video_pts = 0;
        }



        video_pts *= av_q2d(is->video_st->time_base);
        video_pts = synchronize_video(is, pFrame, video_pts);
//——————————————————————————《

        while(1)
        {


            audio_pts = is->audio_clock;
            if (video_pts <= audio_pts) break;

            int delayTime = (video_pts - audio_pts) * 1000;

            delayTime = delayTime > 5 ? 5:delayTime;

            SDL_Delay(delayTime);
        }

        if (got_picture) {
            sws_scale(img_convert_ctx,
                    (uint8_t const * const *) pFrame->data,
                    pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data,
                    pFrameYUV->linesize);

            SDL_UpdateYUVTexture(sdlTexture, &sdlRect,
                        pFrameYUV->data[0], pFrameYUV->linesize[0],
                        pFrameYUV->data[1], pFrameYUV->linesize[1],
                        pFrameYUV->data[2], pFrameYUV->linesize[2]);
            /*
                        //把这个RGB数据 用QImage加载
                        QImage tmpImg((uchar *)out_buffer_rgb,pCodecCtx->width,pCodecCtx->height,QImage::Format_RGB32);
                        QImage image = tmpImg.copy(); //把图像复制一份 传递给界面显示
                        is->player->disPlayVideo(image); //调用激发信号的函数
                        */
                        SDL_RenderClear( sdlRenderer );
                        SDL_RenderCopy( sdlRenderer, sdlTexture,  NULL, &sdlRect);
                        SDL_RenderPresent( sdlRenderer );

/*
            //把这个RGB数据 用QImage加载
            QImage tmpImg((uchar *)out_buffer_rgb,pCodecCtx->width,pCodecCtx->height,QImage::Format_RGB32);
            QImage image = tmpImg.copy(); //把图像复制一份 传递给界面显示
            is->player->disPlayVideo(image); //调用激发信号的函数
  */
        }



        av_free_packet(packet);

    }

    av_free(pFrame);
    av_free(pFrameYUV);
    av_free(out_buffer_rgb);

    return 0;
}