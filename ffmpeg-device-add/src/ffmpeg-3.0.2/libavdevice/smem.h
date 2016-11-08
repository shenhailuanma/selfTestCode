
#ifndef SMEM_H
#define SMEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
    
enum SMemMediaType{
    SMEM_MEDIA_TYPE_UNKNOWN = -1,  ///< Usually treated as AVMEDIA_TYPE_DATA
    SMEM_MEDIA_TYPE_VIDEO,
    SMEM_MEDIA_TYPE_AUDIO,
    SMEM_MEDIA_TYPE_DATA,          ///< Opaque data information usually continuous
    SMEM_MEDIA_TYPE_SUBTITLE
};

enum SMemCodecID{
    SMEM_CODEC_ID_NONE,

    /* video codecs */
    SMEM_CODEC_ID_MPEG1VIDEO,
    SMEM_CODEC_ID_MPEG2VIDEO, ///< preferred ID for MPEG-1/2 video decoding
    SMEM_CODEC_ID_H261,
    SMEM_CODEC_ID_H263,
    SMEM_CODEC_ID_MJPEG,
    SMEM_CODEC_ID_MPEG4,
    SMEM_CODEC_ID_RAWVIDEO,
    SMEM_CODEC_ID_H264,
    SMEM_CODEC_ID_PNG,
    SMEM_CODEC_ID_JPEG2000,
    SMEM_CODEC_ID_VP9,
    SMEM_CODEC_ID_HEVC,
#define SMEM_CODEC_ID_H265 SMEM_CODEC_ID_HEVC

    /* various PCM "codecs" */
    SMEM_CODEC_ID_FIRST_AUDIO = 0x10000,     ///< A dummy id pointing at the start of audio codecs
    SMEM_CODEC_ID_PCM_S16LE = 0x10000,
    SMEM_CODEC_ID_PCM_S16BE,
    SMEM_CODEC_ID_PCM_U16LE,
    SMEM_CODEC_ID_PCM_U16BE,
    SMEM_CODEC_ID_PCM_S8,
    SMEM_CODEC_ID_PCM_U8,
    SMEM_CODEC_ID_PCM_MULAW,
    SMEM_CODEC_ID_PCM_ALAW,
    SMEM_CODEC_ID_PCM_S32LE,
    SMEM_CODEC_ID_PCM_S32BE,
    SMEM_CODEC_ID_PCM_U32LE,
    SMEM_CODEC_ID_PCM_U32BE,
    SMEM_CODEC_ID_PCM_S24LE,
    SMEM_CODEC_ID_PCM_S24BE,
    SMEM_CODEC_ID_PCM_U24LE,
    SMEM_CODEC_ID_PCM_U24BE,
    SMEM_CODEC_ID_PCM_S24DAUD,
    SMEM_CODEC_ID_PCM_ZORK,
    SMEM_CODEC_ID_PCM_S16LE_PLANAR,
    SMEM_CODEC_ID_PCM_DVD,
    SMEM_CODEC_ID_PCM_F32BE,
    SMEM_CODEC_ID_PCM_F32LE,
    SMEM_CODEC_ID_PCM_F64BE,
    SMEM_CODEC_ID_PCM_F64LE,
    SMEM_CODEC_ID_PCM_BLURAY,
    SMEM_CODEC_ID_PCM_LXF,
    SMEM_CODEC_ID_S302M,
    SMEM_CODEC_ID_PCM_S8_PLANAR,
    SMEM_CODEC_ID_PCM_S24LE_PLANAR,
    SMEM_CODEC_ID_PCM_S32LE_PLANAR,
    SMEM_CODEC_ID_PCM_S16BE_PLANAR,


    /* audio codecs */
    SMEM_CODEC_ID_MP2 = 0x15000,
    SMEM_CODEC_ID_MP3, ///< preferred ID for decoding MPEG audio layer 1, 2 or 3
    SMEM_CODEC_ID_AAC,
    SMEM_CODEC_ID_AC3,
    SMEM_CODEC_ID_DTS,
    SMEM_CODEC_ID_VORBIS,
    SMEM_CODEC_ID_WMAV1,
    SMEM_CODEC_ID_WMAV2,
    SMEM_CODEC_ID_FLAC,

