#include <stdint.h>
#include <string.h>

#include "libavutil/common.h"
#include "libavutil/fifo.h"
#include "libavutil/opt.h"

#include "avcodec.h"
#include "internal.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "nvEncodeAPI.h"

#if defined(_WIN32)
#define CUDAAPI __stdcall
#else
#define CUDAAPI
#endif

#if defined(_WIN32)
#define LOAD_FUNC(l, s) GetProcAddress(l, s)
#define DL_CLOSE_FUNC(l) FreeLibrary(l)
#else
#define LOAD_FUNC(l, s) dlsym(l, s)
#define DL_CLOSE_FUNC(l) dlclose(l)
#endif

typedef enum cudaError_enum {
    CUDA_SUCCESS = 0
} CUresult;

typedef int CUdevice;
typedef void* CUcontext;
typedef void *CUvideodecoder;
typedef struct _CUcontextlock_st *CUvideoctxlock;

typedef enum cudaVideoCodec_enum {
    cudaVideoCodec_MPEG1=0,
    cudaVideoCodec_MPEG2,
    cudaVideoCodec_MPEG4,
    cudaVideoCodec_VC1,
    cudaVideoCodec_H264,
    cudaVideoCodec_JPEG,
    cudaVideoCodec_H264_SVC,
    cudaVideoCodec_H264_MVC,
    cudaVideoCodec_HEVC,
    cudaVideoCodec_NumCodecs,
    // Uncompressed YUV
    cudaVideoCodec_YUV420 = (('I'<<24)|('Y'<<16)|('U'<<8)|('V')),   // Y,U,V (4:2:0)
    cudaVideoCodec_YV12   = (('Y'<<24)|('V'<<16)|('1'<<8)|('2')),   // Y,V,U (4:2:0)
    cudaVideoCodec_NV12   = (('N'<<24)|('V'<<16)|('1'<<8)|('2')),   // Y,U,V (4:2:0)
    cudaVideoCodec_YUYV   = (('Y'<<24)|('U'<<16)|('Y'<<8)|('V')),   // YUYV/YUY2 (4:2:2)
    cudaVideoCodec_UYVY   = (('U'<<24)|('Y'<<16)|('V'<<8)|('Y'))    // UYVY (4:2:2)
} cudaVideoCodec;

typedef enum cudaVideoChromaFormat_enum {
    cudaVideoChromaFormat_Monochrome=0,
    cudaVideoChromaFormat_420,
    cudaVideoChromaFormat_422,
    cudaVideoChromaFormat_444
} cudaVideoChromaFormat;

typedef enum cudaVideoSurfaceFormat_enum {
    cudaVideoSurfaceFormat_NV12=0   // NV12 (currently the only supported output format)
} cudaVideoSurfaceFormat;

typedef enum cudaVideoDeinterlaceMode_enum {
    cudaVideoDeinterlaceMode_Weave=0,   // Weave both fields (no deinterlacing)
    cudaVideoDeinterlaceMode_Bob,       // Drop one field
    cudaVideoDeinterlaceMode_Adaptive   // Adaptive deinterlacing
} cudaVideoDeinterlaceMode;

typedef enum cudaVideoCreateFlags_enum {
    cudaVideoCreate_Default = 0x00,     // Default operation mode: use dedicated video engines
    cudaVideoCreate_PreferCUDA = 0x01,  // Use a CUDA-based decoder if faster than dedicated engines (requires a valid vidLock object for multi-threading)
    cudaVideoCreate_PreferDXVA = 0x02,  // Go through DXVA internally if possible (requires D3D9 interop)
    cudaVideoCreate_PreferCUVID = 0x04  // Use dedicated video engines directly
} cudaVideoCreateFlags;


