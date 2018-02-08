package com.bql.streamuser;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import com.core.BQLVideoView;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.SurfaceTexture;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.View;

 
@SuppressLint("ViewConstructor")
public class CustomVideoView extends BQLVideoView implements 
												View.OnTouchListener,
												GestureDetector.OnGestureListener {

	private static String TAG = "CustomVideoView";

	private final float TOUCH_SCALE_FACTOR = 180.0f / 180;
	
	private GestureDetector mGestureDetector = null;
	
	private float mPreviousDistance=0.0f;
	    
	private volatile float mAngleX=-90.0f;
	private volatile float mAngleY = 0.0f;
	private volatile float mRate=2.5f;
	    
	
    public CustomVideoView(Context context) {
        super(context); // DO NOT REMOVE IT !      
        mGestureDetector = new GestureDetector(getContext(),this);
        mGestureDetector.setIsLongpressEnabled(true);
        setInitRateAngleXY(mRate ,mAngleX, mAngleY);
    }
    

    public CustomVideoView(Context context, AttributeSet attrs) {
		super(context, attrs); // DO NOT REMOVE IT !
        mGestureDetector = new GestureDetector(getContext(),this);
        mGestureDetector.setIsLongpressEnabled(true);
        setInitRateAngleXY(mRate ,mAngleX, mAngleY);
	}

	 
	// View.OnTouchListener 
    @Override
    public boolean onTouchEvent(MotionEvent e) {
    	// TODO
		//return super.onTouchEvent(e); // Do Call Super onTouchEvent
    	Log.d(TAG, "custom onTouchEvent");

        int nCnt=e.getPointerCount();
		int n=e.getAction();
        float x = e.getX();
        float y = e.getY();
        float mNowDistance = 0;
        
		if (nCnt == 2) {
			switch (e.getAction()) {
			case MotionEvent.ACTION_MOVE:
				int xLen = Math.abs((int) e.getX(0) - (int) e.getX(1));
				int yLen = Math.abs((int) e.getY(0) - (int) e.getY(1));
				mNowDistance = (float) Math.sqrt(xLen * xLen + yLen * yLen);
				float temp = mNowDistance - mPreviousDistance;
				if (temp > 0) {
					if(mRate >= 0.1) {
						mRate -= 0.05;
					} else {
						mRate = 0.05f;
					}
			 
				}
				if (temp < 0) {
					if(mRate <= 2.95) {
						mRate += 0.05;
					} else {
						mRate = 3.4f;
					}
				}
				setRenderRate(mRate, true);
			}
			mPreviousDistance = mNowDistance;
		}
    	
    	return true ;
	}

	@Override
	public boolean onTouch(View v, MotionEvent event) {
		Log.d(TAG, "custom onTouch");
		mGestureDetector.onTouchEvent(event);
		return false;
	}
	
	
	@Override
	public boolean onDown(MotionEvent e) {
		Log.d(TAG, "custom onDown");	
		return false;
	}
	
	@Override
	public void onShowPress(MotionEvent e) {
		Log.d(TAG, "custom onShowPress");	
	}

	@Override
	public boolean onSingleTapUp(MotionEvent e) {
		Log.d(TAG, "custom onSingleTapUp");
		return false;
	}
	
	
	@Override 
	public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {// OnGestureListener

		mAngleX -= distanceX * TOUCH_SCALE_FACTOR/10;
		
		if (mAngleY - distanceY * TOUCH_SCALE_FACTOR/10 >=90.0f){
			mAngleY = 90.0f;
		} else if(mAngleY - distanceY * TOUCH_SCALE_FACTOR/10 <=-90.0f){
			mAngleY = -90.0f;
		}
		else{
			mAngleY -= distanceY * TOUCH_SCALE_FACTOR/10; 
		}
		setRenderAngleXY(mAngleX , mAngleY , true );
		
		return false;
	}

	@Override
	public void onLongPress(MotionEvent e) { 
		Log.d(TAG, "custom onLongPress");			
	}

	@Override
	public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX,
			float velocityY) {
		Log.d(TAG, "custom onFling");
		return false;
	}


}
