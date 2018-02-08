package com.demo.lib.av;

public interface DataSourceSink {
	void onMeta(MediaMeta meta);
	void onData(byte []data);
	void onStop();
}
