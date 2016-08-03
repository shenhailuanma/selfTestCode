
#include "libavutil/opt.h"
#include "config.h"
#include "libavformat/avformat.h"
#include "libavformat/internal.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"

#include "smem_dec.h"
#include "smem_client.h"



struct memory_info2 {
    int index;  // stream index
    int64_t pts;
    int64_t dts;
    int stream_info_offset;
    int stream_info_number;
    int data_offset;
    int data_size;
};

struct stream_info2 {
    int index; 

    enum AVMediaType codec_type;
    enum AVCodecID     codec_id;
    AVRational time_base;

    /* video */
    int width;
    int height;
    enum AVPixelFormat pix_fmt;

    /* audio */
    int sample_rate; ///< samples per second
    int channels;    ///< number of audio channels
    enum AVSampleFormat sample_fmt;  ///< sample format

    /* other stream not support yet */
};


typedef struct smem_dec_ctx {
    const AVClass *class;

    struct smemContext * sctx;

    int stream_number;
    struct stream_info2 * stream_infos;
    int stream_info_size;

    /* Params */
    char ip[64];    /* the share memory server ip address */
    int  port;      /* the share memory server port */
    char channel[128];  /* the channel in share memory server */

    int width;
    int height;

    /* Options */
    int timeout;

    /* test */
    FILE * yuv_file;
}smem_dec_ctx;

#define TEST_FILE_OUT "/root/video_out.yuv"

static int get_stream_info(AVFormatContext *avctx)
{
    struct smem_dec_ctx * ctx = avctx->priv_data;
    struct memory_info2 * m_info;

    int mem_id = -1;
    uint8_t *mem_ptr = NULL;
    int ret;

    int64_t last_time = av_gettime_relative();

    while(1){
        // to get one share memory
        mem_id = smemGetShareMemory(ctx->sctx, 0);
        if(mem_id > 0){
            av_log(avctx, AV_LOG_VERBOSE, "get memory id: %d\n", mem_id);

            // get the memory ptr
            mem_ptr = shmat(mem_id, 0, 0);
            if(mem_ptr == (void *)-1){
                av_log(avctx, AV_LOG_ERROR, "get share memory(%d) pointer error.\n", mem_id);
                smemFreeShareMemory(ctx->sctx, mem_id);
                return -1;
            }
            av_log(avctx, AV_LOG_VERBOSE, "get memory mem_ptr: %ld\n", mem_ptr);

            
            // get stream info
            m_info = (struct memory_info2 * )mem_ptr;
            ctx->stream_number = m_info->stream_info_number;

            ctx->stream_info_size = ctx->stream_number * sizeof(struct stream_info2);
            ctx->stream_infos = av_malloc(ctx->stream_info_size);


            memcpy(ctx->stream_infos, mem_ptr + m_info->stream_info_offset, ctx->stream_info_size);

            // fixme: the data should be save

            // free the share memory 
            if(mem_ptr){
                ret = shmdt(mem_ptr);
                if(ret < 0){
                    av_log(avctx, AV_LOG_ERROR, "release share memory ptr error:%d\n", ret);
                    return -1;
                }
            }

            // free the memory id
            smemFreeShareMemory(ctx->sctx, mem_id);
            av_log(avctx, AV_LOG_VERBOSE, "smemFreeShareMemory\n");

            break;
        }
        else{
            av_usleep(1);
            // fixme: should be add timeout
            // when timeout break
            if(av_gettime_relative() - last_time > ctx->timeout*1000000){
                av_log(avctx, AV_LOG_INFO, "timeout:%d\n", ctx->timeout);
                return -1;
            }

        }
    }
    return 0;
}

