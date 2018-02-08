package com.demo.mediacodec;

import java.nio.ByteBuffer;
import com.demo.lib.av.DataSource;
import com.demo.lib.av.DataSourceSink;
import com.demo.lib.av.MediaMeta;
import android.content.Context;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.Process;
import android.util.Log;
import android.view.Surface;

public class MyMediaCodec implements DataSourceSink {
	private static final String TAG = "MyMediaCodec";
    private Context context;
    private Surface surface;
    private DataSource ds = null;
    
	private final int VIDEO_FRAME_RATE = 30;
    
    private MediaCodec decoder = null;
    private Object mutexDecoder = new Object();

	private ByteBuffer inputByteBuf[];
	private ByteBuffer ouputByteBuf[];
	private boolean gotFirstIFrame = false;
	private boolean queueBufLoop = false;
	private boolean dequeueBufLoop = false;
	private boolean hardwareCodec = true;
	private Thread queueBufThread = null;
	private Thread dequeBufThread = null;
    
    public void init(Context c, DataSource ds, Surface s) {
        context = c;
        this.ds = ds;
        surface = s;
    }
    
    public void deinit() {
    	stopQueueBufThread();
    	stopDequeBufThread();
    	releaseVideoStreamCodec();
    	
    	gotFirstIFrame = false;
    	context = null;
    	surface = null;
    }

	void releaseVideoStreamCodec() {
		synchronized (mutexDecoder) {
			try {
				if (null != decoder) {
					decoder.stop();
					decoder.release();
					decoder = null;

					inputByteBuf = null;
					ouputByteBuf = null;
				}
			} catch (Exception exception) {
				Log.e(TAG,
						"====== release codec error: " + exception.toString());
			}
		}
	}
    
	@Override
	public void onMeta(MediaMeta meta) {
		try {
	        String mimeType = "video/avc";
	        int width = meta.Width;
	        int height = meta.Height;
	        byte[] header_sps = meta.Sps;
	        byte[] header_pps = meta.Pps;
	
	        // create decoder
	        MediaFormat format = MediaFormat.createVideoFormat(mimeType, width, height);
			if (android.os.Build.VERSION.SDK_INT < 21) {
				format.setInteger(
						"color-format",
						MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar);
			} else {
				format.setInteger("color-format",
						MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
			}
	        format.setByteBuffer("csd-0", ByteBuffer.wrap(header_sps));
	        format.setByteBuffer("csd-1", ByteBuffer.wrap(header_pps));
	        format.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, width * height);
			format.setInteger(MediaFormat.KEY_FRAME_RATE, VIDEO_FRAME_RATE);
	        format.setInteger("durationUs", 888888888);
	
	        // start decoder 
	        decoder = MediaCodec.createDecoderByType(mimeType);
	        decoder.configure(format, surface, null, 0);
	        decoder.start();
	        
			sleepMS(20);
			try {
				inputByteBuf = decoder.getInputBuffers();
				ouputByteBuf = decoder.getOutputBuffers();
			} catch (Exception exception) {
				Log.w(TAG, "====== get buffer error: " + exception.toString());
			}
	        
	        startQueueBufThread();
	        startDequeBufThread();
		} catch (Exception e) {
			e.printStackTrace();
			//kame, reconnect rtsp
		}
	}

	@Override
	public void onData(byte[] data) {
		/*
		synchronized (this) { 
			packetList.add(data.clone());
		}
		*/
	}
	
	@Override
	public void onStop() {
		decoder.stop();
		decoder.release();
	}
	
	void queueVideoBuffer(byte[] data) {
		if (null == decoder || null == data || data.length < 4) {
			return;
		}
		
		/*if((data[4] & 0x1F) == 0x7 || (data[4] & 0x1F) == 0x8) {
			Log.i(TAG, "--------- data[4] " + Integer.toHexString(data[4]));
		}*/

		if (!gotFirstIFrame) {
			Log.e(TAG, "--------- data[4] " + data[4]);
			if ((data[4] & 0x1F) != 0x5) {
				sleepMS(10);
				return;
			} else {
				Log.e(TAG, "--------- Got first I frame");
				gotFirstIFrame = true;
			}
		}

		queueBuffer(data);
	}

