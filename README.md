# ☕️ Caffe2Kit
[Caffe2](https://github.com/caffe2/caffe2) for iOS.
A simple, one step integration into existing projects.

![Caffe2Kit - Simple integration of Caffe2 on iOS.](https://github.com/RobertBiehl/caffe2-ios/blob/master/.github/cover.png)

[![Twitter](https://img.shields.io/badge/Twitter-robeffect-4099FF.svg?style=flat)](https://twitter.com/robeffect)
[![Caffe2](https://img.shields.io/badge/Dep-caffe2-green.svg)](https://caffe2.ai)
[![Req](https://img.shields.io/badge/Req-iOS_10.3-green.svg)](https://github.com/RobertBiehl/caffe2-ios)
[![Build time](https://img.shields.io/badge/pod_install_takes-0h_40m_45s-red.svg)](https://github.com/RobertBiehl/caffe2-ios)

## 🚨 Attention
> Please note that this pod is in a very early stage and currently has multiple shortcomings:
> * Only works on the device! -- *no simulator support*
> * `pod install` takes ages! -- **I'm not kidding!! I waited 0h 40m 45s on a Mid 2012 Macbook Pro Retina! Currently caffe2 is being built on install.**
> * Only runs on iOS 10.3! -- *should be fixed soon by udating the build_ios_pod.sh build script*
> * Wrapper currently only supports classification tasks.

## 📲 Installation

Caffe2Kit ~~is~~ *will soon be* available through [CocoaPods](http://cocoapods.org). To install
it, simply add the following line to your Podfile:

```ruby
pod 'Caffe2Kit', :git => 'git://github.com/RobertBiehl/caffe2-ios', :submodules => true
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

## ⏱ Performance

Prediciting the class in the example app `examples/Caffe2Test` takes approx, 2ms on an iPhone 7 Plus and 6ms on an iPhone 6.

## ✅ Requirements

Deployment target of your App is >= iOS 10.3

## 🤖 Author(s)

Robert Biehl, robert@oksnap.me

## 📄 License

Caffe2Kit is available under the Apache License 2.0. See the LICENSE file for more info.

Caffe2 is released under the [BSD 2-Clause license](https://github.com/Yangqing/caffe2/blob/master/LICENSE).
