# Caffe2Kit
[Caffe2](https://github.com/caffe2/caffe2) for iOS.
A simple, one step integration into existing projects.

![Caffe2Kit - Simple integration of Caffe2 on iOS.](https://github.com/RobertBiehl/caffe2-ios/blob/master/.github/cover.png)

## 🚨 Attention
> Please note that this pod is in a very early stage and currently has multiple shortcomings:
> * Only works on the device! -- *no simulator support*
> * `pod install` takes ages! -- *currently caffe2 is being built on install*
> * Only runs on iOS 10.3! -- *should be fixed soon by udating the build_ios_pod.sh build script"
> * Wrapper currently only supports classification tasks.

## 📲 Installation

Caffe2Kit is available through [CocoaPods](http://cocoapods.org). To install
it, simply add the following line to your Podfile:

```ruby
pod "Caffe2Kit"
```

## 🚀 Using Caffe2Kit

```swift
import Caffe2Kit

caffe = Caffe2("squeeze_init_net", predict:"squeeze_predict_net")
let 🌅 = #imageLiteral(resourceName: "lion.png")
    
if let res = caffe?.predict(🌅) {
  // find top 5 classes
  let sorted = res
    .map{$0.floatValue}
    .enumerated()
    .sorted(by: {$0.element > $1.element})[0...5]
      
  // generate output
  let text = sorted
    .map{"\($0.offset): \(classes[$0.offset]) \($0.element*100)%"}
    .joined(separator: "\n")

  println("Result\n \(text)")
}
```

**Result:**

```
291: 🦁 57.5012%
283: 🐈 19.2037%
378: 🐒 11.6096%
903: 💇 6.85409%
568: fur coat 3.20833%
539: doormat, welcome mat 0.846964%
```

## ✅ Requirements

Deployment target of your App is >= iOS 10.3

## 🤖 Author(s)

Robert Biehl, robert@oksnap.me

## 📄 License

Caffe2Kit is available under the Apache License 2.0. See the LICENSE file for more info.

Caffe2 is released under the [BSD 2-Clause license](https://github.com/Yangqing/caffe2/blob/master/LICENSE).
