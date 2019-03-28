#if defined (WIN32)
#include <winsock2.h>
#include <windows.h>
#endif

#include "stdio.h"
#include "stdlib.h"

#include "XinDawnMultiScreen.h"

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#include "VideoSource.h"





XinDawnMultiScreen::XinDawnMultiScreen()
{

	m_pAirplay = NULL;
	m_pAirPlayFactory = NULL;
	

}

XinDawnMultiScreen::~XinDawnMultiScreen() {

}




int XinDawnMultiScreen::StartAirplay()
{
	StopAirplay();



	m_pAirPlayFactory = new CAirPlayFactory();
	if(!m_pAirPlayFactory)
		 return -1;

	m_pAirplay = m_pAirPlayFactory->Create();// new AirPlay();
	if (!m_pAirplay)
		return -1;

	m_pAirplay->Init("FF:FF:FF:FF:FF:F2","XinDawn",".\\Debug\\libairplay.dll","","000000000",1920,1080,47000,7000,7100,1,60,10,6);
	m_pAirplay->setAirPlayCallback(this);


	m_pAirplay->run(AIRPLAY_MAX_COUNT);


	return 0;
}
void XinDawnMultiScreen::StopAirplay()
{
	if (m_pAirplay) {
		m_pAirplay->stop();
		//delete m_pAirplay;
		m_pAirPlayFactory->Destroy(m_pAirplay);
		m_pAirplay = 0;
	}
	if(m_pAirPlayFactory)
	{
		delete m_pAirPlayFactory;
		m_pAirPlayFactory = 0x0;
	}
}




void XinDawnMultiScreen::onAudioInit(ChannelID channelId,int bits, int channels, int samplerate,int isaudio)
{
	VideoSource* vs = (VideoSource*)channelId;
	vs->audio_init((void *)vs, bits, channels, samplerate, isaudio);

	printf("=====================%s=========================\n", __FUNCTION__);


}





void XinDawnMultiScreen::onAudioProcess(ChannelID channelId,const char* data,unsigned int datasize,double timestamp,int seqnum)
{

	VideoSource* vs = (VideoSource*)channelId;
	vs->audio_process((void *)vs, data, datasize, timestamp, seqnum);

	printf("=====================%s=========================\n", __FUNCTION__);

}





void XinDawnMultiScreen::onAudioDestroy(ChannelID channelId)
{

	VideoSource* vs = (VideoSource*)channelId;
	vs->audio_destory((void *)vs);
	printf("=====================%s=========================\n", __FUNCTION__);

}


void XinDawnMultiScreen::onAudioSetVolume(ChannelID channelId, int volume)
{
	VideoSource* vs = (VideoSource*)channelId;
	vs->audio_setvolume((void *)vs, volume);
	printf("=====================%s=========================\n", __FUNCTION__);
}
void XinDawnMultiScreen::onAudioSetMetadata(ChannelID channelId, const void *buffer, int buflen)
{
	VideoSource* vs = (VideoSource*)channelId;
	vs->audio_setmetadata((void *)vs, buffer,buflen);
	printf("=====================%s=========================\n", __FUNCTION__);

}

void XinDawnMultiScreen::onAudioSetCoverart(ChannelID channelId, const void *buffer, int buflen)
{
	VideoSource* vs = (VideoSource*)channelId;
	vs->audio_setmetadata((void *)vs, buffer, buflen);
	printf("=====================%s=========================\n", __FUNCTION__);
}


void XinDawnMultiScreen::onAudioFlush(ChannelID channelId)
{
	VideoSource* vs = (VideoSource*)channelId;
	vs->audio_flush((void *)vs);
	printf("=====================%s=========================\n", __FUNCTION__);
}




