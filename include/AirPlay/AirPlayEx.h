
#pragma once

#include "IAirPlayCallback.h"

class CAirPlayEx
{
public:
	CAirPlayEx(void);
	virtual ~CAirPlayEx(void);
public:

	virtual int Init(
			char *mac,
			char *friendname,
			char *dllpath,
			char *password,
			char *activecode,
			int  width,
			int  height,
	        int  airtunes_port,
	        int  airplay_port,
	        int  airvideo_port,
	        int  keepAlive,
	        int  keepIdle,
	        int  keepInterval,
	        int  keepCount) = 0;

	virtual void setAirPlayCallback(IAirPlayCallback* pCallback) = 0;

	virtual void updateState(int cmd,char *deviceip,char *value,char *data) = 0;

	virtual void changePassword(char *password) = 0;

	virtual void rename(char *friendname) = 0;

	virtual void run(int nMaxConnections) = 0;
	virtual void stop() = 0;

	virtual void ShutDown(ChannelID nChannelId) = 0;

};
