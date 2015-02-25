// ***********************************************************************
// Author           : Juan R. Terven, jrterven@hotmail.com
// Created          : 02-25-2015
//
// Last Modified By : Juan R. Terven
// Last Modified On : 02-25-2015
// ***********************************************************************
#include "stdafx.h"
#include <windows.h>
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

// 2 modes: WEBCAM,RTSP
//#define WEBCAM
#define RTSP

#ifdef RTSP
#include <vlc/vlc.h>

// RTSP address
const char* rtspAddress = "rtsp://192.168.1.160/live";
// Video resolution WxH
cv::Size rtspRes(848, 480);

struct VideoDataStruct
{
	int param;
};

int done = 0;
libvlc_media_player_t *mp;
unsigned int videoBufferSize = 0;
uint8_t *videoBuffer = 0;

void cbVideoPrerender(void *p_video_data, uint8_t **pp_pixel_buffer, int size)
{
	if (size > videoBufferSize || !videoBuffer)
	{
		printf("Reallocate raw video buffer %d bytes\n", size);
		free(videoBuffer);
		videoBuffer = (uint8_t *)malloc(size);
		videoBufferSize = size;
	}

	// videoBuffer = (uint8_t *)malloc(size);
	*pp_pixel_buffer = videoBuffer;
}
void cbVideoPostrender(void *p_video_data, uint8_t *p_pixel_buffer, int width, int height, int pixel_pitch, int size, int64_t pts)
{
	// Unlocking
	//CloseHandle(hMutex);

}

static void handleEvent(const libvlc_event_t* pEvt, void* pUserData)
{
	libvlc_time_t time;
	switch (pEvt->type)
	{
	case libvlc_MediaPlayerTimeChanged:
		time = libvlc_media_player_get_time(mp);
		printf("MediaPlayerTimeChanged %lld ms\n", (long long)time);
		break;
	case libvlc_MediaPlayerEndReached:
		printf("MediaPlayerEndReached\n");
		done = 1;
		break;
	default:
		printf("%s\n", libvlc_event_type_name(pEvt->type));
	}
}
#endif

int main()
{

#ifdef RTSP
	// VLC pointers 
	libvlc_instance_t *inst;
	libvlc_media_t *m;
	void *pUserData = 0;

	VideoDataStruct dataStruct;

	// VLC options
	char smem_options[1000];

	// RV24
	sprintf(smem_options
		, "#transcode{vcodec=RV24}:smem{"
		"video-prerender-callback=%lld,"
		"video-postrender-callback=%lld,"
		"video-data=%lld,"
		"no-time-sync},"
		, (long long int)(intptr_t)(void*)&cbVideoPrerender
		, (long long int)(intptr_t)(void*)&cbVideoPostrender
		, (long long int)(intptr_t)(void*)&dataStruct
		);

	const char * const vlc_args[] = {
		"-I", "dummy",            // Don't use any interface
		"--ignore-config",        // Don't use VLC's config
		"--extraintf=logger",     // Log anything
		"--verbose=1",            // Be verbose
		"--sout", smem_options    // Stream to memory
	};

	// Launch VLC
	inst = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);

	// Create a new item
	m = libvlc_media_new_location(inst, rtspAddress);

	// Create a media player playing environement
	mp = libvlc_media_player_new_from_media(m);

	libvlc_event_manager_t* eventManager = libvlc_media_player_event_manager(mp);
	libvlc_event_attach(eventManager, libvlc_MediaPlayerTimeChanged, handleEvent, pUserData);
	libvlc_event_attach(eventManager, libvlc_MediaPlayerEndReached, handleEvent, pUserData);
	libvlc_event_attach(eventManager, libvlc_MediaPlayerPositionChanged, handleEvent, pUserData);

	//libvlc_video_set_format(mp, "RV24", 240, 320, 240 * 3 );

	// play the media_player
	libvlc_media_player_play(mp);
#endif

#ifdef WEBCAM
	// use the first camera it finds
	cv::VideoCapture cap(0);

	// You can also get frames from a video
	//	cv::VideoCapture cap("vid.wmv");

	if (!cap.isOpened())
	{
		std::cout << "Could not open video source" << std::endl;
		return -1;
	}
#endif 

	// Create a window for displaying the video
	std::string winName("Demo Video");
	cv::namedWindow(winName, cv::WINDOW_AUTOSIZE);

	cv::Mat frame;
	int key = 0;

	// Endless loop, press Esc to quit
	while (key != 27)
	{
#ifdef RTSP
		// Check for invalid input
		if (videoBuffer)                                             
		{
			// CV_8UC3 = 8 bits, 3 chanels
			frame = cv::Mat(rtspRes, CV_8UC3, videoBuffer);
		}
#elif defined WEBCAM
		// get a new frame from camera or video
		cap >> frame;
#endif

		if (frame.rows == 0 || frame.cols == 0)
			continue;

		cv::imshow(winName, frame);
		key = cv::waitKey(33);
	}

#ifdef RTSP
	libvlc_release(inst);
#endif
}
