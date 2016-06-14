
#include "libavutil/opt.h"
#include "config.h"
#include "libavformat/avformat.h"
#include "libavformat/internal.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"

#include "smem_dec.h"
#include "smem_client.h"

struct smem_dec_ctx {
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

    /* create the streams */

    // add video stream
    stream = avformat_new_stream(avctx, NULL);
    if (!stream) {
        av_log(avctx, AV_LOG_ERROR, "Cannot add stream\n");
        goto ff_smem_read_header_error;
    }

    ctx->width = 1920;
    ctx->height= 1080;

    stream->codec->codec_type  = AVMEDIA_TYPE_VIDEO;
    stream->codec->width       = ctx->width;
    stream->codec->height      = ctx->height;
    stream->codec->time_base.den      = 25;
    stream->codec->time_base.num      = 1;

        stream->codec->codec_id    = AV_CODEC_ID_RAWVIDEO; // fixme: should auto set
        stream->codec->pix_fmt     = AV_PIX_FMT_YUV420P;  // fixme: should auto set
        stream->codec->codec_tag   = MKTAG('I','4','2','0');

    stream->codec->bit_rate    = av_image_get_buffer_size(stream->codec->pix_fmt, ctx->width, ctx->height, 1) * 1/av_q2d(stream->codec->time_base) * 8;

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

    packet_size = avpicture_get_size(st->codec->pix_fmt, ctx->width, ctx->height);
    if (packet_size < 0)
        return -1;

    av_log(avctx, AV_LOG_VERBOSE, "packet_size: %d\n", packet_size);

    uint8_t *buffer = NULL;
    int mem_id = -1;
    uint8_t *mem_ptr = NULL;

    int cnt = 0;
    while(1){
        // get memory 
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

            av_init_packet(pkt);

            av_log(avctx, AV_LOG_VERBOSE, "av_init_packet\n");
            buffer = av_malloc(packet_size);
            av_log(avctx, AV_LOG_VERBOSE, "get memory buffer: %ld\n", buffer);

            memcpy(buffer, mem_ptr, packet_size);

            av_log(avctx, AV_LOG_VERBOSE, "memcpy\n");

            pkt->data = buffer;
            pkt->size = packet_size;

            // free the memory id
            smemFreeShareMemory(ctx->sctx, mem_id);
            av_log(avctx, AV_LOG_VERBOSE, "smemFreeShareMemory\n");

            cnt = 0;
            break;
        }

        av_usleep(10000);
        av_log(avctx, AV_LOG_VERBOSE, "av_usleep %d\n", cnt++);
    }
    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_read_packet over\n");
    return 0;
}


av_cold static int ff_smem_read_close(AVFormatContext *avctx)
{
    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_read_close\n");
    return 0;
}


static const AVOption options[] = {
{ NULL },
};

static const AVClass smem_demuxer_class = {
    .class_name = "smem demuxer",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
    .category   = AV_CLASS_CATEGORY_DEVICE_VIDEO_INPUT,
};


AVInputFormat ff_smem_demuxer = {
    .name           = "smem",
    .long_name      = NULL_IF_CONFIG_SMALL("Test smem input"),
    .flags          = AVFMT_NOFILE | AVFMT_RAWPICTURE,
    .priv_class     = &smem_demuxer_class,
    .priv_data_size = sizeof(struct smem_dec_ctx),
    .read_header   = ff_smem_read_header,
    .read_packet   = ff_smem_read_packet,
    .read_close    = ff_smem_read_close,
};
