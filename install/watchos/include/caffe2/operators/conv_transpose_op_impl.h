// conv_transpose_op_impl.h is the templated implementation of the
// conv_transpose_op.h file.
#ifndef CAFFE2_OPERATORS_CONV_TRANSPOSE_OP_IMPL_H_
#define CAFFE2_OPERATORS_CONV_TRANSPOSE_OP_IMPL_H_

#include "caffe2/core/context.h"
#include "caffe2/core/logging.h"
#include "caffe2/core/operator.h"
#include "caffe2/operators/conv_op_shared.h"
#include "caffe2/operators/conv_transpose_op.h"
#include "caffe2/operators/conv_transpose_unpool_op_base.h"
#include "caffe2/utils/math.h"

CAFFE2_DECLARE_bool(caffe2_force_shared_col_buffer);

namespace caffe2 {

template <typename T, class Context>
bool ConvTransposeOp<T, Context>::RunOnDeviceWithOrderNCHW() {
  const Tensor<Context>& X = Input(INPUT);
  auto& filter = Input(FILTER);
  auto& bias = Input(BIAS);
  Tensor<Context>* Y = Output(0);
  const int N = X.dim32(0), M = X.dim32(1), H = X.dim32(2), W = X.dim32(3);
  CAFFE_ENFORCE(filter.ndim() == 4, "filter must be 4D tensor");
  CAFFE_ENFORCE(
      filter.dim32(0) == M,
      "filter number must be equal to input channel number");
  const int C = filter.dim32(1);
  CAFFE_ENFORCE(
      filter.dim32(2) == kernel_h_,
      "filter height must be equal to kernel height");
  CAFFE_ENFORCE(
      filter.dim32(3) == kernel_w_,
      "filter width must be equal to kernel width");
  CAFFE_ENFORCE(bias.ndim() == 1, "bias must be 1D tensor");
  CAFFE_ENFORCE(
      bias.dim32(0) == C,
      "bias dimension must be equal to output channel number");
  ConvTransposeUnpoolBase<Context>::SetOutputSize(X, Y, C);

  const int kernel_dim = C * kernel_h_ * kernel_w_;
  const int input_image_size = H * W;
  const int output_image_size = Y->dim32(2) * Y->dim32(3);

#ifndef __ARM_NEON__
  if (bias_multiplier_.size() != output_image_size) {
    bias_multiplier_.Resize(vector<TIndex>(1, output_image_size));
    math::Set<T, Context>(
      output_image_size,
      static_cast<T>(1),
      bias_multiplier_.template mutable_data<T>(),
      &context_);
  }
#endif // !__ARM_NEON__

  const T* Xdata = X.template data<T>();
  T* Ydata = Y->template mutable_data<T>();

  auto f = [&](Tensor<Context>* col_buffer) {
    col_buffer->Resize(vector<TIndex>{C, kernel_h_, kernel_w_, H, W});
    T* col_buffer_data = col_buffer->template mutable_data<T>();
    for (auto image_id = 0; image_id < N; ++image_id) {
      // Weight term
      math::Gemm<T, Context>(
          CblasTrans,
          CblasNoTrans,
          kernel_dim,
          input_image_size,
          M,
          1,
          filter.template data<T>(),
          Xdata,
          0,
          col_buffer_data,
          &context_);

      // Col2im
      math::Col2im<T, Context, StorageOrder::NCHW>(
          col_buffer_data,
          C,
          Y->dim32(2),
          Y->dim32(3),
          kernel_h_,
          kernel_w_,
          1,
          1,
          pad_t_,
          pad_l_,
          pad_b_,
          pad_r_,
          stride_h_,
          stride_w_,
          Ydata,
          &context_);

      // Bias term
#ifndef __ARM_NEON__
      math::Gemm<T, Context>(
          CblasNoTrans,
          CblasNoTrans,
          C,
          output_image_size,
          1,
          1,
          bias.template data<T>(),
          bias_multiplier_.template data<T>(),
          1,
          Ydata,
          &context_);
#else
      math::BiasCHW<T, Context>(
        bias.template data<T>(),
        C,
        output_image_size,
        Ydata,
        &context_);
#endif // !__ARM_NEON__

      Xdata += M * H * W;
      Ydata += Y->size() / Y->dim32(0);
    }
  };
  if (FLAGS_caffe2_force_shared_col_buffer || shared_buffer_) {
    runWithSharedBuffer<Context>(ws_, f);
  } else {
    f(&col_buffer_);
  }
  return true;
}

template <typename T, class Context>
bool ConvTransposeOp<T, Context>::RunOnDeviceWithOrderNHWC() {
  const Tensor<Context>& X = Input(INPUT);
  auto& filter = Input(FILTER);
  auto& bias = Input(BIAS);
  Tensor<Context>* Y = Output(0);
  const auto N = X.dim32(0), H = X.dim32(1), W = X.dim32(2), M = X.dim32(3);
  CAFFE_ENFORCE(filter.ndim() == 4, "filter must be 4D tensor");
  CAFFE_ENFORCE(
      filter.dim32(0) == M,
      "filter number must be equal to input channel number");
  CAFFE_ENFORCE(
      filter.dim32(1) == kernel_h_,
      "filter height must be equal to kernel height");
  CAFFE_ENFORCE(
      filter.dim32(2) == kernel_w_,
      "filter width must be equal to kernel width");
  const int C = filter.dim32(3);
  CAFFE_ENFORCE(bias.ndim() == 1, "bias must be 1D tensor");
  CAFFE_ENFORCE(
      bias.dim32(0) == C,
      "bias dimension must be equal to output channel number");
  ConvTransposeUnpoolBase<Context>::SetOutputSize(X, Y, C);

  const auto kernel_dim = C * kernel_h_ * kernel_w_;
  const auto input_image_size = H * W;
  const auto output_image_size = Y->dim32(1) * Y->dim32(2);

  if (bias_multiplier_.size() != output_image_size) {
    bias_multiplier_.Resize(vector<TIndex>(1, output_image_size));
    math::Set<T, Context>(
        output_image_size,
        static_cast<T>(1),
        bias_multiplier_.template mutable_data<T>(),
        &context_);
  }
  const T* Xdata = X.template data<T>();
  T* Ydata = Y->template mutable_data<T>();

  auto f = [&](Tensor<Context>* col_buffer) {
    col_buffer_.Resize(vector<TIndex>{H, W, kernel_h_, kernel_w_, C});
    T* col_buffer_data = col_buffer_.template mutable_data<T>();
    for (auto image_id = 0; image_id < N; ++image_id) {
      // Weight term
      math::Gemm<T, Context>(
          CblasNoTrans,
          CblasNoTrans,
          input_image_size,
          kernel_dim,
          M,
          1,
          Xdata,
          filter.template data<T>(),
          0,
          col_buffer_data,
          &context_);
      // Col2im
      math::Col2im<T, Context, StorageOrder::NHWC>(
          col_buffer_data,
          C,
          Y->dim32(1),
          Y->dim32(2),
          kernel_h_,
          kernel_w_,
          1,
          1,
          pad_t_,
          pad_l_,
          pad_b_,
          pad_r_,
          stride_h_,
          stride_w_,
          Ydata,
          &context_);
      // Bias term
      math::Gemm<T, Context>(
          CblasNoTrans,
          CblasNoTrans,
          output_image_size,
          C,
          1,
          1,
          bias_multiplier_.template data<T>(),
          bias.template data<T>(),
          1,
          Ydata,
          &context_);
      Xdata += M * H * W;
      Ydata += Y->size() / Y->dim32(0);
    }
  };
  if (FLAGS_caffe2_force_shared_col_buffer || shared_buffer_) {
    runWithSharedBuffer<Context>(ws_, f);
  } else {
    f(&col_buffer_);
  }
  return true;
}

template <typename T, class Context>
bool ConvTransposeGradientOp<T, Context>::RunOnDeviceWithOrderNCHW() {
  auto& X = Input(INPUT);
  auto& filter = Input(FILTER);
  auto& dY = Input(OUTPUT_GRAD);
  auto* dfilter = Output(FILTER_GRAD);
  auto* dbias = Output(BIAS_GRAD);
  const int N = X.dim32(0), M = X.dim32(1), H = X.dim32(2), W = X.dim32(3);
  // We only handle LegacyPadding::NOTSET case and ignore cases of
  // LegacyPadding::VALID and LegacyPadding::SAME
  // Thus, we don't need to manually compute padding values
  // We simply use the values from the user
  CAFFE_ENFORCE(filter.ndim() == 4);
  const int C = filter.dim32(1);
  CAFFE_ENFORCE(
      filter.dim32(2) == kernel_h_,
      "filter height must be equal to kernel height");
  CAFFE_ENFORCE(
      filter.dim32(3) == kernel_w_,
      "filter width must be equal to kernel width");
  dfilter->ResizeLike(filter);
  dbias->Resize(C);

  const int kernel_dim = C * kernel_h_ * kernel_w_;
  const int output_image_size = dY.dim32(2) * dY.dim32(3);
  // The col buffer is stored in CHW order as well
  col_buffer_.Resize(vector<TIndex>{C, kernel_h_, kernel_w_, H, W});
  if (bias_multiplier_.size() != output_image_size) {
    bias_multiplier_.Resize(1, output_image_size);
    math::Set<T, Context>(
        output_image_size,
        static_cast<T>(1),
        bias_multiplier_.template mutable_data<T>(),
        &context_);
  }
  T* col_buffer_data = col_buffer_.template mutable_data<T>();
  const T* Xdata = X.template data<T>();
  const T* dYdata = dY.template data<T>();
  T* dfilter_data = dfilter->template mutable_data<T>();
  T* dbias_data = dbias->template mutable_data<T>();
  // Pre-setting the gradients to zero
  math::Set<T, Context>(dfilter->size(), 0, dfilter_data, &context_);
  math::Set<T, Context>(dbias->size(), 0, dbias_data, &context_);
  for (auto image_id = 0; image_id < N; ++image_id) {
    // gradient w.r.t. filters. Im2col followed by Gemm
    // Im2col.
    math::Im2col<T, Context, StorageOrder::NCHW>(
        dYdata,
        C,
        dY.dim32(2),
        dY.dim32(3),
        kernel_h_,
        kernel_w_,
        1,
        1,
        pad_t_,
        pad_l_,
        pad_b_,
        pad_r_,
        stride_h_,
        stride_w_,
        col_buffer_data,
        &context_);
    // Gemm
    math::Gemm<T, Context>(
        CblasNoTrans,
        CblasTrans,
        M,
        kernel_dim,
        H * W,
        1,
        Xdata,
        col_buffer_data,
        1,
        dfilter_data,
        &context_);
    // gradient w.r.t. bias
    math::Gemm<T, Context>(
        CblasNoTrans,
        CblasNoTrans,
        C,
        1,
        output_image_size,
        1,
        dYdata,
        bias_multiplier_.template data<T>(),
        1,
        dbias_data,
        &context_);
    dYdata += dY.size() / dY.dim32(0);
    Xdata += X.size() / X.dim32(0);
  }
  if (OutputSize() == 3) {
    // Compute gradients w.r.t. the input
    dYdata = dY.template data<T>();
    auto* dX = Output(INPUT_GRAD);
    dX->ResizeLike(X);
    T* dXdata = dX->template mutable_data<T>();

    for (auto image_id = 0; image_id < N; ++image_id) {
      // Im2col.
      // TODO(zyan3): Probably duplicate work as in gradient computation
      // w.r.t filters
      math::Im2col<T, Context, StorageOrder::NCHW>(
          dYdata,
          C,
          dY.dim32(2),
          dY.dim32(3),
          kernel_h_,
          kernel_w_,
          1,
          1,
          pad_t_,
          pad_l_,
          pad_b_,
          pad_r_,
          stride_h_,
          stride_w_,
          col_buffer_data,
          &context_);
      // Gemm
      math::Gemm<T, Context>(
          CblasNoTrans,
          CblasNoTrans,
          M,
          H * W,
          kernel_dim,
          1,
          filter.template data<T>(),
          col_buffer_data,
          0,
          dXdata,
          &context_);
      dYdata += dY.size() / dY.dim32(0);
      dXdata += X.size() / X.dim32(0);
    }
  }
  return true;
}

template <typename T, class Context>
bool ConvTransposeGradientOp<T, Context>::RunOnDeviceWithOrderNHWC() {
  auto& X = Input(INPUT);
  auto& filter = Input(FILTER);
  auto& dY = Input(OUTPUT_GRAD);
  auto* dfilter = Output(FILTER_GRAD);
  auto* dbias = Output(BIAS_GRAD);
  const int N = X.dim32(0), H = X.dim32(1), W = X.dim32(2), M = X.dim32(3);
  // We only handle LegacyPadding::NOTSET case and ignore cases of
  // LegacyPadding::VALID and LegacyPadding::SAME
  // Thus, we don't need to manually compute padding values
  // We simply use the values from the user
  CAFFE_ENFORCE(filter.ndim() == 4, "filter must be 4D tensor");
  CAFFE_ENFORCE(
      filter.dim32(1) == kernel_h_,
      "filter height must be equal to kernel height");
  CAFFE_ENFORCE(
      filter.dim32(2) == kernel_w_,
      "filter width must be equal to kernel width");
  const int C = filter.dim32(3);
  dfilter->ResizeLike(filter);
  dbias->Resize(C);

  const int kernel_dim = C * kernel_h_ * kernel_w_;
  const int output_image_size = dY.dim32(1) * dY.dim32(2);
  // The col buffer is stored in HWC order as well
  col_buffer_.Resize(vector<TIndex>{H, W, kernel_h_, kernel_w_, C});
  if (bias_multiplier_.size() != output_image_size) {
    bias_multiplier_.Resize(1, output_image_size);
    math::Set<T, Context>(
        output_image_size,
        static_cast<T>(1),
        bias_multiplier_.template mutable_data<T>(),
        &context_);
  }
  T* col_buffer_data = col_buffer_.template mutable_data<T>();
  const T* Xdata = X.template data<T>();
  const T* dYdata = dY.template data<T>();
  T* dfilter_data = dfilter->template mutable_data<T>();
  T* dbias_data = dbias->template mutable_data<T>();
  // Pre-setting the gradients to zero
  math::Set<T, Context>(dfilter->size(), 0, dfilter_data, &context_);
  math::Set<T, Context>(dbias->size(), 0, dbias_data, &context_);
  for (auto image_id = 0; image_id < N; ++image_id) {
    // gradient w.r.t. filters. Im2col followed by Gemm
    // Im2col.
    math::Im2col<T, Context, StorageOrder::NHWC>(
        dYdata,
        C,
        dY.dim32(1),
        dY.dim32(2),
        kernel_h_,
        kernel_w_,
        1,
        1,
        pad_t_,
        pad_l_,
        pad_b_,
        pad_r_,
        stride_h_,
        stride_w_,
        col_buffer_data,
        &context_);
    // Gemm
    math::Gemm<T, Context>(
        CblasTrans,
        CblasNoTrans,
        M,
        kernel_dim,
        H * W,
        1,
        Xdata,
        col_buffer_data,
        1,
        dfilter_data,
        &context_);
    // gradients w.r.t. bias
    math::Gemm<T, Context>(
        CblasTrans,
        CblasNoTrans,
        C,
        1,
        output_image_size,
        1,
        dYdata,
        bias_multiplier_.template data<T>(),
        1,
        dbias_data,
        &context_);
    dYdata += dY.size() / dY.dim32(0);
    Xdata += X.size() / X.dim32(0);
  }
  if (OutputSize() == 3) {
    // Compute gradients w.r.t. the input
    dYdata = dY.template data<T>();
    auto* dX = Output(INPUT_GRAD);
    dX->ResizeLike(X);
    T* dXdata = dX->template mutable_data<T>();

    for (auto image_id = 0; image_id < N; ++image_id) {
      // Im2col.
      // TODO(zyan3): Probably duplicate work as in gradient computation
      // w.r.t filters
      math::Im2col<T, Context, StorageOrder::NHWC>(
          dYdata,
          C,
          dY.dim32(1),
          dY.dim32(2),
          kernel_h_,
          kernel_w_,
          1,
          1,
          pad_t_,
          pad_l_,
          pad_b_,
          pad_r_,
          stride_h_,
          stride_w_,
          col_buffer_data,
          &context_);
      // Gemm
      math::Gemm<T, Context>(
          CblasNoTrans,
          CblasTrans,
          H * W,
          M,
          kernel_dim,
          1,
          col_buffer_data,
          filter.template data<T>(),
          0,
          dXdata,
          &context_);
      dYdata += dY.size() / dY.dim32(0);
      dXdata += X.size() / X.dim32(0);
    }
  }
  return true;
}

} // namespace caffe2
#endif // CAFFE2_OPERATORS_CONV_TRANSPOSE_OP_IMPL_H_
