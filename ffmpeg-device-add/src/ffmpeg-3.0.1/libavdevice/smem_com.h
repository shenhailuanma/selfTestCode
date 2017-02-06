#ifndef SMEM_COM_H
#define SMEM_COM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "libavutil/opt.h"
#include "config.h"
#include "libavformat/avformat.h"
#include "libavformat/internal.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"
#include "libavutil/buffer.h"
#include "libavutil/buffer_internal.h"

#include "smem.h"


int smem2av_media_type(int media_type);

int smem2av_codec_id(int codec_id);

int av2smem_codec_id(int codec_id);

int smem2av_pix_fmt(int pix_fmt);

int av2smem_pix_fmt(int pix_fmt);

int smem2av_sample_fmt(int sample_fmt);

int av2smem_sample_fmt(int sample_fmt);

void print_hex(char * phex, int size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif   