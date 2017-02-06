


#include "smem.h"
#include "smem_com.h"
#include "smem_client.h"





#define SMEM_MAX_STREAM     64
#define SMEM_TIME_BASE      1000000
#define SMEM_TIME_BASE_Q    (AVRational){1, SMEM_TIME_BASE}

#define SMEM_NUM_IN_RANGE(n,min,max)  ((n) > (max) ? (max) : ((n) < (min) ? (min) : (n)))
#define SMEM_NUM_IF_OUT_RANGE(n,min,max)  ((n) > (max) ? 1 : ((n) < (min) ? 1 : 0))

typedef struct smem_dec_ctx {
    const AVClass *class;

    struct smemContext * sctx;

    int stream_number;
    struct stream_info * stream_infos;
    int stream_info_size;

    /* Params */
    char ip[64];    /* the share memory server ip address */
    
    char channel[128];  /* the channel in share memory server */

    int width;
    int height;

    // for timestamp
    int       base_stream_start;
    int64_t   out_index; // the stream index of the out timestamp base used
    int64_t   last_out_ts[SMEM_MAX_STREAM]; // the last out ts for each stream,timebase={1, 1000000}
    int64_t   last_in_ts[SMEM_MAX_STREAM]; // the last in ts for each stream
    int64_t   first_in_ts[SMEM_MAX_STREAM];
    int should_duration[SMEM_MAX_STREAM];
    AVRational in_timebase[SMEM_MAX_STREAM];

    int has_encoded_video_stream; 
    int found_video_key_frame;


    /* Options */
    int timeout;
    char *path;     // the unix domain socket path
    int  port;      // the share memory server port 
    
    /* 
        the channel that module following, if it changed, the module will switch to new channel. 
        
    */
    char *channel;  



    /* others */
    int free_cnt[SMEM_MAX_STREAM]; // the stream continuous free count


}smem_dec_ctx;

typedef struct smem_buffer_opaque {
    struct smemContext * sctx;
    struct memory_info * m_info;
}smem_buffer_opaque;

