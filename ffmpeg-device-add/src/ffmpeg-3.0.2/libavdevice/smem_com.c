#include "smem_com.h"

int smem2av_media_type(int media_type)
{
    switch(media_type){
        case SMEM_MEDIA_TYPE_VIDEO: return AVMEDIA_TYPE_VIDEO;
        case SMEM_MEDIA_TYPE_AUDIO: return AVMEDIA_TYPE_AUDIO;
        case SMEM_MEDIA_TYPE_DATA:  return AVMEDIA_TYPE_DATA;
        case SMEM_MEDIA_TYPE_SUBTITLE: return AVMEDIA_TYPE_SUBTITLE;
        default: return AVMEDIA_TYPE_UNKNOWN;
    }

    return AVMEDIA_TYPE_UNKNOWN;
}

int smem2av_codec_id(int codec_id)
{
    switch(codec_id){
        /* video codecs */
        case SMEM_CODEC_ID_MPEG1VIDEO: return AV_CODEC_ID_MPEG1VIDEO;
        case SMEM_CODEC_ID_MPEG2VIDEO: return AV_CODEC_ID_MPEG2VIDEO;
        case SMEM_CODEC_ID_H261: return AV_CODEC_ID_H261;
        case SMEM_CODEC_ID_H263: return AV_CODEC_ID_H263;
        case SMEM_CODEC_ID_MJPEG: return AV_CODEC_ID_MJPEG;
        case SMEM_CODEC_ID_MPEG4: return AV_CODEC_ID_MPEG4;
        case SMEM_CODEC_ID_RAWVIDEO: return AV_CODEC_ID_RAWVIDEO;
        case SMEM_CODEC_ID_H264: return AV_CODEC_ID_H264;
        case SMEM_CODEC_ID_PNG: return AV_CODEC_ID_PNG;
        case SMEM_CODEC_ID_JPEG2000: return AV_CODEC_ID_JPEG2000;
        case SMEM_CODEC_ID_VP9: return AV_CODEC_ID_VP9;
        case SMEM_CODEC_ID_HEVC: return AV_CODEC_ID_HEVC;
        //case SMEM_CODEC_ID_H265: return AV_CODEC_ID_H265;

        /* various PCM "codecs" */
        //case SMEM_CODEC_ID_FIRST_AUDIO: return AV_CODEC_ID_FIRST_AUDIO;
        case SMEM_CODEC_ID_PCM_S16LE: return AV_CODEC_ID_PCM_S16LE;
        case SMEM_CODEC_ID_PCM_S16BE: return AV_CODEC_ID_PCM_S16BE;
        case SMEM_CODEC_ID_PCM_U16LE: return AV_CODEC_ID_PCM_U16LE;
        case SMEM_CODEC_ID_PCM_U16BE: return AV_CODEC_ID_PCM_U16BE;
        case SMEM_CODEC_ID_PCM_S8: return AV_CODEC_ID_PCM_S8;
        case SMEM_CODEC_ID_PCM_U8: return AV_CODEC_ID_PCM_U8;
        case SMEM_CODEC_ID_PCM_MULAW: return AV_CODEC_ID_PCM_MULAW;
        case SMEM_CODEC_ID_PCM_ALAW: return AV_CODEC_ID_PCM_ALAW;
        case SMEM_CODEC_ID_PCM_S32LE: return AV_CODEC_ID_PCM_S32LE;
        case SMEM_CODEC_ID_PCM_S32BE: return AV_CODEC_ID_PCM_S32BE;
        case SMEM_CODEC_ID_PCM_U32LE: return AV_CODEC_ID_PCM_U32LE;
        case SMEM_CODEC_ID_PCM_U32BE: return AV_CODEC_ID_PCM_U32BE;
        case SMEM_CODEC_ID_PCM_S24LE: return AV_CODEC_ID_PCM_S24LE;
        case SMEM_CODEC_ID_PCM_S24BE: return AV_CODEC_ID_PCM_S24BE;
        case SMEM_CODEC_ID_PCM_U24LE: return AV_CODEC_ID_PCM_U24LE;
        case SMEM_CODEC_ID_PCM_U24BE: return AV_CODEC_ID_PCM_U24BE;
        case SMEM_CODEC_ID_PCM_S24DAUD: return AV_CODEC_ID_PCM_S24DAUD;
        case SMEM_CODEC_ID_PCM_ZORK: return AV_CODEC_ID_PCM_ZORK;
        case SMEM_CODEC_ID_PCM_S16LE_PLANAR: return AV_CODEC_ID_PCM_S16LE_PLANAR;
        case SMEM_CODEC_ID_PCM_DVD: return AV_CODEC_ID_PCM_DVD;
        case SMEM_CODEC_ID_PCM_F32BE: return AV_CODEC_ID_PCM_F32BE;
        case SMEM_CODEC_ID_PCM_F32LE: return AV_CODEC_ID_PCM_F32LE;
        case SMEM_CODEC_ID_PCM_F64BE: return AV_CODEC_ID_PCM_F64BE;
        case SMEM_CODEC_ID_PCM_F64LE: return AV_CODEC_ID_PCM_F64LE;
        case SMEM_CODEC_ID_PCM_BLURAY: return AV_CODEC_ID_PCM_BLURAY;
        case SMEM_CODEC_ID_PCM_LXF: return AV_CODEC_ID_PCM_LXF;
        case SMEM_CODEC_ID_S302M: return AV_CODEC_ID_S302M;
        case SMEM_CODEC_ID_PCM_S8_PLANAR: return AV_CODEC_ID_PCM_S8_PLANAR;
        case SMEM_CODEC_ID_PCM_S24LE_PLANAR: return AV_CODEC_ID_PCM_S24LE_PLANAR;
        case SMEM_CODEC_ID_PCM_S32LE_PLANAR: return AV_CODEC_ID_PCM_S32LE_PLANAR;
        case SMEM_CODEC_ID_PCM_S16BE_PLANAR: return AV_CODEC_ID_PCM_S16BE_PLANAR;

        /* audio codecs */
        case SMEM_CODEC_ID_MP2: return AV_CODEC_ID_MP2;
        case SMEM_CODEC_ID_MP3: return AV_CODEC_ID_MP3;
        case SMEM_CODEC_ID_AAC: return AV_CODEC_ID_AAC;
        case SMEM_CODEC_ID_AC3: return AV_CODEC_ID_AC3;
        case SMEM_CODEC_ID_DTS: return AV_CODEC_ID_DTS;
        case SMEM_CODEC_ID_VORBIS: return AV_CODEC_ID_VORBIS;
        case SMEM_CODEC_ID_WMAV1: return AV_CODEC_ID_WMAV1;
        case SMEM_CODEC_ID_WMAV2: return AV_CODEC_ID_WMAV2;
        case SMEM_CODEC_ID_FLAC: return AV_CODEC_ID_FLAC;

        /* subtitle codecs */

        default: return AV_CODEC_ID_NONE;
    }


    return AV_CODEC_ID_NONE;
}

