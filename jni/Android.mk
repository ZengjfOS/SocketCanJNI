#
# Copyright 2009 Cedric Priscal
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License. 
#

# LOCAL_PATH := $(call my-dir)
# 
# include $(CLEAR_VARS)
# 
# LOCAL_MODULE    := Linux_test
# LOCAL_SRC_FILES := Linux_test.c
# LOCAL_LDLIBS    := -llog
# LOCAL_C_INCLUDES := /cygdrive/d/ndk/android-ndk-r10d/platforms/android-21/arch-arm/usr/include
# 
# include $(BUILD_EXECUTABLE)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libCanSocket
# LOCAL_MODULE    := CanSocket
LOCAL_SRC_FILES := CanSocket.cpp
LOCAL_LDLIBS    := -llog
LOCAL_C_INCLUDES := ./include

include $(BUILD_SHARED_LIBRARY)