typedef struct _CUVIDDECODECREATEINFO
{
    // Decoding
    unsigned long ulWidth;          // Coded Sequence Width
    unsigned long ulHeight;         // Coded Sequence Height
    unsigned long ulNumDecodeSurfaces;  // Maximum number of internal decode surfaces
    cudaVideoCodec CodecType;        // cudaVideoCodec_XXX
    cudaVideoChromaFormat ChromaFormat; // cudaVideoChromaFormat_XXX (only 4:2:0 is currently supported)
    unsigned long ulCreationFlags;  // Decoder creation flags (cudaVideoCreateFlags_XXX)
    unsigned long Reserved1[5];     // Reserved for future use - set to zero
    struct {                        // area of the frame that should be displayed
        short left;
        short top;
        short right;
        short bottom;
    } display_area;
    // Output format
    cudaVideoSurfaceFormat OutputFormat;       // cudaVideoSurfaceFormat_XXX
    cudaVideoDeinterlaceMode DeinterlaceMode;  // cudaVideoDeinterlaceMode_XXX
    unsigned long ulTargetWidth;    // Post-processed Output Width 
    unsigned long ulTargetHeight;   // Post-processed Output Height
    unsigned long ulNumOutputSurfaces; // Maximum number of output surfaces simultaneously mapped
    CUvideoctxlock vidLock;         // If non-NULL, context lock used for synchronizing ownership of the cuda context
    struct {                        // target rectangle in the output frame (for aspect ratio conversion)
        short left;
        short top;
        short right;
        short bottom;
    } target_rect;                  // if a null rectangle is specified, {0,0,ulTargetWidth,ulTargetHeight} will be used
    unsigned long Reserved2[5];     // Reserved for future use - set to zero
} CUVIDDECODECREATEINFO;


// Picture Parameters for Decoding 
typedef struct _CUVIDPICPARAMS
{
    int PicWidthInMbs;      // Coded Frame Size
    int FrameHeightInMbs;   // Coded Frame Height
    int CurrPicIdx;         // Output index of the current picture
    int field_pic_flag;     // 0=frame picture, 1=field picture
    int bottom_field_flag;  // 0=top field, 1=bottom field (ignored if field_pic_flag=0)
    int second_field;       // Second field of a complementary field pair
    // Bitstream data
    unsigned int nBitstreamDataLen;        // Number of bytes in bitstream data buffer
    const unsigned char *pBitstreamData;   // Ptr to bitstream data for this picture (slice-layer)
    unsigned int nNumSlices;               // Number of slices in this picture
    const unsigned int *pSliceDataOffsets; // nNumSlices entries, contains offset of each slice within the bitstream data buffer
    int ref_pic_flag;       // This picture is a reference picture
    int intra_pic_flag;     // This picture is entirely intra coded
    unsigned int Reserved[30];             // Reserved for future use
    // Codec-specific data
    union {
        CUVIDMPEG2PICPARAMS mpeg2;          // Also used for MPEG-1
        CUVIDH264PICPARAMS h264;
        CUVIDVC1PICPARAMS vc1;
        CUVIDMPEG4PICPARAMS mpeg4;
        CUVIDJPEGPICPARAMS jpeg;
        CUVIDHEVCPICPARAMS hevc;
        unsigned int CodecReserved[1024];
    } CodecSpecific;
} CUVIDPICPARAMS;



typedef CUresult(CUDAAPI *PCUINIT)(unsigned int Flags);
typedef CUresult(CUDAAPI *PCUDEVICEGETCOUNT)(int *count);
typedef CUresult(CUDAAPI *PCUDEVICEGET)(CUdevice *device, int ordinal);
typedef CUresult(CUDAAPI *PCUDEVICEGETNAME)(char *name, int len, CUdevice dev);
typedef CUresult(CUDAAPI *PCUDEVICECOMPUTECAPABILITY)(int *major, int *minor, CUdevice dev);
typedef CUresult(CUDAAPI *PCUCTXCREATE)(CUcontext *pctx, unsigned int flags, CUdevice dev);
typedef CUresult(CUDAAPI *PCUCTXPOPCURRENT)(CUcontext *pctx);
typedef CUresult(CUDAAPI *PCUCTXDESTROY)(CUcontext ctx);

