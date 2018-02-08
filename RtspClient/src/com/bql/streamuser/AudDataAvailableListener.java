package com.bql.streamuser;

import com.core.BQLBuffer;
import com.core.BQLMediaPlayer;

import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class AudDataAvailableListener implements BQLMediaPlayer.OnDataAvailableListener
{
	private final String TAG = "MyDataAvailableListener";
	
	private Handler mPCMSink= null ; 
	private Handler mRGBSink= null;
	
	public AudDataAvailableListener(){
		
	}
	public void setPCMSink(Handler handler)
	{
		mPCMSink = handler; 
	}
	
	public void setRGBSink(Handler handler)
	{
		mRGBSink = handler; 
	}
	
	@Override
	public void onData(BQLMediaPlayer mp, BQLBuffer data) { // Non-Block 
		
		// Log.d(TAG, "thread name " + Thread.currentThread().getName() );
		switch( data.mDataType  ){
		case BQLMediaPlayer.MEDIA_DATA_TYPE_PCM:
			if(mPCMSink != null){
				Message msg = mPCMSink.obtainMessage( WorkerHandler.WORK_NEXT_PCM ); 
				msg.obj = data ; 
				mPCMSink.sendMessage(msg);
			}else{
				Log.d(TAG, "do NOT require pcm data now");
				data.releaseBuffer(); 
			}
			break;
		case BQLMediaPlayer.MEDIA_DATA_TYPE_RGB:
			/*
			 * TO DO 
			 * */
			data.releaseBuffer(); 
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