int av2smem_codec_id(int codec_id)
{
    switch(codec_id){
        /* video codecs */
        case AV_CODEC_ID_MPEG1VIDEO: return SMEM_CODEC_ID_MPEG1VIDEO;
        case AV_CODEC_ID_MPEG2VIDEO: return SMEM_CODEC_ID_MPEG2VIDEO;
        case AV_CODEC_ID_H261: return SMEM_CODEC_ID_H261;
        case AV_CODEC_ID_H263: return SMEM_CODEC_ID_H263;
        case AV_CODEC_ID_MJPEG: return SMEM_CODEC_ID_MJPEG;
        case AV_CODEC_ID_MPEG4: return SMEM_CODEC_ID_MPEG4;
        case AV_CODEC_ID_RAWVIDEO: return SMEM_CODEC_ID_RAWVIDEO;
        case AV_CODEC_ID_H264: return SMEM_CODEC_ID_H264;
        case AV_CODEC_ID_PNG: return SMEM_CODEC_ID_PNG;
        case AV_CODEC_ID_JPEG2000: return SMEM_CODEC_ID_JPEG2000;
        case AV_CODEC_ID_VP9: return SMEM_CODEC_ID_VP9;
        case AV_CODEC_ID_HEVC: return SMEM_CODEC_ID_HEVC;
        //case AV_CODEC_ID_H265: return SMEM_CODEC_ID_H265;

        /* various PCM "codecs" */
        //case AV_CODEC_ID_FIRST_AUDIO: return SMEM_CODEC_ID_FIRST_AUDIO;
        case AV_CODEC_ID_PCM_S16LE: return SMEM_CODEC_ID_PCM_S16LE;
        case AV_CODEC_ID_PCM_S16BE: return SMEM_CODEC_ID_PCM_S16BE;
        case AV_CODEC_ID_PCM_U16LE: return SMEM_CODEC_ID_PCM_U16LE;
        case AV_CODEC_ID_PCM_U16BE: return SMEM_CODEC_ID_PCM_U16BE;
        case AV_CODEC_ID_PCM_S8: return SMEM_CODEC_ID_PCM_S8;
        case AV_CODEC_ID_PCM_U8: return SMEM_CODEC_ID_PCM_U8;
        case AV_CODEC_ID_PCM_MULAW: return SMEM_CODEC_ID_PCM_MULAW;
        case AV_CODEC_ID_PCM_ALAW: return SMEM_CODEC_ID_PCM_ALAW;
        case AV_CODEC_ID_PCM_S32LE: return SMEM_CODEC_ID_PCM_S32LE;
        case AV_CODEC_ID_PCM_S32BE: return SMEM_CODEC_ID_PCM_S32BE;
        case AV_CODEC_ID_PCM_U32LE: return SMEM_CODEC_ID_PCM_U32LE;
        case AV_CODEC_ID_PCM_U32BE: return SMEM_CODEC_ID_PCM_U32BE;
        case AV_CODEC_ID_PCM_S24LE: return SMEM_CODEC_ID_PCM_S24LE;
        case AV_CODEC_ID_PCM_S24BE: return SMEM_CODEC_ID_PCM_S24BE;
        case AV_CODEC_ID_PCM_U24LE: return SMEM_CODEC_ID_PCM_U24LE;
        case AV_CODEC_ID_PCM_U24BE: return SMEM_CODEC_ID_PCM_U24BE;
        case AV_CODEC_ID_PCM_S24DAUD: return SMEM_CODEC_ID_PCM_S24DAUD;
        case AV_CODEC_ID_PCM_ZORK: return SMEM_CODEC_ID_PCM_ZORK;
        case AV_CODEC_ID_PCM_S16LE_PLANAR: return SMEM_CODEC_ID_PCM_S16LE_PLANAR;
        case AV_CODEC_ID_PCM_DVD: return SMEM_CODEC_ID_PCM_DVD;
        case AV_CODEC_ID_PCM_F32BE: return SMEM_CODEC_ID_PCM_F32BE;
        case AV_CODEC_ID_PCM_F32LE: return SMEM_CODEC_ID_PCM_F32LE;
        case AV_CODEC_ID_PCM_F64BE: return SMEM_CODEC_ID_PCM_F64BE;
        case AV_CODEC_ID_PCM_F64LE: return SMEM_CODEC_ID_PCM_F64LE;
        case AV_CODEC_ID_PCM_BLURAY: return SMEM_CODEC_ID_PCM_BLURAY;
        case AV_CODEC_ID_PCM_LXF: return SMEM_CODEC_ID_PCM_LXF;
        case AV_CODEC_ID_S302M: return SMEM_CODEC_ID_S302M;
        case AV_CODEC_ID_PCM_S8_PLANAR: return SMEM_CODEC_ID_PCM_S8_PLANAR;
        case AV_CODEC_ID_PCM_S24LE_PLANAR: return SMEM_CODEC_ID_PCM_S24LE_PLANAR;
        case AV_CODEC_ID_PCM_S32LE_PLANAR: return SMEM_CODEC_ID_PCM_S32LE_PLANAR;
        case AV_CODEC_ID_PCM_S16BE_PLANAR: return SMEM_CODEC_ID_PCM_S16BE_PLANAR;

        /* audio codecs */
        case AV_CODEC_ID_MP2: return SMEM_CODEC_ID_MP2;
        case AV_CODEC_ID_MP3: return SMEM_CODEC_ID_MP3;
        case AV_CODEC_ID_AAC: return SMEM_CODEC_ID_AAC;
        case AV_CODEC_ID_AC3: return SMEM_CODEC_ID_AC3;
        case AV_CODEC_ID_DTS: return SMEM_CODEC_ID_DTS;
        case AV_CODEC_ID_VORBIS: return SMEM_CODEC_ID_VORBIS;
        case AV_CODEC_ID_WMAV1: return SMEM_CODEC_ID_WMAV1;
        case AV_CODEC_ID_WMAV2: return SMEM_CODEC_ID_WMAV2;
        case AV_CODEC_ID_FLAC: return SMEM_CODEC_ID_FLAC;

        /* subtitle codecs */

        default: return SMEM_CODEC_ID_NONE;
    }

    return SMEM_CODEC_ID_NONE;
}

