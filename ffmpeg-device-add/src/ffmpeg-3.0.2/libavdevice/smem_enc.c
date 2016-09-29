
#include "libavutil/opt.h"
#include "config.h"
#include "libavformat/avformat.h"
#include "libavformat/internal.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"

#include <string.h>

#include "smem.h"
#include "smem_com.h"
#include "smem_client.h"


static const AVCodecTag smem_codec_ids[] = {
    { AV_CODEC_ID_RAWVIDEO    , 0x01 },
    { AV_CODEC_ID_PCM_S16LE   , 0x02 },
    { AV_CODEC_ID_MPEG4       , 0x20 },
    { AV_CODEC_ID_H264        , 0x21 },
    { AV_CODEC_ID_HEVC        , 0x23 },
    { AV_CODEC_ID_MPEG2VIDEO  , 0x61 }, /* MPEG2 */
    { AV_CODEC_ID_AAC         , 0x66 }, /* AAC  */
    { AV_CODEC_ID_MP3         , 0x69 }, /* 13818-3 */
    { AV_CODEC_ID_MP2         , 0x69 }, /* 11172-3 */
    { AV_CODEC_ID_MPEG1VIDEO  , 0x6A }, /* 11172-2 */
    { AV_CODEC_ID_MP3         , 0x6B }, /* 11172-3 */
    { AV_CODEC_ID_MJPEG       , 0x6C }, /* 10918-1 */
    { AV_CODEC_ID_PNG         , 0x6D },
    { AV_CODEC_ID_JPEG2000    , 0x6E }, /* 15444-1 */
    { AV_CODEC_ID_VC1         , 0xA3 },
    { AV_CODEC_ID_AC3         , 0xA5 },
    { AV_CODEC_ID_EAC3        , 0xA6 },
    { AV_CODEC_ID_DTS         , 0xA9 }, /* mp4ra.org */
    { AV_CODEC_ID_VORBIS      , 0xDD }, /* non standard, gpac uses it */
    { AV_CODEC_ID_NONE        ,    0 },
};



typedef struct smem_enc_ctx {
    const AVClass *class;

    struct smemContext * sctx;

    int stream_number;
    struct stream_info * stream_infos;
    int stream_info_size;
    int pic_size;


    /* Params */
    char ip[64];    /* the share memory server ip address */
    char channel[128];  /* the channel in share memory server */

    int width;
    int height;

    /* Options */
    char *path; // the unix domain socket path
    int  port;      /* the share memory server port */
}smem_enc_ctx;


