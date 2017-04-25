#ifndef CAFFE2_OPERATORS_TILE_OP_H_
#define CAFFE2_OPERATORS_TILE_OP_H_

#include "caffe2/core/common_omp.h"
#include "caffe2/core/context.h"
#include "caffe2/core/logging.h"
#include "caffe2/core/operator.h"
#include "caffe2/utils/math.h"

namespace caffe2 {

// Copy a Blob n times along a specified axis.
template <typename T, class Context>
class TileOp : public Operator<Context> {
 public:
  USE_OPERATOR_CONTEXT_FUNCTIONS;
  TileOp(const OperatorDef& operator_def, Workspace* ws)
      : Operator<Context>(operator_def, ws),
        tiles_(OperatorBase::GetSingleArgument<int32_t>("tiles", 1)),
        axis_(OperatorBase::GetSingleArgument<int32_t>("axis", 0)) {}
  ~TileOp() {}

  bool RunOnDevice() override {
    CAFFE_ENFORCE(
        OperatorBase::HasArgument("tiles"), "Argument `tiles` is missing.");
    CAFFE_ENFORCE(
        OperatorBase::HasArgument("axis"), "Argument `axis` is missing.");
    CAFFE_ENFORCE_GT(tiles_, 0, "`tiles` must be > 0");

    const auto& input = Input(0);
    auto* output = Output(0);
    const auto axis = input.canonical_axis_index(axis_);

    // reshape output to be input tiled along the axis
    vector<TIndex> output_dims(input.dims());
    output_dims[axis_] = output_dims[axis_] * tiles_;
    output->Resize(output_dims);

    // size up to (and not including) axis
    const auto outer_dim = input.size_to_dim(axis);
    // size from axis up
    const auto inner_dim = input.size_from_dim(axis);

    /**
     * How this works:
     * Imagine a 2D tensor (matrix) of size 3x10, tiled 2 times.
     * - Tiling along axis 0 (row) means copying the entire 3x10 Matrix 2
     * times. outer_dim = 0, inner_dim = 30.
     * - Tiling along axis 1 (column) means copying each row 2 times, then
     * proceed to the next row, until the end. outer_dim = 3, inner_dim = 10.
     */
    const char* input_data = static_cast<const char*>(input.raw_data());
    char* output_data =
        static_cast<char*>(output->raw_mutable_data(input.meta()));
    for (auto i = 0; i < outer_dim; ++i) {
      for (auto t = 0; t < tiles_; ++t) {
        context_.template CopyItems<Context, Context>(
            input.meta(), inner_dim, input_data, output_data);
        output_data += inner_dim * input.itemsize();
      }
      input_data += inner_dim * input.itemsize();
    }

    return true;
  }

 private:
  int32_t tiles_;
  int32_t axis_;
};

template <typename T, class Context>
class TileGradientOp : public Operator<Context> {
 public:
  USE_OPERATOR_CONTEXT_FUNCTIONS;
  TileGradientOp(const OperatorDef& operator_def, Workspace* ws)
      : Operator<Context>(operator_def, ws),
        tiles_(OperatorBase::GetSingleArgument<int32_t>("tiles", 1)),
        axis_(OperatorBase::GetSingleArgument<int32_t>("axis", 0)) {}
  ~TileGradientOp() {}

  bool RunOnDevice() override {
    CAFFE_ENFORCE(
        OperatorBase::HasArgument("tiles"), "Argument `tiles` is missing.");
    CAFFE_ENFORCE(
        OperatorBase::HasArgument("axis"), "Argument `axis` is missing.");
    CAFFE_ENFORCE_GT(tiles_, 0, "`tiles` must be > 0");

    const auto& input = Input(0);
    auto* output = Output(0);
    const auto axis = input.canonical_axis_index(axis_);

    // reshape output to be input "untiled" along the axis
    vector<TIndex> output_dims(input.dims());
    output_dims[axis_] = output_dims[axis_] / tiles_;
    output->Resize(output_dims);

    // size up to (and not including) axis
    const auto outer_dim = output->size_to_dim(axis);
    // size from axis up
    const auto inner_dim = output->size_from_dim(axis);

    /**
     * How this works:
     * Imagine a 2D tensor (matrix) of size 3x10, tiled 2 times along axis 1
     * (column).
     * This is equivalent to multiplying by a vector of 1s transposed.
     * The gradient of this is all 1s in the shape of the input matrix
     * (call it X).
     * So the the output gradient should be the matrix multipication result
     * of input gradient (gradient of tiled tensor output) and X.
     */
    const char* input_data = static_cast<const char*>(input.raw_data());
    char* output_data =
        static_cast<char*>(output->raw_mutable_data(input.meta()));

    for (auto i = 0; i < outer_dim; ++i) {
      context_.template CopyItems<Context, Context>(
          input.meta(), inner_dim, input_data, output_data);
      input_data += inner_dim * input.itemsize();
      for (auto t = 1; t < tiles_; ++t) {
        math::Axpy<T, Context>(
            inner_dim,
            T(1),
            reinterpret_cast<const T*>(input_data),
            reinterpret_cast<T*>(output_data),
            &context_);
        input_data += inner_dim * input.itemsize();
      }
      output_data += inner_dim * input.itemsize();
    }

    return true;
  }

 private:
  int32_t tiles_;
  int32_t axis_;
};

} // namespace caffe2

#endif // CAFFE2_OPERATORS_TILE_OP_H_