typedef CUresult(CUDAAPI *PCUVIDCREATEDECODER)(CUvideodecoder *phDecoder, CUVIDDECODECREATEINFO *pdci);
typedef CUresult(CUDAAPI *PCUVIDDESTROYDECODER)(CUvideodecoder hDecoder);
typedef CUresult(CUDAAPI *PCUVIDCTXLOCKCREATE)(CUvideoctxlock *pLock, CUcontext ctx);
typedef CUresult(CUDAAPI *PCUVIDCTXLOCKDESTROY)(CUvideoctxlock lck);
typedef CUresult(CUDAAPI *PCUVIDCTXLOCK)(CUvideoctxlock lck, unsigned int reserved_flags);
typedef CUresult(CUDAAPI *PCUVIDCTXUNLOCK)(CUvideoctxlock lck, unsigned int reserved_flags);

typedef NVENCSTATUS (NVENCAPI* PNVENCODEAPICREATEINSTANCE)(NV_ENCODE_API_FUNCTION_LIST *functionList);

typedef struct NvencInputSurface
{
    NV_ENC_INPUT_PTR input_surface;
    int width;
    int height;

    int lockCount;

    NV_ENC_BUFFER_FORMAT format;
} NvencInputSurface;

typedef struct NvencOutputSurface
{
    NV_ENC_OUTPUT_PTR output_surface;
    int size;

    NvencInputSurface* input_surface;

    int busy;
} NvencOutputSurface;

typedef struct NvencData
{
    union {
        int64_t timestamp;
        NvencOutputSurface *surface;
    } u;
} NvencData;

typedef struct NvencDataList
{
    NvencData* data;

    uint32_t pos;
    uint32_t count;
    uint32_t size;
} NvencDataList;

typedef struct NvencDynLoadFunctions
{
    PCUINIT cu_init;
    PCUDEVICEGETCOUNT cu_device_get_count;
    PCUDEVICEGET cu_device_get;
    PCUDEVICEGETNAME cu_device_get_name;
    PCUDEVICECOMPUTECAPABILITY cu_device_compute_capability;
    PCUCTXCREATE cu_ctx_create;
    PCUCTXPOPCURRENT cu_ctx_pop_current;
    PCUCTXDESTROY cu_ctx_destroy;

    NV_ENCODE_API_FUNCTION_LIST nvenc_funcs;
    int nvenc_device_count;
    CUdevice nvenc_devices[16];

    // nvcuvid
    PCUVIDCREATEDECODER     cuvid_create_decoder;
    PCUVIDDESTROYDECODER    cuvid_destroy_decoder;
    PCUVIDCTXLOCKCREATE     cuvid_lock_create;
    PCUVIDCTXLOCKDESTROY    cuvid_lock_destroy;
    PCUVIDCTXLOCK           cuvid_lock;
    PCUVIDCTXUNLOCK         cuvid_unlock;

#if defined(_WIN32)
    HMODULE cuda_lib;
    HMODULE nvenc_lib;
    HMODULE nvenc_dec_lib;
#else
    void* cuda_lib;
    void* nvenc_lib;
    void* nvenc_dec_lib;
#endif
} NvencDynLoadFunctions;



typedef struct NvencDecContext {
    AVClass *class;

    NvencDynLoadFunctions nvenc_dload_funcs;
    int device_count; // cuda device count
    CUcontext cu_context; // cuda context
    CUVIDDECODECREATEINFO   video_decode_create_info;
    CUvideodecoder          h_decoder;
    cudaVideoCreateFlags    video_create_flags;

    CUvideoctxlock          vid_ctx_lock;

    // the filter for converting to Annex B
    AVBitStreamFilterContext *bsf;


    /* Options */
    int gpu;

} NvencDecContext;








#define CHECK_LOAD_FUNC(t, f, s) \
do { \
    (f) = (t)LOAD_FUNC(dl_fn->cuda_lib, s); \
    if (!(f)) { \
        av_log(avctx, AV_LOG_FATAL, "Failed loading %s from CUDA library\n", s); \
        goto cuda_error; \
    } \
} while (0)

