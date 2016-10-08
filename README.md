# SocketCanJNI

尝试将[libsocket-can-java](https://github.com/AplexOS/libsocket-can-java)移植到Android上的，里面对[libsocket-can-java](https://github.com/AplexOS/libsocket-can-java)添加了注释，发现该库还需要加强。

## 说明

* 本人对[libsocket-can-java](https://github.com/AplexOS/libsocket-can-java)源码进行了解读，相关部分都加了注释；
* [libsocket-can-java](https://github.com/AplexOS/libsocket-can-java)其源码中没有对滤波的实现，当然其中还要一些有关CAN的设定也是没有的；
* 本demo仅仅做了移植这部分内容的测试，并未去实现其他的相关任何内容。

## JNI NDK 编译方法

* jni/Android.mk

```
    LOCAL_PATH := $(call my-dir)

    include $(CLEAR_VARS)

    LOCAL_MODULE    := CanSocket
    LOCAL_SRC_FILES := CanSocket.cpp
    LOCAL_LDLIBS    := -llog
    LOCAL_C_INCLUDES := ./include

    include $(BUILD_SHARED_LIBRARY)
```

* jni/Application.mk

```
    APP_PLATFORM := android-9
    APP_STL := stlport_static
```

## JNI Android Source Code 编译方法

* jni/Android.mk
```
    LOCAL_PATH := $(call my-dir)

    include $(CLEAR_VARS)

    LOCAL_MODULE    := libCanSocket
    LOCAL_SRC_FILES := CanSocket.cpp
    LOCAL_LDLIBS    := -llog
    LOCAL_C_INCLUDES := ndk/sources/cxx-stl/stlport/stlport
    LOCAL_STATIC_LIBRARIES += libstlport_static

    include $(BUILD_SHARED_LIBRARY)
```

## Author

* [曾剑锋](http://www.cnblogs.com/zengjfgit/)
* [陈颖奇](http://www.cnblogs.com/ChYQ/)
