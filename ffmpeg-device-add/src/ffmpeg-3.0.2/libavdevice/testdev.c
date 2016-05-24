#include "libavformat/avformat.h"
#include "libavutil/opt.h"


struct testdev_cctx {
    const AVClass *cclass;

    void *ctx;

    /* Options */
};


av_cold static int ff_testdev_read_header(AVFormatContext *avctx)
{
    av_log(avctx, AV_LOG_VERBOSE, "ff_testdev_read_header\n");
    return 0;
}


static int ff_testdev_read_packet(AVFormatContext *avctx, AVPacket *pkt)
{
    av_log(avctx, AV_LOG_VERBOSE, "lavfi_read_packet\n");
    return 0;
}


av_cold static int ff_testdev_read_close(AVFormatContext *avctx)
{
    av_log(avctx, AV_LOG_VERBOSE, "ff_testdev_read_close\n");
    return 0;
}


static const AVOption options[] = {
{ NULL },
};

static const AVClass testdev_demuxer_class = {
    .class_name = "Test device demuxer",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
    .category   = AV_CLASS_CATEGORY_DEVICE_VIDEO_INPUT,
};


AVInputFormat ff_testdev_demuxer = {
    .name           = "testdev",
    .long_name      = NULL_IF_CONFIG_SMALL("Test device input"),
    .flags          = AVFMT_NOFILE | AVFMT_RAWPICTURE,
    .priv_class     = &testdev_demuxer_class,
    .priv_data_size = sizeof(struct testdev_cctx),
    .read_header   = ff_testdev_read_header,
    .read_packet   = ff_testdev_read_packet,
    .read_close    = ff_testdev_read_close,
};
