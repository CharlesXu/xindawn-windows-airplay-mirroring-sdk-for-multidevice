/**
* John Bradley (jrb@turrettech.com)
*/
#if defined (WIN32)
#include <winsock2.h>
#include <windows.h>
#endif

#include "VideoSource.h"
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#if (_MSC_VER >= 1900) 

extern "C" { FILE __iob_func[3] = { *stdin,*stdout,*stderr }; }
#pragma  comment(lib, "legacy_stdio_definitions.lib") 

#endif


#define SAVE_264_ENABLE 0

#if SAVE_264_ENABLE
FILE  *m_handle = NULL;
#endif



#if 0
extern "C" bool LoadImageFromMemory(const BYTE *buffer, unsigned int size, const char *mime, unsigned int maxwidth, unsigned int maxheight, ImageInfo *info);
extern "C" bool ReleaseImage(ImageInfo *info);
#endif

//static double airtunes_audio_timestamp = -1.0;
//static double airtunes_audio_clock()
//{

//	return airtunes_audio_timestamp;

//}



VideoSource::VideoSource(SDLconfig *config)
{
	audio_volume = 80;
	audio_quit = true;

	memset(&xdw_decoder_q, 0, sizeof(struct __xdw_air_decoder_q));
	xdw_q_init(&xdw_decoder_q.audio_pkt_q, fn_q_mode_unblock);
	xdw_q_set_property(&xdw_decoder_q.audio_pkt_q, fn_q_node_max_nr, (void *)MAX_CACHED_AFRAME_NUM, NULL);
	xdw_q_init(&xdw_decoder_q.video_pkt_q, fn_q_mode_unblock);
	xdw_q_set_property(&xdw_decoder_q.video_pkt_q, fn_q_node_max_nr, (void *)MAX_CACHED_VFRAME_NUM, NULL);

	//airtunes_audio_timestamp = -1.0;

	vs = nullptr;

	char *args[] = 
	{
		"--no-osd",
		"--disable-screensaver",
		"--ffmpeg-hw",
		"--no-video-title-show",
	};

	vlc = libvlc_new(4, args);

    mediaPlayer = nullptr;
    pixelData   = nullptr;
	mediaWidth  = 0;
	mediaHeight = 0;

	MUTEX_CREATE(textureLock);

	media_sws_context = nullptr;
	pMediaFrame    = avcodec_alloc_frame();
	pMediaFrameYUV = avcodec_alloc_frame();



	screen	= config->screen;
	bmp		= config->bmp;
	rect.x	= config->rect.x;
	rect.y	= config->rect.y;
	rect.w	= config->rect.w;
	rect.h	= config->rect.h;
	screen_w = config->rect.w;
	screen_h = config->rect.h;


	//start_airplay();


}

VideoSource::~VideoSource()
{ 


	if (mediaPlayer != nullptr) 
	{
		// media list and media list player

		// media player
		libvlc_video_set_callbacks(mediaPlayer, nullptr, nullptr, nullptr, nullptr);
		libvlc_media_player_stop(mediaPlayer);


		libvlc_media_player_release(mediaPlayer);
		mediaPlayer = nullptr;

		if (pixelData) 
		{
			free(pixelData);
			pixelData = nullptr;
		}
	}


	if (vs != nullptr)
	{
		delete vs;
		vs = nullptr;
	}

	av_free(pMediaFrame);
	av_free(pMediaFrameYUV);

	if (media_sws_context)
	{
		sws_freeContext(media_sws_context);
		media_sws_context = nullptr;
	}

	MUTEX_DESTROY(textureLock);

	//stop_airplay();


}




void *lock(void *data, void **pixelData)
{
    VideoSource *_this = static_cast<VideoSource *>(data);

    *pixelData = _this->pixelData;

	MUTEX_LOCK(_this->textureLock);

    return NULL;
}