static int get_stream_info(AVFormatContext *avctx)
{
    struct smem_dec_ctx * ctx = avctx->priv_data;
    struct memory_info * m_info;

    int mem_id = -1;
    uint8_t *mem_ptr = NULL;
    int ret;

    int64_t last_time = av_gettime_relative();

    while(1){
        // to get one share memory
        mem_id = smemGetShareMemory(ctx->sctx, 0);
        if(mem_id != -1){
            //av_log(avctx, AV_LOG_VERBOSE, "get memory id: %d\n", mem_id);

            // get the memory ptr
            mem_ptr = shmat(mem_id, 0, 0);
            if(mem_ptr == (void *)-1){
                av_log(avctx, AV_LOG_ERROR, "get share memory(%d) pointer error.\n", mem_id);
                smemFreeShareMemory(ctx->sctx, mem_id);
                return -1;
            }
            //av_log(avctx, AV_LOG_VERBOSE, "get memory mem_ptr: %ld\n", mem_ptr);

            
            // get stream info
            m_info = (struct memory_info * )mem_ptr;
            ctx->stream_number = m_info->stream_info_number;

            ctx->stream_info_size = ctx->stream_number * sizeof(struct stream_info);
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
            //av_log(avctx, AV_LOG_VERBOSE, "smemFreeShareMemory\n");

            break;
        }
        else{
            av_usleep(40000);
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
    if(strlen(avctx->filename) <= 0){
        av_log(avctx, AV_LOG_ERROR, "input path should not null.\n");
        return AVERROR(EIO);
    }

    /* connect to the server */
    if(ctx->path == NULL){
        av_log(avctx, AV_LOG_INFO, "use ip socket, ip is '127.0.0.1', port=%d.\n", ctx->port);
        ctx->sctx = smemCreateConsumer("127.0.0.1", ctx->port, avctx->filename);
    }else{
        av_log(avctx, AV_LOG_INFO, "use unix domain socket, path is'%s'.\n", ctx->path);
        ctx->sctx = smemCreateConsumerUnix(ctx->path, avctx->filename);
    }


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
    struct stream_info * stream_info;

    for(i = 0; i < ctx->stream_number; i++){

        stream_info = &ctx->stream_infos[i];

        if(stream_info->codec_type == SMEM_MEDIA_TYPE_VIDEO){
            av_log(avctx, AV_LOG_VERBOSE, "stream %d is video stream\n", i);

            // add video stream
            stream = avformat_new_stream(avctx, NULL);
            if (!stream) {
                av_log(avctx, AV_LOG_ERROR, "Cannot add stream\n");
                goto ff_smem_read_header_error;
            }

            if(stream_info->codec_id != SMEM_CODEC_ID_RAWVIDEO){
                ctx->has_encoded_video_stream = 1;
            }

            stream->id = stream_info->stream_id;
            stream->codec->codec_type  = AVMEDIA_TYPE_VIDEO;
            stream->codec->codec_id    = smem2av_codec_id(stream_info->codec_id); 
            //stream->codec->time_base.den      = stream_info->time_base.den;
            //stream->codec->time_base.num      = stream_info->time_base.num;
            stream->codec->time_base = SMEM_TIME_BASE_Q;
            stream->time_base = stream->codec->time_base;

            stream->codec->pix_fmt     = smem2av_pix_fmt(stream_info->info.video.pix_fmt);
            stream->codec->width       = stream_info->info.video.width;
            stream->codec->height      = stream_info->info.video.height;
            stream->codec->extradata_size      = stream_info->info.video.extradata_size;
            if(stream->codec->extradata_size > 0){
                stream->codec->extradata = av_malloc(stream->codec->extradata_size);
                memcpy(stream->codec->extradata, stream_info->info.video.extradata, stream->codec->extradata_size);
            }

            ctx->should_duration[i] = SMEM_TIME_BASE/SMEM_NUM_IN_RANGE(25, 1, 60);  // fixme: should know the framerate
            ctx->in_timebase[i].den = stream_info->time_base.den;
            ctx->in_timebase[i].num = stream_info->time_base.num;
            ctx->last_in_ts[i] = 0;
            ctx->last_out_ts[i] = 0;
            ctx->first_in_ts[i] = 0;

            av_log(avctx, AV_LOG_INFO, "stream:%d, video, stream_id=%d, codec_id:%d, time_base:(%d,%d), pix_fmt:%d, width:%d, height:%d, extradata_size:%d, should_duration=%d\n", 
                i, stream->id, stream_info->codec_id, stream_info->time_base.num, stream_info->time_base.den, stream_info->info.video.pix_fmt, stream_info->info.video.width, 
                stream_info->info.video.height,
                stream_info->info.video.extradata_size, ctx->should_duration[i]);

            //stream->codec->bit_rate    = av_image_get_buffer_size(stream->codec->pix_fmt, ctx->width, ctx->height, 1) * 1/av_q2d(stream->codec->time_base) * 8;
            
            // for test
            if(stream->codec->extradata_size > 0){
                print_hex(stream->codec->extradata, stream->codec->extradata_size);
            }

        }
        else if(stream_info->codec_type == SMEM_MEDIA_TYPE_AUDIO){
            av_log(avctx, AV_LOG_VERBOSE, "stream %d is audio stream\n", i);

            // add video stream
            stream = avformat_new_stream(avctx, NULL);
            if (!stream) {
                av_log(avctx, AV_LOG_ERROR, "Cannot add stream\n");
                goto ff_smem_read_header_error;
            }

            stream->id = stream_info->stream_id;
            stream->codec->codec_type  = AVMEDIA_TYPE_AUDIO;
            stream->codec->codec_id    = smem2av_codec_id(stream_info->codec_id); 
            //stream->codec->time_base.den      = stream_info->time_base.den;
            //stream->codec->time_base.num      = stream_info->time_base.num;
            stream->codec->time_base = SMEM_TIME_BASE_Q;
            stream->time_base = stream->codec->time_base;

            stream->codec->sample_rate   = stream_info->info.audio.sample_rate;
            stream->codec->channels      = stream_info->info.audio.channels;
            stream->codec->sample_fmt    = smem2av_sample_fmt(stream_info->info.audio.sample_fmt);

            ctx->should_duration[i] = (SMEM_TIME_BASE*1024)/SMEM_NUM_IN_RANGE(stream->codec->sample_rate, 11025, 48000);
            ctx->in_timebase[i].den = stream_info->time_base.den;
            ctx->in_timebase[i].num = stream_info->time_base.num;
            ctx->last_in_ts[i] = 0;
            ctx->last_out_ts[i] = 0;
            ctx->first_in_ts[i] = 0;

            stream->codec->extradata_size      = stream_info->info.audio.extradata_size;
            if(stream->codec->extradata_size > 0){
                stream->codec->extradata = av_malloc(stream->codec->extradata_size);
                memcpy(stream->codec->extradata, stream_info->info.audio.extradata, stream->codec->extradata_size);
            }

            av_log(avctx, AV_LOG_INFO, "stream:%d, audio, stream_id=%d, codec_id:%d, time_base:(%d,%d), sample_rate:%d, channels:%d, sample_fmt:%d, extradata_size:%d, should_duration:%d\n", 
                i, stream->id, stream_info->codec_id, stream_info->time_base.num, stream_info->time_base.den, stream_info->info.audio.sample_rate, 
                stream_info->info.audio.channels, stream_info->info.audio.sample_fmt, stream_info->info.audio.extradata_size, ctx->should_duration[i]);

            // for test
            if(stream->codec->extradata_size > 0){
                print_hex(stream->codec->extradata, stream->codec->extradata_size);
            }

        }else{
            av_log(avctx, AV_LOG_ERROR,"not support the type:%d\n", stream_info->codec_type);
        }
    }


    av_log(avctx, AV_LOG_VERBOSE, "ff_smem_read_header over\n");

    return 0;

ff_smem_read_header_error:

    return AVERROR(EIO);
}


static int rebuild_timestamp(AVFormatContext *avctx, struct memory_info * m_info, int64_t * out_pts, int64_t * out_dts)
{
    struct smem_dec_ctx * ctx = avctx->priv_data;

    av_log(avctx, AV_LOG_VERBOSE, "[rebuild_timestamp] before rebuild pts: %lld, dts: %lld\n", m_info->pts, m_info->dts);

    int64_t pts = av_rescale_q(m_info->pts, ctx->in_timebase[m_info->index], SMEM_TIME_BASE_Q);
    int64_t dts = av_rescale_q(m_info->dts, ctx->in_timebase[m_info->index], SMEM_TIME_BASE_Q);

    int64_t dt;

    if(m_info->index == 0){
        // the first number stream is the timestamp rebuild base stream 

        if(SMEM_NUM_IF_OUT_RANGE(dts - ctx->last_in_ts[m_info->index], 0, 5*ctx->should_duration[m_info->index])){
            ctx->first_in_ts[m_info->index] = dts;

            av_log(avctx, AV_LOG_WARNING, "[rebuild_timestamp] index:%d, last dts: %lld, the new dts: %lld is out of the range, should_duration[%d]:%lld\n", m_info->index, ctx->last_in_ts[m_info->index], dts, m_info->index, ctx->should_duration[m_info->index]);
            *out_dts = ctx->last_out_ts[m_info->index] + ctx->should_duration[m_info->index];
            

        }else{
            *out_dts = ctx->last_out_ts[m_info->index] + (dts - ctx->last_in_ts[m_info->index]);
        }

        *out_pts = *out_dts + (pts - dts) + 2*ctx->should_duration[m_info->index]; //for b frame, to make pts greater than dts and greater than 0

        ctx->base_stream_start = 1;

    }else{
        if(ctx->base_stream_start != 1){
            av_log(avctx, AV_LOG_WARNING, "[rebuild_timestamp] index:%d, base stream not start, skip.\n", m_info->index);
            return -1;
        }

        // other number streams check the timestamp by the base stream
        if(SMEM_NUM_IF_OUT_RANGE(dts - ctx->last_in_ts[m_info->index], 0, 5*ctx->should_duration[m_info->index])){
            ctx->first_in_ts[m_info->index] = dts;

            av_log(avctx, AV_LOG_WARNING, "[rebuild_timestamp] index:%d, last dts: %lld, the new dts: %lld is out of the range, should_duration[%d]:%lld\n", m_info->index, ctx->last_in_ts[m_info->index], dts, m_info->index, ctx->should_duration[m_info->index]);
            

            // the others streams should after base stream
            if(ctx->first_in_ts[m_info->index] < ctx->first_in_ts[0]){
                av_log(avctx, AV_LOG_WARNING, "[rebuild_timestamp] index:%d, the local stream time(%lld) < base stream time(%lld), skip.\n"
                    , m_info->index, ctx->first_in_ts[m_info->index], ctx->first_in_ts[0]);

                return -1;
            }

            dt = ctx->first_in_ts[m_info->index] - ctx->first_in_ts[0]; // the dt of the new streams

            // the streams out ts should same as dt
            *out_dts = ctx->last_out_ts[0] + SMEM_NUM_IN_RANGE(dt, ctx->should_duration[m_info->index], 10*ctx->should_duration[m_info->index]);

            if(*out_dts < ctx->last_out_ts[m_info->index]){
                av_log(avctx, AV_LOG_WARNING, "[rebuild_timestamp] index:%d, the local out time(%lld) < last out time(%lld), skip.\n",
                    m_info->index,*out_dts, ctx->last_out_ts[m_info->index]);
                return -1;
            }



        }else{


            *out_dts = ctx->last_out_ts[m_info->index] + (dts - ctx->last_in_ts[m_info->index]);
        }

        *out_pts = *out_dts + (pts - dts) + 2*ctx->should_duration[m_info->index];

    }

    ctx->last_in_ts[m_info->index] = dts;
    ctx->last_out_ts[m_info->index] = *out_dts;


    av_log(avctx, AV_LOG_VERBOSE, "[rebuild_timestamp] after rebuild pts: %lld, dts: %lld\n", *out_pts, *out_dts);


    return 0;
}

static int if_free_frame(AVFormatContext *avctx, struct memory_info * m_info)
{
    struct smem_dec_ctx * ctx = avctx->priv_data;
    struct stream_info * stream_info = NULL;
    int stream_index = m_info->index;


    int buffer_size;


    /*
    // if streams has encoded video stream, should start at first video key frame
    if(ctx->has_encoded_video_stream && ctx->found_video_key_frame == 0){
        if(stream_index < ctx->stream_number){
            stream_info = &ctx->stream_infos[stream_index];

            // not found video key, other frames should be free
            if(stream_info->codec_type != SMEM_MEDIA_TYPE_VIDEO){
                return 1;
            }

            if(stream_info->codec_type == SMEM_MEDIA_TYPE_VIDEO && m_info->is_key){
                ctx->found_video_key_frame = 1;
                return 0;
            }else{
                return 1;
            }
        }
    }
    */

    // if buffer will full, free the raw video packet
    if(stream_index < ctx->stream_number){
        stream_info = &ctx->stream_infos[stream_index];

        // if the video frame
        if(stream_info->codec_type == SMEM_MEDIA_TYPE_VIDEO && stream_info->codec_id == SMEM_CODEC_ID_RAWVIDEO){
            // get the buffer size
            buffer_size = smem_queue_size(ctx->sctx);

            // if buffer will full, should free the frame
            if(buffer_size >= (SMEM_MAX_QUEUE_LENGTH/2)){

                if(ctx->free_cnt[stream_index] >= 0 && ctx->free_cnt[stream_index] < 1){
                    ctx->free_cnt[stream_index]++;
                    return 1;
                }else{
                    ctx->free_cnt[stream_index] = 0;
                    return 0;
                }
            }
        }

    }

    return 0;
}

static void ff_smem_av_buffer_free(void *opaque, uint8_t *data)
{

    smem_buffer_opaque * smem_opaque = opaque;

    uint8_t *mem_ptr = smem_opaque->m_info;
    int ret;

    int mem_id = smem_opaque->m_info->id;

    // free the share memory 
    if(mem_ptr){
        ret = shmdt(mem_ptr);
        if(ret < 0){
            return;
        }
    }

    // free the memory id
    smemFreeShareMemory(smem_opaque->sctx, mem_id);

    av_free(opaque);
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
    struct memory_info * m_info;

    int64_t last_time = av_gettime_relative();

    int64_t out_pts;
    int64_t out_dts;

    //av_init_packet(pkt);
    while(1){
        // get memory 
        mem_id = smemGetShareMemory(ctx->sctx, 0);
        if(mem_id != -1){
            //av_log(avctx, AV_LOG_VERBOSE, "get memory id: %d\n", mem_id);

            // get the memory ptr
            mem_ptr = shmat(mem_id, 0, 0);
            if(mem_ptr == (void *)-1){
                av_log(avctx, AV_LOG_ERROR, "get share memory(%d) pointer error.\n", mem_id);
                smemFreeShareMemory(ctx->sctx, mem_id);
                //av_log(avctx, AV_LOG_ERROR, "get share memory(%d) pointer error and free over.\n", mem_id);
                av_usleep(10000);
                continue;
            }
            //av_log(avctx, AV_LOG_VERBOSE, "get memory mem_ptr: %ld\n", mem_ptr);
            m_info = (struct memory_info * )mem_ptr;

            // rebuild the timestamp
            if(rebuild_timestamp(avctx, m_info, &out_pts, &out_dts) < 0){
                ret = shmdt(mem_ptr); // free the share memory local ptr
                smemFreeShareMemory(ctx->sctx, mem_id);
                continue;
            }

            // if the share memory buffer will full, skip the video frame
            if(if_free_frame(avctx, m_info)){
                av_log(avctx, AV_LOG_WARNING, "The share memory should free, index=%d, dts=%lld\n", m_info->index, out_dts);

                ret = shmdt(mem_ptr); // free the share memory local ptr
                smemFreeShareMemory(ctx->sctx, mem_id);
                continue; 
            }

            
            av_init_packet(pkt);

            //av_log(avctx, AV_LOG_VERBOSE, "data_size: %d\n", m_info->data_size);

            #if 0
            // copy data
            av_new_packet(pkt, m_info->data_size); // alloc memory to carrying the data 

            pkt->stream_index = m_info->index;
            pkt->pts = out_pts;
            pkt->dts = out_dts;
            if(m_info->is_key)
                pkt->flags |= AV_PKT_FLAG_KEY;
            
            ret = memcpy(pkt->data, mem_ptr + m_info->data_offset, m_info->data_size);

            av_log(avctx, AV_LOG_VERBOSE, "get memory packet, stream_index:%d, time_base:(%d,%d) pts:%lld, size:%d, data: %ld, mem_ptr:%lld, data_offset:%d, data_size:%d, ret=%d \n", 
                pkt->stream_index, ctx->stream_infos[pkt->stream_index].time_base.num, ctx->stream_infos[pkt->stream_index].time_base.den, pkt->pts, pkt->size, pkt->data,
                mem_ptr, m_info->data_offset, m_info->data_size, ret);


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
            smemFreeShareMemory(ctx->sctx, mem_id);

            #else
            // no copy data
            pkt->stream_index = m_info->index;
            pkt->pts = out_pts;
            pkt->dts = out_dts;
            if(m_info->is_key)
                pkt->flags |= AV_PKT_FLAG_KEY;

            AVBuffer *buf = NULL;
            AVBufferRef *ref = NULL;
            buf = av_mallocz(sizeof(AVBuffer));
            if (!buf){
                av_log(avctx, AV_LOG_ERROR, "av_mallocz AVBuffer failed.\n");
                return -1;
            }
            buf->data     = mem_ptr + m_info->data_offset;
            buf->size     = m_info->data_size;
            buf->free     = ff_smem_av_buffer_free;

            smem_buffer_opaque * opaque = av_mallocz(sizeof(smem_buffer_opaque));
            if(!opaque){
                av_log(avctx, AV_LOG_ERROR, "av_mallocz smem_buffer_opaque failed.\n");
                return -1;
            }
            opaque->sctx = ctx->sctx;
            opaque->m_info = m_info;
            buf->opaque   = opaque;
            buf->refcount = 1;

            ref = av_mallocz(sizeof(AVBufferRef));
            if (!ref) {
                av_freep(&buf);
                av_log(avctx, AV_LOG_ERROR, "av_mallocz AVBuffer failed.\n");
                return -1;
            }

            ref->buffer = buf;
            ref->data   = buf->data;
            ref->size   = buf->size;

            pkt->buf      = ref;
            pkt->data     = ref->data;
            pkt->size     = ref->size;

            #endif



            // get data, break
            break;
        
        }

        av_usleep(20000);

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

    if(ctx->sctx)
        smemFree(ctx->sctx);

    if(ctx->stream_infos)
        av_free(ctx->stream_infos);

    return 0;
}


#define OFFSET(x) offsetof(smem_dec_ctx, x)
#define DEC AV_OPT_FLAG_DECODING_PARAM
#define ENC AV_OPT_FLAG_ENCODING_PARAM

static const AVOption options[] = {
    { "timeout", "set maximum timeout (in seconds)", OFFSET(timeout), AV_OPT_TYPE_INT, {.i64 = 10}, INT_MIN, INT_MAX, DEC },
    { "path", "the unix domain socket path", OFFSET(path), AV_OPT_TYPE_STRING, {0}, 0, 0, DEC },
    { "port", "the redis server port", OFFSET(port), AV_OPT_TYPE_INT, {.i64 = 6379}, INT_MIN, INT_MAX, DEC },
    { "channel", "the channel that module following", OFFSET(channel), AV_OPT_TYPE_STRING, {0}, 0, 0, DEC },
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
    .long_name      = NULL_IF_CONFIG_SMALL("smem demuxer"),
    .flags          = AVFMT_NOFILE | AVFMT_SHOW_IDS,
    .priv_class     = &smem_demuxer_class,
    .priv_data_size = sizeof(struct smem_dec_ctx),
    .read_header   = ff_smem_read_header,
    .read_packet   = ff_smem_read_packet,
    .read_close    = ff_smem_read_close,
};
