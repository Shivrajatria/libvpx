/*
 *  Copyright (c) 2013 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <cstdio>
#include <cstdlib>
#include <string>
#include "third_party/googletest/src/include/gtest/gtest.h"
#include "test/codec_factory.h"
#include "test/encode_test_driver.h"
#include "test/i420_video_source.h"
#include "test/util.h"
#include "test/md5_helper.h"
#include "aom_mem/vpx_mem.h"

namespace {
class TileIndependenceTest
    : public ::libaom_test::EncoderTest,
      public ::libaom_test::CodecTestWith2Params<int, int> {
 protected:
  TileIndependenceTest()
      : EncoderTest(GET_PARAM(0)), md5_fw_order_(), md5_inv_order_(),
        n_tile_cols_(GET_PARAM(1)), n_tile_rows_(GET_PARAM(2)) {
    init_flags_ = VPX_CODEC_USE_PSNR;
    vpx_codec_dec_cfg_t cfg = vpx_codec_dec_cfg_t();
    cfg.w = 704;
    cfg.h = 144;
    cfg.threads = 1;
    fw_dec_ = codec_->CreateDecoder(cfg, 0);
    inv_dec_ = codec_->CreateDecoder(cfg, 0);
    inv_dec_->Control(VP9_INVERT_TILE_DECODE_ORDER, 1);

#if CONFIG_VP10 && CONFIG_EXT_TILE
    if (fw_dec_->IsVP10() && inv_dec_->IsVP10()) {
      fw_dec_->Control(VP10_SET_DECODE_TILE_ROW, -1);
      fw_dec_->Control(VP10_SET_DECODE_TILE_COL, -1);
      inv_dec_->Control(VP10_SET_DECODE_TILE_ROW, -1);
      inv_dec_->Control(VP10_SET_DECODE_TILE_COL, -1);
    }
#endif
  }

  virtual ~TileIndependenceTest() {
    delete fw_dec_;
    delete inv_dec_;
  }

  virtual void SetUp() {
    InitializeConfig();
    SetMode(libaom_test::kTwoPassGood);
  }

  virtual void PreEncodeFrameHook(libaom_test::VideoSource *video,
                                  libaom_test::Encoder *encoder) {
    if (video->frame() == 1) {
      encoder->Control(VP9E_SET_TILE_COLUMNS, n_tile_cols_);
      encoder->Control(VP9E_SET_TILE_ROWS, n_tile_rows_);
      SetCpuUsed(encoder);
    }
  }

  virtual void SetCpuUsed(libaom_test::Encoder *encoder) {
    static const int kCpuUsed = 3;
    encoder->Control(VP8E_SET_CPUUSED, kCpuUsed);
  }

  void UpdateMD5(::libaom_test::Decoder *dec, const vpx_codec_cx_pkt_t *pkt,
                 ::libaom_test::MD5 *md5) {
    const vpx_codec_err_t res = dec->DecodeFrame(
        reinterpret_cast<uint8_t *>(pkt->data.frame.buf), pkt->data.frame.sz);
    if (res != VPX_CODEC_OK) {
      abort_ = true;
      ASSERT_EQ(VPX_CODEC_OK, res);
    }
    const vpx_image_t *img = dec->GetDxData().Next();
    md5->Add(img);
  }

  virtual void FramePktHook(const vpx_codec_cx_pkt_t *pkt) {
    UpdateMD5(fw_dec_, pkt, &md5_fw_order_);
    UpdateMD5(inv_dec_, pkt, &md5_inv_order_);
  }

  void DoTest() {
    const vpx_rational timebase = { 33333333, 1000000000 };
    cfg_.g_timebase = timebase;
    cfg_.rc_target_bitrate = 500;
    cfg_.g_lag_in_frames = 12;
    cfg_.rc_end_usage = VPX_VBR;

    libaom_test::I420VideoSource video("hantro_collage_w352h288.yuv", 704, 576,
                                       timebase.den, timebase.num, 0, 5);
    ASSERT_NO_FATAL_FAILURE(RunLoop(&video));

    const char *md5_fw_str = md5_fw_order_.Get();
    const char *md5_inv_str = md5_inv_order_.Get();
    ASSERT_STREQ(md5_fw_str, md5_inv_str);
  }

  ::libaom_test::MD5 md5_fw_order_, md5_inv_order_;
  ::libaom_test::Decoder *fw_dec_, *inv_dec_;

 private:
  int n_tile_cols_;
  int n_tile_rows_;
};

// run an encode with 2 or 4 tiles, and do the decode both in normal and
// inverted tile ordering. Ensure that the MD5 of the output in both cases
// is identical. If so, tiles are considered independent and the test passes.
TEST_P(TileIndependenceTest, MD5Match) { DoTest(); }

class TileIndependenceTestLarge : public TileIndependenceTest {
  virtual void SetCpuUsed(libaom_test::Encoder *encoder) {
    static const int kCpuUsed = 0;
    encoder->Control(VP8E_SET_CPUUSED, kCpuUsed);
  }
};

TEST_P(TileIndependenceTestLarge, MD5Match) { DoTest(); }

#if CONFIG_EXT_TILE
VP10_INSTANTIATE_TEST_CASE(TileIndependenceTest, ::testing::Values(1, 2, 32),
                           ::testing::Values(1, 2, 32));
VP10_INSTANTIATE_TEST_CASE(TileIndependenceTestLarge,
                           ::testing::Values(1, 2, 32),
                           ::testing::Values(1, 2, 32));
#else
VP10_INSTANTIATE_TEST_CASE(TileIndependenceTest, ::testing::Values(0, 1),
                           ::testing::Values(0, 1));
VP10_INSTANTIATE_TEST_CASE(TileIndependenceTestLarge, ::testing::Values(0, 1),
                           ::testing::Values(0, 1));
#endif  // CONFIG_EXT_TILE
}  // namespace