av_cold static int ff_smem_write_header(AVFormatContext *avctx)
{
    AVStream *stream = NULL;


    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_write_header begin\n");

    struct smem_enc_ctx * ctx = avctx->priv_data;

    ctx->pic_size = 0;

    /* get the input stream url, the stream format should be like :  "smem://127.0.0.1:6379/channel_name"*/
    av_log(avctx, AV_LOG_VERBOSE, "channel name:%s\n", avctx->filename);

    if(strlen(avctx->filename) <= 0){
        av_log(avctx, AV_LOG_ERROR, "output path should not null.\n");
        return AVERROR(EIO);
    }

    /* setup the streams */
    ctx->stream_number = avctx->nb_streams;
    if(ctx->stream_number > 0){
        ctx->stream_info_size = ctx->stream_number * sizeof(struct stream_info);
        ctx->stream_infos = av_malloc(ctx->stream_info_size);

        memset(ctx->stream_infos, 0 , ctx->stream_info_size);

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
            //av_log(avctx, AV_LOG_VERBOSE, "ff_smem_write_header stream[%d] is audio stream.\n", n);

            ctx->stream_infos[n].index = n;
            ctx->stream_infos[n].codec_type = SMEM_MEDIA_TYPE_AUDIO;
            ctx->stream_infos[n].codec_id = av2smem_codec_id(c->codec_id);
            ctx->stream_infos[n].time_base.den = c->time_base.den;
            ctx->stream_infos[n].time_base.num = c->time_base.num;
            //ctx->stream_infos[n].time_base = AV_TIME_BASE_Q;

            ctx->stream_infos[n].sample_rate = c->sample_rate;
            ctx->stream_infos[n].channels = c->channels;
            ctx->stream_infos[n].sample_fmt = av2smem_sample_fmt(c->sample_fmt);
            ctx->stream_infos[n].audio_extradata_size = c->extradata_size;
            if(c->extradata_size > 0){
                memcpy(ctx->stream_infos[n].audio_extradata, c->extradata, c->extradata_size);
            }

            av_log(avctx, AV_LOG_INFO, "ff_smem_write_header, stream[%d] is audio stream, codec_id=%d, sample_rate=%d, channels=%d, sample_fmt=%d,extradata_size=%d.\n", 
                n, c->codec_id, c->sample_rate, c->channels, c->sample_fmt, c->extradata_size);

            // for test
            //if(c->extradata_size > 0){
            //    print_hex(c->extradata, c->extradata_size);
            //}

        } else if (c->codec_type == AVMEDIA_TYPE_VIDEO) {
            //av_log(avctx, AV_LOG_VERBOSE, "ff_smem_write_header stream[%d] is video stream.\n", n);

            ctx->width = c->width;
            ctx->height = c->height;


            ctx->stream_infos[n].index = n;
            ctx->stream_infos[n].codec_type = SMEM_MEDIA_TYPE_VIDEO;
            ctx->stream_infos[n].codec_id = av2smem_codec_id(c->codec_id);
            ctx->stream_infos[n].time_base.den = c->time_base.den;
            ctx->stream_infos[n].time_base.num = c->time_base.num;
            //ctx->stream_infos[n].time_base = AV_TIME_BASE_Q;

            ctx->stream_infos[n].width = c->width;
            ctx->stream_infos[n].height = c->height;
            ctx->stream_infos[n].pix_fmt = av2smem_pix_fmt(c->pix_fmt);
            ctx->stream_infos[n].video_extradata_size = c->extradata_size;
            if(c->extradata_size > 0){
                memcpy(ctx->stream_infos[n].video_extradata, c->extradata, c->extradata_size);
            }

            ctx->pic_size = av_image_get_buffer_size(c->pix_fmt, c->width, c->height, 1);

            av_log(avctx, AV_LOG_INFO, "ff_smem_write_header, stream[%d] is video stream, codec_id=%d, pix_fmt=%d, width=%d, height=%d, extradata_size=%d.\n", 
                n, c->codec_id, c->pix_fmt, c->width, c->height, c->extradata_size);

        } else {
            av_log(avctx, AV_LOG_ERROR, "Unsupported stream type.\n");
            goto ff_smem_write_header_error;
        }
    }


    /* connect to the server */
    if(ctx->path == NULL){
        av_log(avctx, AV_LOG_INFO, "use ip socket, ip is '127.0.0.1', port=%d.\n", ctx->port);
        ctx->sctx = smemCreateProducer("127.0.0.1", ctx->port, avctx->filename);
    }else{
        av_log(avctx, AV_LOG_INFO, "use unix domain socket, path is'%s'.\n", ctx->path);
        ctx->sctx = smemCreateProducerUnix(ctx->path, avctx->filename);
    }

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

    int mem_id = -1;
    uint8_t *mem_ptr = NULL;
    int ret;

    // get the mem_size
    int pic_size = ctx->pic_size;
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
        av_log(avctx, AV_LOG_ERROR,"get memory failed...\n");
        return 0;  // fixme: should retry to get memory
    }

    // set the memory info
    struct memory_info * m_info = (struct memory_info * )mem_ptr;
    m_info->id = mem_id;
    m_info->index = pkt->stream_index;
    //m_info->pts = av_rescale_q(pkt->pts, ctx->stream_infos[pkt->stream_index].time_base, AV_TIME_BASE_Q);
    m_info->pts = pkt->pts; // fixme:
    m_info->dts = pkt->dts;
    m_info->stream_info_offset = sizeof(struct memory_info);
    m_info->stream_info_number = ctx->stream_number;
    m_info->data_offset = sizeof(struct memory_info) + ctx->stream_info_size;
    m_info->data_size = pic_size;

    if(pkt->flags & AV_PKT_FLAG_KEY)
        m_info->is_key = 1;
    else
        m_info->is_key = 0;

    // copy the streams info
    memcpy(mem_ptr+m_info->stream_info_offset, ctx->stream_infos, ctx->stream_info_size);

    //av_log(avctx, AV_LOG_ERROR,"linesize[0]=%d, linesize[1]=%d, linesize[2]=%d, width=%d, height=%d, pix_fmt=%d,\n", 
    //    avpicture->linesize[0], avpicture->linesize[1], avpicture->linesize[2], 
    //    ctx->stream_infos[pkt->stream_index].width, ctx->stream_infos[pkt->stream_index].height, ctx->stream_infos[pkt->stream_index].pix_fmt);

    // copy the video data 
    //ret = av_image_copy_to_buffer(mem_ptr + m_info->data_offset, 
    //    pic_size, (const uint8_t **)pkt->data, avpicture->linesize, ctx->stream_infos[pkt->stream_index].pix_fmt, 
    //    ctx->stream_infos[pkt->stream_index].width, ctx->stream_infos[pkt->stream_index].height, 1);

    memcpy(mem_ptr+m_info->data_offset, pkt->data, m_info->data_size);
    
    av_log(avctx, AV_LOG_VERBOSE,"stream_index=%d, pts=%lld, time_base=(%d, %d), stream_info_offset=%d, data_offset=%d, data_size=%d, ret=%d.\n", pkt->stream_index, m_info->pts, 
        ctx->stream_infos[pkt->stream_index].time_base.num, ctx->stream_infos[pkt->stream_index].time_base.den,
        m_info->stream_info_offset, m_info->data_offset, m_info->data_size, ret);




    // release share memory
    if(mem_ptr){
        ret = shmdt(mem_ptr);
        if(ret < 0){
            av_log(avctx, AV_LOG_ERROR,"release share memory ptr error:%d\n", ret);
            return -1;
        }
    }

    // publish share memory id
    smemPublish(ctx->sctx, mem_id);   // publish the memory

    // free share memory id
    //smemFreeShareMemory(ctx->sctx, mem_id); // free the share memory

    return 0;
}