av_cold static int ff_smem_read_header(AVFormatContext *avctx)
{
    AVStream *stream = NULL;


    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_read_header begin\n");

    struct smem_dec_ctx * ctx = avctx->priv_data;

    /* get the input stream url, the stream format should be like :  "smem://127.0.0.1:6379/channel_name"*/
    av_log(avctx, AV_LOG_VERBOSE, "channel name:%s\n", avctx->filename);


    /* connect to the server */
    ctx->sctx = smemCreateConsumer("127.0.0.1", 6379, "test");
    if(!ctx->sctx || ctx->sctx->err < 0){
        if (ctx->sctx) {
            av_log(avctx, AV_LOG_ERROR,"Connection error: %s\n", ctx->sctx->errstr);
            smemFree(ctx->sctx);
        } else {
            av_log(avctx, AV_LOG_ERROR,"Connection error: can't allocate redis context\n");
        }
    }

    /* get the stream info */
    get_stream_info(avctx);

    /* create the streams */
    int i = 0;
    struct stream_info2 * stream_info;

    for(i = 0; i < ctx->stream_number; i++){

        stream_info = &ctx->stream_infos[i];

        if(stream_info->codec_type == AVMEDIA_TYPE_VIDEO){
            av_log(avctx, AV_LOG_VERBOSE, "stream %d is video stream\n", i);

            // add video stream
            stream = avformat_new_stream(avctx, NULL);
            if (!stream) {
                av_log(avctx, AV_LOG_ERROR, "Cannot add stream\n");
                goto ff_smem_read_header_error;
            }

            stream->codec->codec_type  = AVMEDIA_TYPE_VIDEO;
            stream->codec->codec_id    = stream_info->codec_id; 
            stream->codec->time_base.den      = stream_info->time_base.den;
            stream->codec->time_base.num      = stream_info->time_base.num;
            //stream->codec->time_base = AV_TIME_BASE_Q;
            stream->time_base = stream->codec->time_base;

            stream->codec->pix_fmt     = stream_info->pix_fmt;
            stream->codec->width       = stream_info->width;
            stream->codec->height      = stream_info->height;

            av_log(avctx, AV_LOG_VERBOSE, "codec_id:%d, time_base:(%d,%d), pix_fmt:%d, width:%d, height:%d \n", 
                stream_info->codec_id, stream_info->time_base.num, stream_info->time_base.den, stream_info->pix_fmt, stream_info->width, stream_info->height);

            //stream->codec->bit_rate    = av_image_get_buffer_size(stream->codec->pix_fmt, ctx->width, ctx->height, 1) * 1/av_q2d(stream->codec->time_base) * 8;
        }
        else if(stream_info->codec_type == AVMEDIA_TYPE_AUDIO){
            av_log(avctx, AV_LOG_VERBOSE, "stream %d is audio stream\n", i);

            // add video stream
            stream = avformat_new_stream(avctx, NULL);
            if (!stream) {
                av_log(avctx, AV_LOG_ERROR, "Cannot add stream\n");
                goto ff_smem_read_header_error;
            }

            stream->codec->codec_type  = AVMEDIA_TYPE_AUDIO;
            stream->codec->codec_id    = stream_info->codec_id; 
            stream->codec->time_base.den      = stream_info->time_base.den;
            stream->codec->time_base.num      = stream_info->time_base.num;
            //stream->codec->time_base = AV_TIME_BASE_Q;
            stream->time_base = stream->codec->time_base;

            stream->codec->sample_rate   = stream_info->sample_rate;
            stream->codec->channels      = stream_info->channels;
            stream->codec->sample_fmt    = stream_info->sample_fmt;

            av_log(avctx, AV_LOG_VERBOSE, "codec_id:%d, time_base:(%d,%d), sample_rate:%d, channels:%d, sample_fmt:%d \n", 
                stream_info->codec_id, stream_info->time_base.num, stream_info->time_base.den, stream_info->sample_rate, stream_info->channels, stream_info->sample_fmt);

        }else{
            av_log(avctx, AV_LOG_ERROR,"not support the type:%d\n", stream_info->codec_type);
        }
    }


    #ifdef TEST_FILE_OUT
        ctx->yuv_file = fopen(TEST_FILE_OUT, "w+");
        if(ctx->yuv_file == NULL){
            av_log(avctx, AV_LOG_ERROR,"open file:%s for test failed.\n", TEST_FILE_OUT);
        }
    #endif 

    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_read_header over\n");

    return 0;

ff_smem_read_header_error:

    return AVERROR(EIO);
}


