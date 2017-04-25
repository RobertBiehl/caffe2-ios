#ifndef CAFFE2_OPERATORS_CONV_TRANSPOSE_OP_H_
#define CAFFE2_OPERATORS_CONV_TRANSPOSE_OP_H_

#include "caffe2/core/context.h"
#include "caffe2/core/operator.h"
#include "caffe2/operators/conv_transpose_unpool_op_base.h"

namespace caffe2 {

template <typename T, class Context>
class ConvTransposeOp final : public ConvTransposeUnpoolBase<Context> {
 public:
  USE_CONV_TRANSPOSE_UNPOOL_BASE_FUNCTIONS(Context);
  ConvTransposeOp(const OperatorDef& operator_def, Workspace* ws)
      : ConvTransposeUnpoolBase<Context>(operator_def, ws) {}

  bool RunOnDeviceWithOrderNCHW() override;
  bool RunOnDeviceWithOrderNHWC() override;

 private:
  Tensor<Context> col_buffer_;
  Tensor<Context> bias_multiplier_;
  // Input: X, W, b
  // Output: Y
  INPUT_TAGS(INPUT, FILTER, BIAS);
};

template <typename T, class Context>
class ConvTransposeGradientOp final : public ConvTransposeUnpoolBase<Context> {
 public:
  USE_CONV_TRANSPOSE_UNPOOL_BASE_FUNCTIONS(Context);
  ConvTransposeGradientOp(const OperatorDef& operator_def, Workspace* ws)
      : ConvTransposeUnpoolBase<Context>(operator_def, ws) {}

  bool RunOnDeviceWithOrderNCHW() override;
  bool RunOnDeviceWithOrderNHWC() override;

 private:
  Tensor<Context> col_buffer_;
  Tensor<Context> bias_multiplier_;
  // input: X, W, dY
  // output: dW, db, and optionally dX
  INPUT_TAGS(INPUT, FILTER, OUTPUT_GRAD);
  OUTPUT_TAGS(FILTER_GRAD, BIAS_GRAD, INPUT_GRAD);
};

} // namespace caffe2

#endif // CAFFE2_OPERATORS_CONV_TRANSPOSE_OP_H_
