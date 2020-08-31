//
// Created by packll on 2017/9/27.
//

#ifndef FFMPEGMUSIC_LOG_H
#define FFMPEGMUSIC_LOG_H
#include <android/log.h>

#define LOGOPEN 1 //日志开关，1为开，其它为关

#if(LOGOPEN==1)
#define LOGD(...) \
    __android_log_print(ANDROID_LOG_DEBUG, "packllNative", __VA_ARGS__)

#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, "packllNative", __VA_ARGS__)
#else
#define LOGV(...) NULL

#define LOGD(...) NULL

#define LOGE(...) NULL
#endif

#endif //FFMPEGMUSIC_LOG_H
