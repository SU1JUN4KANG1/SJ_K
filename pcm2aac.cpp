
#include <iostream>

extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "libavutil/samplefmt.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}

//g++ pcm2mp3.cpp -o pcm2mp3  -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale -lpthread



/* PCM转AAC */
int main()
{

    char *padts = (char *)malloc(sizeof(char) * 7);
    int profile = 2;	                                        //AAC LC
    int freqIdx = 4;                                            //44.1KHz
    int chanCfg = 2;            //MPEG-4 Audio Channel Configuration. 1 Channel front-center
    padts[0] = (char)0xFF;      // 11111111     = syncword
    padts[1] = (char)0xF1;      // 1111 1 00 1  = syncword MPEG-2 Layer CRC
    padts[2] = (char)(((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
    padts[6] = (char)0xFC;

    AVCodec *pCodec;
    AVCodecContext *pCodecCtx = NULL;
    int i, ret, got_output;
    FILE *fp_in;
    FILE *fp_out;

    AVFrame *pFrame;
    uint8_t* frame_buf;
    int size = 0;

    AVPacket pkt;
    int y_size;
    int framecnt = 0;

    char filename_in[] = "your_univers.pcm";

    AVCodecID codec_id = AV_CODEC_ID_AAC;
    char filename_out[] = "your_univers.aac";

    int framenum = 100000;

    avcodec_register_all();

    //pCodec = avcodec_find_encoder(codec_id);
    pCodec = avcodec_find_encoder_by_name("libfdk_aac"); //--SJK
    if (!pCodec) {
        printf("Codec not found\n");
        return -1;
    }

    pCodecCtx = avcodec_alloc_context3(pCodec);


    if (!pCodecCtx) {
        printf("Could not allocate video codec context\n");
        return -1;
    }

    pCodecCtx->codec_id = codec_id;
    pCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;

    //pCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    pCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16; //--sjk



    pCodecCtx->sample_rate = 44100;


    pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
    pCodecCtx->channels = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
    std::cout << av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);



    if ((ret = avcodec_open2(pCodecCtx, pCodec, NULL)) < 0) {
        std::cout << "avcodec_open2 error ----> " << ret;

        printf("Could not open codec\n");
        return -1;
    }

    pFrame = av_frame_alloc();

    pFrame->nb_samples = pCodecCtx->frame_size;	//1024,默认每一帧的采样个数是frame_size,貌似也改变不了
    pFrame->format = pCodecCtx->sample_fmt;
    pFrame->channels = 2;

    size = av_samples_get_buffer_size(NULL, pCodecCtx->channels, pCodecCtx->frame_size, pCodecCtx->sample_fmt, 0);
    frame_buf = (uint8_t *)av_malloc(size);
    /**
    *   avcodec_fill_audio_frame 实现：
    *   frame_buf是根据声道数、采样率和采样格式决定大小的。
    *   调用次函数后，AVFrame存储音频数据的成员有以下变化：data[0]指向frame_buf，data[1]指向frame_buf长度的一半位置
    *   data[0] == frame_buf , data[1] == frame_buf + pCodecCtx->frame_size * av_get_bytes_per_sample(pCodecCtx->sample_fmt)
    */
    ret = avcodec_fill_audio_frame(pFrame, pCodecCtx->channels, pCodecCtx->sample_fmt, (const uint8_t*)frame_buf, size, 0);

    if (ret < 0)
    {
        std::cout << "avcodec_fill_audio_frame error ";
        return 0;
    }

    //Input raw data
    fp_in = fopen(filename_in, "rb");
    if (!fp_in) {
        printf("Could not open %s\n", filename_in);
        return -1;
    }

    //Output bitstream
    fp_out = fopen(filename_out, "wb");
    if (!fp_out) {
        printf("Could not open %s\n", filename_out);
        return -1;
    }

    //Encode
    for (i = 0; i < framenum; i++) {
        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;
        //Read raw data
        if (fread(frame_buf, 1, size, fp_in) <= 0) {
            printf("Failed to read raw data! \n");
            return -1;
        }
        else if (feof(fp_in)) {
            break;
        }

        pFrame->pts = i;

        ret = avcodec_encode_audio2(pCodecCtx, &pkt, pFrame, &got_output);

        if (ret < 0) {
            std::cout << "error encoding";
            return -1;
        }

        if (pkt.data == NULL)
        {
            av_free_packet(&pkt);
            continue;
        }

        std::cout << "got_ouput = " << got_output;
        if (got_output) {
            std::cout << "Succeed to encode frame : " << framecnt << " size :" << pkt.size;

            framecnt++;

            std::cout <<"1 sjk " <<std::endl;


            padts[3] = (char)(((chanCfg & 3) << 6) + ((7 + pkt.size) >> 11));
            padts[4] = (char)(((7 + pkt.size) & 0x7FF) >> 3);
            padts[5] = (char)((((7 + pkt.size) & 7) << 5) + 0x1F);
            fwrite(padts, 7, 1, fp_out);
            fwrite(pkt.data, 1, pkt.size, fp_out);

            std::cout <<"2 sjk " <<std::endl;

            av_free_packet(&pkt);
            std::cout <<"3 sjk " <<std::endl;
        }
    }
    //Flush Encoder
    for (got_output = 1; got_output; i++) {
        ret = avcodec_encode_audio2(pCodecCtx, &pkt, NULL, &got_output);
        if (ret < 0) {
            printf("Error encoding frame\n");
            return -1;
        }
        if (got_output) {
            printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", pkt.size);
            padts[3] = (char)(((chanCfg & 3) << 6) + ((7 + pkt.size) >> 11));
            padts[4] = (char)(((7 + pkt.size) & 0x7FF) >> 3);
            padts[5] = (char)((((7 + pkt.size) & 7) << 5) + 0x1F);

            fwrite(padts, 7, 1, fp_out);
            fwrite(pkt.data, 1, pkt.size, fp_out);
            av_free_packet(&pkt);
        }
    }

    std::cout <<std::endl <<"end sjk " <<std::endl;

    fclose(fp_out);
    avcodec_close(pCodecCtx);
    av_free(pCodecCtx);
    av_freep(&pFrame->data[0]);
    av_frame_free(&pFrame);

    return 0;
}





