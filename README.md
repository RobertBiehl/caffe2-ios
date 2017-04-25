# ☕️ Caffe2Kit
[Caffe2](https://github.com/caffe2/caffe2) for iOS.
A simple integration into existing projects.

![Caffe2Kit - Simple integration of Caffe2 on iOS.](https://github.com/RobertBiehl/caffe2-ios/blob/master/.github/cover.png)

[![Twitter](https://img.shields.io/badge/Twitter-robeffect-4099FF.svg?style=flat)](https://twitter.com/robeffect)
[![Caffe2](https://img.shields.io/badge/Dep-caffe2-green.svg)](https://caffe2.ai)
[![Req](https://img.shields.io/badge/Req-iOS_10.3-green.svg)](https://github.com/RobertBiehl/caffe2-ios)

## 🚨 Attention
> Please note that this pod is in a very early stage and currently has multiple shortcomings:
> * Only works on the device! -- *no simulator support*
> * No officially on CocoaPods yet! -- *because this lib does not run on the simulator yet* 
> * Only runs on iOS 10.3! -- *should be fixed soon by udating the build_ios_pod.sh build script*
> * Wrapper currently only supports classification tasks.

## 📲 Installation

Caffe2Kit ~~is~~ *will soon be* available through [CocoaPods](http://cocoapods.org). To install
it, simply add the following line to your Podfile:

```ruby
pod 'Caffe2Kit', :git => 'git://github.com/RobertBiehl/caffe2-ios'
```

and run `pod install`.

### Disable Bitcode
Since caffe2 is not yet built with bitcode support you need to add this to your Podfile
```
post_install do |installer|
  installer.pods_project.targets.each do |target|
    target.build_configurations.each do |config|
      config.build_settings['ENABLE_BITCODE'] = 'NO'
    end
  end
end
```
and disable bitcode for your Target by setting **Build Settings -> Enable Bitcode** to `No`.

### Additional steps:
*These steps will hopefully be removed in later versions.*
1) in **Build Phases -> Your Target -> Link Binary with Libraries** add `libstc++.tdb`.
2) in **Build Settings -> Other Linker Flags** remove `$(inherited)`and `-force_load "$(PODS_ROOT)/Caffe2Kit/install/lib/libCaffe2_CPU.a"` from the list.

## 🚀 Using Caffe2Kit

```swift
import Caffe2Kit

let caffe = Caffe2("squeeze_init_net", predict:"squeeze_predict_net")
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

  print("Result\n \(text)")
}
```

**Result:**

```
291: 🦁 lion, king of beasts, Panthera leo 100.0%
373: 🐒 macaque 3.59472e-08%
231: 🐕 collie 4.77662e-09%
374: 🐒 langur 1.63787e-09%
371: 🐒 patas, hussar monkey, Erythrocebus patas 5.34424e-10%
259: 🐶 Pomeranian 2.12385e-10%
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
