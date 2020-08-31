//
// Created by packll on 2017/9/27.
//



#ifndef FFMPEGMUSIC_FFMPEGVIDEO_H
#define FFMPEGMUSIC_FFMPEGVIDEO_H

#endif //FFMPEGMUSIC_FFMPEGVIDEO_H
#include <queue>
#include "FFmpegAudio.h"
extern "C"
{
#include <unistd.h>
#include <pthread.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include "Log.h"
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>


class FFmpegVideo{
public:
    FFmpegVideo();
    ~FFmpegVideo();

    int get(AVPacket *packet);

    int put(AVPacket *packet);

    void play();

    void stop();

    void setAvCodecContext(AVCodecContext *codecContext);

    void setTargetSize(int width, int height);
    /**
     * 设置回调接口
     * @param call
     */
    void setPlayCall(void (*call)(AVFrame* frame));

    double synchronize(AVFrame *frame, double play);

    void setAudio(FFmpegAudio *audio);

public:
    //    是否正在播放
    int isPlay;
//    流索引
    int index;
//音频队列
    std::queue<AVPacket *> queue;
//    处理线程
    pthread_t p_playid;
//    解码器上下文
    AVCodecContext *codec;

//    同步锁
    pthread_mutex_t mutex;
//    条件变量
    pthread_cond_t cond;

    int targetWidth;
    int targetHeight;

    /**
     * ------新增--------
     */
//头文件写在"extend"C外
    FFmpegAudio* audio;
    AVRational time_base;
    double  clock;

};

};