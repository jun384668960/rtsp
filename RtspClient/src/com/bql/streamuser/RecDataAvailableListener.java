package com.bql.streamuser;

import com.core.BQLBuffer;
import com.core.BQLMediaPlayer;

import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class RecDataAvailableListener implements BQLMediaPlayer.OnDataAvailableListener
{
	private final String TAG = "RecDataAvailableListener";
	
 
	private Handler mRecSink= null;
	
	public RecDataAvailableListener(){
		
	}
	public void setRecSink(Handler handler)
	{
		mRecSink = handler; 
	}


	@Override
	public void onData(BQLMediaPlayer mp, BQLBuffer data) { // Non-Block 
		
		// Log.d(TAG, "thread name " + Thread.currentThread().getName() );
		switch( data.mDataType  ){
		case BQLMediaPlayer.MEDIA_DATA_TYPE_PCM:
			if(mRecSink != null){
				Message msg = mRecSink.obtainMessage( WorkerHandler.WORK_WRITE_REC_NEXT_PCM ); 
				msg.obj = data ; 
				mRecSink.sendMessage(msg);
			}else{
				Log.e(TAG, "do NOT require PCM data to recoder now");
				data.releaseBuffer(); 
			}
			break;
		case BQLMediaPlayer.MEDIA_DATA_TYPE_RGB:
			if(mRecSink != null){
				Message msg = mRecSink.obtainMessage( WorkerHandler.WORK_WRITE_REC_NEXT_RGB ); 
				msg.obj = data ; 
				mRecSink.sendMessage(msg);
			}else{
				Log.e(TAG, "do NOT require RGB data to recoder now");
				data.releaseBuffer(); 
			} 
			
			break;
		default:
			/*
			 * Note:
			 * 		make sure release the buffer in time when not used 
			 * 		otherwise OUT OF MEMORY
			 */
			Log.e(TAG, "post not-required data , please check code , data_type = " +  data.mDataType );
			data.releaseBuffer(); 
			break;
		
		}    
	}

}
