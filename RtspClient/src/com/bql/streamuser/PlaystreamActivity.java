package com.bql.streamuser;


import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Timer;
import java.util.TimerTask;


import com.core.BQLBuffer;
import com.core.BQLMediaPlayer;
import com.core.BQLMediaPlayer.OnSnapShotListener;
import com.core.BQLMediaRecorder;
import com.core.BQLVideoView;
import com.core.IBQLViewCallback;

import android.app.Activity;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.Toast;
import android.widget.SeekBar.OnSeekBarChangeListener;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.MediaRecorder;

import android.hardware.Camera;  
import android.hardware.Camera.Parameters;  
import android.hardware.Camera.PreviewCallback;  
import android.hardware.Camera.AutoFocusCallback;

import android.graphics.ImageFormat;
import android.view.MotionEvent;

public class PlaystreamActivity extends Activity implements PreviewCallback {

	/*
	 * 	Note:
	 *  	if product does NOT support panorama, set false ; 
	 *  	otherwise, set true; 
	 */
	private static final boolean CONFIG_PANORAMA = true ; //true: fjb/c22/k23/c20
	private static final boolean NO_RTCP = true ; //true: fjb/c22/k23/c20
	
//	private static final String STREAM_URL_FILE_VOD  = "file://mnt/sdcard/3840x2178@30fps.mp4";
//	private static final String STREAM_URL_FILE_VOD  = "file://mnt/sdcard/wushun.3gp";
	private static final String STREAM_URL_FILE_VOD  = "file://mnt/sdcard/S06.mp4";
//	private static final String STREAM_URL_FILE_VOD  = "file://mnt/sdcard/rec_2017_03_10.mp4";
//	private static final String STREAM_URL_FILE_VOD  = "file://mnt/sdcard/210323BB.MP4.thumb"; //c29缩略图
//	private static final String STREAM_URL_FILE_VOD  = "file://mnt/sdcard/210323BB.MP4";
//	private static final String STREAM_URL_FILE_VOD  = "file://mnt/sdcard/CM360S/PHOTOS/V000000054920170210152027020CE7212428.3gp";	
//	private static final String STREAM_URL_FILE_VOD  = "file://mnt/sdcard/2016.mp4"; //jpeg
//	private static final String STREAM_URL_FILE_VOD  = "rtsp://192.168.42.1/tmp/SD0/DCIM/170306000/182031BB.MP4"; //c29回放
//	private static final String STREAM_URL_FILE_VOD  = "rtsp://192.168.43.1:8086/wushun.3gp"; //fjb/c22/k23
//	private static final String STREAM_URL_FILE_VOD  = "rtmp://192.168.11.26/test/d5f742ff87bb4aaab0fa37e21d26ec1f";
//	private static final String STREAM_URL_FILE_VOD  = "rtmp://192.168.11.26/vod/hb3.mp4";
//	private static final String STREAM_URL_FILE_VOD  = "rtsp://192.168.43.1:5086"; //c20
	private static final String STREAM_URL_FILE_LIVE = "rtsp://192.168.43.1:8086"; //fjb/c22/k23/k25
//	private static final String STREAM_URL_FILE_LIVE = "rtsp://192.168.42.1/live"; //c29
//	private static final String STREAM_URL_FILE_LIVE = "rtsp://192.168.43.1:5086"; //c20
//	private static final String STREAM_URL_FILE_LIVE = "rtsp://192.168.2.1/jc";    //f06
//	private static final String STREAM_URL_FILE_LIVE = "rtsp://192.168.43.1:8086/mjpg_SwsScale";
//	private static final String STREAM_URL_FILE_LIVE = "rtmp://live-baoqianli.8686c.com/live/a7d4c162b2774e5080dd27ee7780b9a0";
//	private static final String STREAM_URL_FILE_LIVE = "rtsp://192.168.1.244:8554/media/test.264"; //c29

	private static final String TAG = "PlaystreamActivity";

	private static final String REC_OFF = "转录"; 
	private static final String REC_ON = "关闭";

	private static final String REC_WR_OFF = "推流"; //手机推流[rtmp]
	private static final String REC_WR_ON  = "关闭";

	private static final String SH_OFF = "隐藏";
	private static final String SH_ON  = "显示";

	private static final String HC_OFF = "软解";
	private static final String HC_ON  = "硬解";
	
