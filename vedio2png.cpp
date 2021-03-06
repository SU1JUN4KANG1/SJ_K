

#include <iostream>

using namespace std;

extern "C"
{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavutil/pixfmt.h"
    #include "libswscale/swscale.h"
}


#define sjk_debug 1

#include <stdio.h>

int jpg(int in_w,int in_h,AVFrame *pFrame);


int WIDTH =0;
int HEIGHT =0;
int videoStream = -1;

//save_png(pFrame,AV_PIX_FMT_RGBA,AV_CODEC_ID_PNG,pImagePath,OUT_PNG_PIC_WIDTH,OUT_PNG_PIC_HEIGHT);
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


///生成一个简单的PPM格式文件
void Save_ppm_Frame(AVFrame *pFrame, int width, int height,int index)
{

  FILE *pFile;
  char szFilename[32];
  int  y;

  // Open file
  sprintf(szFilename, "./file_bufer/frame%d.ppm", index);
  pFile=fopen(szFilename, "wb");

  if(pFile==NULL)
    return;

  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);

  // Write pixel data
  for(y=0; y<height; y++)
  {
    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
  }

  // Close file
  fclose(pFile);

}

int main(int argc, char *argv[])
{

    FILE *inFile;
    FILE *outputFile;

    string outformat ;
    char * infilename;

    //outformat =(char *)malloc(5);

    AVFormatContext *pFormatCtx;
    
    if(argc <2 || argc >4)
    {
        printf("help :./vedio2pic inputfile outputformat\n");
    }
    else if(argc == 2)
    {
        /*  */
        infilename =argv[1];
        outformat ="png";
    }
    else
    {
        infilename =argv[1];
        outformat =argv[2];
    }
    cout <<argc <<argv[0] <<argv[1] <<argv[2] <<endl;
    cout <<outformat <<endl;
    av_register_all(); 
    pFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx, infilename, NULL, NULL) != 0) { //其中负责申请一个AVFormatContext结构的内存,并进行简单初始化,绑定file_path
        printf("can't open the file. \n");
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {// 通过读取媒体文件的中的包来获取媒体文件中的流信息,对于没有头信息的文件如(mpeg)是非常有用的,
        printf("Could't find stream infomation.\n");
        return -1;
    }

    

    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
        }
    }

     if (videoStream == -1) {
        printf("Didn't find a video stream.\n");
        return -1;
    }

    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);


    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) { //为数据结构关联编解码器参数
        printf("Could not open codec.\n");
        return -1;
    }

    AVFrame *pFrame, *pFrameRGB;
    pFrame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();

    int numBytes;

    numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width,pCodecCtx->height);

    uint8_t *out_buffer;
    out_buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

    avpicture_fill((AVPicture *) pFrameRGB, out_buffer, AV_PIX_FMT_RGB24,
            pCodecCtx->width, pCodecCtx->height);   //把ptr和picture以pix_fmt格式关联起来，意思就是指针ptr=picture

    int y_size = pCodecCtx->width * pCodecCtx->height;
     WIDTH =pCodecCtx->width;
     HEIGHT =pCodecCtx->height;


    AVPacket *packet;
    packet = (AVPacket *) malloc(sizeof(AVPacket)); //分配一个packet
    av_new_packet(packet, y_size); //分配packet的数据

    av_dump_format(pFormatCtx, 0, infilename, 0); //输出视频信息

    int index = 0;
    int ret =-1 ,got_picture =-1;
    int index_jpg =0;

    //char FORMAT[] ="ppm";
    if(outformat =="png")
    {
        printf("\n png \n");

        static struct SwsContext *img_convert_ctx;img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL); 
        while(1)
        {

            if (av_read_frame(pFormatCtx, packet) < 0)
            {
                break; //这里认为视频读取完了
            }

            if (packet->stream_index == videoStream) 
            {
                ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,packet); //使用av_read_frame读取媒体流后需要进行判断,如果为视频流则调用该函数解码
                if (ret < 0 ) {
                    printf("decode error. \n");
                    return -1;
                }

                if (got_picture) 
                {
                    sws_scale(img_convert_ctx,(uint8_t const * const *) pFrame->data,pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data,pFrameRGB->linesize);   //转换数据

                    //Save_ppm_Frame(pFrameRGB, pCodecCtx->width,pCodecCtx->height,index++); //保存图片
                    char szFilename[32];
                    sprintf(szFilename, "./file_bufer/frame%d.png", index);
                    save_png(pFrameRGB,AV_PIX_FMT_RGB24,AV_CODEC_ID_PNG,szFilename,pCodecCtx->width,pCodecCtx->height);
                    //if (index > 5) return 0; //这里我们就保存50张图片
                    break;
                }
            }
            av_free_packet(packet);
        }
        


        av_free(out_buffer);
        av_free(pFrameRGB);
        avcodec_close(pCodecCtx);
        avformat_close_input(&pFormatCtx);
    }
 
    
    return 0;


}


