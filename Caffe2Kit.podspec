Pod::Spec.new do |s|
    s.name             = 'Caffe2Kit'
    s.version          = '0.0.8'
    s.summary          = 'Caffe2 for iOS (Swift, ObjC). A simple one step integration'

    s.description      = <<-DESC
      Caffe2 for iOS. A simple, one step integration into existing projects.
      Caffe2 is a lightweight, modular, and scalable deep learning framework.
    DESC

    s.homepage         = 'https://github.com/RobertBiehl/caffe2-ios'
    s.license          = { :type => 'Apache License 2.0', :file => 'LICENSE' }
    s.authors          = 'robert@oksnap.me'
    s.source           = { :git => 'https://github.com/RobertBiehl/caffe2-ios', :tag => s.version}

    s.ios.deployment_target = '10.3'
    
    s.default_subspecs = %[Core]
    
    s.subspec 'Core' do |ss|
      ss.dependency 'Caffe2Kit/CPU'

      ss.source_files = 'src/*{.h,.m,.hh,.mm}'
      ss.public_header_files = 'src/Caffe2.h'
      
      s.libraries = 'stdc++'
    end
    
    s.subspec 'CPU' do |ss|
      ss.header_mappings_dir = 'install/include/'
      ss.preserve_paths = 'install/include/**/*.h', 'install/include/Eigen/*', 'lib/caffe2/LICENSE', 'lib/caffe2/PATENTS'

      ss.xcconfig = {
        'HEADER_SEARCH_PATHS' =>  '$(inherited) "$(PODS_ROOT)/Caffe2Kit/install/include/" "$(PODS_ROOT)/Caffe2Kit/install/include/Eigen/"',
        'OTHER_LDFLAGS' => '-force_load "$(PODS_ROOT)/Caffe2Kit/install/lib/libCaffe2_CPU.a"'
      }
      
      ss.vendored_libraries  = 'install/lib/libCaffe2_CPU.a',
                                'install/lib/libprotobuf-lite.a',
                                'install/lib/libprotobuf.a',
                                'install/lib/libCAFFE2_NNPACK.a',
                                'install/lib/libCAFFE2_PTHREADPOOL.a'
      ss.libraries =  'stdc++'
    end
    
    s.frameworks = [
      "Accelerate",
      "UIKit",
    ]
    s.libraries =  'stdc++'
end