static av_cold int nvenc_dyload_cuda(AVCodecContext *avctx)
{
    NvencDecContext *ctx = avctx->priv_data;
    NvencDynLoadFunctions *dl_fn = &ctx->nvenc_dload_funcs;

    if (dl_fn->cuda_lib)
        return 1;

#if defined(_WIN32)
    dl_fn->cuda_lib = LoadLibrary(TEXT("nvcuda.dll"));
#else
    dl_fn->cuda_lib = dlopen("libcuda.so", RTLD_LAZY);
#endif

    if (!dl_fn->cuda_lib) {
        av_log(avctx, AV_LOG_FATAL, "Failed loading CUDA library\n");
        goto cuda_error;
    }

    CHECK_LOAD_FUNC(PCUINIT, dl_fn->cu_init, "cuInit");
    CHECK_LOAD_FUNC(PCUDEVICEGETCOUNT, dl_fn->cu_device_get_count, "cuDeviceGetCount");
    CHECK_LOAD_FUNC(PCUDEVICEGET, dl_fn->cu_device_get, "cuDeviceGet");
    CHECK_LOAD_FUNC(PCUDEVICEGETNAME, dl_fn->cu_device_get_name, "cuDeviceGetName");
    CHECK_LOAD_FUNC(PCUDEVICECOMPUTECAPABILITY, dl_fn->cu_device_compute_capability, "cuDeviceComputeCapability");
    //CHECK_LOAD_FUNC(PCUCTXCREATE, dl_fn->cu_ctx_create, "cuCtxCreate");
    CHECK_LOAD_FUNC(PCUCTXCREATE, dl_fn->cu_ctx_create, "cuCtxCreate_v2");
    CHECK_LOAD_FUNC(PCUCTXPOPCURRENT, dl_fn->cu_ctx_pop_current, "cuCtxPopCurrent_v2");
    CHECK_LOAD_FUNC(PCUCTXDESTROY, dl_fn->cu_ctx_destroy, "cuCtxDestroy_v2");

    return 1;

cuda_error:

    if (dl_fn->cuda_lib)
        DL_CLOSE_FUNC(dl_fn->cuda_lib);

    dl_fn->cuda_lib = NULL;

    return 0;
}


static av_cold int nvenc_decode_close(AVCodecContext *avctx)
{
    NvencDecContext *ctx = avctx->priv_data;

    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_close begin\n");

    //ff_qsv_decode_close(&s->qsv);

    av_bitstream_filter_close(ctx->bsf);

    return 0;
}


static av_cold int check_cuda_errors(AVCodecContext *avctx, CUresult err, const char *func)
{
    if (err != CUDA_SUCCESS) {
        av_log(avctx, AV_LOG_FATAL, ">> %s - failed with error code 0x%x\n", func, err);
        return 0;
    }
    return 1;
}
#define check_cuda_errors(f) if (!check_cuda_errors(avctx, f, #f)) goto nvenc_decode_init_fail

#define CHECK_LOAD_CUVID_FUNC(t, f, s) \
do { \
    (f) = (t)LOAD_FUNC(dl_fn->nvenc_dec_lib, s); \
    if (!(f)) { \
        av_log(avctx, AV_LOG_FATAL, "Failed loading %s from CUVID library\n", s); \
        goto nvenc_decode_init_fail; \
    } \
} while (0)