int smem2av_pix_fmt(int pix_fmt)
{
    switch(pix_fmt){
        case SMEM_PIX_FMT_NONE: return AV_PIX_FMT_NONE;
        case SMEM_PIX_FMT_YUV420P: return AV_PIX_FMT_YUV420P;
        case SMEM_PIX_FMT_YUYV422: return AV_PIX_FMT_YUYV422;
        case SMEM_PIX_FMT_RGB24: return AV_PIX_FMT_RGB24; 
        case SMEM_PIX_FMT_BGR24: return AV_PIX_FMT_BGR24;
        case SMEM_PIX_FMT_YUV422P: return AV_PIX_FMT_YUV422P;
        case SMEM_PIX_FMT_YUV444P: return AV_PIX_FMT_YUV444P;
        case SMEM_PIX_FMT_YUV410P: return AV_PIX_FMT_YUV410P;
        case SMEM_PIX_FMT_YUV411P: return AV_PIX_FMT_YUV411P;
        case SMEM_PIX_FMT_GRAY8: return AV_PIX_FMT_GRAY8; 
        case SMEM_PIX_FMT_MONOWHITE: return AV_PIX_FMT_MONOWHITE;
        case SMEM_PIX_FMT_MONOBLACK: return AV_PIX_FMT_MONOBLACK; 
        case SMEM_PIX_FMT_PAL8: return AV_PIX_FMT_PAL8;     
        case SMEM_PIX_FMT_YUVJ420P: return AV_PIX_FMT_YUVJ420P;  
        case SMEM_PIX_FMT_YUVJ422P: return AV_PIX_FMT_YUVJ422P;  
        case SMEM_PIX_FMT_YUVJ444P: return AV_PIX_FMT_YUVJ444P;  

        case SMEM_PIX_FMT_UYVY422: return AV_PIX_FMT_UYVY422;  
        case SMEM_PIX_FMT_UYYVYY411: return AV_PIX_FMT_UYYVYY411; 
        case SMEM_PIX_FMT_BGR8: return AV_PIX_FMT_BGR8;      
        case SMEM_PIX_FMT_BGR4: return AV_PIX_FMT_BGR4;     
        case SMEM_PIX_FMT_BGR4_BYTE: return AV_PIX_FMT_BGR4_BYTE; 
        case SMEM_PIX_FMT_RGB8: return AV_PIX_FMT_RGB8;      
        case SMEM_PIX_FMT_RGB4: return AV_PIX_FMT_RGB4;      
        case SMEM_PIX_FMT_RGB4_BYTE: return AV_PIX_FMT_RGB4_BYTE; 
        case SMEM_PIX_FMT_NV12: return AV_PIX_FMT_NV12;      
        case SMEM_PIX_FMT_NV21: return AV_PIX_FMT_NV21;      

        case SMEM_PIX_FMT_ARGB: return AV_PIX_FMT_ARGB;      
        case SMEM_PIX_FMT_RGBA: return AV_PIX_FMT_RGBA;      
        case SMEM_PIX_FMT_ABGR: return AV_PIX_FMT_ABGR;      
        case SMEM_PIX_FMT_BGRA: return AV_PIX_FMT_BGRA;     

        case SMEM_PIX_FMT_GRAY16BE: return AV_PIX_FMT_GRAY16BE;  
        case SMEM_PIX_FMT_GRAY16LE: return AV_PIX_FMT_GRAY16LE;  
        case SMEM_PIX_FMT_YUV440P: return AV_PIX_FMT_YUV440P;   
        case SMEM_PIX_FMT_YUVJ440P: return AV_PIX_FMT_YUVJ440P;  
        case SMEM_PIX_FMT_YUVA420P: return AV_PIX_FMT_YUVA420P;  

        case SMEM_PIX_FMT_RGB48BE: return AV_PIX_FMT_RGB48BE;   
        case SMEM_PIX_FMT_RGB48LE: return AV_PIX_FMT_RGB48LE;   

        case SMEM_PIX_FMT_RGB565BE: return AV_PIX_FMT_RGB565BE;  
        case SMEM_PIX_FMT_RGB565LE: return AV_PIX_FMT_RGB565LE;  
        case SMEM_PIX_FMT_RGB555BE: return AV_PIX_FMT_RGB555BE;  
        case SMEM_PIX_FMT_RGB555LE: return AV_PIX_FMT_RGB555LE; 

        case SMEM_PIX_FMT_BGR565BE: return AV_PIX_FMT_BGR565BE; 
        case SMEM_PIX_FMT_BGR565LE: return AV_PIX_FMT_BGR565LE;  
        case SMEM_PIX_FMT_BGR555BE: return AV_PIX_FMT_BGR555BE;  
        case SMEM_PIX_FMT_BGR555LE: return AV_PIX_FMT_BGR555LE;  

        default: return AV_PIX_FMT_NONE;
    }

    return AV_PIX_FMT_NONE;
}