void unlock(void *data, void *id, void *const *pixelData)
{
    VideoSource *_this = static_cast<VideoSource *>(data);



	_this->pMediaFrame->data[0] = (uint8_t *)malloc(_this->mediaWidth * _this->mediaHeight);
	_this->pMediaFrame->data[1] = (uint8_t *)malloc((_this->mediaWidth / 2) * (_this->mediaHeight / 2));
	_this->pMediaFrame->data[2] = (uint8_t *)malloc((_this->mediaWidth / 2) * (_this->mediaHeight / 2));


	_this->pMediaFrame->linesize[0] = _this->mediaWidth;
	_this->pMediaFrame->linesize[1] = _this->mediaWidth >> 1;
	_this->pMediaFrame->linesize[2] = _this->mediaWidth >> 1;

	uint8_t *Ydata = _this->pMediaFrame->data[0];
	uint8_t *Udata = _this->pMediaFrame->data[1];
	uint8_t *Vdata = _this->pMediaFrame->data[2];
	uint8_t *PData = (uint8_t *)(*pixelData);

	for (int i = 0; i < _this->mediaHeight; i++)
	{
		for (int j = 0; j < _this->mediaWidth / 2; j++)
		{
			if ((i & 1) == 0)
			{
				*Ydata++ = *PData++;
				*Udata++ = *PData++;
				*Ydata++ = *PData++;
				*Vdata++ = *PData++;
			}
			else
			{
				*Ydata++ = *PData++;
				*PData++;
				*Ydata++ = *PData++;
				*PData++;
			}
		}
	}


	MUTEX_UNLOCK(_this->textureLock);





	SDL_Rect rect;

	int w = _this->mediaWidth;
	int h = _this->mediaHeight;

	if (!_this->media_sws_context)
	{
		_this->media_sws_context = sws_getContext(w, h, AV_PIX_FMT_YUVJ420P,
			_this->rect.w, _this->rect.h, PIX_FMT_YUV420P, SWS_BICUBIC /*SWS_POINT*/,
			NULL, NULL, NULL);

		memset(_this->bmp->pixels[0], 0x0,    _this->bmp->w*_this->bmp->h);
		memset(_this->bmp->pixels[1], 0x80,  (_this->bmp->w*_this->bmp->h) >> 2);
		memset(_this->bmp->pixels[2], 0x80,  (_this->bmp->w*_this->bmp->h) >> 2);
	}

	SDL_LockYUVOverlay(_this->bmp);
	_this->pMediaFrameYUV->data[0] = _this->bmp->pixels[0];
	_this->pMediaFrameYUV->data[1] = _this->bmp->pixels[2];
	_this->pMediaFrameYUV->data[2] = _this->bmp->pixels[1];
	_this->pMediaFrameYUV->linesize[0] = _this->bmp->pitches[0];
	_this->pMediaFrameYUV->linesize[1] = _this->bmp->pitches[2];
	_this->pMediaFrameYUV->linesize[2] = _this->bmp->pitches[1];
	sws_scale(_this->media_sws_context, (const uint8_t* const*)_this->pMediaFrame->data, _this->pMediaFrame->linesize, 0,
		_this->mediaHeight, _this->pMediaFrameYUV->data, _this->pMediaFrameYUV->linesize);


	SDL_UnlockYUVOverlay(_this->bmp);

	free(_this->pMediaFrame->data[0]);
	free(_this->pMediaFrame->data[1]);
	free(_this->pMediaFrame->data[2]);


	rect.x = _this->rect.x;
	rect.y = _this->rect.y;
	rect.w = _this->rect.w;
	rect.h = _this->rect.h;

	SDL_DisplayYUVOverlay(_this->bmp, &rect);


}

void display(void *data, void *id)
{
}

static void vlcEvent(const libvlc_event_t *e, void *data)
{
    VideoSource *_this = reinterpret_cast<VideoSource *>(data);
        
    if (e->type == libvlc_MediaPlayerEndReached) {
        // clear the texture and memory data 
        // because we can still renders from the video
        // and OBS that could fetch old data
		MUTEX_LOCK(_this->textureLock);
        //EnterCriticalSection(&_this->textureLock);
        
        //if (!_this->config->isPlaylistLooping && !--_this->remainingVideos) {
            _this->isRendering = false;
          //  _this->ClearTexture();
       // }
		MUTEX_UNLOCK(_this->textureLock);
        //LeaveCriticalSection(&_this->textureLock);
    } else if (e->type == libvlc_MediaPlayerPlaying) {

		libvlc_time_t time;

		time = libvlc_media_player_get_length(_this->mediaPlayer);
		printf("MediaPlayerTimeChanged %lld ms\n", (long long)time);

		_this->totalDuration = (long long)time;

		_this->isPlaying = 1;
		if (_this->isPaused == 1)
		{
			_this->isPaused = 0;
		}

		MUTEX_LOCK(_this->textureLock);
        //EnterCriticalSection(&_this->textureLock);
        _this->isRendering = true;
        //LeaveCriticalSection(&_this->textureLock);
		MUTEX_UNLOCK(_this->textureLock);
    } else if (e->type == libvlc_MediaPlayerPositionChanged) {
       // libvlc_event_detach(libvlc_media_player_event_manager(_this->mediaPlayer), libvlc_MediaPlayerPositionChanged, vlcEvent, _this);
	


		float new_position = e->u.media_player_position_changed.new_position;


		printf("MediaPlayerTimeChanged %f ms\n", new_position);


		//Log(TEXT("=========%f======="),new_position);

#if 0
        libvlc_video_set_adjust_int(_this->mediaPlayer, libvlc_adjust_Enable, _this->config->isApplyingVideoFilter ? 1 : 0);
        if (_this->config->isApplyingVideoFilter) {
            libvlc_video_set_adjust_float(_this->mediaPlayer, libvlc_adjust_Gamma, float(_this->config->videoGamma) / 100);
            libvlc_video_set_adjust_float(_this->mediaPlayer, libvlc_adjust_Contrast, float(_this->config->videoContrast) / 100);
            libvlc_video_set_adjust_float(_this->mediaPlayer, libvlc_adjust_Brightness, float(_this->config->videoBrightness) / 100);
        }
#endif
    }
	else if (e->type == libvlc_MediaPlayerPaused)
	{
		_this->isPaused = 1;
	}
	else if (e->type == libvlc_MediaPlayerStopped)
	{
		_this->isPlaying = 0;
	}
	else if (e->type == libvlc_MediaDurationChanged)
	{
		int64_t duration = e->u.media_duration_changed.new_duration;
	}
	else if (e->type == libvlc_MediaPlayerTimeChanged)
	{
		libvlc_time_t time;
		time = libvlc_media_player_get_time(_this->mediaPlayer);
		printf("MediaPlayerTimeChanged %lld ms\n", (long long)time);

		_this->currentPosition = (long)time;
	}


}