static av_cold int nvenc_decode_init(AVCodecContext *avctx)
{
    NvencDecContext *ctx = avctx->priv_data;

    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init begin\n");

    int ret = -1;
    CUresult cu_res;

    /* init the cuda */
    NvencDynLoadFunctions *dl_fn = &ctx->nvenc_dload_funcs;

    if(!nvenc_dyload_cuda(avctx)){
        av_log(avctx, AV_LOG_ERROR, "nvenc_decode_init: nvenc_dyload_cuda failed.\n");
        return -1;
    }
    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init: after nvenc_dyload_cuda()\n");

    check_cuda_errors(dl_fn->cu_init(0));
    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init: after cu_init(0);\n");

    check_cuda_errors(dl_fn->cu_device_get_count(&ctx->device_count));
    if (!ctx->device_count) {
        av_log(avctx, AV_LOG_FATAL, "No CUDA capable devices found\n");
        goto nvenc_decode_init_fail;
    }
    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init: after cu_device_get_count, device_count=%d, select gpu:%d.\n", ctx->device_count, ctx->gpu);


    av_log(avctx, AV_LOG_VERBOSE, "video width=%d, height=%d\n", avctx->width, avctx->height);


    CUdevice cu_device = 0;
    char gpu_name[128];
    int smminor = 0, smmajor = 0;
    int i, smver, target_smver;
    dl_fn->nvenc_device_count = 0;

    for (i = 0; i < ctx->device_count; ++i) {
        check_cuda_errors(dl_fn->cu_device_get(&cu_device, i));
        check_cuda_errors(dl_fn->cu_device_get_name(gpu_name, sizeof(gpu_name), cu_device));
        check_cuda_errors(dl_fn->cu_device_compute_capability(&smmajor, &smminor, cu_device));

        smver = (smmajor << 4) | smminor;

        av_log(avctx, AV_LOG_VERBOSE, "[ GPU #%d - < %s > has Compute SM %d.%d, NVENC %s ]\n", i, gpu_name, smmajor, smminor, (smver >= target_smver) ? "Available" : "Not Available");

        if (smver >= target_smver)
            dl_fn->nvenc_devices[dl_fn->nvenc_device_count++] = cu_device;
    }

    if (!dl_fn->nvenc_device_count) {
        av_log(avctx, AV_LOG_FATAL, "No NVENC capable devices found\n");
        goto nvenc_decode_init_fail;
    }


    // open the decoder lib
    #if defined(_WIN32)
        if (sizeof(void*) == 8) {
            dl_fn->nvenc_dec_lib = LoadLibrary(TEXT("nvcuvid.dll"));
        } else {
            dl_fn->nvenc_dec_lib = LoadLibrary(TEXT("nvcuvid.dll"));
        }
    #else
        dl_fn->nvenc_dec_lib = dlopen("libnvcuvid.so", RTLD_LAZY);
    #endif

    if (dl_fn->nvenc_dec_lib == NULL){
        av_log(avctx, AV_LOG_FATAL, "Failed loading the nvcuvid library\n");
        goto nvenc_decode_init_fail;
    }
    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init after load libnvcuvid.\n");

    CHECK_LOAD_CUVID_FUNC(PCUVIDCREATEDECODER, dl_fn->cuvid_create_decoder, "cuvidCreateDecoder");
    CHECK_LOAD_CUVID_FUNC(PCUVIDDESTROYDECODER, dl_fn->cuvid_destroy_decoder, "cuvidDestroyDecoder");
    CHECK_LOAD_CUVID_FUNC(PCUVIDCTXLOCKCREATE, dl_fn->cuvid_lock_create, "cuvidCtxLockCreate");
    CHECK_LOAD_CUVID_FUNC(PCUVIDCTXLOCKDESTROY, dl_fn->cuvid_lock_destroy, "cuvidCtxLockDestroy");
    CHECK_LOAD_CUVID_FUNC(PCUVIDCTXLOCK, dl_fn->cuvid_lock, "cuvidCtxLock");
    CHECK_LOAD_CUVID_FUNC(PCUVIDCTXUNLOCK, dl_fn->cuvid_unlock, "cuvidCtxUnlock");
    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init after load libnvcuvid functions.\n");


    // create cuda context
    ctx->cu_context = NULL;
    check_cuda_errors(dl_fn->cu_ctx_create(&ctx->cu_context, 4, dl_fn->nvenc_devices[ctx->gpu]));  // CU_CTX_SCHED_BLOCKING_SYNC=4, avoid CPU spins


    /*** to open the decoder ***/
    memset(&ctx->video_decode_create_info, 0, sizeof(ctx->video_decode_create_info));
    ctx->video_decode_create_info.CodecType = cudaVideoCodec_H264;
    ctx->video_decode_create_info.ulWidth   = avctx->width;
    ctx->video_decode_create_info.ulHeight  = avctx->height;
    ctx->video_decode_create_info.ulNumDecodeSurfaces = 6; // fixme
    ctx->video_decode_create_info.ChromaFormat        = cudaVideoChromaFormat_420; // fixme
    ctx->video_decode_create_info.OutputFormat        = cudaVideoSurfaceFormat_NV12;
    ctx->video_decode_create_info.DeinterlaceMode     = cudaVideoDeinterlaceMode_Adaptive;

    // No scaling
    ctx->video_decode_create_info.ulTargetWidth       = ctx->video_decode_create_info.ulWidth;
    ctx->video_decode_create_info.ulTargetHeight      = ctx->video_decode_create_info.ulHeight;
    ctx->video_decode_create_info.ulNumOutputSurfaces = 6;  // fixme
    ctx->video_decode_create_info.ulCreationFlags     = cudaVideoCreate_Default; // fixme

    check_cuda_errors(dl_fn->cuvid_lock_create(&ctx->vid_ctx_lock, ctx->cu_context));
    ctx->video_decode_create_info.vidLock             = ctx->vid_ctx_lock; // fixme
    check_cuda_errors(dl_fn->cuvid_create_decoder(&ctx->h_decoder, &ctx->video_decode_create_info));
    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init after create decoder.\n");





    // others
    if (avctx->codec_id == AV_CODEC_ID_H264)
        ctx->bsf = av_bitstream_filter_init("h264_mp4toannexb");
    else
        ctx->bsf = av_bitstream_filter_init("hevc_mp4toannexb");
    if (!ctx->bsf) {
        ret = AVERROR(ENOMEM);
        goto nvenc_decode_init_fail;
    }

    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_init over\n");
    return 0;

nvenc_decode_init_fail:

    nvenc_decode_close(avctx);
    return ret;
}

