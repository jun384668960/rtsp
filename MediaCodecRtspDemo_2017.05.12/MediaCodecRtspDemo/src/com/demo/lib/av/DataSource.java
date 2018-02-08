package com.demo.lib.av;

import android.util.Log;

/**
 * Created by cxm on 8/23/15.
 */
public class DataSource {
    static {
    	System.loadLibrary("gnustl_shared");
        System.loadLibrary("freetype");
        System.loadLibrary("avutil-52");
        System.loadLibrary("avcodec-55");
        System.loadLibrary("swresample-0");
        System.loadLibrary("postproc-52");
        System.loadLibrary("avfilter-4");
        System.loadLibrary("avformat-55");
        System.loadLibrary("swscale-2");

        System.loadLibrary("data-source");
    }
    
    public static final int DATA_SOURCE_EVENT_MEDIA_META = 0;
    public static final int DATA_SOURCE_EVENT_MEDIA_DATA = 1;
    
    public DataSourceSink Sink = null;

    public native int init(String path);
    public native void deinit();
    public native byte []read();

    public void nativeCallback(int event, Object args) {
    	if (DATA_SOURCE_EVENT_MEDIA_META == event) {
    		MediaMeta meta = (MediaMeta)args;
        	Log.d("MY_LOG_TAG", "Media meta: " + meta.toString());
        	
        	if (null != Sink)
        		Sink.onMeta(meta);
    	} else if (DATA_SOURCE_EVENT_MEDIA_DATA == event) {
    		byte []buffer = (byte[])args;
    		String msg = "Media data packet size: " + buffer.length + " first bits: ";
    		for (int i = 0; i < 16; i++)
				msg += "[" + i + "]=" + buffer[i];
    		
    		// Log.d("MY_LOG_TAG", msg);
    		if (null != Sink)
    			Sink.onData(buffer);
    	}
    }
    
    public static native void test();
}
