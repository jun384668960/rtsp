package com.core;

import java.lang.ref.WeakReference;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

public class BQLMediaRecorder {
	private final static String TAG = "BQLMediaRecorder" ;
	private EventHandler mEventHandler;
	private long mJNIContext = 0 ;
	private long mRecorderContext = 0 ;
	private long mPlayerContext = 0 ;

	private int mTimeScale = 0 ;
	private int mWidth = 0 ;
	private int mHeight = 0 ;
	private int mQval = 0 ;
	private int mVideoBitrate = 0 ;
	private int mVideoType = -1 ;
 
	private int mChannel = 0;
	private int mDepth = 0;
	private int mSamplerate = 0;
	private int mAudioBitrate = 0; 
	private int mAudioType = -1 ;
 
	public BQLMediaRecorder(BQLMediaPlayer player, String cfg , String packetName)
	{
        Looper looper;
        if ((looper = Looper.myLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else if ((looper = Looper.getMainLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else {
            mEventHandler = null;
        }

		if( player == null ) {
			mPlayerContext = 0 ; 
		}else{
			mPlayerContext = player.mPlayerContext ;
		}
		mJNIContext = native_malloc( cfg , packetName );
	}
	
	/*
	 * type:
	 *  	MediaPlayer.MEDIA_DATA_TYPE_H264  
	 *  	MediaPlayer.MEDIA_DATA_TYPE_AAC				 
	 *  	MediaPlayer.MEDIA_DATA_TYPE_JPG			 
	 */
	public void setEncodeAudioParams(int type , int channel , int depth , int samplerate , int bitrate )
	{
		mAudioType 	= type ;
		mChannel 	= channel;
		mDepth 		= depth;
		mSamplerate = samplerate;
		mAudioBitrate = bitrate; 
	}
	
	public void setEncodeVideoParams(int type , int fps, int w , int h , int qval/* h264: 1~ 25 */, int bitrate) 
	{
		mVideoType 	= type ;
		mTimeScale	= fps;
		mWidth 		= w;
		mHeight 	= h;
		mQval 		= qval;  
		mVideoBitrate = bitrate ;
	}

    public interface OnErrorListener
    {
        boolean onError(BQLMediaRecorder mp, int arg1, int arg2);
    }

    public void setOnErrorListener(OnErrorListener listener)
    {
        mOnErrorListener = listener;
    }

    private OnErrorListener mOnErrorListener;

    private class EventHandler extends Handler
    {
        private BQLMediaRecorder mMediaRecorder;

        public EventHandler(BQLMediaRecorder mp, Looper looper) {
            super(looper);
            mMediaRecorder = mp;
        }

		@Override
		public void handleMessage(Message msg) {      	
			if( mOnErrorListener != null) {
				Log.e(TAG, "onError: " + msg.what + ", " + msg.arg1);
				if( msg.what == 0 )
					mOnErrorListener.onError(mMediaRecorder, msg.what, msg.arg1);
			}
		}
    }

    private static void postEventFromNative(Object mediarecorder_ref, int what, int arg1)
    {
    	BQLMediaRecorder mp = (BQLMediaRecorder)((WeakReference)mediarecorder_ref).get();
    	if (mp == null) {
    		return;
    	}
  
    	if (mp.mOnErrorListener != null) {
    		Message m = mp.mEventHandler.obtainMessage(what, arg1, 0, 0);
			mp.mEventHandler.sendMessage(m);
    	}
    }

	public boolean start(String path ){
		boolean start_ok = false ;
		
		if( mVideoType <= 0 && mAudioType <= 0 ){
			return false ;
		}
		
		if( mRecorderContext != 0 ) native_deleteRecorder(mRecorderContext);

		mRecorderContext = native_createRecorder( new WeakReference<BQLMediaRecorder>(this), mPlayerContext , path , mVideoType , mAudioType );		
		if( mRecorderContext == 0 ){
			return false; 
		}
		
		if( mVideoType > 0 ){
			start_ok = native_setVideoStream(mRecorderContext , mTimeScale ,  mWidth ,  mHeight ,  mQval ,mVideoBitrate);
			if(!start_ok) return false ;
		}
		
		if( mAudioType > 0 ){
			start_ok = native_setAudioStream(mRecorderContext , mChannel , mDepth , mSamplerate  , mAudioBitrate );
			if(!start_ok) return false ;
		}
		
		start_ok = native_prepareWrite(mRecorderContext );
		if(!start_ok) return false ;
		
		return true ;
	}
	
	public BQLBuffer acquireBuffer(int total_size)
	{
		return (BQLBuffer)native_acquireBuffer(mJNIContext , total_size);
	}

	
	public long setBufferSize(BQLBuffer buffer , int actual_size )
	{
		return native_setBufferSize( buffer.mNativeBufferThiz , actual_size );
	}
	
	
	/*
	 * Note:
	 * 		write(xxxx, NO_PTS , xxxxx)
	 * 		pts will set to current time in JNI layer
	 * 
	 * 		keep the value same as AV_NOPTS_VALUE @ libavutil/avutil.h 
	 * */
	public static final long  NO_PTS      =    0x8000000000000000L;
	
	/*
	 * Note:
	 * 		1. it's not necessary to call BQLBuffer.releaseBuffer after write
	 * 		2. BQLBuffer.mArg[X]/mDataType is not used in Native Level in this case
	 * 		3. Fill your data(aac/h264..) to BQLBuffer.mData
	 * */
	public void write( int type, long pts , BQLBuffer buffer ){
		native_write(mRecorderContext, type , pts , buffer.mNativeBufferThiz  );
		buffer.mNativeBufferThiz = 0; // mark release 
	}

	//返回值单位: 毫秒
	public int getRecordTime()
	{
		if( mRecorderContext == 0L ) return 0;
		return native_getRecordTime(mRecorderContext);
	}

	public void StopAndRelease()
	{
		mOnErrorListener = null;
		if(mRecorderContext != 0){
			native_deleteRecorder(mRecorderContext);
			mRecorderContext = 0L ;
		}
		if(mJNIContext != 0){
			native_free(mJNIContext);
			mJNIContext = 0L;
		}
	}

	private native long 	native_malloc(String cfg , String packageName  );
	private native long  	native_createRecorder(Object mediarecorder_this, long cplayer , String uri , int vcodecid , int acodecid);
	private native boolean 	native_setVideoStream(long rplayer , int fps,  int width,  int height,  int qval , int bitrate);
	private native boolean 	native_setAudioStream(long rplayer , int channel , int depth , int samplerate , int bitrate );
	private native boolean 	native_prepareWrite(long rplayer);
	private native boolean 	native_write(long rplayer, int codecid, long pts, long buf );
	private native int 		native_getRecordTime(long rplayer);
	private native void 	native_deleteRecorder(long rplayer);
	private native void 	native_free(long ctx );
	private native Object 	native_acquireBuffer(long ctx , int total_size);
	private native long 	native_setBufferSize(long cbuffer , int actual_size);
	
	static {
		System.loadLibrary("streamuser");
	}	
}
