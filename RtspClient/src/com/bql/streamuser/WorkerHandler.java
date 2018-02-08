package com.bql.streamuser;

import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.Date;

import com.core.BQLBuffer;
import com.core.BQLMediaPlayer;
import com.core.BQLMediaRecorder;

import android.content.pm.ApplicationInfo;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

public class WorkerHandler extends Handler {

	public static final String TAG = "WorkerHandler" ;
	public static final int WORK_SETUP_TRACK 	= 0 ;
	public static final int WORK_RELEASE_TRACK 	= 1 ;
	public static final int WORK_NEXT_PCM 		= 2 ;
	public static final int WORK_WRITE_REC_START = 3 ;
	public static final int WORK_WRITE_REC_NEXT_RGB  = 4 ;
	public static final int WORK_WRITE_REC_NEXT_PCM  = 5 ;
	public static final int WORK_WRITE_REC_STOP  = 6 ;
 
	public AudioTrack mAudioTrack = null ;
	public BQLMediaRecorder mWrRecoder = null;
	
	public WorkerHandler(Looper looper) {
		super(looper);
	}

	@Override
	public void handleMessage(Message msg) {
		
		switch( msg.what ){
		case WORK_SETUP_TRACK:
			if( mAudioTrack != null ){
				Log.e(TAG, "duplcate create audio track 1");
				Log.e(TAG, "duplcate create audio track 2");
				Log.e(TAG, "duplcate create audio track 3");
				return ;
			}
			{
				Bundle data		= msg.getData() ; 
				int sampleRate	= data.getInt("s");
				int channel 	= data.getInt("c");
				int depth 		= data.getInt("e") ;
	
				int config_channel = AudioFormat.CHANNEL_OUT_STEREO; 
				int config_depth 	= AudioFormat.ENCODING_PCM_16BIT ;
				
	 			if(channel == 1){
	 				config_channel = AudioFormat.CHANNEL_OUT_MONO ;
				}
				if(depth == 8){
					config_depth = AudioFormat.ENCODING_PCM_8BIT ;
				}
				
				Log.d(TAG,  " sample:" + sampleRate + 
							" channel:" + (config_channel==AudioFormat.CHANNEL_OUT_MONO?1:2) +
							" depth:" + (config_depth == AudioFormat.ENCODING_PCM_8BIT?8:16) );
				
				int bufferSize = AudioTrack.getMinBufferSize(sampleRate, config_channel, config_depth);
				mAudioTrack = new AudioTrack( 	AudioManager.STREAM_MUSIC ,  
												sampleRate,  
												config_channel,  
												config_depth,  
												bufferSize,  
												AudioTrack.MODE_STREAM); 
				
				mAudioTrack.setVolume(1.0f);
				mAudioTrack.play();
			}
			return ;
			
		case WORK_RELEASE_TRACK:
			if( mAudioTrack != null){
				mAudioTrack.stop();
				Log.d(TAG, "stop track");
				mAudioTrack.release();
				mAudioTrack = null;
			}
			return ;
			
		case WORK_NEXT_PCM:
			
			BQLBuffer bqlBuffer = (BQLBuffer)msg.obj;
			
			if( mAudioTrack != null ) {
				if(Build.VERSION.SDK_INT >= 21  ){ // Build.VERSION_CODES.LOLLIPOP Android 5 
					/* 
					 * Note:
					 * 		set		(int) if compile in armeabi-v7a/armeabl and running on arm64 
					 * 		cancel	(int) if compile in arm64-v8a  and running on arm64 
					 */ 
					Log.d(TAG, "play audio: " + Integer.toHexString( (int)bqlBuffer.mNativeBufferThiz ));
					mAudioTrack.write(bqlBuffer.mData, bqlBuffer.mData.remaining() , AudioTrack.WRITE_BLOCKING);
				}else{
					ByteBuffer bb = ByteBuffer.allocate(bqlBuffer.mData.remaining());
					bb.put(bqlBuffer.mData);
					bb.flip();
					Log.i(TAG, "play audio: " + Long.toHexString( bqlBuffer.mNativeBufferThiz ));
					mAudioTrack.write(bb.array(), bb.position(), bb.remaining());
				}
			}else{
				Log.e(TAG, "play audio but AudioTrack is null:" + Integer.toHexString( (int)bqlBuffer.mNativeBufferThiz ));
			}
			/*
			 * Note:
			 * 		make sure release the buffer in time when not used 
			 * 		otherwise OUT OF MEMORY
			 */
			bqlBuffer.releaseBuffer();  	
			return ;
			
			
		case WORK_WRITE_REC_START:
		{
			if( mWrRecoder != null){
				Log.e(TAG, "mWrRecoder duplcate create");
				return ;
			}

			Bundle data = msg.getData() ;
			int height  = data.getInt("h");
			int width   = data.getInt("w");
			int sample  = data.getInt("s");
			int channel = data.getInt("c");
			int depth 	= data.getInt("e");
			String cfg  = data.getString("cfg");
			String name = data.getString("name");
			Log.i(TAG, "height = " + height + " width " + width + " sample " + sample 
								+ " channel " + channel + " depth " + depth
								+ " cfg " + cfg + " name " + name );
			/*
			 * Note:
			 * 	local file prefix should be file://
			 * */
			SimpleDateFormat df = new SimpleDateFormat("yyyy_MM_dd_HH_mm_ss");
			String file_uri = String.format("file://mnt/sdcard/wr_recoder_%s.mp4",   df.format(new Date())  ) ;
		 
			mWrRecoder = new BQLMediaRecorder(null, cfg , name );
			if(channel > 0 && sample > 0 && depth >  0){
				mWrRecoder.setEncodeAudioParams(BQLMediaPlayer.MEDIA_DATA_TYPE_AAC , channel, depth, sample, 128000); // 128kbps
			}else{
				Log.i(TAG, "no audio recode for Write Recoder");
			}
			
			if(width > 0 &&  height > 0 ){
				mWrRecoder.setEncodeVideoParams(BQLMediaPlayer.MEDIA_DATA_TYPE_H264, 20, width , height , 25 , 2000000); // 25
			}else{
				Log.i(TAG, "no video recode for  Write Recoder");
			}
			mWrRecoder.start( file_uri ) ;
			Log.d(TAG, "create incoming recoder done");
		}
		break;
			
		case WORK_WRITE_REC_NEXT_RGB:
		{
			BQLBuffer rgb = (BQLBuffer)msg.obj;
			
			ByteBuffer src = rgb.mData ;
			long pts = rgb.mArg1 ; 
			int total_size = src.remaining() ;
			
			BQLBuffer acquire = mWrRecoder.acquireBuffer(total_size);
			ByteBuffer dst = acquire.mData ;
			
			/*
			 *  do something else (e.g process the data ) 
			 * */
			int actual_length = 0 ;
			Log.d(TAG, "acquire 1 [0x" + Long.toHexString( 0x00000000FFFFFFFFL & acquire.mNativeBufferThiz ) + "] src [" + 
						" " + Integer.toHexString(0xFF&src.get(0)) + 
						" " + Integer.toHexString(0xFF&src.get(1)) + 
						" " + Integer.toHexString(0xFF&src.get(2)) + 
						" " + Integer.toHexString(0xFF&src.get(3)) + 
						" " + Integer.toHexString(0xFF&src.get(4)) +"]" + 
						"[" + Integer.toHexString(0xFF&src.get(src.remaining() - 3 )) +
						" " + Integer.toHexString(0xFF&src.get(src.remaining() - 2 )) +
						" " + Integer.toHexString(0xFF&src.get(src.remaining() - 1 )) + "]"
					);
			
//			while( src.remaining() > 0 ){
//				dst.put( src.get() ) ;
//				actual_length ++ ;
//			}
			
			dst.put(src) ;
			actual_length = total_size ; // just for demo
			
			Log.d(TAG, "acquire 2 [0x" + Long.toHexString( 0x00000000FFFFFFFFL & acquire.mNativeBufferThiz ) + "]" + "rgb pts = " + pts + " total_size = " + total_size + " pos = " + dst.position() );
			if(  actual_length != total_size ){
				Log.w(TAG, "why ? actual_length = " + actual_length + " total_size = " + total_size );
			}
			mWrRecoder.setBufferSize(acquire, actual_length) ; 
			mWrRecoder.write(BQLMediaPlayer.MEDIA_DATA_TYPE_RGB, pts, acquire);
			
			rgb.releaseBuffer(); 
		}
		break;
		
		case WORK_WRITE_REC_NEXT_PCM:
		{
			BQLBuffer pcm = (BQLBuffer)msg.obj;
			ByteBuffer src = pcm.mData ;
			long pts = pcm.mArg1 ; 
			int total_size = src.remaining() ;
			BQLBuffer acquire = mWrRecoder.acquireBuffer(total_size);
			ByteBuffer dst = acquire.mData ;
			
			int actual_length = 0 ;
//			while( src.remaining() > 0 ){
//				dst.put( src.get() ) ;
//				actual_length ++ ;
//			}
			dst.put(src) ;
			actual_length = total_size ; // just for demo
			
			Log.d(TAG, "pcm pts = " + pts + " total_size = " + total_size + " pos = " + dst.position() );
			if(  actual_length != total_size ){
				Log.w(TAG, "why ? actual_length = " + actual_length + " total_size = " + total_size );
			}
			mWrRecoder.setBufferSize(acquire, total_size);
			mWrRecoder.write(BQLMediaPlayer.MEDIA_DATA_TYPE_PCM, pts, acquire);
			
			pcm.releaseBuffer(); 
		}
		break;

		case WORK_WRITE_REC_STOP:
		{
			if( mWrRecoder != null){
				mWrRecoder.StopAndRelease();
				mWrRecoder = null; 
				Log.d(TAG, "destory write recoder done");
			}
		}
		break;
		
		default:
			Log.e(TAG, "unknown msg , please check it  = " + msg.what );
			break;
		}
		
		super.handleMessage(msg);
	}
	
	
}