unsigned VideoSource::VideoFormatCallback(
    char *chroma,
    unsigned *width, 
    unsigned *height,
    unsigned *pitches, 
    unsigned *lines)
{

    memcpy(chroma, CHROMA, sizeof(CHROMA) - 1);
    *pitches = *width * 2;
    *lines = *height;



	MUTEX_LOCK(textureLock);

	if (mediaWidth != *width || mediaHeight != *height) 
	{

        if (pixelData) 
		{
            free(pixelData);
            pixelData = nullptr;
        }
    }
    

    if (!pixelData) 
	{
        pixelData = calloc((*width) * (*height) * 2, 1);
    }
	MUTEX_UNLOCK(textureLock);


	mediaWidth   = *width;
	mediaHeight  = *height;

    return 1;
}


void VideoSource::VideoFormatCleanup()
{
}




void VideoSource::vlcplay(char *url, float fPosition)
{

	MUTEX_LOCK(textureLock);
	//EnterCriticalSection(&textureLock);
	isRendering = false;
	//LeaveCriticalSection(&textureLock);

	MUTEX_UNLOCK(textureLock);

	//libvlc_media_t *media = libvlc_media_new_path(vlc, url);

	libvlc_media_t *media = libvlc_media_new_location(vlc, url);
	/* Create a media player playing environement */
	mediaPlayer = libvlc_media_player_new_from_media(media);
	/* No need to keep the media now */
	libvlc_media_release(media);


	libvlc_event_attach(libvlc_media_player_event_manager(mediaPlayer), libvlc_MediaPlayerEndReached, vlcEvent, this);
	libvlc_event_attach(libvlc_media_player_event_manager(mediaPlayer), libvlc_MediaPlayerPlaying, vlcEvent, this);
	libvlc_event_attach(libvlc_media_player_event_manager(mediaPlayer), libvlc_MediaPlayerOpening, vlcEvent, this);
	libvlc_event_attach(libvlc_media_player_event_manager(mediaPlayer), libvlc_MediaPlayerBuffering, vlcEvent, this);
	libvlc_event_attach(libvlc_media_player_event_manager(mediaPlayer), libvlc_MediaPlayerPaused, vlcEvent, this);
	libvlc_event_attach(libvlc_media_player_event_manager(mediaPlayer), libvlc_MediaPlayerStopped, vlcEvent, this);





	libvlc_video_set_callbacks(mediaPlayer, lock, unlock, display, this);
	libvlc_video_set_format_callbacks(mediaPlayer, videoFormatProxy, videoCleanupProxy);


#if 0
	if (!audioOutputStreamHandler) 
	{
		audioOutputStreamHandler = new AudioOutputStreamHandler(vlc, mediaPlayer);
	}

	audioOutputStreamHandler->SetOutputParameters(
		config->audioOutputType,
		config->audioOutputTypeDevice,
		config->audioOutputDevice,
		config->isAudioOutputToStream);

	audioOutputStreamHandler->SetVolume(config->volume);

	// set (possibly in vane) the volume.  If it doesn't work it will try later until it works
	// vlc... que pasa amigo
	hasSetAsyncProperties = libvlc_audio_set_volume(mediaPlayer, config->volume) == 0;
#endif

	MUTEX_LOCK(textureLock);
	//EnterCriticalSection(&textureLock);
	isRendering = true;
	//LeaveCriticalSection(&textureLock);
	MUTEX_UNLOCK(textureLock);


	libvlc_event_attach(libvlc_media_player_event_manager(mediaPlayer), libvlc_MediaDurationChanged, vlcEvent, this);
	libvlc_event_attach(libvlc_media_player_event_manager(mediaPlayer), libvlc_MediaPlayerPositionChanged, vlcEvent, this);
	libvlc_event_attach(libvlc_media_player_event_manager(mediaPlayer), libvlc_MediaPlayerTimeChanged, vlcEvent, this);




	// play the media_player  
	libvlc_media_player_play(mediaPlayer);


	libvlc_media_player_set_position(mediaPlayer, fPosition);

}





int VideoState::queue_picture(AVFrame *pFrame)
{

	SDL_Rect rect;


	if (this->sws_context == NULL || width != pFrame->width || height != pFrame->height)
	{

		//
		memset(((VideoSource *)this->vsource)->bmp->pixels[0], 0x0, ((VideoSource *)this->vsource)->bmp->w*((VideoSource *)this->vsource)->bmp->h);
		memset(((VideoSource *)this->vsource)->bmp->pixels[1], 0x80, (((VideoSource *)this->vsource)->bmp->w*((VideoSource *)this->vsource)->bmp->h)>>2);
		memset(((VideoSource *)this->vsource)->bmp->pixels[2], 0x80, (((VideoSource *)this->vsource)->bmp->w*((VideoSource *)this->vsource)->bmp->h)>>2);

		if (this->sws_context)
		{
			sws_freeContext(this->sws_context);
			this->sws_context = NULL;
		}

		int w = pFrame->width;
		int h = pFrame->height;


		this->sws_context = sws_getContext(w, h, (AVPixelFormat)pFrame->format,
			((VideoSource *)this->vsource)->rect.w, ((VideoSource *)this->vsource)->rect.h, PIX_FMT_YUV420P, SWS_BICUBIC /*SWS_POINT*/,
			NULL, NULL, NULL);
	}

	width = pFrame->width;
	height = pFrame->height;

	SDL_LockYUVOverlay(((VideoSource *)this->vsource)->bmp);
	this->pFrameYUV->data[0] = ((VideoSource *)this->vsource)->bmp->pixels[0];// +((((VideoSource *)this->vsource)->screen_w - width) >> 1);
	this->pFrameYUV->data[1] = ((VideoSource *)this->vsource)->bmp->pixels[2];// +((((VideoSource *)this->vsource)->screen_w - width) >> 2);
	this->pFrameYUV->data[2] = ((VideoSource *)this->vsource)->bmp->pixels[1];// +((((VideoSource *)this->vsource)->screen_w - width) >> 2);
	this->pFrameYUV->linesize[0] = ((VideoSource *)this->vsource)->bmp->pitches[0];
	this->pFrameYUV->linesize[1] = ((VideoSource *)this->vsource)->bmp->pitches[2];
	this->pFrameYUV->linesize[2] = ((VideoSource *)this->vsource)->bmp->pitches[1];
	sws_scale(this->sws_context, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0,
		pFrame->height, this->pFrameYUV->data, this->pFrameYUV->linesize);


	SDL_UnlockYUVOverlay(((VideoSource *)this->vsource)->bmp);

	rect.x =  ((VideoSource *)this->vsource)->rect.x;
	rect.y =  ((VideoSource *)this->vsource)->rect.y;
	rect.w =  ((VideoSource *)this->vsource)->rect.w;
	rect.h =  ((VideoSource *)this->vsource)->rect.h;

	SDL_DisplayYUVOverlay(((VideoSource *)this->vsource)->bmp, &rect);

	return 0;

}






