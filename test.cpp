
 #include <iostream>

 using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <math.h>
#include <unistd.h>

#include <pthread.h>
 
extern "C" {  
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
 
}  
 
#define STREAM_DURATION   10.0
#define STREAM_FRAME_RATE 25 /* 每秒25帧*/
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* 默认的像素格式 */
 
#define SCALE_FLAGS SWS_BICUBIC

#define WIDTH 576
#define HEIGHT 1024
 
char tempts[AV_TS_MAX_STRING_SIZE] = {0};
char temperro[AV_ERROR_MAX_STRING_SIZE]={0};
 
char * av_ts2str_f(int64_t ts)
{
	return av_ts_make_string(tempts, ts);
}
 
char* av_ts2timestr_f(int64_t ts, AVRational *tb) 
{	
	return av_ts_make_time_string(tempts, ts, tb);
}
 
char* av_err2str_f(int64_t errnum)
{
	return av_make_error_string(temperro, AV_ERROR_MAX_STRING_SIZE, errnum);
}
 
 
class node_sjk
{
public:
    char * buffer;
    int bufferSize;
    node_sjk* next;

};

class Quene_sjk
{
public:

    pthread_mutex_t number_mutex;
    node_sjk *DataQueneHead;
    node_sjk *DataQueneTail;

    int node_size;

    Quene_sjk();

};

Quene_sjk::Quene_sjk()
{
    pthread_mutex_init(&number_mutex,NULL);

    DataQueneHead =NULL;
    DataQueneTail =NULL;

    node_size =0;
}



void DataQuene_Input(char * buffer,int size,Quene_sjk *quene)
{
    node_sjk * node = (node_sjk*)malloc(sizeof(node_sjk));
    node->buffer = (char *)malloc(size);
    node->bufferSize = size;
    node->next = NULL;

    memcpy(node->buffer,buffer,size);

    pthread_mutex_lock(&quene->number_mutex);

    if (quene->DataQueneHead == NULL)
    {
        quene->DataQueneHead = node;
    }
    else
    {
        quene->DataQueneTail->next = node;
    }

    quene->DataQueneTail = node;

    quene->node_size++;

    pthread_mutex_unlock(&quene->number_mutex);
}


node_sjk *DataQuene_get(Quene_sjk *quene)
{
    node_sjk * node = NULL;

    pthread_mutex_lock(&quene->number_mutex);

    if (quene->DataQueneHead != NULL)
    {
        node = quene->DataQueneHead;

        if (quene->DataQueneTail == quene->DataQueneHead)
        {
            quene->DataQueneTail = NULL;
        }
        quene->node_size--;

        quene->DataQueneHead = quene->DataQueneHead->next;
    }

    pthread_mutex_unlock(&quene->number_mutex);

    return node;
}

Quene_sjk Quene_video;
Quene_sjk Quene_audio;

// 单个输出流的包装器
typedef struct OutputStream {
	AVStream *st;
	AVCodecContext *enc;
 
	/* 将要产生的下一帧的播放时间戳 */
	int64_t next_pts;
	int samples_count;
 
	AVFrame *frame;
	AVFrame *tmp_frame;
 
	float t, tincr, tincr2;
 
	struct SwsContext *sws_ctx;
	struct SwrContext *swr_ctx;
} OutputStream;
 


static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;
 
    printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           av_ts2str_f(pkt->pts), av_ts2timestr_f(pkt->pts, time_base),
           av_ts2str_f(pkt->dts), av_ts2timestr_f(pkt->dts, time_base),
           av_ts2str_f(pkt->duration), av_ts2timestr_f(pkt->duration, time_base),
           pkt->stream_index);
}
 
static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
    /* 调节输出包的时间基，从编码器时间基到流时间基 */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;
 
    /* 写入压缩后的帧到输出文件. */
    log_packet(fmt_ctx, pkt);
    return av_interleaved_write_frame(fmt_ctx, pkt);
}
//
///* 添加一个输出流. */
static void add_stream(OutputStream *ost, AVFormatContext *oc,
                       AVCodec **codec,
                       enum AVCodecID codec_id)
{
    AVCodecContext *c;
    int i;
 
