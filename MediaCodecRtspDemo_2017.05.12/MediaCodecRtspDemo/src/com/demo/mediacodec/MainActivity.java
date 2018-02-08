package com.demo.mediacodec;

import com.demo.lib.av.DataSource;
import com.demo.mediacodec.R;
import android.app.Activity;
import android.content.res.Configuration;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;

public class MainActivity extends Activity {
	private static final String TAG = "MainActivity";

	private SurfaceView surfaceView;
	private SurfaceHolder surfaceHolder;
	private Surface surface;

	private MyMediaCodec mediaCodec = null;
	private DataSource ds = null;
	private String Rtsp_Path = "rtsp://192.168.43.1:8554/cam0.h264";
	private boolean play_state = false;

	private EditText et_rtsp;
	private Button btn_play;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.i(TAG, "onCreate");
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);

		setContentView(R.layout.activity_main);

		surfaceView = (SurfaceView) findViewById(R.id.surfaceView);

		et_rtsp = (EditText) findViewById(R.id.et_rtsp);
		btn_play = (Button) findViewById(R.id.btn_play);
		btn_play.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				if (!play_state) {
					String rtsp = et_rtsp.getText().toString();
					if (!rtsp.trim().isEmpty()) {
						Rtsp_Path = rtsp;
					}

					startRtsp();

					play_state = true;
					btn_play.setText("Stop");
				} else {
					stopRtsp();

					play_state = false;
					btn_play.setText("Play");
				}
			}

		});
	}

	@Override
	protected void onResume() {
		// TODO Auto-generated method stub
		Log.i(TAG, "onResume");
		super.onResume();

		surfaceHolder = surfaceView.getHolder();
		surface = surfaceHolder.getSurface();
		if (play_state) {
			startRtsp();
		}
		Log.i(TAG, "onResume OK");
	}

	@Override
	protected void onPause() {
		// TODO Auto-generated method stub
		Log.i(TAG, "onPause");
		super.onPause();

		if (play_state) {
			stopRtsp();
		}
		Log.i(TAG, "onPause OK");
	}

	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
		Log.i(TAG, "onDestroy");
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		Log.i(TAG, "onConfigurationChanged()");

		if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
			Log.i(TAG, "Configuration.ORIENTATION_LANDSCAPE");
		} else {
			Log.i(TAG, "Configuration.ORIENTATION_PORTRAIT");
		}
	}

	private synchronized void startRtsp() {
		Log.i(TAG, "startRtsp()");
		ds = new DataSource();

		Log.i(TAG, "startRtsp() 2");
		mediaCodec = new MyMediaCodec();
		mediaCodec.init(this, ds, surface);

		Log.i(TAG, "startRtsp() 3");
		ds.Sink = mediaCodec;
		ds.init(Rtsp_Path);
		Log.i(TAG, "startRtsp() 4");
	}

	private synchronized void stopRtsp() {
		if (null != mediaCodec) {
			Log.i(TAG, "mediaCodec.deinit");
			mediaCodec.deinit();
		}

		if (null != ds) {
			Log.i(TAG, "ds.deinit()");
			ds.deinit();
			ds = null;
		}
	}
}