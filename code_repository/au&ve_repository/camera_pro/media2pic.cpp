
#include <stdio.h>
#include <unistd.h>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
}


//g++ media2pic.cpp -o media2pic  -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale

#define jpg_or_png 1 //jpg is 1

// save_png(pFrameRGB,AV_PIX_FMT_RGB24,AV_CODEC_ID_PNG,szFilename,pCodecCtx->width,pCodecCtx->height);
bool save_png(AVFrame *frm, AVPixelFormat pfmt, AVCodecID cid, const char* filename, int width, int height)
{
	int outbuf_size = width * height*4;
	uint8_t * outbuf = (uint8_t*)malloc(outbuf_size);
	int got_pkt = 0;

	FILE* pf;
	pf = fopen(filename, "wb");
	if (pf == NULL)
		return false;
	AVPacket pkt;
	AVCodec *pCodecRGB24;
	AVCodecContext *ctx = NULL;
	pCodecRGB24 = avcodec_find_encoder(cid);
	if (!pCodecRGB24)
		return false;
	ctx = avcodec_alloc_context3(pCodecRGB24);
	ctx->bit_rate = 3000000;
	ctx->width = width;
	ctx->height = height;
	AVRational rate;
	rate.num = 1;
	rate.den = 25;
	ctx->time_base = rate;
	ctx->gop_size = 10;
	ctx->max_b_frames = 0;
	ctx->thread_count = 1;
	ctx->pix_fmt = pfmt;


	int ret = avcodec_open2(ctx, pCodecRGB24, NULL);
	if (ret < 0)
		return false;

	//	int size = ctx->width * ctx->height * 4;
	av_init_packet(&pkt);
	static int got_packet_ptr = 0;
	pkt.size = outbuf_size;
	pkt.data = outbuf;
    frm->height =height;
    frm->width =width;
    frm->format =pfmt;
	got_pkt = avcodec_encode_video2(ctx, &pkt, frm, &got_packet_ptr);
	frm->pts++;
	if (got_pkt == 0)
	{
		fwrite(pkt.data, 1, pkt.size, pf);
	}
	else
	{
		return false;
	}
	fclose(pf);
	return true;
}

int saveJpg(AVFrame *pFrame, char *out_name ,int width,int height) {

    //printf("width =%d height =%d \n",pFrame->width, pFrame->height);
    pFrame->width =width;
    pFrame->height =height;
    pFrame->format =AV_PIX_FMT_YUV420P;
    //int width = pFrame->width;
    //int height = pFrame->height;
    AVCodecContext *pCodeCtx = NULL;
    
    
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    // 设置输出文件格式
    pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);

    // 创建并初始化输出AVIOContext
    if (avio_open(&pFormatCtx->pb, out_name, AVIO_FLAG_READ_WRITE) < 0) {
        printf("Couldn't open output file.");
        return -1;
    }

    // 构建一个新stream
    AVStream *pAVStream = avformat_new_stream(pFormatCtx, 0);
    if (pAVStream == NULL) {
        return -1;
    }

    AVCodecParameters *parameters = pAVStream->codecpar;
    parameters->codec_id = pFormatCtx->oformat->video_codec;
    parameters->codec_type = AVMEDIA_TYPE_VIDEO;
    parameters->format = AV_PIX_FMT_YUVJ420P;
    parameters->width = pFrame->width;
    parameters->height = pFrame->height;

    AVCodec *pCodec = avcodec_find_encoder(pAVStream->codecpar->codec_id);

    if (!pCodec) {
        printf("Could not find encoder\n");
        return -1;
    }

    pCodeCtx = avcodec_alloc_context3(pCodec);
    if (!pCodeCtx) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    if ((avcodec_parameters_to_context(pCodeCtx, pAVStream->codecpar)) < 0) {
        fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        return -1;
    }

    pCodeCtx->time_base = (AVRational) {1, 25};

    if (avcodec_open2(pCodeCtx, pCodec, NULL) < 0) {
        printf("Could not open codec.");
        return -1;
    }


    int ret = avformat_write_header(pFormatCtx, NULL);
    if (ret < 0) {
        printf("write_header fail\n");
        return -1;
    }

    int y_size = width * height;

    //Encode
    // 给AVPacket分配足够大的空间
    AVPacket pkt;
    av_new_packet(&pkt, y_size * 3);

    // 编码数据
    ret = avcodec_send_frame(pCodeCtx, pFrame);
    if (ret < 0) {
        printf("Could not avcodec_send_frame.");
        return -1;
    }

    // 得到编码后数据
    ret = avcodec_receive_packet(pCodeCtx, &pkt);
    if (ret < 0) {
        printf("Could not avcodec_receive_packet");
        return -1;
    }

    ret = av_write_frame(pFormatCtx, &pkt);

    if (ret < 0) {
        printf("Could not av_write_frame");
        return -1;
    }

    av_packet_unref(&pkt);

    //Write Trailer
    av_write_trailer(pFormatCtx);


    avcodec_close(pCodeCtx);
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);

    return 0;
}