    /* 查找编码器 */
    if(86018 ==codec_id) //codec_id 为aac时，换成libfdk_aac，很烦
    {
        *codec = avcodec_find_encoder_by_name("libfdk_aac"); //--SJK
        codec_id =(*codec)->id;
    }
    else
    {
        *codec = avcodec_find_encoder(codec_id);
    }
    
    
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        exit(1);
    }
 
    ost->st = avformat_new_stream(oc, NULL);
    if (!ost->st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    ost->st->id = oc->nb_streams-1;
    c = avcodec_alloc_context3(*codec);
    if (!c) {
        fprintf(stderr, "Could not alloc an encoding context\n");
        exit(1);
    }
    ost->enc = c;
 
    switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
    /*
        c->sample_fmt  = (*codec)->sample_fmts ?
            (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
*/
        c->sample_fmt =AV_SAMPLE_FMT_S16;
        c->channel_layout = AV_CH_LAYOUT_STEREO;
        




        c->bit_rate    = 64000;
        c->sample_rate = 44100;
        
        if ((*codec)->supported_samplerates) {
            c->sample_rate = (*codec)->supported_samplerates[0];
            for (i = 0; (*codec)->supported_samplerates[i]; i++) {
                if ((*codec)->supported_samplerates[i] == 44100)
                    c->sample_rate = 44100;
            }
        }
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        c->channel_layout = AV_CH_LAYOUT_STEREO;
        if ((*codec)->channel_layouts) {
            c->channel_layout = (*codec)->channel_layouts[0];
            for (i = 0; (*codec)->channel_layouts[i]; i++) {
                if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                    c->channel_layout = AV_CH_LAYOUT_STEREO;
            }
        }
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
		ost->st->time_base.num = 1;
		ost->st->time_base.den = c->sample_rate;
        break;
 
    case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;
 
        c->bit_rate = 400000;
        /* 设置要合成的视频源的分辨率 */
        c->width    = 1440;
        c->height   = 900;
        /* 设置每一帧的展示时间（时间基）. 对于固定帧率，时间基就是帧率的倒数 */
		ost->st->time_base.num = 1;
		ost->st->time_base.den = STREAM_FRAME_RATE;
        c->time_base       = ost->st->time_base;
 
        c->gop_size      = 12; /*每12帧一个关键帧 */
        c->pix_fmt       = STREAM_PIX_FMT;
        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            /* 测试放入B 帧 */
            c->max_b_frames = 2;
        }
        if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            /* 避免使用宏块.*/
            c->mb_decision = 2;
        }
    break;
 
    default:
        break;
    }
 
    /* 一些格式流的头分开放置 */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}
 
/**************************************************************/
/* 音频输出 */
 
static AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples)
{
    AVFrame *frame = av_frame_alloc();
    int ret;
 
    if (!frame) {
        fprintf(stderr, "Error allocating an audio frame\n");
        exit(1);
    }
 
    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;
 
    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            fprintf(stderr, "Error allocating an audio buffer\n");
            exit(1);
        }
    }
 
    return frame;
}
 
static void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    AVCodecContext *c;
    int nb_samples;
    int ret;
    AVDictionary *opt = NULL;
 
    c = ost->enc;
 
    /* 打开一片编码器 */
    av_dict_copy(&opt, opt_arg, 0);
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open audio codec: %s\n", av_err2str_f(ret));
        exit(1);
    }
 
    /* 初始化信号生成器 */
    ost->t     = 0;
    ost->tincr = 2 * M_PI * 110.0 / c->sample_rate;
    /* 每秒频率增量 */
    ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;
 
    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = c->frame_size;
 
    ost->frame     = alloc_audio_frame(c->sample_fmt, c->channel_layout,
                                       c->sample_rate, nb_samples);
    ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
                                       c->sample_rate, nb_samples);
 
    /* 拷贝流参数到合成器 */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }
 
    /* 创建重采样上下文 */
        ost->swr_ctx = swr_alloc();
        if (!ost->swr_ctx) {
            fprintf(stderr, "Could not allocate resampler context\n");
            exit(1);
        }
 
        /* 设置选项 */
        av_opt_set_int       (ost->swr_ctx, "in_channel_count",   c->channels,       0);
        av_opt_set_int       (ost->swr_ctx, "in_sample_rate",     c->sample_rate,    0);
        av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
        av_opt_set_int       (ost->swr_ctx, "out_channel_count",  c->channels,       0);
        av_opt_set_int       (ost->swr_ctx, "out_sample_rate",    c->sample_rate,    0);
        av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);
 
        /* 初始化重采样上下文 */
        if ((ret = swr_init(ost->swr_ctx)) < 0) {
            fprintf(stderr, "Failed to initialize the resampling context\n");
            exit(1);
        }
}
 
bool audio_statu =false;

