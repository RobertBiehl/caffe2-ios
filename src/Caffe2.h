//
//  Caffe2.h
//  Caffe2Test
//
//  Created by Robert Biehl on 21.04.17.
//  Copyright © 2017 Robert Biehl. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface Caffe2: NSObject

// set the networks enforced image input size. If not set, the images dimensions will be used.
@property (atomic, assign) CGSize imageInputDimensions;

- (null_unspecified instancetype)init UNAVAILABLE_ATTRIBUTE;

- (null_unspecified instancetype) init:(nonnull NSString*)initNetFilename predict:(nonnull NSString*)predictNetFilename error:(NSError * _Nullable * _Nullable)error
NS_SWIFT_NAME(init(initNetNamed:predictNetNamed:));

- (nullable NSArray<NSNumber*>*) predict:(nonnull UIImage*) image
NS_SWIFT_NAME(prediction(regarding:));
@end
