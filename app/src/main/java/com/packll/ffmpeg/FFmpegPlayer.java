package com.packll.ffmpeg;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * Created by packll on 2017/9/20.
 */

public class FFmpegPlayer implements SurfaceHolder.Callback {
    static{
        System.loadLibrary("avcodec");
        System.loadLibrary("avfilter");
        System.loadLibrary("avformat");
        System.loadLibrary("avutil");
        System.loadLibrary("swresample");
        System.loadLibrary("swscale");
        System.loadLibrary("native-lib");
    }

    private SurfaceView surfaceView;
    public void playJava(String path) {
        if (surfaceView == null) {
            return;
        }
        play(path);
    }

    public void setSurfaceView(SurfaceView surfaceView) {
        this.surfaceView = surfaceView;
        surfaceView.getHolder().addCallback(this);

    }

    public void setCallback(PlayCallback callback) {
        registerCallback(callback);
    }

    public native void play(String path);

    public native void display(Surface surface, int width, int height);


    public native void release();

    public native void releaseaa();

    public native void registerCallback(PlayCallback callback);


    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width,
                               int height) {
        display(holder.getSurface(), width, height);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {

    }


    public interface PlayCallback {
        void onPlayProgress(long currentTime, long duration);
        void onPlayError(int errorCode, String msg);
    }
}
