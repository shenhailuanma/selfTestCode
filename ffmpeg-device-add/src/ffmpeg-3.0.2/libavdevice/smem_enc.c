
#include "libavutil/opt.h"
#include "config.h"
#include "libavformat/avformat.h"
#include "libavformat/internal.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"

#include <string.h>

#include "smem_enc.h"
#include "smem_client.h"

struct memory_info {
    int index;  // stream index
    int64_t pts;
    int64_t dts;
    int stream_info_offset;
    int stream_info_number;
    int data_offset;
    int data_size;
};

struct stream_info {
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

struct smem_enc_ctx {
    const AVClass *class;

    struct smemContext * sctx;

    int stream_number;
    struct stream_info * stream_infos;
    int stream_info_size;


    /* Params */
    char ip[64];    /* the share memory server ip address */
    int  port;      /* the share memory server port */
    char channel[128];  /* the channel in share memory server */

    int width;
    int height;

    /* Options */
};


av_cold static int ff_smem_write_header(AVFormatContext *avctx)
{
    AVStream *stream = NULL;


    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_write_header begin\n");

    struct smem_enc_ctx * ctx = avctx->priv_data;

    /* get the input stream url, the stream format should be like :  "smem://127.0.0.1:6379/channel_name"*/
    av_log(avctx, AV_LOG_VERBOSE, "channel name:%s\n", avctx->filename);



    /* setup the streams */
    ctx->stream_number = avctx->nb_streams;
    if(ctx->stream_number > 0){
        ctx->stream_info_size = ctx->stream_number * sizeof(struct stream_info);
        ctx->stream_infos = av_malloc(ctx->stream_info_size);

        if(!ctx->stream_infos){
            av_log(avctx, AV_LOG_ERROR, "av_malloc stream_info failed.\n");
            return AVERROR(ENOMEM);
        }
    }

    int n;
    AVCodecContext *c = NULL;
    for (n = 0; n < avctx->nb_streams; n++) {
        stream = avctx->streams[n];
        c = stream->codec;
        if(c->codec_type == AVMEDIA_TYPE_AUDIO) {
            av_log(avctx, AV_LOG_VERBOSE, "ff_smem_write_header stream[%d] is audio stream.\n", n);

            ctx->stream_infos[n].index = n;
            ctx->stream_infos[n].codec_type = AVMEDIA_TYPE_AUDIO;
            ctx->stream_infos[n].codec_id = c->codec_id;
            ctx->stream_infos[n].time_base = c->time_base;

            ctx->stream_infos[n].sample_rate = c->sample_rate;
            ctx->stream_infos[n].channels = c->channels;
            ctx->stream_infos[n].sample_fmt = c->sample_fmt;

        } else if (c->codec_type == AVMEDIA_TYPE_VIDEO) {
            av_log(avctx, AV_LOG_VERBOSE, "ff_smem_write_header stream[%d] is video stream.\n", n);

            ctx->width = c->width;
            ctx->height = c->height;


            ctx->stream_infos[n].index = n;
            ctx->stream_infos[n].codec_type = AVMEDIA_TYPE_VIDEO;
            ctx->stream_infos[n].codec_id = c->codec_id;
            ctx->stream_infos[n].time_base = c->time_base;

            ctx->stream_infos[n].width = c->width;
            ctx->stream_infos[n].height = c->height;
            ctx->stream_infos[n].pix_fmt = c->pix_fmt;

            av_log(avctx, AV_LOG_VERBOSE, "ff_smem_write_header video width=%d, height=%d.\n", c->width, c->height);

        } else {
            av_log(avctx, AV_LOG_ERROR, "Unsupported stream type.\n");
            goto ff_smem_write_header_error;
        }
    }


    /* connect to the server */
    ctx->sctx = smemCreateProducer("127.0.0.1", 6379, "test");
    if(!ctx->sctx || ctx->sctx->err < 0){
        if (ctx->sctx) {
            av_log(avctx, AV_LOG_ERROR,"Connection error: %s\n", ctx->sctx->errstr);
            smemFree(ctx->sctx);
        } else {
            av_log(avctx, AV_LOG_ERROR,"Connection error: can't allocate redis context\n");
        }
    }



    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_write_header over\n");

    return 0;

ff_smem_write_header_error:

    return AVERROR(EIO);
}


static int ff_smem_write_video_packet(AVFormatContext *avctx, AVPacket *pkt)
{
    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_write_video_packet\n");
    struct smem_enc_ctx * ctx = avctx->priv_data;

    AVPicture *avpicture = (AVPicture *) pkt->data;
    AVFrame *avframe, *tmp;
    int mem_id = -1;
    uint8_t *mem_ptr = NULL;
    int ret;

    // get the mem_size
    int pic_size = av_image_get_buffer_size(ctx->stream_infos[pkt->stream_index].pix_fmt, ctx->stream_infos[pkt->stream_index].width, ctx->stream_infos[pkt->stream_index].height, 1);
    int mem_size = sizeof(struct memory_info) + ctx->stream_info_size + pic_size;

    av_log(avctx, AV_LOG_VERBOSE, "[ff_smem_write_video_packet], sizeof(memory_info)=%d, tream_info_size=%d, pic_size=%d\n", 
        sizeof(struct memory_info),ctx->stream_info_size, pic_size);

    // get share memory id
    mem_id = smemGetShareMemory(ctx->sctx, mem_size);  // get share memory
    if(mem_id > 0){
        //printf("get memory id: %d\n", mem_id);

        // get the memory ptr
        mem_ptr = shmat(mem_id, 0, 0);
        if(mem_ptr == (void *)-1){
            av_log(avctx, AV_LOG_ERROR,"get share memory(%d) ptr error.\n", mem_id);
            return 0; // fixme: should retry 
        }
        memset(mem_ptr, 0, mem_size);

    }else{
        printf("get memory failed...\n");
        return 0;  // fixme: should retry to get memory
    }

    // set the memory info
    struct memory_info * m_info = (struct memory_info * )mem_ptr;
    m_info->index = pkt->stream_index;
    m_info->pts = av_rescale_q(pkt->pts, ctx->stream_infos[pkt->stream_index].time_base, AV_TIME_BASE_Q);
    m_info->stream_info_offset = sizeof(struct memory_info);
    m_info->stream_info_number = ctx->stream_number;
    m_info->data_offset = sizeof(struct memory_info) + ctx->stream_info_size;
    m_info->data_size = pic_size;

    // copy the streams info
    memcpy(mem_ptr+m_info->stream_info_offset, ctx->stream_infos, ctx->stream_info_size);

    // copy the video data 
    av_image_copy_to_buffer(mem_ptr + m_info->data_offset, 
        pic_size, (const uint8_t **)pkt->data, avpicture->linesize, ctx->stream_infos[pkt->stream_index].pix_fmt, 
        ctx->stream_infos[pkt->stream_index].width, ctx->stream_infos[pkt->stream_index].height, 1);

    

    // publish share memory id
    smemPublish(ctx->sctx, mem_id);   // publish the memory

    // release share memory
    if(mem_ptr){
        ret = shmdt(mem_ptr);
        if(ret < 0){
            av_log(avctx, AV_LOG_ERROR,"release share memory ptr error:%d\n", ret);
            return -1;
        }
    }

    
    // free share memory id
    smemFreeShareMemory(ctx->sctx, mem_id); // free the memory

    return 0;
}

static int ff_smem_write_audio_packet(AVFormatContext *avctx, AVPacket *pkt)
{
    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_write_audio_packet\n");
    struct smem_enc_ctx * ctx = avctx->priv_data;

    AVPicture *avpicture = (AVPicture *) pkt->data;
    AVFrame *avframe, *tmp;
    int mem_id = -1;
    uint8_t *mem_ptr = NULL;
    int ret;

    // get the mem_size
    int buf_size = pkt->size;
    int mem_size = sizeof(struct memory_info) + ctx->stream_info_size + buf_size;

    av_log(avctx, AV_LOG_VERBOSE, "[ff_smem_write_audio_packet], sizeof(memory_info)=%d, stream_info_size=%d, buf_size=%d\n", 
        sizeof(struct memory_info),ctx->stream_info_size, buf_size);

    // get share memory id
    mem_id = smemGetShareMemory(ctx->sctx, mem_size);  // get share memory
    if(mem_id > 0){
        //printf("get memory id: %d\n", mem_id);

        // get the memory ptr
        mem_ptr = shmat(mem_id, 0, 0);
        if(mem_ptr == (void *)-1){
            av_log(avctx, AV_LOG_ERROR,"get share memory(%d) ptr error.\n", mem_id);
            return 0; // fixme: should retry 
        }
        memset(mem_ptr, 0, mem_size);

    }else{
        printf("get memory failed...\n");
        return 0;  // fixme: should retry to get memory
    }

    // set the memory info
    struct memory_info * m_info = (struct memory_info * )mem_ptr;
    m_info->index = pkt->stream_index;
    m_info->pts = av_rescale_q(pkt->pts, ctx->stream_infos[pkt->stream_index].time_base, AV_TIME_BASE_Q);
    m_info->stream_info_offset = sizeof(struct memory_info);
    m_info->stream_info_number = ctx->stream_number;
    m_info->data_offset = sizeof(struct memory_info) + ctx->stream_info_size;
    m_info->data_size = buf_size;

    // copy the streams info
    memcpy(mem_ptr+m_info->stream_info_offset, ctx->stream_infos, ctx->stream_info_size);

    // copy the video data 
    memcpy(mem_ptr+m_info->data_offset, pkt->data, m_info->data_size);

    // publish share memory id
    smemPublish(ctx->sctx, mem_id);   // publish the memory

    // release share memory
    if(mem_ptr){
        ret = shmdt(mem_ptr);
        if(ret < 0){
            av_log(avctx, AV_LOG_ERROR,"release share memory ptr error:%d\n", ret);
            return -1;
        }
    }

    
    // free share memory id
    smemFreeShareMemory(ctx->sctx, mem_id); // free the memory

    return 0;
}

static int ff_smem_write_packet(AVFormatContext *avctx, AVPacket *pkt)
{
    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_write_packet\n");
    struct smem_enc_ctx * ctx = avctx->priv_data;


    AVStream *st = avctx->streams[pkt->stream_index];

    if      (st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        return ff_smem_write_video_packet(avctx, pkt);
    else if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        return ff_smem_write_audio_packet(avctx, pkt);


    return AVERROR(EIO);
}


av_cold static int ff_smem_write_close(AVFormatContext *avctx)
{
    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_write_close\n");
    return 0;
}


static const AVOption options[] = {
{ NULL },
};

static const AVClass smem_muxer_class = {
    .class_name = "smem muxer",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
    .category   = AV_CLASS_CATEGORY_DEVICE_VIDEO_OUTPUT,
};


AVOutputFormat ff_smem_muxer = {
    .name           = "smem",
    .long_name      = NULL_IF_CONFIG_SMALL("Test smem output"),
    .audio_codec    = AV_CODEC_ID_PCM_S16LE,
    .video_codec    = AV_CODEC_ID_RAWVIDEO,
    .subtitle_codec = AV_CODEC_ID_NONE,
    .flags          = AVFMT_NOFILE | AVFMT_RAWPICTURE,
    .priv_class     = &smem_muxer_class,
    .priv_data_size = sizeof(struct smem_enc_ctx),
    .write_header   = ff_smem_write_header,
    .write_packet   = ff_smem_write_packet,
    .write_trailer  = ff_smem_write_close,
};
