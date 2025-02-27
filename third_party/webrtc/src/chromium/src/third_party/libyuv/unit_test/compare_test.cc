/*
 *  Copyright 2011 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../unit_test/unit_test.h"
#include "libyuv/basic_types.h"
#include "libyuv/compare.h"
#include "libyuv/cpu_id.h"
#include "libyuv/row.h"
#include "libyuv/video_common.h"

namespace libyuv {

// hash seed of 5381 recommended.
static uint32 ReferenceHashDjb2(const uint8* src, uint64 count, uint32 seed) {
  uint32 hash = seed;
  if (count > 0) {
    do {
      hash = hash * 33 + *src++;
    } while (--count);
  }
  return hash;
}

TEST_F(libyuvTest, Djb2_Test) {
  const int kMaxTest = benchmark_width_ * benchmark_height_;
  align_buffer_64(src_a, kMaxTest);
  align_buffer_64(src_b, kMaxTest);

  const char* fox = "The quick brown fox jumps over the lazy dog"
      " and feels as if he were in the seventh heaven of typography"
      " together with Hermann Zapf";
  uint32 foxhash = HashDjb2(reinterpret_cast<const uint8*>(fox), 131, 5381);
  const uint32 kExpectedFoxHash = 2611006483u;
  EXPECT_EQ(kExpectedFoxHash, foxhash);

  for (int i = 0; i < kMaxTest; ++i) {
    src_a[i] = (fastrand() & 0xff);
    src_b[i] = (fastrand() & 0xff);
  }
  // Compare different buffers. Expect hash is different.
  uint32 h1 = HashDjb2(src_a, kMaxTest, 5381);
  uint32 h2 = HashDjb2(src_b, kMaxTest, 5381);
  EXPECT_NE(h1, h2);

  // Make last half same. Expect hash is different.
  memcpy(src_a + kMaxTest / 2, src_b + kMaxTest / 2, kMaxTest / 2);
  h1 = HashDjb2(src_a, kMaxTest, 5381);
  h2 = HashDjb2(src_b, kMaxTest, 5381);
  EXPECT_NE(h1, h2);

  // Make first half same. Expect hash is different.
  memcpy(src_a + kMaxTest / 2, src_a, kMaxTest / 2);
  memcpy(src_b + kMaxTest / 2, src_b, kMaxTest / 2);
  memcpy(src_a, src_b, kMaxTest / 2);
  h1 = HashDjb2(src_a, kMaxTest, 5381);
  h2 = HashDjb2(src_b, kMaxTest, 5381);
  EXPECT_NE(h1, h2);

  // Make same. Expect hash is same.
  memcpy(src_a, src_b, kMaxTest);
  h1 = HashDjb2(src_a, kMaxTest, 5381);
  h2 = HashDjb2(src_b, kMaxTest, 5381);
  EXPECT_EQ(h1, h2);

  // Mask seed different. Expect hash is different.
  memcpy(src_a, src_b, kMaxTest);
  h1 = HashDjb2(src_a, kMaxTest, 5381);
  h2 = HashDjb2(src_b, kMaxTest, 1234);
  EXPECT_NE(h1, h2);

  // Make one byte different in middle. Expect hash is different.
  memcpy(src_a, src_b, kMaxTest);
  ++src_b[kMaxTest / 2];
  h1 = HashDjb2(src_a, kMaxTest, 5381);
  h2 = HashDjb2(src_b, kMaxTest, 5381);
  EXPECT_NE(h1, h2);

  // Make first byte different. Expect hash is different.
  memcpy(src_a, src_b, kMaxTest);
  ++src_b[0];
  h1 = HashDjb2(src_a, kMaxTest, 5381);
  h2 = HashDjb2(src_b, kMaxTest, 5381);
  EXPECT_NE(h1, h2);

  // Make last byte different. Expect hash is different.
  memcpy(src_a, src_b, kMaxTest);
  ++src_b[kMaxTest - 1];
  h1 = HashDjb2(src_a, kMaxTest, 5381);
  h2 = HashDjb2(src_b, kMaxTest, 5381);
  EXPECT_NE(h1, h2);

  // Make a zeros. Test different lengths. Expect hash is different.
  memset(src_a, 0, kMaxTest);
  h1 = HashDjb2(src_a, kMaxTest, 5381);
  h2 = HashDjb2(src_a, kMaxTest / 2, 5381);
  EXPECT_NE(h1, h2);

  // Make a zeros and seed of zero. Test different lengths. Expect hash is same.
  memset(src_a, 0, kMaxTest);
  h1 = HashDjb2(src_a, kMaxTest, 0);
  h2 = HashDjb2(src_a, kMaxTest / 2, 0);
  EXPECT_EQ(h1, h2);

  free_aligned_buffer_64(src_a);
  free_aligned_buffer_64(src_b);
}

TEST_F(libyuvTest, BenchmarkDjb2_Opt) {
  const int kMaxTest = benchmark_width_ * benchmark_height_;
  align_buffer_64(src_a, kMaxTest);

  for (int i = 0; i < kMaxTest; ++i) {
    src_a[i] = i;
  }
  uint32 h2 = ReferenceHashDjb2(src_a, kMaxTest, 5381);
  uint32 h1;
  for (int i = 0; i < benchmark_iterations_; ++i) {
    h1 = HashDjb2(src_a, kMaxTest, 5381);
  }
  EXPECT_EQ(h1, h2);
  free_aligned_buffer_64(src_a);
}

TEST_F(libyuvTest, BenchmarkDjb2_Unaligned) {
  const int kMaxTest = benchmark_width_ * benchmark_height_;
  align_buffer_64(src_a, kMaxTest + 1);
  for (int i = 0; i < kMaxTest; ++i) {
    src_a[i + 1] = i;
  }
  uint32 h2 = ReferenceHashDjb2(src_a + 1, kMaxTest, 5381);
  uint32 h1;
  for (int i = 0; i < benchmark_iterations_; ++i) {
    h1 = HashDjb2(src_a + 1, kMaxTest, 5381);
  }
  EXPECT_EQ(h1, h2);
  free_aligned_buffer_64(src_a);
}

TEST_F(libyuvTest, BenchmarkARGBDetect_Opt) {
  uint32 fourcc;
  const int kMaxTest = benchmark_width_ * benchmark_height_ * 4;
  align_buffer_64(src_a, kMaxTest);
  for (int i = 0; i < kMaxTest; ++i) {
    src_a[i] = 255;
  }

  src_a[0] = 0;
  fourcc = ARGBDetect(src_a, benchmark_width_ * 4,
                      benchmark_width_, benchmark_height_);
  EXPECT_EQ(libyuv::FOURCC_BGRA, fourcc);
  src_a[0] = 255;
  src_a[3] = 0;
  fourcc = ARGBDetect(src_a, benchmark_width_ * 4,
                      benchmark_width_, benchmark_height_);
  EXPECT_EQ(libyuv::FOURCC_ARGB, fourcc);
  src_a[3] = 255;

  for (int i = 0; i < benchmark_iterations_; ++i) {
    fourcc = ARGBDetect(src_a, benchmark_width_ * 4,
                        benchmark_width_, benchmark_height_);
  }
  EXPECT_EQ(0, fourcc);

  free_aligned_buffer_64(src_a);
}

TEST_F(libyuvTest, BenchmarkARGBDetect_Unaligned) {
  uint32 fourcc;
  const int kMaxTest = benchmark_width_ * benchmark_height_ * 4 + 1;
  align_buffer_64(src_a, kMaxTest);
  for (int i = 0; i < kMaxTest; ++i) {
    src_a[i + 1] = 255;
  }

  src_a[0 + 1] = 0;
  fourcc = ARGBDetect(src_a + 1, benchmark_width_ * 4,
                      benchmark_width_, benchmark_height_);
  EXPECT_EQ(libyuv::FOURCC_BGRA, fourcc);
  src_a[0 + 1] = 255;
  src_a[3 + 1] = 0;
  fourcc = ARGBDetect(src_a + 1, benchmark_width_ * 4,
                      benchmark_width_, benchmark_height_);
  EXPECT_EQ(libyuv::FOURCC_ARGB, fourcc);
  src_a[3 + 1] = 255;

  for (int i = 0; i < benchmark_iterations_; ++i) {
    fourcc = ARGBDetect(src_a + 1, benchmark_width_ * 4,
                        benchmark_width_, benchmark_height_);
  }
  EXPECT_EQ(0, fourcc);

  free_aligned_buffer_64(src_a);
}
TEST_F(libyuvTest, BenchmarkSumSquareError_Opt) {
  const int kMaxWidth = 4096 * 3;
  align_buffer_64(src_a, kMaxWidth);
  align_buffer_64(src_b, kMaxWidth);
  memset(src_a, 0, kMaxWidth);
  memset(src_b, 0, kMaxWidth);

  memcpy(src_a, "test0123test4567", 16);
  memcpy(src_b, "tick0123tock4567", 16);
  uint64 h1 = ComputeSumSquareError(src_a, src_b, 16);
  EXPECT_EQ(790u, h1);

  for (int i = 0; i < kMaxWidth; ++i) {
    src_a[i] = i;
    src_b[i] = i;
  }
  memset(src_a, 0, kMaxWidth);
  memset(src_b, 0, kMaxWidth);

  int count = benchmark_iterations_ *
    ((benchmark_width_ * benchmark_height_ + kMaxWidth - 1) / kMaxWidth);
  for (int i = 0; i < count; ++i) {
    h1 = ComputeSumSquareError(src_a, src_b, kMaxWidth);
  }

  EXPECT_EQ(0, h1);

  free_aligned_buffer_64(src_a);
  free_aligned_buffer_64(src_b);
}

TEST_F(libyuvTest, SumSquareError) {
  const int kMaxWidth = 4096 * 3;
  align_buffer_64(src_a, kMaxWidth);
  align_buffer_64(src_b, kMaxWidth);
  memset(src_a, 0, kMaxWidth);
  memset(src_b, 0, kMaxWidth);

  uint64 err;
  err = ComputeSumSquareError(src_a, src_b, kMaxWidth);

  EXPECT_EQ(0, err);

  memset(src_a, 1, kMaxWidth);
  err = ComputeSumSquareError(src_a, src_b, kMaxWidth);

  EXPECT_EQ(err, kMaxWidth);

  memset(src_a, 190, kMaxWidth);
  memset(src_b, 193, kMaxWidth);
  err = ComputeSumSquareError(src_a, src_b, kMaxWidth);

  EXPECT_EQ(kMaxWidth * 3 * 3, err);

  for (int i = 0; i < kMaxWidth; ++i) {
    src_a[i] = (fastrand() & 0xff);
    src_b[i] = (fastrand() & 0xff);
  }

  MaskCpuFlags(disable_cpu_flags_);
  uint64 c_err = ComputeSumSquareError(src_a, src_b, kMaxWidth);

  MaskCpuFlags(-1);
  uint64 opt_err = ComputeSumSquareError(src_a, src_b, kMaxWidth);

  EXPECT_EQ(c_err, opt_err);

  free_aligned_buffer_64(src_a);
  free_aligned_buffer_64(src_b);
}

TEST_F(libyuvTest, BenchmarkPsnr_Opt) {
  align_buffer_64(src_a, benchmark_width_ * benchmark_height_);
  align_buffer_64(src_b, benchmark_width_ * benchmark_height_);
  for (int i = 0; i < benchmark_width_ * benchmark_height_; ++i) {
    src_a[i] = i;
    src_b[i] = i;
  }

  MaskCpuFlags(-1);

  double opt_time = get_time();
  for (int i = 0; i < benchmark_iterations_; ++i)
    CalcFramePsnr(src_a, benchmark_width_,
                  src_b, benchmark_width_,
                  benchmark_width_, benchmark_height_);

  opt_time = (get_time() - opt_time) / benchmark_iterations_;
  printf("BenchmarkPsnr_Opt - %8.2f us opt\n", opt_time * 1e6);

  EXPECT_EQ(0, 0);

  free_aligned_buffer_64(src_a);
  free_aligned_buffer_64(src_b);
}

TEST_F(libyuvTest, BenchmarkPsnr_Unaligned) {
  align_buffer_64(src_a, benchmark_width_ * benchmark_height_ + 1);
  align_buffer_64(src_b, benchmark_width_ * benchmark_height_);
  for (int i = 0; i < benchmark_width_ * benchmark_height_; ++i) {
    src_a[i + 1] = i;
    src_b[i] = i;
  }

  MaskCpuFlags(-1);

  double opt_time = get_time();
  for (int i = 0; i < benchmark_iterations_; ++i)
    CalcFramePsnr(src_a + 1, benchmark_width_,
                  src_b, benchmark_width_,
                  benchmark_width_, benchmark_height_);

  opt_time = (get_time() - opt_time) / benchmark_iterations_;
  printf("BenchmarkPsnr_Opt - %8.2f us opt\n", opt_time * 1e6);

  EXPECT_EQ(0, 0);

  free_aligned_buffer_64(src_a);
  free_aligned_buffer_64(src_b);
}

TEST_F(libyuvTest, Psnr) {
  const int kSrcWidth = benchmark_width_;
  const int kSrcHeight = benchmark_height_;
  const int b = 128;
  const int kSrcPlaneSize = (kSrcWidth + b * 2) * (kSrcHeight + b * 2);
  const int kSrcStride = 2 * b + kSrcWidth;
  align_buffer_64(src_a, kSrcPlaneSize);
  align_buffer_64(src_b, kSrcPlaneSize);
  memset(src_a, 0, kSrcPlaneSize);
  memset(src_b, 0, kSrcPlaneSize);

  double err;
  err = CalcFramePsnr(src_a + kSrcStride * b + b, kSrcStride,
                      src_b + kSrcStride * b + b, kSrcStride,
                      kSrcWidth, kSrcHeight);

  EXPECT_EQ(err, kMaxPsnr);

  memset(src_a, 255, kSrcPlaneSize);

  err = CalcFramePsnr(src_a + kSrcStride * b + b, kSrcStride,
                      src_b + kSrcStride * b + b, kSrcStride,
                      kSrcWidth, kSrcHeight);

  EXPECT_EQ(err, 0.0);

  memset(src_a, 1, kSrcPlaneSize);

  err = CalcFramePsnr(src_a + kSrcStride * b + b, kSrcStride,
                      src_b + kSrcStride * b + b, kSrcStride,
                      kSrcWidth, kSrcHeight);

  EXPECT_GT(err, 48.0);
  EXPECT_LT(err, 49.0);

  for (int i = 0; i < kSrcPlaneSize; ++i) {
    src_a[i] = i;
  }

  err = CalcFramePsnr(src_a + kSrcStride * b + b, kSrcStride,
                      src_b + kSrcStride * b + b, kSrcStride,
                      kSrcWidth, kSrcHeight);

  EXPECT_GT(err, 2.0);
  if (kSrcWidth * kSrcHeight >= 256) {
    EXPECT_LT(err, 6.0);
  }

  memset(src_a, 0, kSrcPlaneSize);
  memset(src_b, 0, kSrcPlaneSize);

  for (int i = b; i < (kSrcHeight + b); ++i) {
    for (int j = b; j < (kSrcWidth + b); ++j) {
      src_a[(i * kSrcStride) + j] = (fastrand() & 0xff);
      src_b[(i * kSrcStride) + j] = (fastrand() & 0xff);
    }
  }

  MaskCpuFlags(disable_cpu_flags_);
  double c_err, opt_err;

  c_err = CalcFramePsnr(src_a + kSrcStride * b + b, kSrcStride,
                        src_b + kSrcStride * b + b, kSrcStride,
                        kSrcWidth, kSrcHeight);

  MaskCpuFlags(-1);

  opt_err = CalcFramePsnr(src_a + kSrcStride * b + b, kSrcStride,
                          src_b + kSrcStride * b + b, kSrcStride,
                          kSrcWidth, kSrcHeight);

  EXPECT_EQ(opt_err, c_err);

  free_aligned_buffer_64(src_a);
  free_aligned_buffer_64(src_b);
}

TEST_F(libyuvTest, DISABLED_BenchmarkSsim_Opt) {
  align_buffer_64(src_a, benchmark_width_ * benchmark_height_);
  align_buffer_64(src_b, benchmark_width_ * benchmark_height_);
  for (int i = 0; i < benchmark_width_ * benchmark_height_; ++i) {
    src_a[i] = i;
    src_b[i] = i;
  }

  MaskCpuFlags(-1);

  double opt_time = get_time();
  for (int i = 0; i < benchmark_iterations_; ++i)
    CalcFrameSsim(src_a, benchmark_width_,
                  src_b, benchmark_width_,
                  benchmark_width_, benchmark_height_);

  opt_time = (get_time() - opt_time) / benchmark_iterations_;
  printf("BenchmarkSsim_Opt - %8.2f us opt\n", opt_time * 1e6);

  EXPECT_EQ(0, 0);  // Pass if we get this far.

  free_aligned_buffer_64(src_a);
  free_aligned_buffer_64(src_b);
}

TEST_F(libyuvTest, Ssim) {
  const int kSrcWidth = benchmark_width_;
  const int kSrcHeight = benchmark_height_;
  const int b = 128;
  const int kSrcPlaneSize = (kSrcWidth + b * 2) * (kSrcHeight + b * 2);
  const int kSrcStride = 2 * b + kSrcWidth;
  align_buffer_64(src_a, kSrcPlaneSize);
  align_buffer_64(src_b, kSrcPlaneSize);
  memset(src_a, 0, kSrcPlaneSize);
  memset(src_b, 0, kSrcPlaneSize);

  if (kSrcWidth <=8 || kSrcHeight <= 8) {
    printf("warning - Ssim size too small.  Testing function executes.\n");
  }

  double err;
  err = CalcFrameSsim(src_a + kSrcStride * b + b, kSrcStride,
                      src_b + kSrcStride * b + b, kSrcStride,
                      kSrcWidth, kSrcHeight);

  if (kSrcWidth > 8 && kSrcHeight > 8) {
    EXPECT_EQ(err, 1.0);
  }

  memset(src_a, 255, kSrcPlaneSize);

  err = CalcFrameSsim(src_a + kSrcStride * b + b, kSrcStride,
                      src_b + kSrcStride * b + b, kSrcStride,
                      kSrcWidth, kSrcHeight);

  if (kSrcWidth > 8 && kSrcHeight > 8) {
    EXPECT_LT(err, 0.0001);
  }

  memset(src_a, 1, kSrcPlaneSize);

  err = CalcFrameSsim(src_a + kSrcStride * b + b, kSrcStride,
                      src_b + kSrcStride * b + b, kSrcStride,
                      kSrcWidth, kSrcHeight);

  if (kSrcWidth > 8 && kSrcHeight > 8) {
    EXPECT_GT(err, 0.0001);
    EXPECT_LT(err, 0.9);
  }

  for (int i = 0; i < kSrcPlaneSize; ++i) {
    src_a[i] = i;
  }

  err = CalcFrameSsim(src_a + kSrcStride * b + b, kSrcStride,
                      src_b + kSrcStride * b + b, kSrcStride,
                      kSrcWidth, kSrcHeight);

  if (kSrcWidth > 8 && kSrcHeight > 8) {
    EXPECT_GT(err, 0.0);
    EXPECT_LT(err, 0.01);
  }

  for (int i = b; i < (kSrcHeight + b); ++i) {
    for (int j = b; j < (kSrcWidth + b); ++j) {
      src_a[(i * kSrcStride) + j] = (fastrand() & 0xff);
      src_b[(i * kSrcStride) + j] = (fastrand() & 0xff);
    }
  }

  MaskCpuFlags(disable_cpu_flags_);
  double c_err, opt_err;

  c_err = CalcFrameSsim(src_a + kSrcStride * b + b, kSrcStride,
                        src_b + kSrcStride * b + b, kSrcStride,
                        kSrcWidth, kSrcHeight);

  MaskCpuFlags(-1);

  opt_err = CalcFrameSsim(src_a + kSrcStride * b + b, kSrcStride,
                          src_b + kSrcStride * b + b, kSrcStride,
                          kSrcWidth, kSrcHeight);

  if (kSrcWidth > 8 && kSrcHeight > 8) {
    EXPECT_EQ(opt_err, c_err);
  }

  free_aligned_buffer_64(src_a);
  free_aligned_buffer_64(src_b);
}

}  // namespace libyuv
