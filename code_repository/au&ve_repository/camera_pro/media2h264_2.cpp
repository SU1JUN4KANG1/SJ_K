
#include <stdio.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
}

//g++ media2h264_2.cpp -o media2h264_2  -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale -lpthread
//	./media2h264_2

uint8_t* picture_buf;
pthread_mutex_t mut;

AVFrame	*pFrameYUV;
bool enconder_begin =false;
bool deconder_begin =true;

bool statu =false;
int endding =1;
bool read_ending =false;

int encoder_func()
{
	AVFormatContext* pFormatCtx;
	AVOutputFormat* fmt;
	AVStream* video_st;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;
	AVPacket pkt;
	
	AVFrame* pFrame;
	int picture_size;
	int y_size;
	int framecnt=0;

	int in_w=640,in_h=480;                              //Input data's width and height
	int framenum=100;                                   //Frames to encode

	const char* out_file = "camera.h264";
 
	av_register_all();
	//Method1.
	pFormatCtx = avformat_alloc_context();
	//Guess Format
	fmt = av_guess_format(NULL, out_file, NULL);
	pFormatCtx->oformat = fmt;
	
	//Method 2.
	//avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_file);
	//fmt = pFormatCtx->oformat;
 
 
	//Open output URL
	if (avio_open(&pFormatCtx->pb,out_file, AVIO_FLAG_READ_WRITE) < 0){
		printf("Failed to open output file! \n");
		return -1;
	}
 
	video_st = avformat_new_stream(pFormatCtx, 0);
	//video_st->time_base.num = 1; 
	//video_st->time_base.den = 25;  
 
	if (video_st==NULL){
		return -1;
	}
	//Param that must set
	pCodecCtx = video_st->codec;
	//pCodecCtx->codec_id =AV_CODEC_ID_HEVC;
	pCodecCtx->codec_id = fmt->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pCodecCtx->width = in_w;  
	pCodecCtx->height = in_h;
	pCodecCtx->bit_rate = 400000;  
	pCodecCtx->gop_size=250;
 
	pCodecCtx->time_base.num = 1;  
	pCodecCtx->time_base.den = 25;  
 
	//H264
	//pCodecCtx->me_range = 16;
	//pCodecCtx->max_qdiff = 4;
	//pCodecCtx->qcompress = 0.6;
	pCodecCtx->qmin = 10;
	pCodecCtx->qmax = 51;
 
	//Optional Param
	pCodecCtx->max_b_frames=3;
 
	// Set Option
	AVDictionary *param = 0;
	//H.264
	if(pCodecCtx->codec_id == AV_CODEC_ID_H264) {
		av_dict_set(&param, "preset", "slow", 0);
		av_dict_set(&param, "tune", "zerolatency", 0);
		//av_dict_set(&param, "profile", "main", 0);
	}
	//H.265
	if(pCodecCtx->codec_id == AV_CODEC_ID_H265){
		av_dict_set(&param, "preset", "ultrafast", 0);
		av_dict_set(&param, "tune", "zero-latency", 0);
	}
 
	//Show some Information
	av_dump_format(pFormatCtx, 0, out_file, 1);
 
	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec){
		printf("Can not find encoder! \n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec,&param) < 0){
		printf("Failed to open encoder! \n");
		return -1;
	}
 
 
	pFrame = av_frame_alloc();
	picture_size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	picture_buf = (uint8_t *)av_malloc(picture_size);
	avpicture_fill((AVPicture *)pFrame, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
 
	//Write File Header
	avformat_write_header(pFormatCtx,NULL);
 
	av_new_packet(&pkt,picture_size);
 
	y_size = pCodecCtx->width * pCodecCtx->height;
 
	for (int i=0;; i++){
		//Read raw YUV data
        
		//while(read_ending)break;

        while(statu ==false)
		{
			if(read_ending)
			{
				break;
			}
		}

		if(read_ending)
		{
			break;
		}


        picture_buf =pFrameYUV->data[0];
		pFrame->data[0] = pFrameYUV->data[0] ;              // Y
		pFrame->data[1] = pFrameYUV->data[1];      // U 
		pFrame->data[2] = pFrameYUV->data[2];  // V
		//PTS
		//pFrame->pts=i;
		pFrame->pts=i*(video_st->time_base.den)/((video_st->time_base.num)*25);
		int got_picture=0;
		//Encode
        
        pFrame->format =AV_PIX_FMT_YUV420P;
        pFrame->height =pCodecCtx->height;
        pFrame->width =pCodecCtx->width;
		int ret = avcodec_encode_video2(pCodecCtx, &pkt,pFrame, &got_picture);
		if(ret < 0){
			printf("Failed to encode! \n");
			return -1;
		}
		if (got_picture==1){
			printf("Succeed to encode frame: %5d\tsize:%5d\n",framecnt,pkt.size);
			framecnt++;
			pkt.stream_index = video_st->index;
			ret = av_write_frame(pFormatCtx, &pkt);
			av_free_packet(&pkt);
            

           
            
		}
        statu =false;      
	}

    printf("end encoder thread while\n");
	//Write file trailer
	av_write_trailer(pFormatCtx);
 
	//Clean
	if (video_st){
		avcodec_close(video_st->codec);
		av_frame_free(&pFrame);
		//av_free(picture_buf);
	}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);
 
	
    
    
	return 0;
}


void * func_thread(void * arg)
{
    sleep(1);
    encoder_func();
}

int main(int argc, char* argv[])
{

    AVFormatContext	*pFormatCtx;
    int				i, videoindex;
    AVCodecContext	*pCodecCtx;
    AVCodec			*pCodec;


    pthread_mutex_init(&mut,NULL);//init the mutex 

    av_register_all();
    avformat_network_init();
    avdevice_register_all();//Register Device

    pFormatCtx = avformat_alloc_context();

    AVInputFormat *pAVInputFormat = av_find_input_format("video4linux2");
    int value = avformat_open_input(&pFormatCtx, "/dev/video0", pAVInputFormat, NULL);
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

    AVFrame	*pFrame;
    pFrame=av_frame_alloc();
    pFrameYUV=av_frame_alloc();
    uint8_t *out_buffer=(uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
    avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

    int ret, got_picture;

    AVPacket *packet=(AVPacket *)av_malloc(sizeof(AVPacket));

    //FILE *fp_yuv=fopen("output_2.yuv","wb");

    struct SwsContext *img_convert_ctx;
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    ///这里打印出视频的宽高
    fprintf(stderr,"w= %d h= %d\n",pCodecCtx->width, pCodecCtx->height);

    pthread_t thread;
    pthread_create(&thread,NULL,func_thread,NULL);

    ///我们就读取100张图像
    for(int i=0;i<100;i++)
    {      
        
        while(statu ==true);

        printf("----------------------------------------------  %d\n",i);

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

                
                statu =true;             

            }
        }
        av_free_packet(packet);
    }

    sws_freeContext(img_convert_ctx);

	read_ending =true;
	pthread_join(thread,NULL);
    //fclose(fp_yuv);


    av_free(out_buffer);
    av_free(pFrameYUV);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    return 0;
}

 