int main(int argc, char* argv[])
{

    char *oup_path =argv[1];

    AVFormatContext	*pFormatCtx;
    int				i, videoindex;
    AVCodecContext	*pCodecCtx;
    AVCodec			*pCodec;

    av_register_all();
    avformat_network_init();
    avdevice_register_all();//Register Device

    pFormatCtx = avformat_alloc_context();

    AVInputFormat *pAVInputFormat = av_find_input_format("video4linux2");
    int value = avformat_open_input(&pFormatCtx, "/dev/video1", pAVInputFormat, NULL);
    if(value <0)
    {
        printf("sjk_input error\n");
    }

    if(avformat_find_stream_info(pFormatCtx,NULL)<0)
    {
        printf("Couldn't find stream information.\n");
        return -1;
    }

    videoindex=-1;

    for(i=0; i<pFormatCtx->nb_streams; i++)
    {
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
        {
            videoindex=i;
        }
    }

    if(videoindex==-1)
    {
        printf("Couldn't find a video stream.\n");
        return -1;
    }

    pCodecCtx=pFormatCtx->streams[videoindex]->codec;
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    
    if(pCodec==NULL)
    {
        printf("Codec not found.\n");
        return -1;
    }

    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)
    {
        printf("Could not open codec.\n");
        return -1;
    }

    AVFrame	*pFrame,*pFrameYUV,*pFrameRGB;
    pFrame=av_frame_alloc();
    uint8_t *out_buffer;

    int ret, got_picture;

    AVPacket *packet=(AVPacket *)av_malloc(sizeof(AVPacket));


    struct SwsContext *img_convert_ctx;
    if(jpg_or_png)
    {
        pFrameYUV=av_frame_alloc();

        out_buffer=(uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));

        avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

        img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    }
    else
    {   
        pFrameRGB = av_frame_alloc();

        out_buffer=(uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height));

        avpicture_fill((AVPicture *) pFrameRGB, out_buffer, AV_PIX_FMT_RGB24,
            pCodecCtx->width, pCodecCtx->height);

        img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

    }
    

    ///这里打印出视频的宽高
    fprintf(stderr,"w= %d h= %d\n",pCodecCtx->width, pCodecCtx->height);


    char buf[1024];
    ///我们就读取100张图像
    for(int i=0;i<5;i++)
    {
        if(av_read_frame(pFormatCtx, packet) < 0)
        {
            break;
        }

        if(packet->stream_index==videoindex)
        {
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);

            if(ret < 0){
                printf("Decode Error.\n");
                return -1;
            }

            if(got_picture)
            {

                

                if(jpg_or_png)
                {
                    sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

                    snprintf(buf, sizeof(buf), "%s/media-%d.jpg", oup_path,i);   
                    saveJpg(pFrameYUV,buf,pCodecCtx->width,pCodecCtx->height);
                    
                }
                else
                {
                    sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
                    
                    snprintf(buf, sizeof(buf), "%s/media-%d.png", oup_path,i); 
                    save_png(pFrameRGB,AV_PIX_FMT_RGB24,AV_CODEC_ID_PNG,buf,pCodecCtx->width,pCodecCtx->height);
                }
                
                

                

            }
        }
        av_free_packet(packet);
        sleep(1);
    }

    sws_freeContext(img_convert_ctx);



    av_free(out_buffer);
    av_free(pFrameYUV);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    return 0;
}



