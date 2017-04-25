#pragma once

#include <algorithm>

#include "caffe2/core/operator.h"

#include "gloo/algorithm.h"
#include "gloo/context.h"

namespace caffe2 {
namespace gloo {

template <class Context>
class BroadcastOp final : public Operator<Context> {
 public:
  USE_OPERATOR_CONTEXT_FUNCTIONS;

  BroadcastOp(const OperatorDef& operator_def, Workspace* ws)
      : Operator<Context>(operator_def, ws),
        root_(OperatorBase::template GetSingleArgument<int>("root", 0)) {}

  virtual ~BroadcastOp() {}

  bool RunOnDevice() override {
    std::call_once(once_, [&] { initialize(); });

    // If any parameter has changed in between runs, the initialized
    // algorithm is invalid and cannot be used.
    update(current_);
    CAFFE_ENFORCE(current_ == init_, "Inputs/outputs have changed");

    algorithm_->run();
    return true;
  }

 protected:
  void initialize() {
    // Store which inputs/outputs this instance initialized with
    update(init_);

    // Verify inputs == ouputs
    CAFFE_ENFORCE_EQ(init_.inputs.size(), init_.outputs.size());
    for (auto i = 0; i < init_.inputs.size(); i++) {
      CAFFE_ENFORCE_EQ(init_.inputs[i], init_.outputs[i]);
    }

    // Verify tensors all have same size
    size_t size = Input(1).size();
    for (auto i = 2; i < InputSize(); i++) {
      CAFFE_ENFORCE_EQ(Input(i).size(), size);
    }

    // Verify tensors all have same size
    TypeMeta meta = Input(1).meta();
    for (auto i = 2; i < InputSize(); i++) {
      CAFFE_ENFORCE(Input(i).meta() == meta);
    }

    // Finally initialize the algorithm
    initializeAlgorithm();
  }

  void initializeAlgorithm();

  const int root_;
  std::once_flag once_;
  std::unique_ptr<::gloo::Algorithm> algorithm_;

  // Captures the parameters passed to Gloo when first initialized.
  // An instance is updated every time this op runs and is compared
  // to the reference instance for equality. If any parameter has
  // changed from run to run, the initialized algorithm is invalid.
  struct GlooParameters {
    std::shared_ptr<::gloo::Context> context;
    std::vector<const void*> inputs;
    std::vector<void*> outputs;
    size_t size;
    TypeMeta meta;

    template <typename T>
    std::vector<const T*> getInputs() {
      std::vector<const T*> result;
      result.reserve(inputs.size());
      for (auto& input : inputs) {
        result.push_back(reinterpret_cast<T*>(input));
      }
      return result;
    }

    template <typename T>
    std::vector<T*> getOutputs() {
      std::vector<T*> result;
      result.reserve(outputs.size());
      for (auto& output : outputs) {
        result.push_back(reinterpret_cast<T*>(output));
      }
      return result;
    }

    template <typename T>
    bool IsType() const {
      return meta.Match<T>();
    }

    bool operator==(GlooParameters const& other) const {
      return context == other.context && inputs == other.inputs &&
          outputs == other.outputs && size == other.size && meta == other.meta;
    }
  };

  void update(GlooParameters& params) {
    params.context = OperatorBase::Input<std::shared_ptr<::gloo::Context>>(0);
    params.inputs.resize(InputSize() - 1);
    params.outputs.resize(OutputSize());
    for (auto i = 0; i < params.inputs.size(); i++) {
      params.inputs[i] = Input(i + 1).template raw_data();
      params.outputs[i] = Output(i)->template raw_mutable_data();
    }
    params.size = Output(0)->size();
    params.meta = Output(0)->meta();
  }

  GlooParameters init_;
  GlooParameters current_;
};

} // namespace gloo
} // namespace caffe2