void XinDawnMultiScreen::onAirPlayOpen(ChannelID channelId,char *deviceip,char *url, float fPosition)
{
	VideoSource* vs = (VideoSource*)channelId;
	vs->airplay_open((void *)vs, url, fPosition);


	printf("=====================%s=========================\n", __FUNCTION__);


}
void XinDawnMultiScreen::onAirPlayPlay(ChannelID channelId)
{
	VideoSource* vs = (VideoSource*)channelId;
	vs->airplay_play((void *)vs);
	printf("=====================%s=========================\n", __FUNCTION__);

}
void XinDawnMultiScreen::onAirPlayStop(ChannelID channelId)
{
	VideoSource* vs = (VideoSource*)channelId;
	vs->airplay_stop((void *)vs);
	printf("=====================%s=========================\n", __FUNCTION__);


}
void XinDawnMultiScreen::onAirPlayPause(ChannelID channelId)
{
	VideoSource* vs = (VideoSource*)channelId;
	vs->airplay_pause((void *)vs);
	printf("=====================%s=========================\n", __FUNCTION__);

}
void XinDawnMultiScreen::onAirPlaySeek(ChannelID channelId,long fPosition)
{
	VideoSource* vs = (VideoSource*)channelId;
	vs->airplay_seek((void *)vs, fPosition);
	printf("=====================%s=========================\n", __FUNCTION__);
}
void XinDawnMultiScreen::onAirPlaySetVolume(ChannelID channelId,int volume)
{

	VideoSource* vs = (VideoSource*)channelId;
	vs->airplay_setvolume((void *)vs, volume);
	printf("=====================%s=========================\n", __FUNCTION__);

}
void XinDawnMultiScreen::onAirPlayShowPhoto(ChannelID channelId,unsigned char *data, long long size)
{
	VideoSource* vs = (VideoSource*)channelId;
	vs->airplay_showphoto((void *)vs, data,size);
	printf("=====================%s=========================\n", __FUNCTION__);

}




void XinDawnMultiScreen::updateState(int cmd,char *deviceip,char *value,char *data)
{
	if (!m_pAirplay)
		return ;
	m_pAirplay->updateState(cmd,deviceip,value,data);
}



ChannelID XinDawnMultiScreen::onAirPlayNewChannel(char *ipAddr)
{

	printf("=====================%s=========================\n", __FUNCTION__);

	for (int i = 0; i < AIRPLAY_MAX_COUNT; i++)
	{
		const std::string& strDeviceMac = m_pAirplayDataProcesser[i]->GetDeviceMac();
		if (strDeviceMac.empty())
		{
			m_pAirplayDataProcesser[i]->SetDeviceMac("Airplay");
			return (ChannelID)m_pAirplayDataProcesser[i];
	
		}
	}

	return NULL;
}


void XinDawnMultiScreen::OnAirPlayStopChannel(ChannelID channelId)
{
	printf("=====================%s=========================\n", __FUNCTION__);
	VideoSource* vs = (VideoSource*)channelId;
	vs->ClearDeviceMac();

}

void XinDawnMultiScreen::onDeviceName(ChannelID channelId,char *name)
{
	printf("=====================%s=========================\n", __FUNCTION__);
}

void XinDawnMultiScreen::onAirPlayMirroring_Play(ChannelID channelId, int width, int height, const void *buffer, int buflen, int payloadtype, int protocol, double timestamp)
{
	VideoSource* vs = (VideoSource*)channelId;
	if (protocol == 0)
		vs->mirroring_play((void *)vs, width, height, buffer, buflen, payloadtype, timestamp);

	printf( "=====================%s=========================\n", __FUNCTION__);
}

void XinDawnMultiScreen::onAirPlayMirroring_Process(ChannelID channelId, const void *buffer, int buflen, int payloadtype, int protocol, double timestamp)
{
	VideoSource* vs = (VideoSource*)channelId;
	if (protocol == 0)
		vs->mirroring_process((void *)vs,  buffer, buflen, payloadtype, timestamp);
	printf("=====================%s=========================\n", __FUNCTION__);
}