int av2smem_pix_fmt(int pix_fmt)
{
    switch(pix_fmt){
        case AV_PIX_FMT_NONE: return SMEM_PIX_FMT_NONE;
        case AV_PIX_FMT_YUV420P: return SMEM_PIX_FMT_YUV420P;
        case AV_PIX_FMT_YUYV422: return SMEM_PIX_FMT_YUYV422;
        case AV_PIX_FMT_RGB24: return SMEM_PIX_FMT_RGB24; 
        case AV_PIX_FMT_BGR24: return SMEM_PIX_FMT_BGR24;
        case AV_PIX_FMT_YUV422P: return SMEM_PIX_FMT_YUV422P;
        case AV_PIX_FMT_YUV444P: return SMEM_PIX_FMT_YUV444P;
        case AV_PIX_FMT_YUV410P: return SMEM_PIX_FMT_YUV410P;
        case AV_PIX_FMT_YUV411P: return SMEM_PIX_FMT_YUV411P;
        case AV_PIX_FMT_GRAY8: return SMEM_PIX_FMT_GRAY8; 
        case AV_PIX_FMT_MONOWHITE: return SMEM_PIX_FMT_MONOWHITE;
        case AV_PIX_FMT_MONOBLACK: return SMEM_PIX_FMT_MONOBLACK; 
        case AV_PIX_FMT_PAL8: return SMEM_PIX_FMT_PAL8;     
        case AV_PIX_FMT_YUVJ420P: return SMEM_PIX_FMT_YUVJ420P;  
        case AV_PIX_FMT_YUVJ422P: return SMEM_PIX_FMT_YUVJ422P;  
        case AV_PIX_FMT_YUVJ444P: return SMEM_PIX_FMT_YUVJ444P;  

        case AV_PIX_FMT_UYVY422: return SMEM_PIX_FMT_UYVY422;  
        case AV_PIX_FMT_UYYVYY411: return SMEM_PIX_FMT_UYYVYY411; 
        case AV_PIX_FMT_BGR8: return SMEM_PIX_FMT_BGR8;      
        case AV_PIX_FMT_BGR4: return SMEM_PIX_FMT_BGR4;     
        case AV_PIX_FMT_BGR4_BYTE: return SMEM_PIX_FMT_BGR4_BYTE; 
        case AV_PIX_FMT_RGB8: return SMEM_PIX_FMT_RGB8;      
        case AV_PIX_FMT_RGB4: return SMEM_PIX_FMT_RGB4;      
        case AV_PIX_FMT_RGB4_BYTE: return SMEM_PIX_FMT_RGB4_BYTE; 
        case AV_PIX_FMT_NV12: return SMEM_PIX_FMT_NV12;      
        case AV_PIX_FMT_NV21: return SMEM_PIX_FMT_NV21;      

        case AV_PIX_FMT_ARGB: return SMEM_PIX_FMT_ARGB;      
        case AV_PIX_FMT_RGBA: return SMEM_PIX_FMT_RGBA;      
        case AV_PIX_FMT_ABGR: return SMEM_PIX_FMT_ABGR;      
        case AV_PIX_FMT_BGRA: return SMEM_PIX_FMT_BGRA;     

        case AV_PIX_FMT_GRAY16BE: return SMEM_PIX_FMT_GRAY16BE;  
        case AV_PIX_FMT_GRAY16LE: return SMEM_PIX_FMT_GRAY16LE;  
        case AV_PIX_FMT_YUV440P: return SMEM_PIX_FMT_YUV440P;   
        case AV_PIX_FMT_YUVJ440P: return SMEM_PIX_FMT_YUVJ440P;  
        case AV_PIX_FMT_YUVA420P: return SMEM_PIX_FMT_YUVA420P;  

        case AV_PIX_FMT_RGB48BE: return SMEM_PIX_FMT_RGB48BE;   
        case AV_PIX_FMT_RGB48LE: return SMEM_PIX_FMT_RGB48LE;   

        case AV_PIX_FMT_RGB565BE: return SMEM_PIX_FMT_RGB565BE;  
        case AV_PIX_FMT_RGB565LE: return SMEM_PIX_FMT_RGB565LE;  
        case AV_PIX_FMT_RGB555BE: return SMEM_PIX_FMT_RGB555BE;  
        case AV_PIX_FMT_RGB555LE: return SMEM_PIX_FMT_RGB555LE; 

        case AV_PIX_FMT_BGR565BE: return SMEM_PIX_FMT_BGR565BE; 
        case AV_PIX_FMT_BGR565LE: return SMEM_PIX_FMT_BGR565LE;  
        case AV_PIX_FMT_BGR555BE: return SMEM_PIX_FMT_BGR555BE;  
        case AV_PIX_FMT_BGR555LE: return SMEM_PIX_FMT_BGR555LE;  
    
        default: return SMEM_PIX_FMT_NONE;
    }

    return SMEM_PIX_FMT_NONE;
}


