#ifndef XINDAWNMULTISCREEN_H_
#define XINDAWNMULTISCREEN_H_

#include <time.h>


#include "AirPlay/IAirPlayCallback.h"

#include "AirPlay/AirPlayEx.h"

#include "AirPlay//AirPlayFactory.h"

#include "VideoSource.h"

#define AIRPLAY_MAX_COUNT 16


class XinDawnMultiScreen : public IAirPlayCallback{
public:
	XinDawnMultiScreen();
	virtual ~XinDawnMultiScreen();


	void updateState(int cmd, char *deviceip, char *value, char *data);

public:


	//Device connect
	virtual ChannelID onAirPlayNewChannel(char *ipAddr);
	//Device disconnect
	virtual void OnAirPlayStopChannel(ChannelID channelId);

	virtual void onDeviceName(ChannelID channelId,char *name);

	//Mirror
	virtual void onAirPlayMirroring_Play(ChannelID channelId, int width, int height, const void *buffer, int buflen, int payloadtype, int protocol, double timestamp);
	virtual void onAirPlayMirroring_Process(ChannelID channelId, const void *buffer, int buflen, int payloadtype, int protocol, double timestamp);
	virtual void onAirPlayMirroring_Stop(ChannelID channelId);

    //Audio
    virtual void onAudioInit(ChannelID channelId, int bits, int channels, int samplerate,int isaudio);
    virtual void onAudioProcess(ChannelID channelId, const char* data,unsigned int datasize,double timestamp,int seqnum);
    virtual void onAudioDestroy(ChannelID channelId);
    virtual void onAudioSetVolume(ChannelID channelId, int volume);//1-100
    virtual void onAudioSetMetadata(ChannelID channelId, const void *buffer, int buflen);
    virtual void onAudioSetCoverart(ChannelID channelId, const void *buffer, int buflen);
    virtual void onAudioFlush(ChannelID channelId);

    //Video etc
    virtual void onAirPlayOpen(ChannelID channelId,char *deviceip,char *url, float fPosition);
    virtual void onAirPlayPlay(ChannelID channelId);
    virtual void onAirPlayStop(ChannelID channelId);
    virtual void onAirPlayPause(ChannelID channelId);
    virtual void onAirPlaySeek(ChannelID channelId,long fPosition);
    virtual void onAirPlaySetVolume(ChannelID channelId,int volume);
    virtual void onAirPlayShowPhoto(ChannelID channelId,unsigned char *data, long long size);



//private:

	int StartAirplay();
	void StopAirplay();

	VideoSource* m_pAirplayDataProcesser[AIRPLAY_MAX_COUNT];

private:


	//AirPlay *m_pAirplay;


	CAirPlayFactory *m_pAirPlayFactory;
	CAirPlayEx      *m_pAirplay;



};



#endif /* XINDAWNMULTISCREEN_H_ */
