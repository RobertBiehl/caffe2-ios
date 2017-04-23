Pod::Spec.new do |s|
    s.name             = 'Caffe2Kit'
    s.version          = '0.0.1'
    s.summary          = 'Caffe2 for iOS (Swift, ObjC). A simple one step integration'

    s.description      = <<-DESC
Caffe2 for iOS. A simple, one step integration into existing projects.
Caffe2 is a lightweight, modular, and scalable deep learning framework.
    DESC

    s.homepage         = 'https://github.com/RobertBiehl/caffe2-ios'
    s.license          = { :type => 'Apache License 2.0', :file => 'LICENSE' }
    s.authors          = 'robert@oksnap.me'
    s.source       = { :path => '/Users/Robert/Documents/Workspace/caffe2-swift' }
#s.source           = { :git => 'https://github.com/RobertBiehl/caffe2-ios', :branch => 'master', :submodules => true}

    s.ios.deployment_target = "10.3"
    
    s.default_subspecs = %[Core]
    
    s.subspec 'Core' do |ss|
      ss.dependency "Caffe2Kit/CPU"

      ss.source_files = "src/*{.h,.m,.hh,.mm}"
      ss.public_header_files = "src/Caffe2.h"
      
      s.libraries = 'stdc++'
    end
    
    s.subspec 'CPU' do |ss|
      ss.header_mappings_dir = "lib/caffe2/install/include/"
      
      ss.xcconfig = {
        'HEADER_SEARCH_PATHS' =>  '$(inherited) "/Users/Robert/Documents/Workspace/caffe2-swift/lib/caffe2/install/include/" ' +
        '"/Users/Robert/Documents/Workspace/caffe2-swift/lib/caffe2/third_party/eigen/"',
        'OTHER_LDFLAGS' => '$(inherited) -Wl,-force_load,/Users/Robert/Documents/Workspace/caffe2-swift/lib/caffe2/install/lib/libCaffe2_CPU.a'
      }
      
      ss.vendored_libraries  = 'lib/caffe2/install/lib/libCaffe2_CPU.a',
                                'lib/caffe2/install/lib/libprotobuf-lite.a',
                                'lib/caffe2/install/lib/libprotobuf.a',
                                'lib/caffe2/build_ios_pod/libCAFFE2_NNPACK.a',
                                'lib/caffe2/build_ios_pod/libCAFFE2_PTHREADPOOL.a'
      ss.libraries =  'stdc++'
    end
    
    s.frameworks = [
      "Accelerate",
      "UIKit",
    ]

    s.prepare_command = <<-CMD
      # build
      scripts/build_ios_pod.sh
      
      # copy missing protoc files to build directory.
      # Not sure why they are in the wrong location
      cp lib/caffe2/build_host_protoc/lib/libprotoc.a lib/caffe2/build_ios_pod/third_party/protobuf/cmake/libprotoc.a
      cp lib/caffe2/build_host_protoc/bin/protoc lib/caffe2/build_ios_pod/third_party/protobuf/cmake/protoc
      
      # install
      cd lib/caffe2/build_ios_pod
      make install
    CMD
end