static int ff_smem_read_packet(AVFormatContext *avctx, AVPacket *pkt)
{
    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_read_packet\n");
    struct smem_dec_ctx * ctx = avctx->priv_data;

    int packet_size, ret, width, height; 

    AVStream *st = avctx->streams[0];


    static uint8_t *buffer = NULL;
    int mem_id = -1;
    uint8_t *mem_ptr = NULL;
    struct memory_info2 * m_info;

    int64_t last_time = av_gettime_relative();

    int cnt = 0;
    av_init_packet(pkt);
    while(1){
        // get memory 
        mem_id = smemGetShareMemory(ctx->sctx, 0);
        if(mem_id > 0){
            //av_log(avctx, AV_LOG_VERBOSE, "get memory id: %d\n", mem_id);

            // get the memory ptr
            mem_ptr = shmat(mem_id, 0, 0);
            if(mem_ptr == (void *)-1){
                av_log(avctx, AV_LOG_ERROR, "get share memory(%d) pointer error.\n", mem_id);
                smemFreeShareMemory(ctx->sctx, mem_id);
                av_log(avctx, AV_LOG_ERROR, "get share memory(%d) pointer error and free over.\n", mem_id);
                av_usleep(1);
                continue;
            }
            //av_log(avctx, AV_LOG_VERBOSE, "get memory mem_ptr: %ld\n", mem_ptr);
            m_info = (struct memory_info2 * )mem_ptr;
            
            av_init_packet(pkt);

            //av_log(avctx, AV_LOG_VERBOSE, "data_size: %d\n", m_info->data_size);

            av_new_packet(pkt, m_info->data_size);
            pkt->stream_index = m_info->index;
            pkt->pts = m_info->pts;
            pkt->dts = m_info->pts;
            pkt->flags |= AV_PKT_FLAG_KEY;
            
            ret = memcpy(pkt->data, mem_ptr + m_info->data_offset, m_info->data_size);
            /*
            if(ctx->stream_infos[pkt->stream_index].codec_type == AVMEDIA_TYPE_AUDIO)
                ret = memcpy(pkt->data, mem_ptr + m_info->data_offset, m_info->data_size);
            else{
                AVPicture *avpicture = (AVPicture *) pkt->data;

                ret = av_image_fill_arrays(avpicture->data, avpicture->linesize,
                                    mem_ptr + m_info->data_offset, ctx->stream_infos[pkt->stream_index].pix_fmt,
                                    ctx->stream_infos[pkt->stream_index].width, ctx->stream_infos[pkt->stream_index].height, 1);
            }
            */
            av_log(avctx, AV_LOG_ERROR, "get memory packet, stream_index:%d, time_base:(%d,%d) pts:%lld, size:%d, data: %ld, mem_ptr:%lld, data_offset:%d, data_size:%d, ret=%d \n", 
                pkt->stream_index, ctx->stream_infos[pkt->stream_index].time_base.num, ctx->stream_infos[pkt->stream_index].time_base.den, pkt->pts, pkt->size, pkt->data,
                mem_ptr, m_info->data_offset, m_info->data_size, ret);

            //av_log(avctx, AV_LOG_VERBOSE, "memcpy\n");
            #ifdef TEST_FILE_OUT
                if(ctx->yuv_file && ctx->stream_infos[pkt->stream_index].codec_type == AVMEDIA_TYPE_VIDEO){
                    fwrite(pkt->data, m_info->data_size, 1, ctx->yuv_file);
                }
            #endif 

            // free the share memory 
            if(mem_ptr){
                ret = shmdt(mem_ptr);
                if(ret < 0){
                    av_log(avctx, AV_LOG_ERROR, "release share memory ptr error:%d\n", ret);
                    //return -1;
                    continue;
                }
            }

            // free the memory id
            av_log(avctx, AV_LOG_ERROR, "smemFreeShareMemory, mem_id=%d\n", mem_id);
            smemFreeShareMemory(ctx->sctx, mem_id);
            av_log(avctx, AV_LOG_ERROR, "smemFreeShareMemory, mem_id=%d over.\n", mem_id);

            cnt = 0;

            break;
        
        }

        av_usleep(1);
        //av_log(avctx, AV_LOG_VERBOSE, "av_usleep %d\n", cnt++);

        // when timeout break
        if(av_gettime_relative() - last_time > ctx->timeout*1000000){
            av_log(avctx, AV_LOG_INFO, "timeout:%d\n", ctx->timeout);
            return -1;
        }


    }
    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_read_packet over\n");
    return 0;
}


av_cold static int ff_smem_read_close(AVFormatContext *avctx)
{
    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_read_close\n");
    struct smem_dec_ctx * ctx = avctx->priv_data;

    #ifdef TEST_FILE_OUT
        if(ctx->yuv_file){
            fclose(ctx->yuv_file);
            ctx->yuv_file = NULL;
        }
    #endif 

    return 0;
}


#define OFFSET(x) offsetof(smem_dec_ctx, x)
#define DEC AV_OPT_FLAG_DECODING_PARAM
#define ENC AV_OPT_FLAG_ENCODING_PARAM

static const AVOption options[] = {
    { "timeout", "set maximum timeout (in seconds)", OFFSET(timeout), AV_OPT_TYPE_INT, {.i64 = 5}, INT_MIN, INT_MAX, DEC },
    { NULL },
};

static const AVClass smem_demuxer_class = {
    .class_name = "smem demuxer",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
    .category   = AV_CLASS_CATEGORY_DEVICE_INPUT,
};


AVInputFormat ff_smem_demuxer = {
    .name           = "smem",
    .long_name      = NULL_IF_CONFIG_SMALL("Test smem input"),
    .flags          = AVFMT_NOFILE ,
    .priv_class     = &smem_demuxer_class,
    .priv_data_size = sizeof(struct smem_dec_ctx),
    .read_header   = ff_smem_read_header,
    .read_packet   = ff_smem_read_packet,
    .read_close    = ff_smem_read_close,
};