/* 准备一个16位的音频帧，样本*/
static AVFrame *get_audio_frame(OutputStream *ost)
{
    AVFrame *frame = ost->tmp_frame;
    int j, i, v;

	node_sjk *audio_node;
	audio_node =DataQuene_get(&Quene_audio);
	if(audio_node ==NULL)
	{
		usleep(10);
		audio_node =DataQuene_get(&Quene_audio);
		if(audio_node ==NULL)
		{
			cout <<"get audio data error" <<endl;
			audio_statu =true;
			return (AVFrame *)-1;
		}
	}
		

    frame->data[0] =(uint8_t *)audio_node->buffer;
 
 /*
    // 检查是否要产生更多的帧 
	AVRational tempratio;
	tempratio.num = 1;
	tempratio.den = 1;
    if (av_compare_ts(ost->next_pts, ost->enc->time_base,
                      STREAM_DURATION, tempratio) >= 0)
        return NULL;
 
    for (j = 0; j <frame->nb_samples; j++) {
        v = (int)(sin(ost->t) * 10000);
        for (i = 0; i < ost->enc->channels; i++)
            *q++ = v;
        ost->t     += ost->tincr;
        ost->tincr += ost->tincr2;
    }
 */
    frame->pts = ost->next_pts;
    ost->next_pts  += frame->nb_samples;
 
    return frame;
}
 
/*
 * 编码一个音频帧，发送到合成器
 * 编码结束返回1, 其他返回0
 */
static int write_audio_frame(AVFormatContext *oc, OutputStream *ost)
{
    AVCodecContext *c;
    AVPacket pkt = { 0 }; // 初始化0;
    AVFrame *frame;
    int ret;
    int got_packet;
    int dst_nb_samples;
 
    av_init_packet(&pkt);
    c = ost->enc;
 
    frame = get_audio_frame(ost);
    if(audio_statu)return -1;
 
    if (frame) {
        /* 样本从初始格式转换为目标格式, 使用重采样器 */
            /* 计算目标的样本数*/
            dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
                                            c->sample_rate, c->sample_rate, AV_ROUND_UP);
            av_assert0(dst_nb_samples == frame->nb_samples);
 
        /* 当我们传递一帧给编码器时，可能内部有引用它，此时我们不能写入
         */
        ret = av_frame_make_writable(ost->frame);
        if (ret < 0)
            exit(1);
 
        /* 转换到目标格式 */
        ret = swr_convert(ost->swr_ctx,
                          ost->frame->data, dst_nb_samples,
                          (const uint8_t **)frame->data, frame->nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            exit(1);
        }
        frame = ost->frame;
		AVRational tempratio;
		tempratio.num = 1;
		tempratio.den = c->sample_rate;
        frame->pts = av_rescale_q(ost->samples_count, tempratio, c->time_base);
        ost->samples_count += dst_nb_samples;
    }
 
    ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        fprintf(stderr, "Error encoding audio frame: %s\n", av_err2str_f(ret));
        exit(1);
    }
 
    if (got_packet) {
        ret = write_frame(oc, &c->time_base, ost->st, &pkt);
        if (ret < 0) {
            fprintf(stderr, "Error while writing audio frame: %s\n",
                    av_err2str_f(ret));
            exit(1);
        }
    }
 
    return (frame || got_packet) ? 0 : 1;
}
 
/**************************************************************/
/* 视频输出 */
 
static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;
    int ret;
 
    picture = av_frame_alloc();
    if (!picture)
        return NULL;
 
    picture->format = pix_fmt;
    picture->width  = width;
    picture->height = height;
 
    /* 分配帧内存 */
    ret = av_frame_get_buffer(picture, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate frame data.\n");
        exit(1);
    }
 
    return picture;
}
 
static void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    int ret;
    AVCodecContext *c = ost->enc;
    AVDictionary *opt = NULL;
 
    av_dict_copy(&opt, opt_arg, 0);
 
    /* 打开视频编码器 */
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open video codec: %s\n", av_err2str_f(ret));
        exit(1);
    }
 
    /* 分配可复用的帧 */
    ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!ost->frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
 
    /* 如果输出格式不是YUV420P，那么一个也需要一个临时的YUV420P图片，随后可以把它转换为需要的输出格式 */
    ost->tmp_frame = NULL;
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
        if (!ost->tmp_frame) {
            fprintf(stderr, "Could not allocate temporary picture\n");
            exit(1);
        }
    }
 
    /* 拷贝流参数到合成器 */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }
}
 
bool video_statu =false;