	private PreviewCallback mPcb = null;
	private SeekBar mSb = null; 
	private BQLVideoView mBQLSv = null;
	private Surface mSurface = null;
	private SurfaceView mSv = null;
	private SurfaceHolder mSh = null;
	private Button mVODBtn = null;
	private Button mPreviewBtn = null;
	private Button mStopBtn = null;
	private Button mPauseBtn = null;
	private Button mResumeBtn = null;
	private Button mAudioBtn = null;
	private Button mRecIncomeBtn = null; 
	private Button mRecWrBtn = null; 
	private Button mShotBtn = null;
	private Button mSHBtn = null;

	static int hwcodecmode = 0; 

	static final int videow = 800; 
	static final int videoh = 480;
	static final int samplerate = 44100; 
	static final int channel = AudioFormat.CHANNEL_CONFIGURATION_MONO; 
	static final int depth = AudioFormat.ENCODING_PCM_16BIT; 
	public boolean mIsRecording = false;	
	Camera  mCamera;

	private BQLMediaPlayer mMediaPlayer = null; 
	public  BQLMediaRecorder mMediaRecorder = null;
	private AudDataAvailableListener mAudDataListener = null;
	private RecDataAvailableListener mRecDataListener = null;
	
	private static HandlerThread mMediaThread = null;
	private static HandlerThread mWorkerThread = null; 
	private static Handler mWorkerHandler = null;
	private RecordAudioThread mRecordAudioThread = null; 
	
	
	/*
	 *  note: 
	 *  	update ui widget 
	 *  	if BQLMediaPlayer is created NOT in main thread,native message will post in non-main thread 
	 *  	in this case,it can NOT change ui widget in callback function,such as onPrepared onCompletion
	 *  	
	 */
	public class StateHandler extends Handler
	{
		public final static int INIT_STATE 			= 0 ;
		public final static int START_VOD_STATE 	= 1 ;
		public final static int DOING_VOD_STATE 	= 2 ;
		public final static int UPDATE_VOD_STATE 	= 3 ; 
		public final static int START_PREVIEW_STATE = 4 ;
		public final static int DOING_PREVIEW_STATE = 5 ;
		public final static int FINISH_SNAPSHOT_FAIL = 6 ;
 
		
		private int mDuration = 0 ;
		@Override
		public void handleMessage(Message msg) { 
	
			switch(msg.what){
			case INIT_STATE:
				removeMessages(UPDATE_VOD_STATE);
				mVODBtn.setEnabled(true);
				mPreviewBtn.setEnabled(true);
				mStopBtn.setEnabled(false);
				mPauseBtn.setEnabled(false);
				mResumeBtn.setEnabled(false);
				mRecIncomeBtn.setEnabled(false); mRecIncomeBtn.setText(REC_OFF);
				mRecWrBtn.setEnabled(true); mRecWrBtn.setText(REC_WR_OFF);
				mSHBtn.setEnabled(true); mSHBtn.setText(SH_OFF);
				mAudioBtn.setEnabled(true);
				mShotBtn.setEnabled(false);
				break;
			case START_VOD_STATE:
				mVODBtn.setEnabled(false);
				mPreviewBtn.setEnabled(false);
				mStopBtn.setEnabled(false);
				mPauseBtn.setEnabled(false);
				mResumeBtn.setEnabled(false);
				mAudioBtn.setEnabled(false);
				break;
			case DOING_VOD_STATE:
				
				Bundle data = msg.getData() ; 
				int duration = data.getInt("d");
				mDuration = duration ;
				
				mSb.setMax( duration );	
				mVODBtn.setEnabled(false);
				mPreviewBtn.setEnabled(false);
				mStopBtn.setEnabled(true);
				mPauseBtn.setEnabled(true);
				mResumeBtn.setEnabled(false);
				mRecIncomeBtn.setEnabled(true); mRecIncomeBtn.setText(REC_OFF);
				mAudioBtn.setEnabled(true);
				mRecWrBtn.setEnabled(false); 	mRecWrBtn.setText(REC_WR_OFF);
				mShotBtn.setEnabled(true);
				this.sendEmptyMessageDelayed(UPDATE_VOD_STATE, 1000);
				break;
			case UPDATE_VOD_STATE:
				if( mMediaPlayer == null ) return ;
				int position = mMediaPlayer.getCurrentPosition() ; 
				Log.d(TAG , " position = " + position);
				 if ( position >= mDuration ){
					 toastMessage("vod completed , but position is too large");
					 // do not get next update 
				 } else {
					 if(mSb != null) mSb.setProgress(position);
					 // get next update 
					 sendEmptyMessageDelayed(UPDATE_VOD_STATE, 1000); 
				 }
				break;
			case START_PREVIEW_STATE:
				mVODBtn.setEnabled(false);
				mPreviewBtn.setEnabled(false);
				mStopBtn.setEnabled(false);
				mPauseBtn.setEnabled(false);
				mResumeBtn.setEnabled(false);
				break;
			case DOING_PREVIEW_STATE:
				mVODBtn.setEnabled(false);
				mPreviewBtn.setEnabled(false);
				mStopBtn.setEnabled(true);
				mPauseBtn.setEnabled(true);
				mResumeBtn.setEnabled(false);
				mRecIncomeBtn.setEnabled(true); 	mRecIncomeBtn.setText(REC_OFF);
				mAudioBtn.setEnabled(false);
				mRecWrBtn.setEnabled(false); 		mRecWrBtn.setText(REC_WR_OFF);
				mShotBtn.setEnabled(true);
				break;	
			case FINISH_SNAPSHOT_FAIL:
				mShotBtn.setEnabled(true);
				break;
 
			}
			super.handleMessage(msg);
		}
	}
	
