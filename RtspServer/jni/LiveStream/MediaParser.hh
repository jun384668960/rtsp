#ifndef __MEDIA_PARSER_HH__
#define __MEDIA_PARSER_HH__

class CMediaParser
{
public:
	static CMediaParser* CreateNew();

protected:
	CMediaParser();
	~CMediaParser();

public:
	int getFrameWidth() {
		return mVideoFrameWidth;
	}

	long getDurationMs() {
		return mVideoDuration / 1000  ; 
	}
	
	float getVideoFrameRate(){ // fps  
		return mVideoFrameRate ;
	}
	
	int getFrameHeight() {
		return mVideoFrameHeight;
	}
	
	int getAudioChannels() {
		return mAudioChannels;
	}
	
	int getAudioSamplingRate() {
		return mAudioSamplingRate;
	}
	
	void prepare();
	void start();
	void pause();
	void start();
	void stop();
	void setState(int state);
	void getState(int state);

private:
	int mAudioSamplingRate;
	int mAudioChannels;
	int mVideoFrameHeight;
	int mVideoFrameWidth;
	long mVideoDuration;
	float mVideoFrameRate;
	
};

#endif