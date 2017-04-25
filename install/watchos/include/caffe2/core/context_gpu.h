#ifndef CAFFE2_CORE_CONTEXT_GPU_H_
#define CAFFE2_CORE_CONTEXT_GPU_H_

#include <ctime>
#include <mutex>

#include "caffe2/core/common_gpu.h"
#include "caffe2/core/context.h"
#include "caffe2/core/tensor.h"
#include "caffe2/core/types.h"
#include "caffe2/proto/caffe2.pb.h"
#include "caffe2/core/logging.h"

namespace caffe2 {

enum class CudaMemoryPoolType {
  NONE = 0,
  CNMEM = 1,
  CUB = 2,
};

/**
 * Gets the current memory pool type used by Caffe2.
 *
 * The memory pool is set up during caffe2's global initialization time.
 */
CudaMemoryPoolType GetCudaMemoryPoolType();


/**
 * A struct to host thread-local cuda objects.
 *
 * In Caffe2, each thread has its own non-default cuda stream as well as
 * related objects such as cublas and curand handles. This is achieved by
 * having the ThreadLocalCUDAObjects wrapper that takes care of allocating
 * and deallocating these objects at the thread scope. This class is solely
 * used inside CUDAContext and should not be used externally.
 */
class ThreadLocalCUDAObjects {
  friend class CUDAContext;
 private:
  ThreadLocalCUDAObjects() {
    for (int i = 0; i < CAFFE2_COMPILE_TIME_MAX_GPUS; ++i) {
      cuda_streams_[i] = vector<cudaStream_t>();
      cublas_handles_[i] = vector<cublasHandle_t>();
    }
  }

  cudaStream_t GetStream(int gpu, int stream_id) {
    vector<cudaStream_t> &gpu_streams = cuda_streams_[gpu];
    if (gpu_streams.size() <= stream_id) {
      gpu_streams.resize(stream_id + 1, nullptr);
    }
    if (!gpu_streams[stream_id]) {
      DeviceGuard guard(gpu);
      CUDA_ENFORCE(cudaStreamCreateWithFlags(
          &gpu_streams[stream_id], cudaStreamNonBlocking));
    }
    return gpu_streams[stream_id];
  }

  cublasHandle_t GetHandle(int gpu, int stream_id) {
    DeviceGuard guard(gpu);
    vector<cublasHandle_t> &gpu_handles = cublas_handles_[gpu];
    if (gpu_handles.size() <= stream_id) {
      gpu_handles.resize(stream_id + 1, nullptr);
    }
    if (!gpu_handles[stream_id]) {
      CUBLAS_ENFORCE(cublasCreate(&gpu_handles[stream_id]));
      // The default is CUBLAS_POINTER_MODE_HOST. You can override
      // it after obtaining the cublas handle, but do that with
      // caution.
      CUBLAS_ENFORCE(cublasSetPointerMode(
          gpu_handles[stream_id], CUBLAS_POINTER_MODE_HOST));
      CUBLAS_ENFORCE(
          cublasSetStream(gpu_handles[stream_id], GetStream(gpu, stream_id)));
    }
    return gpu_handles[stream_id];
  }

  ~ThreadLocalCUDAObjects() noexcept {
    for (int i = 0; i < CAFFE2_COMPILE_TIME_MAX_GPUS; ++i) {
      for (auto& handle : cublas_handles_[i]) {
        if (handle) {
          CUBLAS_CHECK(cublasDestroy(handle));
        }
      }
      for (auto& stream : cuda_streams_[i]) {
        if (stream) {
          CUDA_CHECK(cudaStreamDestroy(stream));
        }
      }
    }
  }
  vector<cudaStream_t> cuda_streams_[CAFFE2_COMPILE_TIME_MAX_GPUS];
  vector<cublasHandle_t> cublas_handles_[CAFFE2_COMPILE_TIME_MAX_GPUS];
};

class CUDAContext final {
 public:
  // The default cuda context constructor.
  explicit CUDAContext(const int gpu_id = -1);
  explicit CUDAContext(const DeviceOption& option);

  ~CUDAContext() {
    if (curand_generator_) {
      CURAND_ENFORCE(curandDestroyGenerator(curand_generator_));
    }
    CAFFE_ENFORCE(FinishDeviceComputation());
  }

  inline void SwitchToDevice(int stream_id) {
    set_stream_id(stream_id);
    CUDA_ENFORCE(cudaSetDevice(gpu_id_));
  }
  inline void SwitchToDevice() {
    SwitchToDevice(0);
  }

  bool FinishDeviceComputation() {
    cudaStreamSynchronize(cuda_objects_.GetStream(gpu_id_, stream_id_));
    cudaError_t error = cudaGetLastError();
    if (error == cudaSuccess) {
      return true;
    } else {
      LOG(ERROR) << "Encountered CUDA error: "
                      << cudaGetErrorString(error);
      return false;
    }
  }

  inline int cuda_gpu_id() const { return gpu_id_; }

  inline cudaStream_t cuda_stream() {
    return cuda_stream(gpu_id_, stream_id_);
  }

  inline cudaStream_t cuda_stream() const {
    return cuda_stream(gpu_id_, stream_id_);
  }

