package com.core;

import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Surface; 

public class BQLMediaPlayer
{
	private final static String TAG = "BQLMediaPlayer" ;
	private long mJNIContext = 0 ;
	public long mPlayerContext = 0 ;
	private EventHandler mEventHandler;
	
	public BQLMediaPlayer(String cfg , String packetName)
	{
        Looper looper;
        if ((looper = Looper.myLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else if ((looper = Looper.getMainLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else {
            mEventHandler = null;
        }
        
		mJNIContext = native_malloc( cfg , packetName );
	}
	
	public boolean setDataSource(String path){
		if(path == null || path.length() == 0 ){
			Log.e(TAG, "path is null !");
			return false;
		}

		if( mPlayerContext != 0 ) native_stop(mPlayerContext);

		mPlayerContext = native_setup(mJNIContext , new WeakReference<BQLMediaPlayer>(this) , path);
		return (mPlayerContext != 0) ? true : false;
	}
	
	public boolean setParams(String value) {
		return native_setParams(mPlayerContext, value);
	}

	//mode=0: software mode=1: hardware
	public void setHardwareDecode(int mode)
	{
		native_setHardwareDecode(mPlayerContext, mode);
	}

	public boolean setSurface(Surface surface) {
		return native_setDisplay(mPlayerContext, surface);
	}
	
	public void prepareAsync()
	{
		native_prepareAsync(mPlayerContext);
	}
	
	public boolean setVolume(float left , float right ){
		
		return native_setVolume(mPlayerContext , left , right ) ; 
	}
	
	public boolean setMute(boolean on){
		return native_setMute(mPlayerContext , on );
	}
	
	public int getPlayBufferTime()
	{
		return native_getPlayBufferTime( mPlayerContext);
	}	
	public boolean setPlayBufferTime(int time){
		return native_setPlayBufferTime(mPlayerContext , time );
	}	
	
	public void play()
	{
		native_play(mPlayerContext);
	}
	
	public void seekTo( int msec ){
		native_seekTo(mPlayerContext , msec);
	}
	
	public int getDuration()
	{
		return native_getDuration( mPlayerContext);
	}
	
	public int getCurrentPosition()
	{
		return native_getCurrentPosition(mPlayerContext);
	}
	
	public boolean pause()
	{
		return native_pause(mPlayerContext);
	}
	
	/* Note:
	 * 	width/height:	
	 * 		width = 0 , height = 0  代表原图大小拍照
	 *	uri:
	 *		file://mnt/sdcard/temp.jpg 
	 * 	return :
	 * 		返回 false
	 * 		a. 还没有完成上一次 拍照 继续调用   
	 */
	public boolean snapshot(int width , int height , String uri)
	{
		return native_snapshot(mPlayerContext , width , height , uri);
	}

	public void stop()
	{
		if( mPlayerContext != 0 ) {
			native_stop(mPlayerContext);
			mPlayerContext = 0L;
		}
	}
	
	public void release()
	{
		if( mJNIContext != 0 ) {
			native_free(mJNIContext);
			mJNIContext = 0L;
		}
	}

	public int getVideoWidth()
	{
		return native_getVideoWidth( mPlayerContext);
	}
	
	public int getVideoHeight()
	{
		return native_getVideoHeight(mPlayerContext);
	}
	
	public int getAudioChannel()
	{
		return native_getAudioChannel(mPlayerContext);
	}
	
	public int getAudioDepth()
	{
		return native_getAudioDepth(mPlayerContext);
	}
	
	public int getAudioSampleRate()
	{
		return native_getAudioSampleRate (mPlayerContext);
	}
	
	
	private native long native_malloc(String cfg, String packageName );
	private native long native_setup(long streamer_ctx, Object mediaplayer_this,  String uri );
	private native boolean native_setParams(long player_ctx, String value );
	private native boolean native_setDisplay(long player_ctx, Surface surface );
	private native void native_prepareAsync(long player_ctx);
	private native boolean native_play(long player_ctx );
	private native boolean native_setVolume(long player_ctx , float left , float right );
	private native boolean native_setMute(long player_ctx , boolean on);
	private native int     native_getPlayBufferTime(long player_ctx );	
	private native boolean native_setPlayBufferTime(long player_ctx , int time);
	private native boolean native_seekTo(long player_ctx , int msec);
	private native void native_setData(long player_ctx , int data_type , boolean isOn);
	private native boolean native_pause(long player_ctx);
	private native boolean native_snapshot(long player_ctx , int width , int height , String uri_path);	
	private native void native_stop(long player_ctx);
	private native void native_free(long streamer_ctx );
	private native void native_setHardwareDecode(long player_ctx, int mode);

	private native int  native_getDuration(long player_ctx );
	private native int  native_getCurrentPosition(long player_ctx );
	private native int  native_getAudioChannel(long player_ctx );
	private native int  native_getAudioDepth(long player_ctx );
	private native int  native_getAudioSampleRate(long player_ctx );
	private native int  native_getVideoWidth(long player_ctx );
	private native int  native_getVideoHeight(long player_ctx );
	
	/**
	  * Do not change these values without updating their counterparts
	  * in jni/src/com_bql_MSG_Native.h
	  */
	public static final int MEDIA_STATUS					= 0 ;
	public static final int MEDIA_PREPARED 					= MEDIA_STATUS + 1 ;
	public static final int MEDIA_SEEK_COMPLETED 			= MEDIA_STATUS + 2 ;
	public static final int MEDIA_PLAY_COMPLETED 			= MEDIA_STATUS + 3 ;
	public static final int MEDIA_SHOT_COMPLETED			= MEDIA_STATUS + 4 ;
	public static final int MEDIA_SHOWFIRSTFRAME			= MEDIA_STATUS + 5 ;
	
	public static final int MEDIA_DATA						= 100 ;					
	public static final int MEDIA_DATA_TYPE_H264			= MEDIA_DATA + 1 ;
	public static final int MEDIA_DATA_TYPE_AAC				= MEDIA_DATA + 2 ;
	public static final int MEDIA_DATA_TYPE_PCM				= MEDIA_DATA + 3 ;
	public static final int MEDIA_DATA_TYPE_RGB				= MEDIA_DATA + 4 ;
	public static final int	MEDIA_DATA_TYPE_JPG				= MEDIA_DATA + 5 ; // only for BQLMediaRecoder.write
	public static final int	MEDIA_DATA_TYPE_YUV				= MEDIA_DATA + 6 ;

	public static final int MEDIA_INFO						= 400 ; 
	public static final int MEDIA_INFO_PAUSE_COMPLETED 		= MEDIA_INFO + 5 ;
	
	public static final int MEDIA_ERR						= 500 ; 
	public static final int MEDIA_ERR_SEEK					= MEDIA_ERR + 1 ;
	public static final int MEDIA_ERR_PREPARE				= MEDIA_ERR + 2 ;
	public static final int MEDIA_ERR_PAUSE					= MEDIA_ERR + 3 ;
	public static final int MEDIA_ERR_PLAY					= MEDIA_ERR + 4 ;	
	public static final int MEDIA_ERR_SHOT					= MEDIA_ERR + 5 ;	
	public static final int MEDIA_ERR_NOSTREAM				= MEDIA_ERR + 6 ;	

	public static final int FRAME_TYPE_P					= 1 ;	
	public static final int FRAME_TYPE_I					= 5 ;	
	public static final int FRAME_TYPE_SPS					= 7 ;	
	public static final int FRAME_TYPE_PPS					= 8 ;	
	
    public interface OnPreparedListener
    {
        void onPrepared(BQLMediaPlayer mp, int what);
    }
    public void setOnPreparedListener(OnPreparedListener listener)
    {
        mOnPreparedListener = listener;
    }
    private OnPreparedListener mOnPreparedListener;
 
    public interface OnSeekCompleteListener
    {
        public void onSeekComplete(BQLMediaPlayer mp);
    }
    public void setOnSeekCompleteListener(OnSeekCompleteListener listener)
    {
        mOnSeekCompleteListener = listener;
    }
    private OnSeekCompleteListener mOnSeekCompleteListener ;

    public interface OnFirstFrameListener
    {
        public void OnFirstFrame(BQLMediaPlayer mp);
    }
    public void setOnFirstFrameListener(OnFirstFrameListener listener)
    {
        mOnFirstFrameListener = listener;
    }
    private OnFirstFrameListener mOnFirstFrameListener ;

    public interface OnCompletionListener
    {
        void onCompletion(BQLMediaPlayer mp);
    }
    public void setOnCompletionListener(OnCompletionListener listener)
    {
        mOnCompletionListener = listener;
    }
    private OnCompletionListener mOnCompletionListener;

	public interface OnSnapShotListener
	{
		 boolean onCompletion(BQLMediaPlayer mp);
	}
    public void setOnSnapShotListener(OnSnapShotListener listener)
    {
    	mOnSnapShotListener = listener;
    }
    private OnSnapShotListener mOnSnapShotListener;

	public interface OnInfoListener
	{
		 boolean onInfo(BQLMediaPlayer mp, int what, int extra);
	}
    public void setOnInfoListener(OnInfoListener listener)
    {
        mOnInfoListener = listener;
    }
    private OnInfoListener mOnInfoListener;
    
    /*
     * 		what					extra		 
     * 		MEDIA_ERR_SEEK			-
	 *		MEDIA_ERR_PREPARE 		-		
	 *
     */
    public interface OnErrorListener
    {
        boolean onError(BQLMediaPlayer mp, int what, int extra);
    }
    public void setOnErrorListener(OnErrorListener listener)
    {
        mOnErrorListener = listener;
    }
    private OnErrorListener mOnErrorListener;
    

    public void setFeedbackDataType(int data_type , boolean isON ){
    	
    	if(data_type != MEDIA_DATA_TYPE_AAC 
    		&& data_type != MEDIA_DATA_TYPE_H264
    		&& data_type != MEDIA_DATA_TYPE_PCM 
    		&& data_type != MEDIA_DATA_TYPE_RGB
    		&& data_type != MEDIA_DATA_TYPE_YUV
    		){
    		Log.e(TAG, "unsupprot data_type " + data_type );
    		Log.e(TAG, "unsupprot data_type " + data_type );
    		Log.e(TAG, "unsupprot data_type " + data_type );
    		throw new java.lang.IllegalArgumentException("unsupprot data type");
    	}
    	native_setData(mPlayerContext , data_type , isON);
    }
    
    public interface OnDataAvailableListener
    {
        void onData(BQLMediaPlayer mp , BQLBuffer data);
    }
    public void setDataAvailableListener(  OnDataAvailableListener listener)
    {
    	mOnDataListener = listener ;
    }
    private OnDataAvailableListener mOnDataListener;


    private class EventHandler extends Handler
    {
        private BQLMediaPlayer mMediaPlayer;

        public EventHandler(BQLMediaPlayer mp, Looper looper) {
            super(looper);
            mMediaPlayer = mp;
        }
        
        @Override
        public void handleMessage(Message msg) {
        	
        	 switch(msg.what) {
        	 	// case MEDIA_STATUS:
	             case MEDIA_PREPARED:
	                 if (mOnPreparedListener != null){
	                     mOnPreparedListener.onPrepared(mMediaPlayer, msg.what);
	                 }
	                 break;
	             case MEDIA_SEEK_COMPLETED:
	                 if (mOnSeekCompleteListener != null) {
	                     mOnSeekCompleteListener.onSeekComplete(mMediaPlayer);
	                 }
	                 break;
	             case MEDIA_PLAY_COMPLETED:
	                 if (mOnCompletionListener != null) {
	                	 mOnCompletionListener.onCompletion(mMediaPlayer);
	                 }
	                 break;
	 
	             case MEDIA_SHOT_COMPLETED:
	                 if (mOnSnapShotListener != null) {
	                	 mOnSnapShotListener.onCompletion(mMediaPlayer);
	                 }
	                 break;
				 case MEDIA_SHOWFIRSTFRAME:
	                 if (mOnFirstFrameListener != null) {
	                	 mOnFirstFrameListener.OnFirstFrame(mMediaPlayer);
	                 }
	                 break;

	              // case MEDIA_ERR:
	             case MEDIA_ERR_SEEK:
	             case MEDIA_ERR_PLAY:
	             case MEDIA_ERR_PREPARE:
	             case MEDIA_ERR_PAUSE:
	             case MEDIA_ERR_SHOT:
	             case MEDIA_ERR_NOSTREAM:
	            	 if( mOnErrorListener != null){
	            		 mOnErrorListener.onError(mMediaPlayer, msg.what, msg.arg1);
	            	 }
	            	 break;
	             // case MEDIA_DATA:
	             case MEDIA_DATA:
	            	 if( mOnDataListener != null){
	            		 mOnDataListener.onData(mMediaPlayer, (BQLBuffer)msg.obj);
	            	 }else{
					     BQLBuffer buffer = (BQLBuffer)msg.obj; 					 
	            		 int data_type = buffer.mDataType ;
	            		 Log.e(TAG, "1-mOnDataListener is null but feedback data_type = " + data_type);
	            		 Log.e(TAG, "2-mOnDataListener is null but feedback data_type = " + data_type);
	            		 Log.e(TAG, "3-mOnDataListener is null but feedback data_type = " + data_type);
	            		 // 强制执行关闭feedback
	            		 // native_setData(mJNIContext , data_type , false);
	            		 // 如果没有监听数据 ,立刻释放buffer 
	            		 buffer.releaseBuffer();
	            	 }
	            	 break;
	             default:
	                 Log.e(TAG, "1-Unknown message type " + msg.what);
	                 Log.e(TAG, "2-Unknown message type " + msg.what);
	                 Log.e(TAG, "3-Unknown message type " + msg.what);
	                 return;
        	 }
        }
    }
    
    /*		
     * 		native post message format as followed :
     * 
     * 		what					arg1			arg2			obj
     * 
     * 		MEDIA_PREPARED			-				-				-
     * 		MEDIA_SEEK_COMPLETED	-				-				-
     * 		MEDIA_PLAY_COMPLETED	-				-				-
     * 
     * 		MEDIA_ERR_SEEK			int (if needed) -				-
     * 		MEDIA_ERR_PREPARE		int (if needed) -				-	
     * 		MEDIA_ERR_PAUSE			int (if needed) -				-		 
	 *		MEDIA_ERR_PLAY			int (if needed) -				-		 
     * 
	 *		MEDIA_DATA_TYPE_H264	-				-				BQLBuffer
	 *		MEDIA_DATA_TYPE_AAC		-				-				BQLBuffer
	 *		MEDIA_DATA_TYPE_PCM		-				-				BQLBuffer
	 *		MEDIA_DATA_TYPE_YUV		-				-				BQLBuffer
	 *		MEDIA_DATA_TYPE_RGB		-				-				BQLBuffer	
     * 		
     * */
    private static void postEventFromNative(Object mediaplayer_ref, int what, int arg1, int arg2, Object obj)
    {
    	BQLMediaPlayer mp = (BQLMediaPlayer)((WeakReference)mediaplayer_ref).get();
    	if (mp == null) {
    		Log.e(TAG, "postEventFromNative: Null mp! what=" + what + ", arg1=" + arg1 + ", arg2=" + arg2);
    		return;
    	}
    	
    	if (mp.mEventHandler != null) {
    		Message m = mp.mEventHandler.obtainMessage(what, arg1, arg2, obj);
    		mp.mEventHandler.sendMessage(m);
    	}
    } 
    
	static {
		System.loadLibrary("streamuser");
	}
}
