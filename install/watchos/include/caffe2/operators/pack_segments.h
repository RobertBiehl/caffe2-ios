#ifndef CAFFE2_OPERATORS_PACK_SEGMENTS_H_
#define CAFFE2_OPERATORS_PACK_SEGMENTS_H_

#include <atomic>
#include <limits>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "caffe2/core/operator.h"
#include "caffe2/core/tensor.h"
#include "caffe2/utils/math.h"

namespace caffe2 {

template <class Context>
class PackSegmentsOp final : public Operator<Context> {
 public:
  USE_OPERATOR_CONTEXT_FUNCTIONS;
  // USE_SIMPLE_CTOR_DTOR(PackSegmentsOp)
  USE_DISPATCH_HELPER;

  PackSegmentsOp(const OperatorDef& operator_def, Workspace* ws)
      : Operator<Context>(operator_def, ws),
        pad_minf_(OperatorBase::GetSingleArgument<bool>("pad_minf", false)) {
    if (pad_minf_) {
      padding_ = -1.0 * std::numeric_limits<float>::infinity();
    } else {
      padding_ = 0;
    }
  }

  bool RunOnDevice() override {
    return DispatchHelper<TensorTypes<int, long>>::call(this, Input(LENGTHS));
  }

  template <typename T>
  bool DoRunWithType() {
    const auto& data = Input(DATA);
    const auto& lengths = Input(LENGTHS);
    auto* output = Output(0);

    CAFFE_ENFORCE(data.ndim() >= 1, "DATA should be at least 1-D");
    CAFFE_ENFORCE(lengths.ndim() == 1, "LENGTH should be 1-D");

    // Find the length of the longest sequence.
    const T* l = lengths.template data<T>();
    T max_length = 0;
    for (T i = 0; i < lengths.dim(0); ++i) {
      max_length = std::max(max_length, l[i]);
    }

    auto shape = data.dims(); // Shape of output is batch_size x max_len x ...
    shape[0] = max_length;
    shape.insert(shape.begin(), lengths.size());
    output->Resize(shape);
    // create output tensor
    auto* out = static_cast<char*>(output->raw_mutable_data(data.meta()));

    if (!data.dim(0)) {
      // Return empty output (with the proper shape)
      return true;
    }

    // Do padding
    if (output->template IsType<float>()) {
      math::Set<float, Context>(
          output->size(),
          padding_,
          output->template mutable_data<float>(),
          &context_);
    }

    int block_size = data.size() / data.dim(0);
    int block_bytesize = data.nbytes() / data.dim(0);
    const auto* d = static_cast<const char*>(data.raw_data());
    int start = 0;
    for (int i = 0; i < lengths.dim(0); ++i) {
      context_.template CopyItems<Context, Context>(
          data.meta(),
          l[i] * block_size,
          d + block_bytesize * start,
          out + block_bytesize * max_length * i);
      start += l[i];
    }

    return true;
  }

  INPUT_TAGS(LENGTHS, DATA);

 private:
  bool pad_minf_;
  float padding_;
};

template <class Context>
class UnpackSegmentsOp final : public Operator<Context> {
 public:
  USE_OPERATOR_CONTEXT_FUNCTIONS;
  USE_SIMPLE_CTOR_DTOR(UnpackSegmentsOp)
  USE_DISPATCH_HELPER;

  bool RunOnDevice() override {
    return DispatchHelper<TensorTypes<int, long>>::call(this, Input(LENGTHS));
  }

  template <typename T>
  bool DoRunWithType() {
    const auto& data = Input(DATA);
    const auto& lengths = Input(LENGTHS);
    auto* output = Output(0);

    CAFFE_ENFORCE(data.ndim() >= 2, "DATA should be at least 2-D");
    CAFFE_ENFORCE(lengths.ndim() == 1, "LENGTH should be 1-D");

    const T* l = lengths.template data<T>();

    T max_length = 0;
    for (T i = 0; i < lengths.dim(0); ++i) {
      max_length = std::max(max_length, l[i]);
    }
    T total_l = std::accumulate(l, l + lengths.dim(0), 0);

    auto shape = data.dims();
    CAFFE_ENFORCE(
        shape[0] == lengths.dim(0), "LENGTH should match DATA in dimension 0");
    shape.erase(shape.begin());
    shape[0] = total_l;
    output->Resize(shape);
    // create output tensor
    auto* out = static_cast<char*>(output->raw_mutable_data(data.meta()));
    if (!(data.dim(0) * data.dim(1))) {
      return true;
    }
    int block_size = data.size() / (data.dim(0) * data.dim(1));
    int block_bytesize = data.nbytes() / (data.dim(0) * data.dim(1));
    const auto* d = static_cast<const char*>(data.raw_data());
    int start = 0;
    for (int i = 0; i < lengths.dim(0); ++i) {
      context_.template CopyItems<Context, Context>(
          data.meta(),
          l[i] * block_size,
          d + block_bytesize * data.dim(1) * i,
          out + block_bytesize * start);
      start += l[i];
    }
    return true;
  }

  INPUT_TAGS(LENGTHS, DATA);
};

} // namspace caffe2
#endif // CAFFE2_OPERATORS_PACK_SEGMENTS_H_