/* 准备一帧测试图片. */
static void fill_yuv_image(AVFrame *pict, int frame_index,
                           int width, int height)
{
    int x, y, i;
 
	int y_size = width * height;
	cout <<"w and h" <<width <<height <<endl;

	node_sjk *video_node =NULL;
	video_node =DataQuene_get(&Quene_video);cout <<"sjk fill_yuv_image 1" <<endl;
	if(video_node ==NULL)
	{
		usleep(10);
		video_node =DataQuene_get(&Quene_video);
		if(video_node ==NULL)
		{
			cout <<"get video data error" <<endl;
			video_statu =true;
			return;
		}
	}
cout <<"sjk fill_yuv_image 1" <<endl;
	pict->data[0] =(uint8_t *)video_node->buffer;
	pict->data[1] =(uint8_t *)video_node->buffer +y_size;
	pict->data[2] =(uint8_t *)video_node->buffer +y_size*5/4;

   /*  i = frame_index;

    //
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;
 
    // Cb and Cr
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
	*/
}
 
static AVFrame *get_video_frame(OutputStream *ost)
{
    AVCodecContext *c = ost->enc;
 
    /* 检查是否要生成更多的帧 */
	AVRational tempratio;
	tempratio.num = 1;
	tempratio.den = 1;
    if (av_compare_ts(ost->next_pts, c->time_base,
                      STREAM_DURATION, tempratio) >= 0)
        return NULL;
 
    /* 当我们传递一帧给编码器时，可能内部有引用它，此时我们不能写入 */
    if (av_frame_make_writable(ost->frame) < 0)
        exit(1);
 
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        /* 如果必要，从YUV420P转换为目标格式*/
        if (!ost->sws_ctx) {
            ost->sws_ctx = sws_getContext(c->width, c->height,
                                          AV_PIX_FMT_YUV420P,
                                          c->width, c->height,
                                          c->pix_fmt,
                                          SCALE_FLAGS, NULL, NULL, NULL);
            if (!ost->sws_ctx) {
                fprintf(stderr,
                        "Could not initialize the conversion context\n");
                exit(1);
            }
        }

        fill_yuv_image(ost->tmp_frame, ost->next_pts, c->width, c->height);
		if(video_statu)return (AVFrame *)-1;
        sws_scale(ost->sws_ctx, (const uint8_t * const *) ost->tmp_frame->data,
                  ost->tmp_frame->linesize, 0, c->height, ost->frame->data,
                  ost->frame->linesize);
    } else {
        fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height);
		if(video_statu)return (AVFrame *)-1;
    }
 
    ost->frame->pts = ost->next_pts++;
 
    return ost->frame;
}
 
/*
 * 编码视频帧并发送到合成器
 * 编码结束返回1, 其他返回0
 */
static int write_video_frame(AVFormatContext *oc, OutputStream *ost)
{
    int ret;
    AVCodecContext *c;
    AVFrame *frame;
    int got_packet = 0;
    AVPacket pkt = { 0 };
 
    c = ost->enc;
 
    frame = get_video_frame(ost);
	if(video_statu)return -1;
    cout <<"sjk write_video_frame 1" <<endl;
 
    av_init_packet(&pkt);
 
    /* 编码图片 */
    ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);cout <<"sjk write_video_frame 2" <<endl;
    if (ret < 0) {
        fprintf(stderr, "Error encoding video frame: %s\n", av_err2str_f(ret));
        exit(1);
    }
 
    if (got_packet) {
        ret = write_frame(oc, &c->time_base, ost->st, &pkt);
    } else {
        ret = 0;
    }
 
    if (ret < 0) {
        fprintf(stderr, "Error while writing video frame: %s\n", av_err2str_f(ret));
        exit(1);
    }
 
    return (frame || got_packet) ? 0 : 1;
}
 
static void close_stream(AVFormatContext *oc, OutputStream *ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
}
 
void * yuv_data_put(void * data)
{
	int y_size = WIDTH * HEIGHT;
	FILE *yuv_file;
	yuv_file =fopen("saber.yuv", "rb");

	 while(1)
    {

        if(Quene_video.node_size >30) //防止队列过大，解码时间较长于读取文件时间
		{
			usleep(100);
		}
		

        char buffer[WIDTH*HEIGHT*3];

        int size = fread(buffer, 1, y_size*3/2, yuv_file);

        if (size < 0){
          printf("Failed to read YUV data! 文件读取错误\n");
          return 0;
        }
		else if(size <y_size*3/2)
		{
			cout <<"video read over" <<endl;
			return 0;
		}

        if (size == y_size*3/2)
        {
            cout <<"ptu video data " <<endl;
            DataQuene_Input((char *)buffer,y_size*3/2,&Quene_video);
        }

        usleep(10*1000);
    }
}