void VideoState::video_thread_loop(VideoState *self)
{
	AVPacket pkt1, *packet = &pkt1;
	int frameFinished;
	AVFrame *pFrame;

	pFrame				= avcodec_alloc_frame();
	self->pFrameYUV		= avcodec_alloc_frame();

	while (!self->video_quit)
	{

		struct xdw_list_head *ptr;
		xdw_pkt_video_qnode_t *pkt_qnode;

		struct xdw_q_head *video_pkt_q = &(self->xdw_decoder_q_video->video_pkt_q);
		ptr = xdw_q_pop(video_pkt_q);
		if (ptr == NULL)
		{
			continue;
		}

		pkt_qnode = (xdw_pkt_video_qnode_t *)list_entry(ptr, xdw_pkt_video_qnode_t, list);
		if (pkt_qnode == NULL)
		{
			continue;
		}

		av_new_packet(packet, pkt_qnode->VideoBuffer->size);
		memcpy(packet->data, pkt_qnode->VideoBuffer->data, pkt_qnode->VideoBuffer->size);


		free((void *)(pkt_qnode->VideoBuffer->data));
		free((void *)(pkt_qnode->VideoBuffer));
		free((void *)pkt_qnode);


		avcodec_decode_video2(self->codecCtx, pFrame, &frameFinished, packet);

		av_free_packet(packet);

		// Did we get a video frame?
		if (frameFinished)
		{
			int ret =0;
			ret  = self->queue_picture(pFrame);
			if (ret < 0)
				break;
		}
	}

	av_free(pFrame);
	av_free(self->pFrameYUV);
}




int VideoState::init(xdw_air_decoder_q *xdw_decoder_q, const void *privatedata, int privatedatalen)
{

	xdw_decoder_q_video = xdw_decoder_q;

	MUTEX_CREATE(pictq_mutex);
	EVENT_CREATE(pictq_cond);

	this->codec = avcodec_find_decoder(CODEC_ID_H264);
	this->codecCtx = avcodec_alloc_context3(codec);

	this->codecCtx->extradata = (uint8_t *)av_malloc(privatedatalen);
	this->codecCtx->extradata_size = privatedatalen;
	memcpy(this->codecCtx->extradata, privatedata, privatedatalen);
	this->codecCtx->pix_fmt = PIX_FMT_YUV420P;

	int res = avcodec_open2(this->codecCtx, this->codec, NULL);
	if (res < 0)
	{
		printf("Failed to initialize decoder\n");
	}

	this->video_quit = false;


	THREAD_CREATE(this->video_thread, (LPTHREAD_START_ROUTINE)video_thread_loop, this);


	//this->audioClock = airtunes_audio_clock;


	return 1;
}

void VideoState::deinit()
{

	this->video_quit = true;

	EVENT_POST(this->pictq_cond);

	THREAD_JOIN(this->video_thread);


	if (sws_context)
	{
		sws_freeContext(sws_context);
		sws_context = NULL;
	}


	av_free(this->codecCtx->extradata);
	avcodec_close(this->codecCtx);
	av_free(this->codecCtx);


	MUTEX_DESTROY(this->pictq_mutex);
	EVENT_DESTORY(this->pictq_cond);



}



void VideoSource::airplay_open(void *cls, char *url, float fPosition)
{

	if (((VideoSource *)cls)->vs != nullptr)
	{
		delete ((VideoSource *)cls)->vs;
		((VideoSource *)cls)->vs = nullptr;
	}

	((VideoSource *)cls)->totalDuration = 0;
	((VideoSource *)cls)->currentPosition = 0;
	((VideoSource *)cls)->isPlaying = 0;
	((VideoSource *)cls)->isPaused = 0;



	if (((VideoSource *)cls)->media_sws_context)
	{
		sws_freeContext(((VideoSource *)cls)->media_sws_context);
		((VideoSource *)cls)->media_sws_context = nullptr;
	}
	((VideoSource *)cls)->mediaWidth = 0;
	((VideoSource *)cls)->mediaHeight = 0;


	((VideoSource *)cls)->vlcplay(url, fPosition);




}