static int ff_smem_write_audio_packet(AVFormatContext *avctx, AVPacket *pkt)
{

    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_write_audio_packet\n");
    struct smem_enc_ctx * ctx = avctx->priv_data;

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
    m_info->id = mem_id;
    m_info->index = pkt->stream_index;
    //m_info->pts = av_rescale_q(pkt->pts, ctx->stream_infos[pkt->stream_index].time_base, AV_TIME_BASE_Q);
    m_info->pts = pkt->pts; // fixme:
    m_info->dts = pkt->dts;
    m_info->stream_info_offset = sizeof(struct memory_info);
    m_info->stream_info_number = ctx->stream_number;
    m_info->data_offset = sizeof(struct memory_info) + ctx->stream_info_size;
    m_info->data_size = buf_size;

    if(pkt->flags & AV_PKT_FLAG_KEY)
        m_info->is_key = 1;
    else
        m_info->is_key = 0;

    // copy the streams info
    memcpy(mem_ptr+m_info->stream_info_offset, ctx->stream_infos, ctx->stream_info_size);

    // copy the audio data 
    memcpy(mem_ptr+m_info->data_offset, pkt->data, m_info->data_size);


    av_log(avctx, AV_LOG_VERBOSE,"stream_index=%d, pts=%lld, time_base=(%d, %d).\n", pkt->stream_index, m_info->pts, 
        ctx->stream_infos[pkt->stream_index].time_base.num, ctx->stream_infos[pkt->stream_index].time_base.den);



    // release share memory
    if(mem_ptr){
        ret = shmdt(mem_ptr);
        if(ret < 0){
            av_log(avctx, AV_LOG_ERROR,"release share memory ptr error:%d\n", ret);
            return -1;
        }
    }

    // publish share memory id
    smemPublish(ctx->sctx, mem_id);   // publish the memory

    // free share memory id
    //smemFreeShareMemory(ctx->sctx, mem_id); // free the memory

    return 0;
}

static int ff_smem_write_packet(AVFormatContext *avctx, AVPacket *pkt)
{

    struct smem_enc_ctx * ctx = avctx->priv_data;


    AVStream *st = avctx->streams[pkt->stream_index];

    if      (st->codec->codec_type == AVMEDIA_TYPE_VIDEO){
        if(st->codec->codec_id == AV_CODEC_ID_RAWVIDEO){
            return ff_smem_write_video_packet(avctx, pkt);
        }else{
            return ff_smem_write_audio_packet(avctx, pkt);
        }
    }else if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO){
        return ff_smem_write_audio_packet(avctx, pkt);
    }

    return AVERROR(EIO);
}


av_cold static int ff_smem_write_close(AVFormatContext *avctx)
{
    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_write_close\n");

    struct smem_enc_ctx * ctx = avctx->priv_data;

    if(ctx->sctx)
        smemFree(ctx->sctx);

    if(ctx->stream_infos)
        av_free(ctx->stream_infos);

    return 0;
}


#define OFFSET(x) offsetof(smem_enc_ctx, x)
#define DEC AV_OPT_FLAG_DECODING_PARAM
#define ENC AV_OPT_FLAG_ENCODING_PARAM

static const AVOption options[] = {
    { "path", "the unix domain socket path", OFFSET(path), AV_OPT_TYPE_STRING, {0}, 0, 0, ENC },
    { "port", "the redis server port", OFFSET(port), AV_OPT_TYPE_INT, {.i64 = 6379}, INT_MIN, INT_MAX, ENC },
    { NULL },
};

static const AVClass smem_muxer_class = {
    .class_name = "smem muxer",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
    .category   = AV_CLASS_CATEGORY_DEVICE_OUTPUT,
};


AVOutputFormat ff_smem_muxer = {
    .name           = "smem",
    .long_name      = NULL_IF_CONFIG_SMALL("smem muxer"),
    .audio_codec    = AV_CODEC_ID_PCM_S16LE,
    .video_codec    = AV_CODEC_ID_RAWVIDEO,
    .subtitle_codec = AV_CODEC_ID_NONE,
    .flags          = AVFMT_NOFILE | AVFMT_GLOBALHEADER | AVFMT_ALLOW_FLUSH | AVFMT_VARIABLE_FPS,
    .priv_class     = &smem_muxer_class,
    .priv_data_size = sizeof(struct smem_enc_ctx),
    .write_header   = ff_smem_write_header,
    .write_packet   = ff_smem_write_packet,
    .write_trailer  = ff_smem_write_close,
    //.codec_tag      = (const AVCodecTag* const []) { smem_codec_ids, 0 },
};