    /* subtitle codecs */

};

/**
 * rational number numerator/denominator
 */
typedef struct SMemRational{
    int num; ///< numerator
    int den; ///< denominator
} SMemRational;


enum SMemPixelFormat{
    SMEM_PIX_FMT_NONE = -1,
    SMEM_PIX_FMT_YUV420P,   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    SMEM_PIX_FMT_YUYV422,   ///< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
    SMEM_PIX_FMT_RGB24,     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
    SMEM_PIX_FMT_BGR24,     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
    SMEM_PIX_FMT_YUV422P,   ///< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
    SMEM_PIX_FMT_YUV444P,   ///< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
    SMEM_PIX_FMT_YUV410P,   ///< planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
    SMEM_PIX_FMT_YUV411P,   ///< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
    SMEM_PIX_FMT_GRAY8,     ///<        Y        ,  8bpp
    SMEM_PIX_FMT_MONOWHITE, ///<        Y        ,  1bpp, 0 is white, 1 is black, in each byte pixels are ordered from the msb to the lsb
    SMEM_PIX_FMT_MONOBLACK, ///<        Y        ,  1bpp, 0 is black, 1 is white, in each byte pixels are ordered from the msb to the lsb
    SMEM_PIX_FMT_PAL8,      ///< 8 bit with SMEM_PIX_FMT_RGB32 palette
    SMEM_PIX_FMT_YUVJ420P,  ///< planar YUV 4:2:0, 12bpp, full scale (JPEG), deprecated in favor of SMEM_PIX_FMT_YUV420P and setting color_range
    SMEM_PIX_FMT_YUVJ422P,  ///< planar YUV 4:2:2, 16bpp, full scale (JPEG), deprecated in favor of SMEM_PIX_FMT_YUV422P and setting color_range
    SMEM_PIX_FMT_YUVJ444P,  ///< planar YUV 4:4:4, 24bpp, full scale (JPEG), deprecated in favor of SMEM_PIX_FMT_YUV444P and setting color_range

    SMEM_PIX_FMT_UYVY422,   ///< packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
    SMEM_PIX_FMT_UYYVYY411, ///< packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
    SMEM_PIX_FMT_BGR8,      ///< packed RGB 3:3:2,  8bpp, (msb)2B 3G 3R(lsb)
    SMEM_PIX_FMT_BGR4,      ///< packed RGB 1:2:1 bitstream,  4bpp, (msb)1B 2G 1R(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits
    SMEM_PIX_FMT_BGR4_BYTE, ///< packed RGB 1:2:1,  8bpp, (msb)1B 2G 1R(lsb)
    SMEM_PIX_FMT_RGB8,      ///< packed RGB 3:3:2,  8bpp, (msb)2R 3G 3B(lsb)
    SMEM_PIX_FMT_RGB4,      ///< packed RGB 1:2:1 bitstream,  4bpp, (msb)1R 2G 1B(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits
    SMEM_PIX_FMT_RGB4_BYTE, ///< packed RGB 1:2:1,  8bpp, (msb)1R 2G 1B(lsb)
    SMEM_PIX_FMT_NV12,      ///< planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V)
    SMEM_PIX_FMT_NV21,      ///< as above, but U and V bytes are swapped

    SMEM_PIX_FMT_ARGB,      ///< packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
    SMEM_PIX_FMT_RGBA,      ///< packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
    SMEM_PIX_FMT_ABGR,      ///< packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
    SMEM_PIX_FMT_BGRA,      ///< packed BGRA 8:8:8:8, 32bpp, BGRABGRA...

    SMEM_PIX_FMT_GRAY16BE,  ///<        Y        , 16bpp, big-endian
    SMEM_PIX_FMT_GRAY16LE,  ///<        Y        , 16bpp, little-endian
    SMEM_PIX_FMT_YUV440P,   ///< planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
    SMEM_PIX_FMT_YUVJ440P,  ///< planar YUV 4:4:0 full scale (JPEG), deprecated in favor of SMEM_PIX_FMT_YUV440P and setting color_range
    SMEM_PIX_FMT_YUVA420P,  ///< planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A samples)

    SMEM_PIX_FMT_RGB48BE,   ///< packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for each R/G/B component is stored as big-endian
    SMEM_PIX_FMT_RGB48LE,   ///< packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for each R/G/B component is stored as little-endian

