#!/bin/bash

SCRIPTS_DIR="$( cd "$(dirname "$0")"; pwd -P)"
CAFFE2_ROOT="$( cd "$(dirname "$0")"/../lib/caffe2 ; pwd -P)"

INSTALL_ROOT_DIR="$SCRIPTS_DIR/../install"
mkdir -p $INSTALL_ROOT_DIR

## SIMULATOR
BUILD_DIR="build_ios_pod_x86_64"
#if [ ! -d "$CAFFE2_ROOT/$BUILD_DIR" ]; then
  # Build for simulator x86_64
#USE_NPACK=OFF IOS_PLATFORM=SIMULATOR BUILD_DIR=$BUILD_DIR INSTALL_DIR="$INSTALL_ROOT_DIR/x86_64" $SCRIPTS_DIR/build_ios_pod.sh
#fi
cd "$CAFFE2_ROOT/$BUILD_DIR"

# copy missing protoc files to build directory... not sure why they are in the wrong location
cp -rf $CAFFE2_ROOT/build_host_protoc/lib/libprotoc.a ./third_party/protobuf/cmake/libprotoc.a
cp -rf $CAFFE2_ROOT/build_host_protoc/bin/protoc ./third_party/protobuf/cmake/protoc

#make install

## DEVICE
# Build for iOS
BUILD_DIR="build_ios_pod_arm"
#if [ ! -d "$CAFFE2_ROOT/$BUILD_DIR" ]; then
#USE_NPACK=ON IOS_PLATFORM=OS BUILD_DIR=$BUILD_DIR INSTALL_DIR="$INSTALL_ROOT_DIR/arm" $SCRIPTS_DIR/build_ios_pod.sh
#fi
cd "$CAFFE2_ROOT/$BUILD_DIR"

# copy missing protoc files to build directory... not sure why they are in the wrong location
cp -rf $CAFFE2_ROOT/build_host_protoc/lib/libprotoc.a ./third_party/protobuf/cmake/libprotoc.a
cp -rf $CAFFE2_ROOT/build_host_protoc/bin/protoc ./third_party/protobuf/cmake/protoc

#make install

# Build for WatchOS
BUILD_DIR="build_ios_pod_watchos"
#if [ ! -d "$CAFFE2_ROOT/$BUILD_DIR" ]; then
#USE_NPACK=ON IOS_PLATFORM=WATCHOS BUILD_DIR=$BUILD_DIR INSTALL_DIR="$INSTALL_ROOT_DIR/watchos" $SCRIPTS_DIR/build_ios_pod.sh
#fi
cd "$CAFFE2_ROOT/$BUILD_DIR"

# copy missing protoc files to build directory... not sure why they are in the wrong location
cp -rf $CAFFE2_ROOT/build_host_protoc/lib/libprotoc.a ./third_party/protobuf/cmake/libprotoc.a
cp -rf $CAFFE2_ROOT/build_host_protoc/bin/protoc ./third_party/protobuf/cmake/protoc

#make install

# merge static libs (fat binaries)
mkdir -p "$INSTALL_ROOT_DIR/lib/"
lipo -create \
    "$CAFFE2_ROOT/build_ios_pod_arm/libCAFFE2_NNPACK.a" \
    "$CAFFE2_ROOT/build_ios_pod_watchos/libCAFFE2_NNPACK.a" \
  -output "$INSTALL_ROOT_DIR/lib/libCAFFE2_NNPACK.a"

lipo -create \
"$CAFFE2_ROOT/build_ios_pod_arm/libCAFFE2_PTHREADPOOL.a" \
"$CAFFE2_ROOT/build_ios_pod_watchos/libCAFFE2_PTHREADPOOL.a" \
-output "$INSTALL_ROOT_DIR/lib/libCAFFE2_PTHREADPOOL.a"


lipo -create \
"$INSTALL_ROOT_DIR/arm/lib/libCaffe2_CPU.a" \
"$INSTALL_ROOT_DIR/x86_64/lib/libCaffe2_CPU.a" \
-output "$INSTALL_ROOT_DIR/lib/libCaffe2_CPU.a"

lipo -create \
"$INSTALL_ROOT_DIR/arm/lib/libprotobuf-lite.a" \
"$INSTALL_ROOT_DIR/watchos/lib/libprotobuf-lite.a" \
"$INSTALL_ROOT_DIR/x86_64/lib/libprotobuf-lite.a" \
-output "$INSTALL_ROOT_DIR/lib/libprotobuf-lite.a"

lipo -create \
"$INSTALL_ROOT_DIR/arm/lib/libprotobuf.a" \
"$INSTALL_ROOT_DIR/watchos/lib/libprotobuf.a" \
"$INSTALL_ROOT_DIR/x86_64/lib/libprotobuf.a" \
-output "$INSTALL_ROOT_DIR/lib/libprotobuf.a"

# copy headers
mkdir -p "$INSTALL_ROOT_DIR/include"
cp -rf "$INSTALL_ROOT_DIR/arm/include/*" "$INSTALL_ROOT_DIR/include"
cp -rf "$CAFFE2_ROOT/third_party/eigen/Eigen/*" "$INSTALL_ROOT_DIR/include/Eigen/"
