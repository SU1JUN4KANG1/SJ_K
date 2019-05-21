
#include <stdio.h>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
}

//g++ media2pic.cpp -o media2pic  -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale

int main(int argc, char* argv[])
{

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

    AVFrame	*pFrame,*pFrameYUV;
    pFrame=av_frame_alloc();
    pFrameYUV=av_frame_alloc();
    uint8_t *out_buffer=(uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
    avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

    int ret, got_picture;

    AVPacket *packet=(AVPacket *)av_malloc(sizeof(AVPacket));

    FILE *fp_yuv=fopen("output.yuv","wb");

    struct SwsContext *img_convert_ctx;
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    ///这里打印出视频的宽高
    fprintf(stderr,"w= %d h= %d\n",pCodecCtx->width, pCodecCtx->height);

    ///我们就读取100张图像
    for(int i=0;i<100;i++)
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

                sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);


                int y_size=pCodecCtx->width*pCodecCtx->height;
                fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y
                fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
                fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V

            }
        }
        av_free_packet(packet);
    }

    sws_freeContext(img_convert_ctx);


    fclose(fp_yuv);


    av_free(out_buffer);
    av_free(pFrameYUV);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    return 0;
}