    SMEM_PIX_FMT_RGB565BE,  ///< packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), big-endian
    SMEM_PIX_FMT_RGB565LE,  ///< packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), little-endian
    SMEM_PIX_FMT_RGB555BE,  ///< packed RGB 5:5:5, 16bpp, (msb)1X 5R 5G 5B(lsb), big-endian   , X=unused/undefined
    SMEM_PIX_FMT_RGB555LE,  ///< packed RGB 5:5:5, 16bpp, (msb)1X 5R 5G 5B(lsb), little-endian, X=unused/undefined

    SMEM_PIX_FMT_BGR565BE,  ///< packed BGR 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb), big-endian
    SMEM_PIX_FMT_BGR565LE,  ///< packed BGR 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb), little-endian
    SMEM_PIX_FMT_BGR555BE,  ///< packed BGR 5:5:5, 16bpp, (msb)1X 5B 5G 5R(lsb), big-endian   , X=unused/undefined
    SMEM_PIX_FMT_BGR555LE,  ///< packed BGR 5:5:5, 16bpp, (msb)1X 5B 5G 5R(lsb), little-endian, X=unused/undefined

};

enum SMemSampleFormat{
    SMEM_SAMPLE_FMT_NONE = -1,
    SMEM_SAMPLE_FMT_U8,          ///< unsigned 8 bits
    SMEM_SAMPLE_FMT_S16,         ///< signed 16 bits
    SMEM_SAMPLE_FMT_S32,         ///< signed 32 bits
    SMEM_SAMPLE_FMT_FLT,         ///< float
    SMEM_SAMPLE_FMT_DBL,         ///< double

    SMEM_SAMPLE_FMT_U8P,         ///< unsigned 8 bits, planar
    SMEM_SAMPLE_FMT_S16P,        ///< signed 16 bits, planar
    SMEM_SAMPLE_FMT_S32P,        ///< signed 32 bits, planar
    SMEM_SAMPLE_FMT_FLTP,        ///< float, planar
    SMEM_SAMPLE_FMT_DBLP,        ///< double, planar

    SMEM_SAMPLE_FMT_NB           ///< Number of sample formats. DO NOT USE if linking dynamically

};

struct memory_info {
    int index;  // stream index
    int64_t pts;
    int64_t dts;
    int stream_info_offset;
    int stream_info_number;
    int data_offset;
    int data_size;
    int is_key;
    int id; // share memory id 

};


/* size_stream_info define large enough size for feature new add */
struct size_stream_info {
    char buffer[256];
};

struct video_stream_info {
    int width;
    int height;
    enum SMemPixelFormat pix_fmt; // enum AVPixelFormat
    int extradata_size;
    uint8_t extradata[128];
};


struct audio_stream_info {
    int sample_rate; ///< samples per second
    int channels;    ///< number of audio channels
    enum SMemSampleFormat sample_fmt;  ///< sample format
    int extradata_size;
    uint8_t extradata[128];
};

struct subtitle_stream_info{
    int test;
};

union stream_info_union {
    struct size_stream_info     size;
    struct video_stream_info    video;
    struct audio_stream_info    audio;
    struct subtitle_stream_info subtitle;
};

struct stream_info {
    int index; 

    enum SMemMediaType codec_type; 
    enum SMemCodecID     codec_id; 
    int stream_id; // stream id, most used in mpegts, add 2016.10.14
    SMemRational time_base;

    union stream_info_union info;
};

struct stream_info_old {
    int index; 

    enum SMemMediaType codec_type; 
    enum SMemCodecID     codec_id; 
    SMemRational time_base;

    /* video */
    int width;
    int height;
    enum SMemPixelFormat pix_fmt; // enum AVPixelFormat
    int video_extradata_size;
    uint8_t video_extradata[128];

    /* audio */
    int sample_rate; ///< samples per second
    int channels;    ///< number of audio channels
    enum SMemSampleFormat sample_fmt;  ///< sample format
    int audio_extradata_size;
    uint8_t audio_extradata[128];


    /* subtitle */


    /* others */
    //int stream_id; // stream id, most used in mpegts, add 2016.10.14

};


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif