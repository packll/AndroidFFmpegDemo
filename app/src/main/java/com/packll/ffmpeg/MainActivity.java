package com.packll.ffmpeg;

import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.provider.Settings;
import android.util.Log;
import android.util.SparseArray;
import android.view.SurfaceView;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ProgressBar;
import android.widget.Spinner;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;


public class MainActivity extends AppCompatActivity {
    private static final String hlsStr = "http://ivi.bupt.edu.cn/hls/cctv1hd.m3u8";
    private static final String mp4Str = "http://vfx.mtime.cn/Video/2019/03/19/mp4/190319212559089721.mp4";
    private static final String rtmpStr = "rtmp://58.200.131.2:1935/livetv/hunantv";
    private static final String localStr = Environment.getExternalStorageDirectory() +  "/aaaa.mp4";
    private String currentStr = hlsStr;

    private static final String[] urls = new String[]{
            hlsStr, mp4Str, rtmpStr, localStr
    };

    private static final String[] urlTypes = new String[]{
            "hls", "httpmp4", "rtmp", "local"
    };


    FFmpegPlayer player;
    SurfaceView surfaceView;
    private ProgressBar mpb;

    private Handler mHandler = new Handler();
    private Spinner spinner;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        surfaceView = (SurfaceView) findViewById(R.id.surface);
        player = new FFmpegPlayer();
        mpb = findViewById(R.id.pb);
        spinner = findViewById(R.id.spinner);
        player.setSurfaceView(surfaceView);

        String androidId = getAndroidId();
        Log.e("packll", "androidId = " + androidId);

        ArrayAdapter<String> compressLevelAdapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, urlTypes);
        spinner.setAdapter(compressLevelAdapter);
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                currentStr = urls[position];
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });


    }

    public void player(View view) {
        player.registerCallback(new FFmpegPlayer.PlayCallback() {
            @Override
            public void onPlayProgress(long currentTime, long duration) {
                final float percent = 1.0f * currentTime / duration;
                Log.d("packll", "currentTime = " + currentTime + ";; duration = " + duration + ";; percent = " + percent);
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        mpb.setProgress((int) (1000 * percent));
                    }
                });
            }

            @Override
            public void onPlayError(int errorCode, final String msg) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(MainActivity.this, msg, Toast.LENGTH_SHORT).show();
                    }
                });
            }
        });
        player.playJava(currentStr);
    }
    public void stop(View view) {
        player.release();
    }


    public String getAndroidId() {
        try {
            return Settings.Secure.getString(getContentResolver(), Settings.Secure.ANDROID_ID);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return "";
    }
}