int smem2av_sample_fmt(int sample_fmt)
{
    switch(sample_fmt){
        case SMEM_SAMPLE_FMT_NONE: return AV_SAMPLE_FMT_NONE;
        case SMEM_SAMPLE_FMT_U8: return AV_SAMPLE_FMT_U8;
        case SMEM_SAMPLE_FMT_S16: return AV_SAMPLE_FMT_S16;
        case SMEM_SAMPLE_FMT_S32: return AV_SAMPLE_FMT_S32;
        case SMEM_SAMPLE_FMT_FLT: return AV_SAMPLE_FMT_FLT;
        case SMEM_SAMPLE_FMT_DBL: return AV_SAMPLE_FMT_DBL;

        case SMEM_SAMPLE_FMT_U8P: return AV_SAMPLE_FMT_U8P;
        case SMEM_SAMPLE_FMT_S16P: return AV_SAMPLE_FMT_S16P;
        case SMEM_SAMPLE_FMT_S32P: return AV_SAMPLE_FMT_S32P;
        case SMEM_SAMPLE_FMT_FLTP: return AV_SAMPLE_FMT_FLTP;
        case SMEM_SAMPLE_FMT_DBLP: return AV_SAMPLE_FMT_DBLP;

        case SMEM_SAMPLE_FMT_NB: return AV_SAMPLE_FMT_NB; 
    }

    return AV_SAMPLE_FMT_NONE;
}