static int nvenc_decode_frame(AVCodecContext *avctx, void *data,
                            int *got_frame, AVPacket *avpkt)
{
    NvencDecContext *ctx = avctx->priv_data;

    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_frame begin\n");


    AVFrame *frame    = data;
    int ret;
    uint8_t *p_filtered = NULL;
    int      n_filtered = NULL;
    AVPacket pkt_filtered = { 0 };


    return -1;
}

static void nvenc_decode_flush(AVCodecContext *avctx)
{
    NvencDecContext *ctx = avctx->priv_data;

    av_log(avctx, AV_LOG_VERBOSE, "nvenc_decode_flush begin\n");

}



#define OFFSET(x) offsetof(NvencDecContext, x)
#define VD AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_DECODING_PARAM
static const AVOption options[] = {
    { "gpu", "Selects which NVENC capable GPU to use. First GPU is 0, second is 1, and so on.", OFFSET(gpu), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, INT_MAX, VD },
    { NULL },
};

static const AVClass nvenc_h264_class = {
    .class_name = "nvenc_h264",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVCodec ff_nvenc_h264_decoder = {
    .name           = "nvenc_h264",
    .long_name      = NULL_IF_CONFIG_SMALL("H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10 (Nvidia nvenc)"),
    .priv_data_size = sizeof(NvencDecContext),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_H264,
    .init           = nvenc_decode_init,
    .decode         = nvenc_decode_frame,
    .flush          = nvenc_decode_flush,
    .close          = nvenc_decode_close,
    .capabilities   = AV_CODEC_CAP_DELAY | AV_CODEC_CAP_DR1,
    .priv_class     = &nvenc_h264_class,
    .pix_fmts       = (const enum AVPixelFormat[]){ AV_PIX_FMT_NV12,
                                                    AV_PIX_FMT_NONE },
};

