#ifndef CAFFE2_OPERATORS_TOP_K_H_
#define CAFFE2_OPERATORS_TOP_K_H_

#include "caffe2/core/logging.h"
#include "caffe2/core/operator.h"
#include "caffe2/utils/math.h"

namespace caffe2 {

namespace {

// Define these two names to allow lookup into the 2d tensors like
// mytensor(i, j)
template <typename T>
using EigenMatrixMapRowMajor = Eigen::Map<
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>;

template <typename T>
using ConstEigenMatrixMapRowMajor = Eigen::Map<
    const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>;

} // namespace

template <typename T, class Context>
class TopKOp : public Operator<Context> {
 public:
  USE_OPERATOR_CONTEXT_FUNCTIONS;

  TopKOp(const OperatorDef& operator_def, Workspace* ws)
      : Operator<Context>(operator_def, ws), OP_SINGLE_ARG(int, "k", k_, -1) {
    CAFFE_ENFORCE(k_ >= 1, "k argument must be >= 1");
  }

  bool RunOnDevice() override {
    auto& input = Input(0);
    auto* values = Output(0);
    auto* indices = Output(1);

    vector<TIndex> in_dims = input.dims();
    // Linearize input tensor except for last dimension
    // e.g. [3, 4, 5] -> [12, 5]
    // [5] -> [5]
    vector<TIndex> linear_shape = {size_to_dim_(in_dims.size() - 1, in_dims),
                                   in_dims[in_dims.size() - 1]};
    auto input_map = ConstEigenMatrixMapRowMajor<T>(
        static_cast<const T*>(input.raw_data()),
        linear_shape[0],
        linear_shape[1]);

    // Resize output tensors to be the same shape as the linearized input except
    // for the last dimension, which will be of size k. E.x. for an input tensor
    // of shape [3, 4, 5] and k=2, both of these will be shape [3, 4, 2]
    vector<TIndex> output_linear_shape = {linear_shape[0], k_};
    values->Resize(output_linear_shape);
    indices->Resize(output_linear_shape);

    // Use Eigen maps to allow indexing into the 2d tensors like values_map(i,j)
    auto values_map = EigenMatrixMapRowMajor<T>(
        values->template mutable_data<T>(), linear_shape[0], k_);
    auto indices_map = EigenMatrixMapRowMajor<TIndex>(
        indices->template mutable_data<TIndex>(), linear_shape[0], k_);

    // Sort preserving indices
    vector<TIndex> idxs(linear_shape[1]);
    for (TIndex i = 0; i < linear_shape[0]; ++i) {
      std::iota(idxs.begin(), idxs.end(), 0);
      std::partial_sort(
          idxs.begin(), idxs.begin() + k_, idxs.end(), [&](TIndex a, TIndex b) {
            if (input_map(i, a) > input_map(i, b)) {
              return true;
            } else if (input_map(i, a) == input_map(i, b)) {
              return a < b;
            } else {
              return false;
            }
          });
      for (TIndex j = 0; j < k_; ++j) {
        values_map(i, j) = input_map(i, idxs[j]);
        indices_map(i, j) = idxs[j];
      }
    }

    // Reshape output tensors to [a_1, a_2, ..., a_n, k]
    auto out_dims = in_dims;
    out_dims[out_dims.size() - 1] = k_;
    values->Reshape(out_dims);
    indices->Reshape(out_dims);
    return true;
  }

 private:
  int k_;
};

} // namespace caffe2

#endif // CAFFE2_OPERATORS_TOP_K_H_
