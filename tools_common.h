/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef TOOLS_COMMON_H_
#define TOOLS_COMMON_H_

#include <stdio.h>

#include "./vpx_config.h"
#include "aom/vpx_codec.h"
#include "aom/vpx_image.h"
#include "aom/vpx_integer.h"
#include "aom_ports/msvc.h"

#if CONFIG_ENCODERS
#include "./y4minput.h"
#endif

#if defined(_MSC_VER)
/* MSVS uses _f{seek,tell}i64. */
#define fseeko _fseeki64
#define ftello _ftelli64
#elif defined(_WIN32)
/* MinGW uses f{seek,tell}o64 for large files. */
#define fseeko fseeko64
#define ftello ftello64
#endif /* _WIN32 */

#if CONFIG_OS_SUPPORT
#if defined(_MSC_VER)
#include <io.h> /* NOLINT */
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h> /* NOLINT */
#endif              /* _MSC_VER */
#endif              /* CONFIG_OS_SUPPORT */

/* Use 32-bit file operations in WebM file format when building ARM
 * executables (.axf) with RVCT. */
#if !CONFIG_OS_SUPPORT
#define fseeko fseek
#define ftello ftell
#endif /* CONFIG_OS_SUPPORT */

#define LITERALU64(hi, lo) ((((uint64_t)hi) << 32) | lo)

#ifndef PATH_MAX
#define PATH_MAX 512
#endif

#define IVF_FRAME_HDR_SZ (4 + 8) /* 4 byte size + 8 byte timestamp */
#define IVF_FILE_HDR_SZ 32

#define RAW_FRAME_HDR_SZ sizeof(uint32_t)

#define VP8_FOURCC 0x30385056
#define VP9_FOURCC 0x30395056
#define VP10_FOURCC 0x303a5056

enum VideoFileType {
  FILE_TYPE_RAW,
  FILE_TYPE_IVF,
  FILE_TYPE_Y4M,
  FILE_TYPE_WEBM
};

struct FileTypeDetectionBuffer {
  char buf[4];
  size_t buf_read;
  size_t position;
};

struct VpxRational {
  int numerator;
  int denominator;
};

struct VpxInputContext {
  const char *filename;
  FILE *file;
  int64_t length;
  struct FileTypeDetectionBuffer detect;
  enum VideoFileType file_type;
  uint32_t width;
  uint32_t height;
  struct VpxRational pixel_aspect_ratio;
  vpx_img_fmt_t fmt;
  vpx_bit_depth_t bit_depth;
  int only_i420;
  uint32_t fourcc;
  struct VpxRational framerate;
#if CONFIG_ENCODERS
  y4m_input y4m;
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__)
#define VPX_NO_RETURN __attribute__((noreturn))
#else
#define VPX_NO_RETURN
#endif

/* Sets a stdio stream into binary mode */
FILE *set_binary_mode(FILE *stream);

void die(const char *fmt, ...) VPX_NO_RETURN;
void fatal(const char *fmt, ...) VPX_NO_RETURN;
void warn(const char *fmt, ...);

void die_codec(vpx_codec_ctx_t *ctx, const char *s) VPX_NO_RETURN;

/* The tool including this file must define usage_exit() */
void usage_exit(void) VPX_NO_RETURN;

#undef VPX_NO_RETURN

int read_yuv_frame(struct VpxInputContext *input_ctx, vpx_image_t *yuv_frame);

typedef struct VpxInterface {
  const char *const name;
  const uint32_t fourcc;
  vpx_codec_iface_t *(*const codec_interface)();
} VpxInterface;

int get_vpx_encoder_count(void);
const VpxInterface *get_vpx_encoder_by_index(int i);
const VpxInterface *get_vpx_encoder_by_name(const char *name);

int get_vpx_decoder_count(void);
const VpxInterface *get_vpx_decoder_by_index(int i);
const VpxInterface *get_vpx_decoder_by_name(const char *name);
const VpxInterface *get_vpx_decoder_by_fourcc(uint32_t fourcc);

// TODO(dkovalev): move this function to vpx_image.{c, h}, so it will be part
// of vpx_image_t support
int vpx_img_plane_width(const vpx_image_t *img, int plane);
int vpx_img_plane_height(const vpx_image_t *img, int plane);
void vpx_img_write(const vpx_image_t *img, FILE *file);
int vpx_img_read(vpx_image_t *img, FILE *file);

double sse_to_psnr(double samples, double peak, double mse);

#if CONFIG_VP9_HIGHBITDEPTH
void vpx_img_upshift(vpx_image_t *dst, vpx_image_t *src, int input_shift);
void vpx_img_downshift(vpx_image_t *dst, vpx_image_t *src, int down_shift);
void vpx_img_truncate_16_to_8(vpx_image_t *dst, vpx_image_t *src);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  // TOOLS_COMMON_H_