	boolean queueBuffer(byte[] data) {
		if (null == inputByteBuf || null == decoder) {
			return false;
		}

		try {
			int inIndex = -1;
			synchronized (mutexDecoder) {
				if (null == decoder) {
					Log.e(TAG, "====== decoder is null");
					return false;
				}
				inIndex = decoder.dequeueInputBuffer(10);
			}

			// Log.i(TAG, "---- inIndex " + inIndex);
			if (inIndex >= 0) {
				//Log.e(TAG, "++++++++++++ queue one packet " + queueImage++);

				ByteBuffer buffer = inputByteBuf[inIndex];
				buffer.clear();
				buffer.rewind();
				buffer.put(data.clone());

				synchronized (mutexDecoder) {
					if (null == decoder) {
						return false;
					}
					decoder.queueInputBuffer(inIndex, 0, buffer.position(), 0,
							0);
				}
			} else {
				return false;
			}
		} catch (Exception exception) {
			Log.e(TAG, "====== queue image error: " + exception.toString());
			//kame, restart decoder
		}

		return true;
	}
	
	private void startQueueBufThread() {
		if (null != queueBufThread) {
			return;
		}

        queueBufThread = new Thread(new Runnable() {
            public void run() {
				Process.setThreadPriority(Process.myTid(), -15);
				queueBufLoop = true;
				
        		while (queueBufLoop && null != ds) {
        			byte []data = ds.read();	//Read one AVPacket from ffmpeg
        			if (null == data) {
        				continue;
        			}
        			
        			if (hardwareCodec) {
						//Log.e(TAG, "---------- HW decode one frame cnt=" + framecnt++);
						if (null != inputByteBuf) {
							queueVideoBuffer(data);
						}
					}
        		}
            }
        });
        queueBufThread.start();
	}

	void stopQueueBufThread() {
		if (null != queueBufThread) {
			queueBufLoop = false;
			queueBufThread.interrupt();
			queueBufThread = null;
		}
	}
	
	private long dequeueVideoBuffer(MediaCodec.BufferInfo info) {
		long sleepTime = 5;
		if (null == ouputByteBuf || ouputByteBuf.length <= 0) {
			return sleepTime;
		}

		try {
			int outIndex = -1;
			synchronized (mutexDecoder) {
				if (null == decoder) {
					sleepTime = 100;
					return sleepTime;
				}
				outIndex = decoder.dequeueOutputBuffer(info, 10);
			}

			//Log.i(TAG, "---- outIndex " + outIndex);
			if (outIndex >= 0) {
				//Log.e(TAG, "------------ dequeue one packet " + dequeImage++);
				synchronized (mutexDecoder) {
					if (null == decoder) {
						sleepTime = 100;
						return sleepTime;
					}
					decoder.releaseOutputBuffer(outIndex, true);
				}
			} else if (MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED == outIndex) {
				Log.w(TAG, "INFO_OUTPUT_BUFFERS_CHANGED");
				synchronized (mutexDecoder) {
					if (null == decoder) {
						sleepTime = 100;
						return sleepTime;
					}
					ouputByteBuf = decoder.getOutputBuffers();
				}
			} else if (MediaCodec.INFO_OUTPUT_FORMAT_CHANGED == outIndex) {
				Log.w(TAG, "INFO_OUTPUT_FORMAT_CHANGED");
				synchronized (mutexDecoder) {
					if (null == decoder) {
						sleepTime = 100;
						return sleepTime;
					}
					decoder.getOutputFormat();
				}
			} else {
				return sleepTime;
			}

		} catch (Exception exception) {
			Log.e(TAG, "====== dequeue image error: " + exception.toString());
			//kame, restart decoder
			return -1;
		}

		return sleepTime;
	}
	
	void startDequeBufThread() {
		if (!hardwareCodec || null != dequeBufThread) {
			return;
		}

		dequeBufThread = new Thread(new Runnable() {

			@Override
			public void run() {
				// TODO Auto-generated method stub
				Process.setThreadPriority(Process.myTid(), -15);
				long sleepTime = 0;
				MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
				dequeueBufLoop = true;

				while (dequeueBufLoop) {
					if (sleepTime > 0) {
						sleepMS(sleepTime);
					} else if(sleepTime < 0) {
						//kame, restart decoder
					}
					
					sleepTime = dequeueVideoBuffer(info);
				}
			}

		});
		dequeBufThread.start();
	}

	void stopDequeBufThread() {
		if (null != dequeBufThread) {
			dequeueBufLoop = false;
			dequeBufThread.interrupt();
			dequeBufThread = null;
		}
	}
	
	public static void sleepMS(long time) {
		try {
			Thread.sleep(time);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			//e.printStackTrace();
		}
	}
}
