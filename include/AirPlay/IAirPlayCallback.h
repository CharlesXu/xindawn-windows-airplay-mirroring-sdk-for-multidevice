
#pragma once


typedef void* ChannelID;





class IAirPlayCallback {
public:
	//Device connect
	virtual ChannelID onAirPlayNewChannel(char *ipAddr) = 0;
	//Device disconnect
	virtual void OnAirPlayStopChannel(ChannelID channelId) = 0;

	virtual void onDeviceName(ChannelID channelId,char *name) = 0;

	//Mirror
	virtual void onAirPlayMirroring_Play(ChannelID channelId, int width, int height, const void *buffer, int buflen, int payloadtype, int protocol, double timestamp) = 0;
	virtual void onAirPlayMirroring_Process(ChannelID channelId, const void *buffer, int buflen, int payloadtype, int protocol, double timestamp) = 0;
	virtual void onAirPlayMirroring_Stop(ChannelID channelId)= 0;

        //Audio
    virtual void onAudioInit(ChannelID channelId, int bits, int channels, int samplerate,int isaudio) = 0;
    virtual void onAudioProcess(ChannelID channelId, const char* data,unsigned int datasize,double timestamp,int seqnum) = 0;
    virtual void onAudioDestroy(ChannelID channelId) = 0;
    virtual void onAudioSetVolume(ChannelID channelId, int volume)= 0;//1-100
    virtual void onAudioSetMetadata(ChannelID channelId, const void *buffer, int buflen)= 0;
    virtual void onAudioSetCoverart(ChannelID channelId, const void *buffer, int buflen)= 0;
    virtual void onAudioFlush(ChannelID channelId)= 0;

    //Video etc
    virtual void onAirPlayOpen(ChannelID channelId,char *deviceip,char *url, float fPosition) = 0;
    virtual void onAirPlayPlay(ChannelID channelId) = 0;
    virtual void onAirPlayStop(ChannelID channelId) = 0;
    virtual void onAirPlayPause(ChannelID channelId) = 0;
    virtual void onAirPlaySeek(ChannelID channelId,long fPosition) = 0;
    virtual void onAirPlaySetVolume(ChannelID channelId,int volume) = 0;
    virtual void onAirPlayShowPhoto(ChannelID channelId,unsigned char *data, long long size) = 0;

};

