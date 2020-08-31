//
// Created by packll on 2017/9/27.
//
#ifndef FFMPEGMUSIC_FFMPEGAUDIO_H
#define FFMPEGMUSIC_FFMPEGAUDIO_H

#include <SLES/OpenSLES_Android.h>
#include <queue>
extern  "C"
{
#include "Log.h"
#include <libavcodec/avcodec.h>
#include <pthread.h>
#include <libswresample/swresample.h>
class FFmpegAudio{
public:
    void setAvCodecContext(AVCodecContext *codecContext, long duration);

    int get(AVPacket *packet);

    int put(AVPacket *packet);

    void play();

    void setAudioCallback(void (*call)(long, long));

    void stop();

    int createPlayer();

    FFmpegAudio();

    ~FFmpegAudio();
//成员变量
public:
//    是否正在播放
    int isPlay;
//    流索引
    int index;
//音频队列
    std::queue<AVPacket *> queue;
//    处理线程
    pthread_t p_playid;
    AVCodecContext *codec;
    long duration = 0;

//    同步锁
    pthread_mutex_t mutex;
//    条件变量
    pthread_cond_t cond;
    /**
     * 新增
     */
    SwrContext *swrContext;
    uint8_t *out_buffer;
    int out_channer_nb;
//    相对于第一帧时间
    double clock;

    AVRational time_base;

    SLObjectItf engineObject;
    SLEngineItf engineEngine;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb;
    SLObjectItf outputMixObject;
    SLObjectItf bqPlayerObject;
    SLEffectSendItf bqPlayerEffectSend;
    SLVolumeItf bqPlayerVolume;
    SLPlayItf bqPlayerPlay;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;


};

};

#endif //FFMPEGMUSIC_FFMPEGAUDIO_H