void VideoSource::airplay_play(void *cls)
{
//	Log(TEXT("AirPlayServer~~~~~~~:airplay_open"));
	libvlc_media_player_set_pause(((VideoSource *)cls)->mediaPlayer, 0);

}

void VideoSource::airplay_pause(void *cls)
{
	//	Log(TEXT("AirPlayServer~~~~~~~:airplay_pause"));

	libvlc_media_player_set_pause(((VideoSource *)cls)->mediaPlayer, 1);

}

void VideoSource::airplay_stop(void *cls)
{

	//Log(TEXT("AirPlayServer~~~~~~~:airplay_stop"));
	libvlc_media_player_stop(((VideoSource *)cls)->mediaPlayer);

}

void VideoSource::airplay_seek(void *cls, long fPosition)
{
	//Log(TEXT("AirPlayServer~~~~~~~:airplay_seek"));

	libvlc_media_player_set_time(((VideoSource *)cls)->mediaPlayer, fPosition * 1000);
}
void VideoSource::airplay_setvolume(void *cls, int volume)
{
//	Log(TEXT("AirPlayServer~~~~~~~:airplay_setvolume"));
}
void VideoSource::airplay_showphoto(void *cls, unsigned char *data, long long size)
{
//	Log(TEXT("AirPlayServer~~~~~~~:airplay_showphoto"));
}



long VideoSource::airplay_getduration(void *cls)
{
	//	Log(TEXT("AirPlayServer~~~~~~~:airplay_pause"));
	return ((VideoSource *)cls)->totalDuration;
}

long VideoSource::airplay_getpostion(void *cls)
{
	//Log(TEXT("AirPlayServer~~~~~~~:airplay_seek"));
	return ((VideoSource *)cls)->currentPosition;
}
int VideoSource::airplay_isplaying(void *cls)
{
	//	Log(TEXT("AirPlayServer~~~~~~~:airplay_setvolume"));
	return ((VideoSource *)cls)->isPlaying;
}
int VideoSource::airplay_ispaused(void *cls)
{
	return ((VideoSource *)cls)->isPaused;
}




void VideoSource::sdl_audio_callback(void *cls, uint8_t *stream, int len)
{

	if (!((VideoSource *)cls)->audio_quit)
	{
		while (len > 0)
		{
			struct xdw_list_head *ptr;
			xdw_pkt_audio_qnode_t *pkt_qnode;

			struct xdw_q_head *audio_pkt_q = &(((VideoSource *)cls)->xdw_decoder_q.audio_pkt_q);
			ptr = xdw_q_pop(audio_pkt_q);
			if (ptr == NULL)
			{
				memset(stream, 0, len);
				break;
			}

			pkt_qnode = (xdw_pkt_audio_qnode_t *)list_entry(ptr, xdw_pkt_audio_qnode_t, list);
			if (pkt_qnode == NULL)
			{
				memset(stream, 0, len);
				break;
			}

			SDL_MixAudio(stream, (unsigned char *)pkt_qnode->abuffer, pkt_qnode->len, ((VideoSource *)cls)->audio_volume);

			len -= pkt_qnode->len;
			stream += pkt_qnode->len;

			free((void *)(pkt_qnode->abuffer));
			free((void *)pkt_qnode);


		}
	}
	else
	{
		memset(stream, 0, len);

	}
}




void VideoSource::audio_init(void *cls, int bits, int channels, int samplerate, int isaudio)
{

	if (((VideoSource *)cls)->mediaPlayer != nullptr) 
	{

		MUTEX_LOCK(((VideoSource *)cls)->textureLock);
		//EnterCriticalSection(&((VideoSource *)cls)->textureLock);
		((VideoSource *)cls)->isRendering = false;
		//LeaveCriticalSection(&((VideoSource *)cls)->textureLock);
		MUTEX_UNLOCK(((VideoSource *)cls)->textureLock);


		// media player
		libvlc_video_set_callbacks(((VideoSource *)cls)->mediaPlayer, nullptr, nullptr, nullptr, nullptr);
		libvlc_media_player_stop(((VideoSource *)cls)->mediaPlayer);


		libvlc_media_player_release(((VideoSource *)cls)->mediaPlayer);
		((VideoSource *)cls)->mediaPlayer = nullptr;

		if (((VideoSource *)cls)->pixelData) 
		{
			free(((VideoSource *)cls)->pixelData);
			((VideoSource *)cls)->pixelData = nullptr;
		}


	}


	struct xdw_q_head *q_head;
	q_head = &(((VideoSource *)cls)->xdw_decoder_q.audio_pkt_q);
	while (!xdw_q_is_empty(q_head))
	{
		struct xdw_list_head *ptr;
		xdw_pkt_audio_qnode_t *frm_node;
		ptr = xdw_q_pop(q_head);

		if (!ptr)
			break; // error
		frm_node = list_entry(ptr, xdw_pkt_audio_qnode_t, list);
		free(frm_node->abuffer);
		free(frm_node);
	}


	SDL_AudioSpec wanted_spec;
	wanted_spec.freq = samplerate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = 2;
	wanted_spec.silence = 0;
	if (isaudio == 3)
		wanted_spec.samples = 4096;
	else if (isaudio == 2)
		wanted_spec.samples = 16384;
	else  if(isaudio == 1)
		wanted_spec.samples = 1408;
	else
		wanted_spec.samples = 1920;
	wanted_spec.callback = sdl_audio_callback;
	wanted_spec.userdata = cls;

	if (SDL_OpenAudio(&wanted_spec, NULL) < 0)
	{

		printf("can't open audio.\n");
		return;

	}

	SDL_PauseAudio(0);


	((VideoSource *)cls)->audio_quit = false;


}
void VideoSource::audio_process(void *cls, const void *buffer, int buflen, double timestamp, uint32_t seqnum)
{

	//airtunes_audio_timestamp = timestamp;

	if (buflen > 0)
	{

		unsigned char *mbuffer = (unsigned char *)malloc(buflen);

		xdw_pkt_audio_qnode_t *frm_node = (xdw_pkt_audio_qnode_t *)malloc(sizeof(xdw_pkt_audio_qnode_t));

		memcpy(mbuffer, buffer, buflen);

		frm_node->abuffer = mbuffer;
		frm_node->len = buflen;
		xdw_q_push(&frm_node->list, &(((VideoSource *)cls)->xdw_decoder_q.audio_pkt_q));

	}


}
void VideoSource::audio_destory(void *cls)
{

	struct xdw_q_head *q_head;
	q_head = &(((VideoSource *)cls)->xdw_decoder_q.audio_pkt_q);
	while (!xdw_q_is_empty(q_head))
	{
		struct xdw_list_head *ptr;
		xdw_pkt_audio_qnode_t *frm_node;
		ptr = xdw_q_pop(q_head);

		if (!ptr)
			break; // error
		frm_node = list_entry(ptr, xdw_pkt_audio_qnode_t, list);
		free(frm_node->abuffer);
		free(frm_node);
	}

	if (((VideoSource *)cls)->audio_quit == false)
	{
		((VideoSource *)cls)->audio_quit = true;
		SDL_CloseAudio();
	}

	//airtunes_audio_timestamp = -1.0;
}


