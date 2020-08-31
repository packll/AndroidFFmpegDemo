#include <jni.h>
#include <string>
#include "FFmpegAudio.h"
#include "FFmpegVideo.h"

#include <android/native_window_jni.h>

extern "C" {
ANativeWindow *window = NULL;
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
//像素处理
#include "libswscale/swscale.h"

#include <unistd.h>

#include "Log.h"

static JavaVM* java_vm = NULL;
const char *path;
FFmpegAudio *audio = NULL;
FFmpegVideo *video = NULL;
pthread_t p_tid;

jobject callbackObj = NULL;
jmethodID progressCallback = NULL;
jmethodID errorCallback = NULL;

int isPlay = 0;
int videoWidth = 0;
int videoHeight = 0;
int windowWidth = 0;
int windowHeight = 0;
int targetWidth = 0;
int targetHeight = 0;

void resizeWindow(ANativeWindow *window, int windowWidth, int windowHeight, int videoWidth, int videoHeight) {
    if (windowWidth != 0 && windowHeight != 0 && videoWidth != 0 && videoHeight != 0) {
        float vRatio = 1.0f * videoWidth / videoHeight;
        float wRatio = 1.0f * windowWidth / windowHeight;

        if (wRatio < vRatio) {
            targetWidth = windowWidth;
            targetHeight = int (1.0f * windowWidth / videoWidth * videoHeight);
        } else {
            targetHeight = windowHeight;
            targetWidth = int (1.0f * windowHeight / videoHeight * videoWidth);
        }

        ANativeWindow_setBuffersGeometry(window, windowWidth,
                                         windowHeight,
                                         WINDOW_FORMAT_RGBA_8888);
        if(video != NULL){
            video->setTargetSize(targetWidth, targetHeight);
        }
        LOGE("windowWidth = %d, windowHeight = %d, videoWidth = %d, videoHeight = %d, targetWidth = %d, targetHeight = %d"
        ,windowWidth, windowHeight, videoWidth, videoHeight, targetWidth, targetHeight);
    }
}

void onPlayProgressCallback(long currentTime, long duration) {
    if(callbackObj == NULL || progressCallback == NULL) {
        return ;
    }
    JNIEnv *env;
    void *void_env;
    bool had_to_attach = false;
    jint status = java_vm->GetEnv(&void_env, JNI_VERSION_1_6);
    if (status == JNI_EDETACHED) {
        java_vm->AttachCurrentThread(&env, NULL);
        had_to_attach = true;
    } else {
        env = static_cast<JNIEnv *>(void_env);
    }

    env->CallVoidMethod(callbackObj, progressCallback, (jlong)currentTime, (jlong)duration);

    if (had_to_attach) {
        java_vm->DetachCurrentThread();
    }
}

void onPlayErrorCallback(int code, char* errorMsg) {
    if(callbackObj == NULL || errorCallback == NULL) {
        return ;
    }
    JNIEnv *env;
    void *void_env;
    bool had_to_attach = false;
    jint status = java_vm->GetEnv(&void_env, JNI_VERSION_1_6);
    if (status == JNI_EDETACHED) {
        java_vm->AttachCurrentThread(&env, NULL);
        had_to_attach = true;
    } else {
        env = static_cast<JNIEnv *>(void_env);
    }
    jstring errorStr = env->NewStringUTF(errorMsg);
    env->CallVoidMethod(callbackObj, errorCallback, (jint)code, errorStr);
    if (had_to_attach) {
        java_vm->DetachCurrentThread();
    }
}

void call_video_play(AVFrame *frame) {
    if (!window) {
        return;
    }
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        return;
    }

    LOGE("绘制 宽%d,高%d, 绘制 宽%d,高%d  行字节 %d ", frame->width, frame->height, window_buffer.width, window_buffer.height, frame->linesize[0]);
    uint8_t *dst = (uint8_t *) window_buffer.bits;
    int dstStride = window_buffer.stride * 4;
    uint8_t *src = frame->data[0];
    int srcStride = frame->linesize[0];
    int xOffset = (windowWidth - targetWidth) / 2 * 4;
    int yOffset = (windowHeight - targetHeight) / 2;
    memset(dst, 0,windowWidth * windowHeight * 4);
    for (int i = yOffset; i < (targetHeight + yOffset); ++i) {
        memcpy(dst + (i * dstStride + xOffset), src + (i - yOffset) * srcStride, srcStride);
    }
    ANativeWindow_unlockAndPost(window);
}

static int interrupt_cb(void *ctx) {
    return (isPlay == 1) ? 0 : 1 ;
}

