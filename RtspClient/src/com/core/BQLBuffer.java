package com.core;

import java.nio.ByteBuffer;

import android.util.Log;

/*
 * 		type					arg1		arg2		arg3		arg4
 * 		MEDIA_DATA_TYPE_H264	pts			isIDR
 *		MEDIA_DATA_TYPE_AAC 	pts			
 *		MEDIA_DATA_TYPE_PCM     pts
 *		MEDIA_DATA_TYPE_YUV     pts         type
 *		MEDIA_DATA_TYPE_RGB     pts
 *
 */
public class BQLBuffer{
	
	static private final String TAG = "BQLBuffer" ;
	
	public long mNativeBufferThiz ;
	public int  mDataType = 0 ;
	public ByteBuffer mData = null ; 
	public long mArg1 = 0; 
	public long mArg2 = 0; 
	public long mArg3 = 0; 
	public long mArg4 = 0; 
	
	
	private BQLBuffer(long nativeThiz , int data_type , ByteBuffer data , 
					  long arg1 , long arg2, long arg3, long arg4 ){
		mNativeBufferThiz = nativeThiz ;
		mDataType = data_type ;
		mData = data ;
		mArg1 = arg1 ;
		mArg2 = arg2 ;
		mArg3 = arg3 ;
		mArg4 = arg4 ;
	}

    public static BQLBuffer getThumbnail(String file, int pos/*单位:毫秒*/, int fmt/*MEDIA_DATA_TYPE_RGB/MEDIA_DATA_TYPE_YUV/MEDIA_DATA_TYPE_JPG*/, int w, int h)
	{//datatype: MEDIA_DATA_TYPE_RGB/MEDIA_DATA_TYPE_YUV/MEDIA_DATA_TYPE_JPG arg1: pts arg2: type arg3: w arg4: h
		return (BQLBuffer)native_getThumbnail(file, pos, fmt, w, h);
	}

	public void releaseBuffer()
	{
		if( mNativeBufferThiz != 0 ) {
			native_releaseBuffer(mNativeBufferThiz);
			mNativeBufferThiz = 0 ;
		}
	}

	@Override
	protected void finalize() throws Throwable {
		
		if( mNativeBufferThiz != 0 ){ 
			Log.e(TAG, "GC finalize BQLBuffer instance, but you do NOT call BQLBuffer.releaseBuffer before " + Long.toHexString(mNativeBufferThiz));
			native_releaseBuffer(mNativeBufferThiz);
			mNativeBufferThiz = 0 ;
		}
		super.finalize();
	}

	native private static Object native_getThumbnail(String file, int pos, int fmt, int w, int h);
	native private void native_releaseBuffer(long thiz);

	static {
		System.loadLibrary("streamuser");
	}
}