void VideoSource::audio_setvolume(void *cls, int volume)
{
	((VideoSource *)cls)->audio_volume = volume;

}
void VideoSource::audio_setmetadata(void *cls, const void *buffer, int buflen)
{

}
void VideoSource::audio_setcoverart(void *cls, const void *buffer, int buflen)
{

}

void VideoSource::audio_flush(void *cls)
{

}



void VideoSource::mirroring_play(void *cls, int width, int height, const void *buffer, int buflen, int payloadtype, double timestamp)
{


	//printf("==================width=%d,height=%d==============\n",width,height);

#if SAVE_264_ENABLE
	{
		m_handle = fopen("test.264", "wb");



		int spscnt;
		int spsnalsize;
		int ppscnt;
		int ppsnalsize;

		unsigned    char *head = (unsigned  char *)buffer;




		spscnt = head[5] & 0x1f;
		spsnalsize = ((uint32_t)head[6] << 8) | ((uint32_t)head[7]);
		ppscnt = head[8 + spsnalsize];
		ppsnalsize = ((uint32_t)head[9 + spsnalsize] << 8) | ((uint32_t)head[10 + spsnalsize]);


		unsigned char *data = (unsigned char *)malloc(4 + spsnalsize + 4 + ppsnalsize);


		data[0] = 0;
		data[1] = 0;
		data[2] = 0;
		data[3] = 1;

		memcpy(data + 4, head + 8, spsnalsize);

		data[4 + spsnalsize] = 0;
		data[5 + spsnalsize] = 0;
		data[6 + spsnalsize] = 0;
		data[7 + spsnalsize] = 1;

		memcpy(data + 8 + spsnalsize, head + 11 + spsnalsize, ppsnalsize);


		fwrite(data, 1, 4 + spsnalsize + 4 + ppsnalsize, m_handle);


		free(data);


	}

#endif


	if (((VideoSource *)cls)->mediaPlayer != nullptr) 
	{
		MUTEX_LOCK(((VideoSource *)cls)->textureLock);
		//EnterCriticalSection(&((VideoSource *)cls)->textureLock);
		((VideoSource *)cls)->isRendering = false;
		//LeaveCriticalSection(&((VideoSource *)cls)->textureLock);
		MUTEX_UNLOCK(((VideoSource *)cls)->textureLock);

		// media player
		libvlc_video_set_callbacks(((VideoSource *)cls)->mediaPlayer, nullptr, nullptr, nullptr, nullptr);
		libvlc_media_player_stop(((VideoSource *)cls)->mediaPlayer);



		libvlc_media_player_release(((VideoSource *)cls)->mediaPlayer);
		((VideoSource *)cls)->mediaPlayer = nullptr;

		if (((VideoSource *)cls)->pixelData) 
		{
			free(((VideoSource *)cls)->pixelData);
			((VideoSource *)cls)->pixelData = nullptr;
		}
	}


	struct xdw_q_head *q_head;

	q_head = &(((VideoSource *)cls)->xdw_decoder_q.video_pkt_q);
	while (!xdw_q_is_empty(q_head))
	{
		struct xdw_list_head *ptr;
		xdw_pkt_video_qnode_t *frm_node;
		ptr = xdw_q_pop(q_head);

		if (!ptr)
			break; // error
		frm_node = list_entry(ptr, xdw_pkt_video_qnode_t, list);
		free(frm_node->VideoBuffer->data);
		free(frm_node->VideoBuffer);
		free(frm_node);
	}


	((VideoSource *)cls)->vs = new VideoState;
	((VideoSource *)cls)->vs->vsource = (VideoSource *)cls;
	((VideoSource *)cls)->vs->init(&(((VideoSource *)cls)->xdw_decoder_q), buffer, buflen);



}