  static cudaStream_t cuda_stream(int gpu_id, int stream_id) {
    return cuda_objects_.GetStream(gpu_id, stream_id);
  }

  cublasHandle_t cublas_handle() {
    return cuda_objects_.GetHandle(gpu_id_, stream_id_);
  }

  curandGenerator_t& curand_generator() {
    if (!curand_generator_) {
      DeviceGuard guard(gpu_id_);
      CURAND_ENFORCE(
          curandCreateGenerator(&curand_generator_, CURAND_RNG_PSEUDO_DEFAULT));
      CURAND_ENFORCE(
          curandSetPseudoRandomGeneratorSeed(curand_generator_, random_seed_));
      CHECK_NOTNULL(curand_generator_);
    }
    CURAND_ENFORCE(curandSetStream(curand_generator_, cuda_stream()));
    return curand_generator_;
  }

  static void* New(size_t nbytes);

  static void Delete(void* data);


  // Get a mutex to lock out cudaMalloc / cudaFree calls when
  // NCCL kernels are being launched. Should remove threat of
  // deadlocks
  static std::mutex& mutex();

  // Functions to query memory stats. Only available if flag
  // --caffe2_gpu_memory_tracking is enabled.
  static std::vector<long> TotalMemoryByGpu();
  static std::vector<long> MaxMemoryByGpu();

  template <class SrcContext, class DstContext>
  inline void CopyBytes(size_t nbytes, const void* src, void* dst) {
    CUDA_ENFORCE(cudaMemcpyAsync(
        dst,
        src,
        nbytes,
        cudaMemcpyDefault,
        cuda_objects_.GetStream(gpu_id_, stream_id_)));
  }

  template <typename T, class SrcContext, class DstContext>
  inline void Copy(int n, const T* src, T* dst) {
    CopyBytes<SrcContext, DstContext>(n * sizeof(T),
                                 static_cast<const void*>(src),
                                 static_cast<void*>(dst));
  }

  template <class SrcContext, class DstContext>
  inline void
  CopyItems(const TypeMeta& meta, size_t n, const void* src, void* dst) {
    CAFFE_ENFORCE(!meta.copy(), "CUDAContext requires fundamental types.");
    CopyBytes<SrcContext, DstContext>(n * meta.itemsize(), src, dst);
  }

  void set_stream_id(int stream_id) {
    stream_id_ = stream_id;
  }

 protected:
  int gpu_id_;
  int stream_id_ = 0;
  int random_seed_;
  curandGenerator_t curand_generator_{nullptr};
  static thread_local ThreadLocalCUDAObjects cuda_objects_;
};

// For the CPU context, we also allow a (probably expensive) function
// to copy the data from a cuda context. Inside the function, we create
// a temporary CUDAContext object to carry out the copy. From the caller's
// side, these functions are synchronous with respect to the host, similar
// to a normal CPUContext::CopyBytes<CPUContext, CPUContext> call.
template<>
inline void CPUContext::CopyBytes<CUDAContext, CPUContext>(
    size_t nbytes, const void* src, void* dst) {
  CUDAContext context(GetGPUIDForPointer(src));
  context.CopyBytes<CUDAContext, CPUContext>(nbytes, src, dst);
}
template<>
inline void CPUContext::CopyBytes<CPUContext, CUDAContext>(
    size_t nbytes, const void* src, void* dst) {
  CUDAContext context(GetGPUIDForPointer(dst));
  context.CopyBytes<CPUContext, CUDAContext>(nbytes, src, dst);
}

/**
 * An allocator that does the CPU memory allocation with pinned memory.
 *
 * This is needed because if we want to do any asynchronous cuda memcpy,
 * the underlying CPU memory also needs to be allocated into pinned memory
 * space. As a result, whenever Caffe2 is built with GPU and there is
 * GPU present during runtime, at global initialization time we will set
 * the CPU memory allocator to allocate pinned memory.
 */
struct PinnedCPUAllocator final : CPUAllocator {
  PinnedCPUAllocator() {}
  ~PinnedCPUAllocator() {}
  void* New(size_t nbytes) override {
    void* data;
    std::lock_guard<std::mutex> lock(CUDAContext::mutex());
    CUDA_ENFORCE(cudaMallocHost(&data, nbytes));
    memset(data, 0, nbytes);
    return data;
  }
  void Delete(void* data) override {
    // Caffe2 uses a lazy way to figure out if one is actually going to use GPUs
    // or not. If a CUDAContext::New() call is made, inside the CUDAContext
    // function we will switch the cpu side allocator to a PinnedCPUAllocator.
    // But, if one calls CPUContext::New() before any cuda allocations,
    // PinnedCPUAllocator can still delete the corresponding memory.
    std::lock_guard<std::mutex> lock(CUDAContext::mutex());
    cudaError_t err = cudaFreeHost(data);
    if (err == cudaErrorInvalidValue) {
      free(data);
      // Calling cudaGetLastError will reset the cuda error.
      cudaGetLastError();
    } else {
      // For all other errors, still do a cuda check.
      CUDA_ENFORCE(err);
    }
  }
};

// For simplicity, we will typedef Tensor<CPUContext> to TensorCPU.
typedef Tensor<CUDAContext> TensorCUDA;

}  // namespace caffe2

#endif  // CAFFE2_CORE_CONTEXT_GPU_H_