int av2smem_sample_fmt(int sample_fmt)
{
    switch(sample_fmt){
        case AV_SAMPLE_FMT_NONE: return SMEM_SAMPLE_FMT_NONE;
        case AV_SAMPLE_FMT_U8: return SMEM_SAMPLE_FMT_U8;
        case AV_SAMPLE_FMT_S16: return SMEM_SAMPLE_FMT_S16;
        case AV_SAMPLE_FMT_S32: return SMEM_SAMPLE_FMT_S32;
        case AV_SAMPLE_FMT_FLT: return SMEM_SAMPLE_FMT_FLT;
        case AV_SAMPLE_FMT_DBL: return SMEM_SAMPLE_FMT_DBL;

        case AV_SAMPLE_FMT_U8P: return SMEM_SAMPLE_FMT_U8P;
        case AV_SAMPLE_FMT_S16P: return SMEM_SAMPLE_FMT_S16P;
        case AV_SAMPLE_FMT_S32P: return SMEM_SAMPLE_FMT_S32P;
        case AV_SAMPLE_FMT_FLTP: return SMEM_SAMPLE_FMT_FLTP;
        case AV_SAMPLE_FMT_DBLP: return SMEM_SAMPLE_FMT_DBLP;

        case AV_SAMPLE_FMT_NB: return SMEM_SAMPLE_FMT_NB; 
    }

    return SMEM_SAMPLE_FMT_NONE;
}