void VideoSource::mirroring_process(void *cls, const void *buffer, int buflen, int payloadtype, double timestamp)
{


	if ((buflen >= 16380) && (buflen < 16388))
	{

		printf("=========================%d===================\n", buflen);
	}


#if SAVE_264_ENABLE

	if (payloadtype == 0)
	{


		{


			int		    rLen;
			unsigned    char *head;



			unsigned char *data = (unsigned char *)malloc(buflen);
			memcpy(data, buffer, buflen);



			rLen = 0;
			head = (unsigned char *)data + rLen;
			while (rLen < buflen)
			{
				rLen += 4;
				rLen += (((uint32_t)head[0] << 24) | ((uint32_t)head[1] << 16) | ((uint32_t)head[2] << 8) | (uint32_t)head[3]);

				head[0] = 0;
				head[1] = 0;
				head[2] = 0;
				head[3] = 1;

				head = (unsigned char *)data + rLen;
			}



			fwrite(data, 1, buflen, m_handle);

			free(data);


		}


	}
	else if (payloadtype == 1)
	{



		{
			int spscnt;
			int spsnalsize;
			int ppscnt;
			int ppsnalsize;

			unsigned    char *head = (unsigned  char *)buffer;




			spscnt = head[5] & 0x1f;
			spsnalsize = ((uint32_t)head[6] << 8) | ((uint32_t)head[7]);
			ppscnt = head[8 + spsnalsize];
			ppsnalsize = ((uint32_t)head[9 + spsnalsize] << 8) | ((uint32_t)head[10 + spsnalsize]);


			unsigned char *data = (unsigned char *)malloc(4 + spsnalsize + 4 + ppsnalsize);


			data[0] = 0;
			data[1] = 0;
			data[2] = 0;
			data[3] = 1;

			memcpy(data + 4, head + 8, spsnalsize);

			data[4 + spsnalsize] = 0;
			data[5 + spsnalsize] = 0;
			data[6 + spsnalsize] = 0;
			data[7 + spsnalsize] = 1;

			memcpy(data + 8 + spsnalsize, head + 11 + spsnalsize, ppsnalsize);


			fwrite(data, 1, 4 + spsnalsize + 4 + ppsnalsize, m_handle);


			free(data);


		}


	}


#endif


	if (buflen > 0)
	{

		if (payloadtype == 1)
		{

			int spscnt;
			int spsnalsize;
			int ppscnt;
			int ppsnalsize;

			unsigned    char *head = (unsigned  char *)buffer;

			xdw_pkt_video_qnode_t *frm_node = (xdw_pkt_video_qnode_t *)malloc(sizeof(xdw_pkt_video_qnode_t));

			xdw_video_frm_t *pkt = (xdw_video_frm_t *)malloc(sizeof(xdw_video_frm_t));
			memset(pkt, 0, sizeof(xdw_video_frm_t));


			spscnt = head[5] & 0x1f;
			spsnalsize = ((uint32_t)head[6] << 8) | ((uint32_t)head[7]);
			ppscnt = head[8 + spsnalsize];
			ppsnalsize = ((uint32_t)head[9 + spsnalsize] << 8) | ((uint32_t)head[10 + spsnalsize]);


			pkt->data = (unsigned char *)malloc(4 + spsnalsize + 4 + ppsnalsize);


			pkt->data[0] = 0;
			pkt->data[1] = 0;
			pkt->data[2] = 0;
			pkt->data[3] = 1;

			memcpy(pkt->data + 4, head + 8, spsnalsize);

			pkt->data[4 + spsnalsize] = 0;
			pkt->data[5 + spsnalsize] = 0;
			pkt->data[6 + spsnalsize] = 0;
			pkt->data[7 + spsnalsize] = 1;

			memcpy(pkt->data + 8 + spsnalsize, head + 11 + spsnalsize, ppsnalsize);

			pkt->size = 4 + spsnalsize + 4 + ppsnalsize;

			frm_node->VideoBuffer = pkt;

			xdw_q_push(&frm_node->list, &(((VideoSource *)cls)->xdw_decoder_q.video_pkt_q));

		}
		else if (payloadtype == 0)
		{


			int		    rLen;
			unsigned    char *data;

			xdw_pkt_video_qnode_t *frm_node = (xdw_pkt_video_qnode_t *)malloc(sizeof(xdw_pkt_video_qnode_t));

			xdw_video_frm_t *pkt = (xdw_video_frm_t *)malloc(sizeof(xdw_video_frm_t));
			memset(pkt, 0, sizeof(xdw_video_frm_t));


			pkt->data = (unsigned char *)malloc(buflen);
			memcpy(pkt->data, buffer, buflen);
			pkt->size = buflen;


			rLen = 0;
			data = (unsigned char *)pkt->data + rLen;
			while (rLen < pkt->size)
			{
				rLen += 4;
				rLen += (((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | (uint32_t)data[3]);

				data[0] = 0;
				data[1] = 0;
				data[2] = 0;
				data[3] = 1;


				//printf("=====mirroring_process[%d]=======\n", data[4] & 0xf);

				data = (unsigned char *)pkt->data + rLen;
			}

			frm_node->VideoBuffer = pkt;

			xdw_q_push(&frm_node->list, &(((VideoSource *)cls)->xdw_decoder_q.video_pkt_q));
		}



	}

}
void VideoSource::mirroring_stop(void *cls)
{

#if SAVE_264_ENABLE
	fflush(m_handle);
	fclose(m_handle);
#endif

	if (((VideoSource *)cls)->vs != nullptr)
	{
		delete ((VideoSource *)cls)->vs;
		((VideoSource *)cls)->vs = nullptr;
	}


	struct xdw_q_head *q_head;

	q_head = &(((VideoSource *)cls)->xdw_decoder_q.video_pkt_q);
	while (!xdw_q_is_empty(q_head))
	{
		struct xdw_list_head *ptr;
		xdw_pkt_video_qnode_t *frm_node;
		ptr = xdw_q_pop(q_head);

		if (!ptr)
			break; // error
		frm_node = list_entry(ptr, xdw_pkt_video_qnode_t, list);
		free(frm_node->VideoBuffer->data);
		free(frm_node->VideoBuffer);
		free(frm_node);
	}

}

void VideoSource::SetDeviceMac(const std::string& strDeviceMac)
{
	m_strDeviceMac = strDeviceMac;
}
const std::string& VideoSource::GetDeviceMac()
{
	return m_strDeviceMac;
}

const void VideoSource::ClearDeviceMac()
{
	 m_strDeviceMac.clear();
}




#if 0
void VideoSource::start_airplay()
{


	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return ;
	}

	SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
	SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT, SDL_IGNORE);


	screen_w = 1280;// this->codecCtx->width;
	screen_h = 1280;// this->codecCtx->height;
	screen = SDL_SetVideoMode(screen_w, screen_h, 0, 0x115);

	bmp = SDL_CreateYUVOverlay(screen_w, screen_h, SDL_YV12_OVERLAY, screen);

	SDL_WM_SetCaption("XinDawn AirPlay Mirroring SDK", NULL);




	//SDL_LockYUVOverlay(bmp);
	//memcpy(bmp->pixels[0], Home_xml,  bmp->w*bmp->h);
	//memcpy(bmp->pixels[2], Home_xml + bmp->w*bmp->h,  (bmp->w * bmp->h) >> 2);
	//memcpy(bmp->pixels[1], Home_xml + bmp->w*bmp->h + ((bmp->w * bmp->h) >> 2), (bmp->w*bmp->h) >> 2);
	//SDL_UnlockYUVOverlay(bmp);
	//rect.x = 0;
	//rect.y = 0;
	//rect.w = screen_w;
	//rect.h = screen_h;
	//SDL_DisplayYUVOverlay(bmp, &rect);



	airplay_callbacks_t ao;
	memset(&ao, 0, sizeof(airplay_callbacks_t));
	ao.cls = this;
	ao.AirPlayPlayback_Open			= AirPlayOutputFunctions::airplay_open;
	ao.AirPlayPlayback_Play			= AirPlayOutputFunctions::airplay_play;
	ao.AirPlayPlayback_Pause		= AirPlayOutputFunctions::airplay_pause;
	ao.AirPlayPlayback_Stop			= AirPlayOutputFunctions::airplay_stop;
	ao.AirPlayPlayback_Seek			= AirPlayOutputFunctions::airplay_seek;
	ao.AirPlayPlayback_SetVolume	= AirPlayOutputFunctions::airplay_setvolume;
	ao.AirPlayPlayback_ShowPhoto	= AirPlayOutputFunctions::airplay_showphoto;
	ao.AirPlayPlayback_GetDuration	= AirPlayOutputFunctions::airplay_getduration;
	ao.AirPlayPlayback_GetPostion   = AirPlayOutputFunctions::airplay_getpostion;
	ao.AirPlayPlayback_IsPlaying	= AirPlayOutputFunctions::airplay_isplaying;
	ao.AirPlayPlayback_IsPaused		= AirPlayOutputFunctions::airplay_ispaused;

	ao.AirPlayAudio_Init			= AirPlayOutputFunctions::audio_init;
	ao.AirPlayAudio_Process			= AirPlayOutputFunctions::audio_process;
	ao.AirPlayAudio_destroy			= AirPlayOutputFunctions::audio_destory;
	ao.AirPlayAudio_SetVolume		= AirPlayOutputFunctions::audio_setvolume;
	ao.AirPlayAudio_SetMetadata		= AirPlayOutputFunctions::audio_setmetadata;
	ao.AirPlayAudio_SetCoverart		= AirPlayOutputFunctions::audio_setcoverart;
	ao.AirPlayAudio_Flush			= AirPlayOutputFunctions::audio_flush;

	ao.AirPlayMirroring_Play		= AirPlayOutputFunctions::mirroring_play;
	ao.AirPlayMirroring_Process		= AirPlayOutputFunctions::mirroring_process;
	ao.AirPlayMirroring_Stop		= AirPlayOutputFunctions::mirroring_stop;


	//XinDawn_StartMediaServer("test","F:\\windows-sdk-example\\Debug\\airmediaserve.dll", 1280, 720,60,"375970503", &ao);


}

void VideoSource::stop_airplay()
{

	SDL_Quit();


	//XinDawn_StopMediaServer();
}

#endif



#if 0

static int running;

int main(int argc, char **argv)
{
	VideoSource *vs = NULL;

	vs =  new VideoSource();


	running = 1;
	while (running)
	{

		Sleep(1000);
	}
	
	if (vs)
	{
		delete vs;
		vs = NULL;
	}

	return 0;
}
#endif