void XinDawnMultiScreen::onAirPlayMirroring_Stop(ChannelID channelId)
{
	VideoSource* vs = (VideoSource*)channelId;
	vs->mirroring_stop((void *)vs);
	printf("=====================%s=========================\n", __FUNCTION__);
}

static void refresh_loop_wait_event(SDL_Event *event) {
	double remaining_time = 0.0;
	SDL_PumpEvents();
	while (!SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_ALLEVENTS)) 
	{
		SDL_PumpEvents();
	}
}



/* handle an event sent by the GUI */
static void event_loop()
{
	SDL_Event event;
	double incr, pos, frac;

	for (;;) 
	{
		double x;
		refresh_loop_wait_event(&event);
		switch (event.type) 
		{
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) 
			{
			case SDLK_ESCAPE:
			case SDLK_q:
				return;
			case SDLK_f:
				//toggle_full_screen(cur_stream);
				//cur_stream->force_refresh = 1;
				break;
			default:
				break;
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			//if (exit_on_mousedown) 
			//{
			//	do_exit(cur_stream);
			//	break;
			//}
		case SDL_VIDEORESIZE:
		//	screen = SDL_SetVideoMode(FFMIN(16383, event.resize.w), event.resize.h, 0,
		//		SDL_HWSURFACE | (is_full_screen ? SDL_FULLSCREEN : SDL_RESIZABLE) | SDL_ASYNCBLIT | SDL_HWACCEL);
		//	if (!screen) {
		//		av_log(NULL, AV_LOG_FATAL, "Failed to set video mode\n");
		//		do_exit(cur_stream);
		//	}
		//	screen_width  = cur_stream->width = screen->w;
		//	screen_height = cur_stream->height = screen->h;
		//	cur_stream->force_refresh = 1;
			break;
		case SDL_QUIT:
			return;
		default:
			break;
		}
	}
}



//static int running;



int main(int argc, char **argv)
{
	XinDawnMultiScreen *cm = NULL;

	//SDL_Surface *screen;
	//SDL_Overlay *bmp;
	//SDL_Rect rect;

	SDLconfig config;





#ifdef WIN32
	int ret = 0;
	WORD wVersionRequested;
	WSADATA wsaData;


	wVersionRequested = MAKEWORD(2, 2);
	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret) {
		return -1;
	}

	if (LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 2) {
		/* Version mismatch, requested version not found */
		return -1;
	}
#endif



	cm = new XinDawnMultiScreen();





	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return 0;
	}

	SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
	SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

	int flags = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL;


	//flags |= SDL_FULLSCREEN;
	flags |= SDL_RESIZABLE;

	memset(&config, 0, sizeof(SDLconfig));
	config.screen = SDL_SetVideoMode(1280, 720, 0, flags);


	for (int i = 0; i < AIRPLAY_MAX_COUNT; i++)
	{


		config.bmp = SDL_CreateYUVOverlay(320, 180, SDL_YV12_OVERLAY, config.screen);

		SDL_WM_SetCaption("XinDawn AirPlay Mirroring SDK", NULL);

		config.rect.x = (i % 4) * 320;
		config.rect.y = (i / 4) * 180;
		config.rect.w = 320;
		config.rect.h = 180;


		cm->m_pAirplayDataProcesser[i] = new VideoSource(&config);

	}



	
	cm->StartAirplay();


	//running = 1;
	//while (running)
	//{

	//	Sleep(1000);
	//}
	event_loop();


	SDL_Quit();


	for (int i = 0; i < AIRPLAY_MAX_COUNT; i++)
	{
		if (cm->m_pAirplayDataProcesser[i])
		{
			delete cm->m_pAirplayDataProcesser[i];
			cm->m_pAirplayDataProcesser[i] = NULL;
		}
	}

	if (cm)
	{
		delete cm;
		cm = NULL;
	}


#ifdef WIN32
	WSACleanup();
#endif

	return 0;
}