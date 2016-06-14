
#include "libavutil/opt.h"
#include "config.h"
#include "libavformat/avformat.h"
#include "libavformat/internal.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"

#include <string.h>

#include "smem_enc.h"
#include "smem_client.h"

struct smem_enc_ctx {
    const AVClass *class;

    struct smemContext * sctx;

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
    int n;
    AVCodecContext *c = NULL;
    for (n = 0; n < avctx->nb_streams; n++) {
        stream = avctx->streams[n];
        c = stream->codec;
        if(c->codec_type == AVMEDIA_TYPE_AUDIO) {
            av_log(avctx, AV_LOG_VERBOSE, "ff_smem_write_header stream[%d] is audio stream.\n", n);
        } else if (c->codec_type == AVMEDIA_TYPE_VIDEO) {
            av_log(avctx, AV_LOG_VERBOSE, "ff_smem_write_header stream[%d] is video stream.\n", n);

            ctx->width = 1920;
            ctx->height = 1080;

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


    // get share memory id
    //mem_id = smemGetShareMemory(ctx->sctx, pkt->size);  // get share memory
    mem_id = smemGetShareMemory(ctx->sctx, 1920*1080*3/2);  // get share memory
    if(mem_id > 0){
        //printf("get memory id: %d\n", mem_id);

        // get the memory ptr
        mem_ptr = shmat(mem_id, 0, 0);
        if(mem_ptr == (void *)-1){
            av_log(avctx, AV_LOG_ERROR,"get share memory(%d) ptr error.\n", mem_id);
            return -1;
        }
    }else{
        printf("get memory failed...\n");
        return -1;
    }

    // write data to share memory 
    //memcpy(mem_ptr,  pkt->data,  pkt->size);
    av_image_copy_to_buffer(mem_ptr, 1920*1080*3/2, (const uint8_t **)pkt->data, avpicture->linesize, AV_PIX_FMT_YUV420P, 1920, 1080, 1);
    //memset(mem_ptr, av_gettime()%255,  1920*1080*3/2);

    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_write_video_packet, packet size=%d\n", pkt->size);

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
