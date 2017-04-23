//
//  Caffe2.m
//  Caffe2Test
//
//  Created by Robert Biehl on 21.04.17.
//  Copyright © 2017 Robert Biehl. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Caffe2.h"

#include "caffe2/core/predictor.h"
#include "caffe2/utils/proto_utils.h"


void ReadProtoIntoNet(std::string fname, caffe2::NetDef* net) {
  int file = open(fname.c_str(), O_RDONLY);
  CAFFE_ENFORCE(net->ParseFromFileDescriptor(file));
  close(file);
}

std::string FilePathForResourceName(NSString* name, NSString* extension) {
  NSString* file_path = [[NSBundle mainBundle] pathForResource:name ofType:extension];
  if (file_path == NULL) {
    LOG(FATAL) << "Couldn't find '" << [name UTF8String] << "."
    << [extension UTF8String] << "' in bundle.";
    return nullptr;
  }
  return file_path.UTF8String;
}

@interface Caffe2(){
  caffe2::NetDef _initNet;
  caffe2::NetDef _predictNet;
  caffe2::Predictor *_predictor;
}

@property (atomic, assign) BOOL busyWithInference;

@end

@implementation Caffe2

- (instancetype) init:(nonnull NSString*) initNetFilename predict:(nonnull NSString*) predictNetFilename{
  self = [super init];
  if(self){
    
    ReadProtoIntoNet(FilePathForResourceName(initNetFilename, @"pb"), &_initNet);
    ReadProtoIntoNet(FilePathForResourceName(predictNetFilename, @"pb"), &_predictNet);
    
    _predictNet.set_name("PredictNet");
    _predictor = new caffe2::Predictor(_initNet, _predictNet);
  }
  return self;
}

- (nullable NSArray<NSNumber*>*) predict:(nonnull UIImage*) image{
  caffe2::Predictor::TensorVector output_vec;
  
  if (self.busyWithInference) {
    return nil;
  } else {
    self.busyWithInference = true;
  }
  
  CFDataRef pixelData = CGDataProviderCopyData(CGImageGetDataProvider(image.CGImage));
  const UInt8* pixels = CFDataGetBytePtr(pixelData);
  
  const int height = (const int)image.size.height;
  const int width = (const int)image.size.width;
  
  if (_predictor && pixels) {
    caffe2::TensorCPU input;
    
    // Reasonable dimensions to feed the predictor.
    const int predHeight = 128;
    const int predWidth = 128;
    const int crops = 1;
    const int channels = 3;
    const int size = predHeight * predWidth;
    const float hscale = ((float)height) / predHeight;
    const float wscale = ((float)width) / predWidth;
    const float scale = std::min(hscale, wscale);
    std::vector<float> inputPlanar(crops * channels * predHeight * predWidth);
    // Scale down the input to a reasonable predictor size.
    for (auto i = 0; i < predHeight; ++i) {
      const int _i = (int) (scale * i);
      for (auto j = 0; j < predWidth; ++j) {
        const int _j = (int) (scale * j);
        // The input is of the form RGBA, we only need the RGB part.
        float blue = (float) pixels[(_i * width + _j) * 4 + 0];
        float green = (float) pixels[(_i * width + _j) * 4 + 1];
        float red = (float) pixels[(_i * width + _j) * 4 + 2];
        if(_j==19){
          printf("%d,%d RGB(%f, %f, %f)\n", i, _j, red, green, blue);
        }
        
        inputPlanar[i * predWidth + j + 0 * size] = blue;
        inputPlanar[i * predWidth + j + 1 * size] = green;
        inputPlanar[i * predWidth + j + 2 * size] = red;
      }
    }
    
    input.Resize(std::vector<int>({crops, channels, predHeight, predWidth}));
    input.ShareExternalPointer(inputPlanar.data());
    
    caffe2::Predictor::TensorVector input_vec{&input};
    _predictor->run(input_vec, &output_vec);
        
    NSMutableArray* result = nil;
    if (output_vec.capacity() > 0) {
      for (auto output : output_vec) {
        // currently only one dimensional output supported
        result = [NSMutableArray arrayWithCapacity:output_vec.size()];
        for (auto i = 0; i < output->size(); ++i) {
          result[i] = @(output->template data<float>()[i]);
        }
      }
    }
    
    self.busyWithInference = false;
    return result;
  }
  
  self.busyWithInference = false;
  return nil;
}

@end