	private Handler mUIHandler = new StateHandler();
	private void toastMessage(final String msg)
	{
		mUIHandler.post(new Runnable(){
				@Override
				public void run() {
					Toast.makeText(PlaystreamActivity.this, msg , Toast.LENGTH_LONG).show();
				}
		 });
	}

	public void onPreviewFrame(byte[] data, Camera camera) {  
		if( mIsRecording )
		{
			BQLBuffer frame = mMediaRecorder.acquireBuffer(data.length);
			frame.mData.put(data);
			mMediaRecorder.write((26/*NV21*/ << 16) + BQLMediaPlayer.MEDIA_DATA_TYPE_YUV, BQLMediaRecorder.NO_PTS, frame);
		}
	}

	class RecordAudioThread extends Thread {
		@Override
		public void run() {
			int size = AudioRecord.getMinBufferSize(samplerate, channel, depth); 
			AudioRecord audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, samplerate, channel, depth, size * 2); 
			audioRecord.startRecording(); 
			int nlen = 1024 * (channel == AudioFormat.CHANNEL_CONFIGURATION_MONO? 1:2) * (depth != AudioFormat.ENCODING_PCM_16BIT? 1:2);
			while(mIsRecording) {
				BQLBuffer frame = mMediaRecorder.acquireBuffer(nlen);
				int nret = audioRecord.read(frame.mData, nlen);	
				assert(nret == nlen);
				mMediaRecorder.write(BQLMediaPlayer.MEDIA_DATA_TYPE_PCM, BQLMediaRecorder.NO_PTS, frame);
			}
			audioRecord.stop(); 
		}
	}

	// for config: not in used now , reserved 
	private static final String CFG = "config.xml" ; 
	
	@Override 
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		getWindow().requestFeature(Window.FEATURE_NO_TITLE);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON); 
//		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN); 
		
		// for test: post native message in other thread, instead of main thread
		mMediaThread = new HandlerThread("MediaThread"); 
		mMediaThread.start();
		
		// for data process: handle the data posted from native  
		mWorkerThread = new HandlerThread("MyWorkerThread");
		mWorkerThread.start();
		mWorkerHandler = new WorkerHandler(mWorkerThread.getLooper());
		
		// for data subscribe call back: we need to separate different kind of data to different sinker/handler
		//mDataListener = new MyDataAvailableListener();
		
		// for panorama: some products does NOT support panorama 
		if( CONFIG_PANORAMA ){
			setContentView(R.layout.activity_bql); 
			mBQLSv = (BQLVideoView)findViewById(R.id.bSurfaceView);
			mBQLSv.addCallback(new MyBQLViewCallback());

			// BQLSurfaceView can NOT drag 
//			mBQLSv.setOnTouchListener(new View.OnTouchListener() {
//
//					@Override
//					public boolean onTouch(View v, MotionEvent event) {							
//						if( event.getAction() == MotionEvent.ACTION_DOWN) {
//							if( mIsRecording) {
//								mCamera.autoFocus(new AutoFocusCallback() {
//								@Override
//								public void onAutoFocus(boolean success, Camera camera) {
//									Log.d("video", "AutoFocus: " + success);
//								}
//								});
//							}
//						}
//						return false;
//					}
//			});
	
		}else{
			setContentView(R.layout.activity_main); 
			mSv = (SurfaceView) findViewById(R.id.bSurfaceView);
			mSh = mSv.getHolder();
			mSh.setKeepScreenOn(true);
			mSh.addCallback(new MySurfaceCallback());

			mSv.setOnTouchListener(new View.OnTouchListener() {
				
					@Override
					public boolean onTouch(View v, MotionEvent event) {							
						if( event.getAction() == MotionEvent.ACTION_DOWN) {
							if( mIsRecording) {
								mCamera.autoFocus(new AutoFocusCallback() {
								@Override
								public void onAutoFocus(boolean success, Camera camera) {
									Log.d("video", "AutoFocus: " + success);
								}
								});
							}
						}
						return false;
					}
			});
		}

		mPcb = this;
	
		mSb = (SeekBar) findViewById(R.id.sbSeekVOD);
		mSb.setOnSeekBarChangeListener( new MySeekBarChangeListener() );
		mVODBtn = (Button) findViewById(R.id.bVod) ;			
		mPreviewBtn = (Button) findViewById(R.id.bLocalLive) ;	
		mStopBtn = (Button) findViewById(R.id.bStop) ;  		
		mPauseBtn =   (Button) findViewById(R.id.bPause) ;		
		mResumeBtn =(Button) findViewById(R.id.bResume)  ;		
		mAudioBtn =(Button) findViewById(R.id.bAudio)  ;	
		mRecIncomeBtn = (Button) findViewById(R.id.bRecIncome)  ;
		mRecWrBtn = (Button) findViewById(R.id.bRecWrite)  ;
		mSHBtn = (Button) findViewById(R.id.bSH)  ;
		mShotBtn = (Button) findViewById(R.id.bShot)  ;
		mUIHandler.sendEmptyMessage(StateHandler.INIT_STATE); 
					
		mVODBtn.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				Log.d(TAG ,"STREAM_URL_FILE_VOD = " + STREAM_URL_FILE_VOD );
				
				if( mSurface != null ){
					if(mMediaPlayer != null){
						stopMediaPlayer();
					}
					mUIHandler.sendEmptyMessage(StateHandler.START_VOD_STATE); 
					mMediaPlayer = new BQLMediaPlayer(CFG, PlaystreamActivity.this.getPackageName()  );
					mMediaPlayer.setDataSource(STREAM_URL_FILE_VOD );
					mMediaPlayer.setParams(NO_RTCP? "disablertcp=1":"disablertcp=0");
					mMediaPlayer.setHardwareDecode(hwcodecmode);
					mMediaPlayer.setSurface( mSurface );
					mMediaPlayer.setOnErrorListener(new MyOnErrorListener());
					mMediaPlayer.setOnPreparedListener(new BQLMediaPlayer.OnPreparedListener() {
						@Override
						public void onPrepared(BQLMediaPlayer mp, int what) {
							
							int duration = mMediaPlayer.getDuration() ; 
							int height =  mMediaPlayer.getVideoHeight();
							int width  =  mMediaPlayer.getVideoWidth();
							int channel = mMediaPlayer.getAudioChannel();
							int sample = mMediaPlayer.getAudioSampleRate();
							int depth = mMediaPlayer.getAudioDepth();
							
							Bundle data = new Bundle();
							data.putInt("h", height);
							data.putInt("w", width);
							data.putInt("d", duration);
							data.putInt("s", sample);
							data.putInt("c", channel);
							data.putInt("e", depth);
			
//							Message msg1 = mWorkerHandler.obtainMessage(WorkerHandler.WORK_SETUP_TRACK);
//							msg1.setData(data);
//							mWorkerHandler.sendMessage(msg1);
//							
//							mDataListener.setPCMSink(mWorkerHandler);
//							mMediaPlayer.setFeedbackDataType(BQLMediaPlayer.MEDIA_DATA_TYPE_PCM, true);
							if( duration == 0 )	
								mSb.setVisibility(mSb.INVISIBLE);
							else
								mSb.setVisibility(mSb.VISIBLE);
							mMediaPlayer.play(); 
							
							Message msg2 = mUIHandler.obtainMessage(StateHandler.DOING_VOD_STATE);
							msg2.setData(data);
							mUIHandler.sendMessage(msg2) ;
						}
					} );
					mMediaPlayer.setOnSeekCompleteListener(new BQLMediaPlayer.OnSeekCompleteListener() {
						@Override
						public void onSeekComplete(BQLMediaPlayer mp) {
							toastMessage("Seek Done"); 
						}
					});
					mMediaPlayer.setOnCompletionListener(new BQLMediaPlayer.OnCompletionListener() {
						@Override
						public void onCompletion(BQLMediaPlayer mp) {
							stopMediaPlayer();
						}
					});
					mMediaPlayer.prepareAsync();
				}else{
					toastMessage("Please wait for surface created");
				} 
			}
		});	
		
		
		mPreviewBtn.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				
				Log.d(TAG ,"STREAM_URL_FILE_LIVE = " + STREAM_URL_FILE_LIVE );
				
				if( mSurface != null ){
					
					mUIHandler.sendEmptyMessage(StateHandler.START_PREVIEW_STATE); 
					
					if(mMediaPlayer != null){
						stopMediaPlayer();
					}
					
					mMediaPlayer = new BQLMediaPlayer(CFG, PlaystreamActivity.this.getPackageName());
					mMediaPlayer.setDataSource(STREAM_URL_FILE_LIVE );
					mMediaPlayer.setParams(NO_RTCP? "disablertcp=1":"disablertcp=0");
					mMediaPlayer.setHardwareDecode(hwcodecmode);					
					mMediaPlayer.setSurface( mSurface );
					mMediaPlayer.setOnErrorListener(new MyOnErrorListener());
					mMediaPlayer.setOnPreparedListener(new BQLMediaPlayer.OnPreparedListener() {
						@Override
						public void onPrepared(BQLMediaPlayer mp, int what) {
							int duration = mMediaPlayer.getDuration(); 
							if( duration == 0 )	
								mSb.setVisibility(mSb.INVISIBLE);
							else
								mSb.setVisibility(mSb.VISIBLE);							
							mMediaPlayer.play(); 
							mUIHandler.sendEmptyMessage(StateHandler.DOING_PREVIEW_STATE); 
						}
					} );
					mMediaPlayer.setOnCompletionListener(new BQLMediaPlayer.OnCompletionListener() {
						@Override
						public void onCompletion(BQLMediaPlayer mp) {
							toastMessage("LIVE Complete Done"); 
							stopMediaPlayer();
						}
					});
					mMediaPlayer.prepareAsync();
				}else{
					Toast.makeText(PlaystreamActivity.this, "Please wait for surface created", Toast.LENGTH_LONG).show();
				}
			}
		});	



		mStopBtn.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				stopMediaPlayer();
			}
		});
		
		mPauseBtn.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				if( mMediaPlayer != null ){  
					mMediaPlayer.pause() ;
					mPauseBtn.setEnabled(false);
					mResumeBtn.setEnabled(true);
					mAudioBtn.setEnabled(false);
				}
			}
		});	

		
		
		mResumeBtn.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				if( mMediaPlayer != null ){ 
					mMediaPlayer.play();
					mPauseBtn.setEnabled(true);
					mResumeBtn.setEnabled(false);
					mAudioBtn.setEnabled(false);
				} 
			}
		});	
		
		mAudioBtn.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if( mAudioBtn.getText().equals( HC_OFF ) ){
					mAudioBtn.setText(HC_ON);
					hwcodecmode = 1;
				} else {
					mAudioBtn.setText(HC_OFF);
					hwcodecmode = 0;					
				}
			}
		});

		mRecIncomeBtn.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if( mRecIncomeBtn.getText().equals( REC_OFF ) ){
					mRecIncomeBtn.setText(REC_ON);
					if( mMediaRecorder != null){
						Log.e(TAG, "mMediaRecorder duplcate create");
						return ;
					}
					int duration = mMediaPlayer.getDuration() ; 
					int height =  mMediaPlayer.getVideoHeight();
					int width  =  mMediaPlayer.getVideoWidth();
					int channel = mMediaPlayer.getAudioChannel();
					int sample = mMediaPlayer.getAudioSampleRate();
					int depth = mMediaPlayer.getAudioDepth();
			 
					/*
					 * Note:
					 * 	local file prefix should be file://
					 * */
					SimpleDateFormat df = new SimpleDateFormat("yyyy_MM_dd_HH_mm_ss");
					String file_uri = String.format("file://mnt/sdcard/rec_%s.mp4", df.format(new Date())) ;
//					String file_uri = "rtmp://192.168.11.26/test/d5f742ff87bb4aaab0fa37e21d26ec1f";

					mMediaRecorder = new BQLMediaRecorder(mMediaPlayer, CFG , PlaystreamActivity.this.getPackageName()  );
					if(channel > 0 && sample > 0 && depth >  0){
						mMediaRecorder.setEncodeAudioParams(BQLMediaPlayer.MEDIA_DATA_TYPE_AAC , channel, depth, sample, 0); // 128kbps
					}else{
						Log.i(TAG, "no audio recode for incoming");
					}
					
					if(width > 0 &&  height > 0 ){
						mMediaRecorder.setEncodeVideoParams(BQLMediaPlayer.MEDIA_DATA_TYPE_H264, 20, width, height, 25, 0);
					}else{
						Log.i(TAG, "no video recode for incoming");
					}
					mMediaRecorder.setOnErrorListener(new MediaRecorderOnErrorListener());					
					mMediaRecorder.start( file_uri ) ;
					Log.d(TAG, "create incoming recoder done");
											
				}else{
					mRecIncomeBtn.setText(REC_OFF);
					if( mMediaRecorder != null){
						mMediaRecorder.StopAndRelease();
						mMediaRecorder = null; 
						Log.d(TAG, "destory live/vod recoder done");
					}
				}
			}
		});
		
		mSHBtn.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if( mSHBtn.getText().equals( SH_OFF ) ){
					mSHBtn.setText(SH_ON);
					mSb.setVisibility(mSb.INVISIBLE);
					mVODBtn.setVisibility(mVODBtn.INVISIBLE);
					mPreviewBtn.setVisibility(mPreviewBtn.INVISIBLE);
					mStopBtn.setVisibility(mStopBtn.INVISIBLE);
					mPauseBtn.setVisibility(mPauseBtn.INVISIBLE);
					mResumeBtn.setVisibility(mResumeBtn.INVISIBLE);
					mAudioBtn.setVisibility(mAudioBtn.INVISIBLE);
					mRecIncomeBtn.setVisibility(mRecIncomeBtn.INVISIBLE);
					mRecWrBtn.setVisibility(mRecWrBtn.INVISIBLE);
					mShotBtn.setVisibility(mShotBtn.INVISIBLE);
				} else {
					mSHBtn.setText(SH_OFF);
					mSb.setVisibility(mSb.VISIBLE);
					mVODBtn.setVisibility(mVODBtn.VISIBLE);
					mPreviewBtn.setVisibility(mPreviewBtn.VISIBLE);
					mStopBtn.setVisibility(mStopBtn.VISIBLE);
					mPauseBtn.setVisibility(mPauseBtn.VISIBLE);
					mResumeBtn.setVisibility(mResumeBtn.VISIBLE);
					mAudioBtn.setVisibility(mAudioBtn.VISIBLE);
					mRecIncomeBtn.setVisibility(mRecIncomeBtn.VISIBLE);
					mRecWrBtn.setVisibility(mRecWrBtn.VISIBLE);
					mShotBtn.setVisibility(mShotBtn.VISIBLE);					
				}
			}
		});

		mRecWrBtn.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if( mRecWrBtn.getText().equals( REC_WR_OFF ) ){
					if( mMediaRecorder != null){
						Log.e(TAG, "mMediaRecorder duplcate create");
						return ;
					}

					mAudioBtn.setEnabled(false); 
					mRecWrBtn.setText(REC_WR_ON);
					
					mIsRecording = true;

					mMediaRecorder = new BQLMediaRecorder(null, CFG, PlaystreamActivity.this.getPackageName() );
					mMediaRecorder.setEncodeAudioParams(BQLMediaPlayer.MEDIA_DATA_TYPE_AAC , channel == AudioFormat.CHANNEL_CONFIGURATION_MONO? 1:2, depth != AudioFormat.ENCODING_PCM_16BIT? 8:16, samplerate, 128000); // 128kbps					
					mMediaRecorder.setEncodeVideoParams(BQLMediaPlayer.MEDIA_DATA_TYPE_H264, 20, videow , videoh, 25, 800000);
					mMediaRecorder.setOnErrorListener(new MediaRecorderOnErrorListener());
					mMediaRecorder.start("rtmp://192.168.11.26/test/d5f742ff87bb4aaab0fa37e21d26ec1f");

					try{
					mCamera = Camera.open();  
					mCamera.setPreviewDisplay(mSh);  
					Parameters params = mCamera.getParameters();  
/*					List<Camera.Size> sizeLsts = params.getSupportedPreviewSizes();
					Iterator<Camera.Size> it = sizeLsts.iterator();
					while(it.hasNext()) {
						Camera.Size v = it.next();
						Log.i("video", v.width + "X" + v.height);
					} */
					params.setPreviewSize(videow, videoh);  
					params.setPreviewFormat(ImageFormat.NV21); 
					params.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
					mCamera.setParameters(params);  
					mCamera.startPreview();
					mCamera.setPreviewCallback(mPcb);

					}catch(Exception e){  
					e.printStackTrace();
					}
					
					mRecordAudioThread = new RecordAudioThread();
					mRecordAudioThread.start();
				} else {
					mAudioBtn.setEnabled(false);
					mRecWrBtn.setText(REC_WR_OFF);
					
					mIsRecording = false;
					
					mRecordAudioThread = null;

					mCamera.setPreviewCallback(null);	
					mCamera.stopPreview();
					mCamera.release();
					mCamera = null;
					
					if( mMediaRecorder != null){
						mMediaRecorder.StopAndRelease();
						mMediaRecorder = null; 
						Log.i(TAG, "destory live recoder done");
					}
				}
			}
		});
	
	
		mShotBtn.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				
				if(mMediaPlayer != null){
					mShotBtn.setEnabled(false); // until complete
					mMediaPlayer.setOnSnapShotListener(new OnSnapShotListener(){
						@Override
						public boolean onCompletion(BQLMediaPlayer mp) {
							toastMessage("snapshot done success !");
							mShotBtn.setEnabled(true); 
							return false;
						}
					});
					
					SimpleDateFormat df = new SimpleDateFormat("yyyy_MM_dd_HH_mm_ss");
					String file_uri = String.format("file://mnt/sdcard/snapshot_%s.jpg",   df.format(new Date())  ) ;
//					String file_uri = "file://mnt/sdcard/snapshot.jpg";
					if( mMediaPlayer.snapshot(0, 0, file_uri) ){
						toastMessage("snapshot request success !");
					}else{
						/* Fail Reason:
						 * 		a. last snapshot request has NOT finished yet (that is , take snapshot too frequently)
						 * 		b. it's recodering now 
						 * */
						toastMessage("snapshot request fail !");
						mShotBtn.setEnabled(true);  
					}
				}else{
					toastMessage("create player before snapshot");
				}
			}
		});
	
	}
	
	public void stopMediaPlayer()
	{
		if( mIsRecording ) {
			mIsRecording = false;
			mRecordAudioThread = null;

			mCamera.setPreviewCallback(null);	
			mCamera.stopPreview();
			mCamera.release();
			mCamera = null;
		}
		
		if( mMediaRecorder != null){
			mMediaRecorder.StopAndRelease();
			mMediaRecorder = null; 
			Log.d(TAG, "destory live/vod recoder done");
		}
		
		if(mMediaPlayer != null){
			mMediaPlayer.setFeedbackDataType(BQLMediaPlayer.MEDIA_DATA_TYPE_RGB, false);
			mMediaPlayer.setFeedbackDataType(BQLMediaPlayer.MEDIA_DATA_TYPE_PCM, false);
			mMediaPlayer.setDataAvailableListener(null);
			mWorkerHandler.sendEmptyMessage(WorkerHandler.WORK_RELEASE_TRACK );
			mWorkerHandler.sendEmptyMessage(WorkerHandler.WORK_WRITE_REC_STOP );
			mMediaPlayer.stop();
			mMediaPlayer.release();
			mMediaPlayer = null;
		}
		mUIHandler.sendEmptyMessage(StateHandler.INIT_STATE); 
	}
	
	private class MediaRecorderOnErrorListener implements BQLMediaRecorder.OnErrorListener
	{
		@Override
		public boolean onError(BQLMediaRecorder mp, int arg1, int arg2) {
			toastMessage("recorder occur ERROR: what=" + arg1 + " code=" + arg2 );
			return true;
		}
	}
	private class MyOnErrorListener implements BQLMediaPlayer.OnErrorListener
	{
		@Override
		public boolean onError(BQLMediaPlayer mp, int what, int extra) {
			Log.e(TAG, "ERROR : what = " + what + " extra = " + extra );
			switch(what){
			case BQLMediaPlayer.MEDIA_ERR_PAUSE:
				toastMessage("pause error");
				break;
			case BQLMediaPlayer.MEDIA_ERR_PLAY:
				toastMessage("play error");
				break;
			case BQLMediaPlayer.MEDIA_ERR_PREPARE:
				toastMessage("prepare error");
				break;
			case BQLMediaPlayer.MEDIA_ERR_SEEK:
				toastMessage("seek error");
				break;
			case BQLMediaPlayer.MEDIA_ERR_SHOT:
				toastMessage("snapshot error");
				// maybe it should check the current status, and then enable/disable mShotBtn here
				mUIHandler.sendEmptyMessage(StateHandler.FINISH_SNAPSHOT_FAIL); 
				break;

			case BQLMediaPlayer.MEDIA_ERR_NOSTREAM:
				toastMessage("no stream");
				break;

			default:
				toastMessage("unknown error");
				break;
			}
			
			if( what != BQLMediaPlayer.MEDIA_ERR_NOSTREAM )
			{
				stopMediaPlayer();
			}

			return true;
		}
	}
	
	
	private class MyBQLViewCallback implements  IBQLViewCallback {
		@Override
		public void surfaceChanged(Surface surface, int format, int width, int height) {
			Log.d(TAG, "surfaceChanged " + " format = " + format + " width = " + width + " height = " + height);
		}

		@Override
		public void surfaceCreated(Surface surface) {
			Log.d(TAG, "surfaceCreated created");
			
			// you can create BQLMediaPlayer here, too
			if(surface != null){
				toastMessage("Surface Created success");
				mSurface  = surface;
			}else{
				toastMessage("Surface Created fail ");
			}
		}

		@Override
		public void surfaceDestroyed(Surface surface) {
			Log.d(TAG, "surfaceCreated destroyed");
		}
	}
	
	private class MySurfaceCallback implements SurfaceHolder.Callback {
		@Override
		public void surfaceChanged(SurfaceHolder arg0, int arg1, int arg2, int arg3) {
			Log.d(TAG, "surfaceChanged " + " arg1 = " + arg1 + " arg2 = " + arg2 + " arg3 = " + arg3);
		}

		@Override
		public void surfaceCreated(SurfaceHolder holder) {
			Log.d(TAG, "surfaceCreated created");
			toastMessage("Surface Created");
			mSurface = holder.getSurface() ; 
		}

		@Override
		public void surfaceDestroyed(SurfaceHolder arg0) {
			Log.d(TAG, "surfaceCreated destroyed");
		}		
	}
	
	class MySeekBarChangeListener implements OnSeekBarChangeListener
	{
		public int last_change = -1 ;
		@Override
		public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
			if(fromUser){
				last_change = progress ;
			}
		}

		@Override
		public void onStartTrackingTouch(SeekBar seekBar) {
			
		}

		@Override
		public void onStopTrackingTouch(SeekBar seekBar) {
			if( last_change != -1){
				// default progress is 0 ~ 100
				// after mSb.setMax( mDuration ); progress is 0 ~ mDuration
				if(mMediaPlayer != null ){ // if VOD
					mMediaPlayer.seekTo(last_change);
					Log.d(TAG, "seekTo " + last_change );
				}
				last_change = -1 ;
			}
		}
	}
	
	@Override
	protected void onPause() {
		stopMediaPlayer();
		super.onPause();
	}

	@Override
	protected void onDestroy() {
		stopMediaPlayer();
		mMediaThread.quitSafely();
		mWorkerThread.quitSafely();
		super.onDestroy();
	}
}
