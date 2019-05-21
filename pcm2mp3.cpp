/** 
实现FFMPEG4.0把AV_SAMPLE_FMT_S16格式双声道pcm数据编码为mp3数据，作者自己测试正确可用，test.pcm数据放于exe目录下
作者：明天继续
使用的ffmpeg版本：ffmpeg-20180508-293a6e8-win32 
开发工具：vs2012 
**/  
  
//#include "stdafx.h"  
//#include <Windows.h>
//#include <wingdi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

extern "C" {  
#include <libavcodec/avcodec.h>  
#include <libavformat/avformat.h>  
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
 
}  
 

 //ffmpeg编译中不支持MP3编码器，没有去验证，下为博客地址
 //https://blog.csdn.net/zhangamxqun/article/details/80490569

 
static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)  
{  
	const enum AVSampleFormat *p = codec->sample_fmts;  
 
	while (*p != AV_SAMPLE_FMT_NONE) {  
		if (*p == sample_fmt)  
			return 1;  
		p++;  
	}  
	return 0;  
}  
 
static void encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt,  
				   FILE *output)  
{  
	int ret;  
 
	/* 编码frame */  
	ret = avcodec_send_frame(ctx, frame);  
	if (ret < 0) {  
		fprintf(stderr, "Error sending the frame to the encoder\n");  
		exit(1);  
	}  
 
	/* 读取编码出的数据包，通常不止一个 */  
	while (ret >= 0) {  
		ret = avcodec_receive_packet(ctx, pkt);  
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)  
			return;  
		else if (ret < 0) {  
			fprintf(stderr, "Error encoding audio frame\n");  
			exit(1);  
		}  
 
		fwrite(pkt->data, 1, pkt->size, output);  
		av_packet_unref(pkt);  
	}  
}  
 
 
int main(int argc, char *argv[])  
{  
	////////////////////////////////////重采样，准备相关变量  
	int64_t src_ch_layout = AV_CH_LAYOUT_STEREO, dst_ch_layout = AV_CH_LAYOUT_STEREO;  
	int src_rate = 44100, dst_rate = 44100;  
	uint8_t **src_data = NULL, **dst_data = NULL;  
	int src_nb_channels = 0, dst_nb_channels = 0;  
	int src_linesize, dst_linesize;  
	int src_nb_samples = 1152, dst_nb_samples, max_dst_nb_samples;  
	enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_S16, dst_sample_fmt = AV_SAMPLE_FMT_S16P;  
	//int dst_bufsize;  
	struct SwrContext *swr_ctx;  
	int ret;  
	///////////////////////////////////////////////////////////  
 
	av_register_all();  
 
 
	//////////////////////////////////////////////////mp3编码准备相关变量  
	//编码器  
	const AVCodec *codecMp3;  
	//编码器上下文  
	AVCodecContext *contexMp3= NULL;  
	//pcm数据帧  
	AVFrame *frameMp3In;  
	//编码后的mp3数据包  
	AVPacket *pktMp3_Out;  
	/* 找到MP3的编码器 */  
	codecMp3 = avcodec_find_encoder(AV_CODEC_ID_MP3);  
	if (!codecMp3) {  
		fprintf(stderr, "Codec not found\n");  
		exit(1);  
	}  
 
	//生成编码器上下文  
	contexMp3 = avcodec_alloc_context3(codecMp3);  
	if (!contexMp3) {  
		fprintf(stderr, "Could not allocate audio codec context\n");  
		exit(1);  
	}  
	/* 设置编码器上下文的bit率 */  
	contexMp3->bit_rate = 64000;   
	contexMp3->sample_fmt = AV_SAMPLE_FMT_S16P;  
	if (!check_sample_fmt(codecMp3, contexMp3->sample_fmt)) {  
		fprintf(stderr, "Encoder does not support sample format %s",  
			av_get_sample_fmt_name(contexMp3->sample_fmt));  
		exit(1);  
	}  
	/* 设置编码器的必要参数 */  
	contexMp3->sample_rate    = dst_rate;  
	contexMp3->channel_layout = AV_CH_LAYOUT_STEREO;  
	contexMp3->channels       = av_get_channel_layout_nb_channels(contexMp3->channel_layout);  
	if (avcodec_open2(contexMp3, codecMp3, NULL) < 0) {  
		fprintf(stderr, "Could not open codec\n");  
		exit(1);  
	}  
	FILE *mp3File = fopen("outmp3.mp3", "wb");  
	/* 初始化mp3编码结果数据的存储包 */  
	pktMp3_Out = av_packet_alloc();  
	if (!pktMp3_Out) {  
		fprintf(stderr, "could not allocate the packet\n");  
		exit(1);  
	}  
	/* 初始化存放pcm数据的frame */  
	frameMp3In = av_frame_alloc();  
	if (!frameMp3In) {  
		fprintf(stderr, "Could not allocate audio frame\n");  
		exit(1);  
	}  
	//设置pcm数据frame的一些参数  
	//frame size 是数据中有多少个样本，每个样本包含两个通道各一个数据，每个数据占16位2个字节。所以frame得实际大小是 frame_size * sizeof(uint16_t)*2  
	frameMp3In->nb_samples     = contexMp3->frame_size;  
	frameMp3In->format         = contexMp3->sample_fmt;  
	frameMp3In->channel_layout = contexMp3->channel_layout;  
	/* 分配frame的内存空间 */  
    ret = av_frame_get_buffer(frameMp3In, 0);  
	if (ret < 0) {  
		fprintf(stderr, "Could not allocate audio data buffers\n");  
		exit(1);  
	}  
	///////////////////////////////////////////////////////////////////  
 
	//src_nb_samples = getAV_SAMPLE_FMT_S16FrameSize();  
 
 
	/* 创建重采样上下文 */  
	swr_ctx = swr_alloc();  
	if (!swr_ctx) {  
		fprintf(stderr, "Could not allocate resampler context\n");  
		ret = AVERROR(ENOMEM);  
		return -1;
	}  
 
	/* 设置选项 */  
	av_opt_set_int(swr_ctx, "in_channel_layout",    src_ch_layout, 0);  
	av_opt_set_int(swr_ctx, "in_sample_rate",       src_rate, 0);  
	av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", src_sample_fmt, 0);  
 
	av_opt_set_int(swr_ctx, "out_channel_layout",    dst_ch_layout, 0);  
	av_opt_set_int(swr_ctx, "out_sample_rate",       dst_rate, 0);  
	av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);  
 
	/* 初始化重采样上下文 */  
	if ((ret = swr_init(swr_ctx)) < 0) {  
		fprintf(stderr, "Failed to initialize the resampling context\n");  
		return -1;
	}  
 
	/* 分配数源数据的临时存储空间 */  
 
	src_nb_channels = av_get_channel_layout_nb_channels(src_ch_layout);  
	ret = av_samples_alloc_array_and_samples(&src_data, &src_linesize, src_nb_channels,  
		src_nb_samples, src_sample_fmt, 0);  
	if (ret < 0) {  
		fprintf(stderr, "Could not allocate source samples\n");  
		return -1;
	}  
 
	/* 计算转换后的样本数量 */  
	max_dst_nb_samples = dst_nb_samples =  
		av_rescale_rnd(src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);  
 
	dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);  
	ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels,  
		dst_nb_samples, dst_sample_fmt, 0);  
	if (ret < 0) {  
		fprintf(stderr, "Could not allocate destination samples\n");  
		return -1;;  
	}  
 
	FILE * inputfile = NULL;  
	inputfile = fopen("your_univers.pcm", "rb");  
	uint8_t * tempdata = frameMp3In->data[0];  
	uint8_t * tempdata1 = frameMp3In->data[1];  
	while(!feof(inputfile))  
	{  
		int readsize = fread(src_data[0],1,src_nb_channels*src_nb_samples*sizeof(uint16_t),inputfile);  
		src_nb_samples = readsize / (src_nb_channels*sizeof(uint16_t));  
		if (!readsize)  
			break;  
		/* 计算目标样本数量 */  
		dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, src_rate) +  
			src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);  
		if (dst_nb_samples > max_dst_nb_samples) {  
			av_freep(&dst_data[0]);  
			ret = av_samples_alloc(dst_data, &dst_linesize, dst_nb_channels,  
				dst_nb_samples, dst_sample_fmt, 1);  
			if (ret < 0)  
				break;  
			max_dst_nb_samples = dst_nb_samples;  
		}  
 
		/* 重采样 */  
		ret = swr_convert(swr_ctx, dst_data, dst_nb_samples, (const uint8_t **)src_data, src_nb_samples);  
 
		if (ret < 0) {  
			fprintf(stderr, "Error while converting\n");  
			return -1;;  
		}  
		frameMp3In->nb_samples = dst_nb_samples;  
		contexMp3->frame_size = dst_nb_samples;  
		frameMp3In->data[0] = dst_data[0]; 
		frameMp3In->data[1] = dst_data[1];
 
		//重采样后的结果编码为mp3  
		encode(contexMp3, frameMp3In, pktMp3_Out, mp3File);  
		fprintf(stdout, "encode data size:%d\n",readsize);  
 
	}  
 
	frameMp3In->data[0] = tempdata; 
	frameMp3In->data[0] = tempdata1;
 
end:  
 
	if (src_data)  
		av_freep(&src_data[0]);  
	av_freep(&src_data);  
 
	if (dst_data)  
		av_freep(&dst_data[0]);  
	av_freep(&dst_data);  
 
	swr_free(&swr_ctx);  
 
 
	fclose(inputfile);  
 
	/////////////////////////////////////////编码mp3  
	fclose(mp3File);  
	av_frame_free(&frameMp3In);  
	av_packet_free(&pktMp3_Out);  
	avcodec_free_context(&contexMp3);  
 
	return 0;  
}