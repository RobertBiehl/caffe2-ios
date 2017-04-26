//
//  Caffe2.h
//  Caffe2Test
//
//  Created by Robert Biehl on 21.04.17.
//  Copyright © 2017 Robert Biehl. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface Caffe2: NSObject
- (null_unspecified instancetype)init UNAVAILABLE_ATTRIBUTE;

- (null_unspecified instancetype) init:(nonnull NSString*)initNetFilename predict:(nonnull NSString*)predictNetFilename error:(NSError**)error
NS_SWIFT_NAME(init(initializingNetNamed:predictingNetNamed:));

- (nullable NSArray<NSNumber*>*) predict:(nonnull UIImage*) image
NS_SWIFT_NAME(prediction(regarding:));
@end