//解码函数
void *process(void *args) {
    LOGE("开启解码线程");
    //1.注册组件
    av_register_all();
    avformat_network_init();
    //封装格式上下文
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    isPlay = 1;
    pFormatCtx->interrupt_callback.callback = interrupt_cb;
    pFormatCtx->interrupt_callback.opaque = pFormatCtx;

    //2.打开输入视频文件
    int result = avformat_open_input(&pFormatCtx, path, NULL, NULL);
    if (result != 0) {
        onPlayErrorCallback(-1, "打开输入视频文件失败");
        LOGE("打开输入视频文件失败 %d", result);
        return NULL;
    }
    //3.获取视频信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("获取视频信息失败");
        onPlayErrorCallback(-1, "获取视频信息失败");
        return NULL;
    }

    int64_t duration = pFormatCtx->duration;
    LOGD("duration = %ld", duration);
    //视频解码，需要找到视频对应的AVStream所在pFormatCtx->streams的索引位置
    int i = 0;
    for (; i < pFormatCtx->nb_streams; i++) {
        //4.获取视频解码器
        AVCodecContext *pCodeCtx = pFormatCtx->streams[i]->codec;
        AVCodec *pCodec = avcodec_find_decoder(pCodeCtx->codec_id);

        AVCodecContext *codec = avcodec_alloc_context3(pCodec);
        avcodec_copy_context(codec, pCodeCtx);
        if (avcodec_open2(codec, pCodec, NULL) < 0) {
            LOGE("%s", "解码器无法打开");
            onPlayErrorCallback(-1, "解码器无法打开");
            continue;
        }
        //根据类型判断，是否是视频流
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            /*找到视频流*/
            video->setAvCodecContext(codec);
            video->index = i;
            video->time_base = pFormatCtx->streams[i]->time_base;
            videoWidth = codec->width;
            videoHeight = codec->height;
            resizeWindow(window, windowWidth, windowHeight, videoWidth, videoHeight);

            LOGE("width = %d, height = %d, %d , %d ", videoWidth, videoHeight, codec->coded_width, codec->coded_height);
        } else if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            int channels = codec->channels;
            int sampleRate = codec->sample_rate;
            int bitsPerSample = codec->bits_per_coded_sample;
            audio->setAvCodecContext(codec, duration);
            audio->index = i;
            audio->time_base = pFormatCtx->streams[i]->time_base;
            LOGE("channels = %d, sampleRate = %d, bitsPersample = %d", channels, sampleRate,
                 bitsPerSample);
        }
    }

//    开启 音频 视频  播放的死循环
    video->setAudio(audio);
    video->play();
    audio->play();
//    解码packet
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
//    解码完整个视频 子线程
    int ret;
    while (isPlay) {
//        如果这个packet  流索引 等于 视频流索引 添加到视频队列
        ret = av_read_frame(pFormatCtx, packet);
        LOGE("av_read_frame  ，ret = %d", ret);
        if (ret == 0) {
            if (video && video->isPlay && packet->stream_index == video->index) {
                video->put(packet);
            } else if (audio && audio->isPlay && packet->stream_index == audio->index) {
                audio->put(packet);
            }
            av_packet_unref(packet);
        } else if (ret == AVERROR_EOF) {
            //读取完毕 但是不一定播放完毕
            while (isPlay) {
                if (video->queue.empty() && audio->queue.empty()) {
                    break;
                }
//                LOGI("等待播放完成");
                av_usleep(10000);
            }
            break;
        }

    }
    LOGE("end of file ");
    isPlay = 0;
    if (video && video->isPlay) {
        video->stop();
    }
    if (audio && audio->isPlay) {
        audio->stop();
    }
    av_free_packet(packet);
    avformat_free_context(pFormatCtx);
    pthread_exit(0);

}
extern "C"
JNIEXPORT void JNICALL
Java_com_packll_ffmpeg_FFmpegPlayer_release(JNIEnv *env, jobject instance) {
    LOGE("FFmpegPlayer_release 1");
    if (isPlay) {
        isPlay = 0;
        pthread_join(p_tid, 0);
    }
    LOGE("FFmpegPlayer_release 2");
    if (video != NULL) {
        if (video->isPlay) {
            video->stop();
        }
        delete (video);
        video = NULL;
    }
    if (audio != NULL) {
        if (audio->isPlay) {
            audio->stop();
        }
        delete (audio);
        audio = NULL;
    }

}
extern "C"
JNIEXPORT void JNICALL
Java_com_packll_ffmpeg_FFmpegPlayer_display(JNIEnv *env, jobject instance, jobject surface, jint wWidth, jint wHeight) {
    windowWidth = wWidth;
    windowHeight = wHeight;
    if (window != NULL) {
        ANativeWindow_release(window);
        window = NULL;
    }
    window = ANativeWindow_fromSurface(env, surface);
    resizeWindow(window, windowWidth, windowHeight, videoWidth, videoHeight);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_packll_ffmpeg_FFmpegPlayer_play(JNIEnv *env, jobject instance, jstring path_) {
    path = env->GetStringUTFChars(path_, 0);
//        实例化对象
    video = new FFmpegVideo;
    audio = new FFmpegAudio;
    video->setPlayCall(call_video_play);
    audio->setAudioCallback(onPlayProgressCallback);
    pthread_create(&p_tid, NULL, process, NULL);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_packll_ffmpeg_FFmpegPlayer_registerCallback(JNIEnv *env, jobject thiz, jobject callback) {
    callbackObj = env->NewGlobalRef(callback);
    jclass callbackClass = env->GetObjectClass(callbackObj);
    progressCallback = env->GetMethodID(callbackClass, "onPlayProgress", "(JJ)V");
    errorCallback = env->GetMethodID(callbackClass, "onPlayError", "(ILjava/lang/String;)V");
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGD("libusbaudio: loaded");
    java_vm = vm;
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved) {
    JNIEnv *env;
    void *void_env;
    java_vm->GetEnv(&void_env, JNI_VERSION_1_6);
    env = reinterpret_cast<JNIEnv*>(void_env);
    env->DeleteGlobalRef(reinterpret_cast<jobject>(progressCallback));
}

}