void * pcm_data_put(void * data)
{
	FILE * pcm_file =fopen("saber.pcm", "rb");
	int audio_input_frame_size =av_samples_get_buffer_size(NULL,2,1024,AV_SAMPLE_FMT_S16,0);
	cout <<"audio_input_frame_size" <<audio_input_frame_size <<endl;

	while(1)
    {
		if(Quene_audio.node_size >30)
		{
			usleep(100);
		}
        

        char buffer[WIDTH*HEIGHT*3];
        int size = fread(buffer, 1, audio_input_frame_size, pcm_file);

        if (size < 0){
          printf("Failed to read YUV data! 文件读取错误\n");
          return 0;
        }
		else if(size <audio_input_frame_size)
		{
			cout <<"audio read over" <<endl;
			return 0;
		}
        /*
        else if(feof(pcmInFp) && isStop==false)
        {
            qDebug()<<"read audio finished!";

            fclose(pcmInFp);
            pcmInFp = fopen(PCM_FILE_NAME,"rb");

            continue;
        }
            */
        if (size == audio_input_frame_size)
        {
            cout <<"ptu audio data " <<endl;
            DataQuene_Input(buffer,audio_input_frame_size,&Quene_audio);
        }

        usleep(10*1000);
    }

    return 0;
}

 int read_sources_pthread()
 {

	pthread_t thread_video;
	pthread_t thread_audio;

	pthread_create(&thread_video,NULL,yuv_data_put,NULL);
	pthread_create(&thread_audio,NULL,pcm_data_put,NULL);

 }
 
int main(int argc, char *argv[])  
{  
	OutputStream video_st = { 0 }, audio_st = { 0 };
    const char *filename;
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVCodec *audio_codec, *video_codec;
    int ret;
    int have_video = 0, have_audio = 0;
    int encode_video = 0, encode_audio = 0;
    AVDictionary *opt = NULL;
 
	av_register_all();
 
    filename = "generated.mp4";
 
    cout <<"sjk 1" <<endl;

    /* 根据文件名的后缀，分配输出格式的上下文 */
    avformat_alloc_output_context2(&oc, NULL, NULL, filename);
    if (!oc) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
    }
    
    if (!oc)
        return 1;

	//得到输出格式
    fmt = oc->oformat;

    /* 使用默认格式的编码器，增加音频和视频流
     * 初始化编码器 */
    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        add_stream(&video_st, oc, &video_codec, fmt->video_codec);
        have_video = 1;
        encode_video = 1;
    }
	video_st.enc->height =1024;
	video_st.enc->width =576;

    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
        add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
        have_audio = 1;
        encode_audio = 1;
    }
    cout <<audio_st.enc->channels <<"channels" <<endl;
   
 /*   
    if(AV_SAMPLE_FMT_S16 ==audio_st.enc->sample_fmt)
    {
        cout <<"AV_SAMPLE_FMT_S16" <<endl;
    }
    else
    {
        audio_st.enc->sample_fmt =AV_SAMPLE_FMT_S16;
    }
    */
    

 cout <<"sjk 3" <<endl;
    /* 设置所有的参数，打开音频和视频编码器，分配必要的编码内存*/
    if (have_video)
        open_video(oc, video_codec, &video_st, opt);
 
    if (have_audio)
        open_audio(oc, audio_codec, &audio_st, opt);

        
 
	//打印输出流的信息
    av_dump_format(oc, 0, filename, 1);
 
    /* 打开输出文件 */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open '%s': %s\n", filename,
                    av_err2str_f(ret));
            return 1;
        }
    }
 cout <<"sjk 4" <<endl;
    /* 写入流的头部 */
    ret = avformat_write_header(oc, &opt);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file: %s\n",
                av_err2str_f(ret));
        return 1;
    }

	//------sjk
	read_sources_pthread();

	cout <<video_st.enc->height <<video_st.enc->width <<endl;
cout <<"sjk 5" <<endl;
sleep(1);
    while (encode_video || encode_audio) {
        /* 选择流进行编码 */
        if (encode_video &&
            (!encode_audio || av_compare_ts(video_st.next_pts, video_st.enc->time_base,
                                            audio_st.next_pts, audio_st.enc->time_base) <= 0)) {
            encode_video = !write_video_frame(oc, &video_st);
        } else {
            encode_audio = !write_audio_frame(oc, &audio_st);
        }

		if(audio_statu && video_statu)break;

    }
 
    /* 写入流的尾部 */
    av_write_trailer(oc);
 
    /* 关闭编码器. */
    if (have_video)
        close_stream(oc, &video_st);
    if (have_audio)
        close_stream(oc, &audio_st);
 
    if (!(fmt->flags & AVFMT_NOFILE))
        /* 关闭输出文件*/
        avio_closep(&oc->pb);
 
    /* 释放编码上下文 */
    avformat_free_context(oc);
	system("PAUSE");
	return 0;  
}