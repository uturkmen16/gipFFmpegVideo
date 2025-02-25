/*
 * gipFFmpegVideo.cpp
 *
 *  Created on: 10 Jul 2021
 *      Author: kayra
 *      Edited By: Umutcan T�rkmen 24 Feb 2023
 */

#include "gipFFmpegVideo.h"

gipFFmpegVideo::gipFFmpegVideo() {
	appfps = 60;
	framecount = 0;
	duration = 0;
	filepath = "";
	width = 0;
	height = 0;
	currentframe = 0;
    isPaused = false;
    couldopen = false;
    closed = false;
    speed = 1.0f;
    position = 0;
}

gipFFmpegVideo::~gipFFmpegVideo() {

}

void gipFFmpegVideo::load(std::string fullPath) {
	filepath = fullPath;
	couldopen = utils.openVideo(fullPath.c_str(), &width, &height, &framecount, &duration, &time_base);

	if(!couldopen) {
		gLoge("Could not open video file at: " + fullPath);
	}

    framebuffer = new gTexture(width, height, GL_RGBA);
	firstframe = true;

	//This part calculates the number of frames with the loaded video data
	//We don't use the metadata of video since it can be corrupted or false
	//Also avgfps is a close approximation and might not be the exact value

	double duration = (double)utils.getState()->formatcontext->duration / AV_TIME_BASE;
	double numframes = (double)utils.getState()->formatcontext->streams[0]->nb_frames;
	avgfps = numframes / duration;

	//This part calculates at which time intervals frames would be changed
	//To give an example if the app is running with 80fps and our video is 25fps
	//quotientnum = 80/25 = 3 fpsintervalnum = 25 / (80/3 - 25) = 25
	//the program will display every quotientnum = 3 times resulting in 80/3 fps
	//also to ensure the same rate every fpsintervalnum = 25 times when it won't display
	//the frame.

	quotientnum = int(appfps / avgfps);
	fpsintervalnum = avgfps / (appfps / quotientnum - avgfps);

	//Approximate fpsintervalnum to the closest integer
	if(fpsintervalnum - floor(fpsintervalnum) >= 0.5) {
		fpsintervalnum = ceil(fpsintervalnum);
	}

	else fpsintervalnum = floor(fpsintervalnum);

	fpsintervalnum = floor(fpsintervalnum);
	fpsintervalno = 0;
}

void gipFFmpegVideo::loadVideo(std::string videoPath) {
	load(gGetVideosDir() + videoPath);
}

void gipFFmpegVideo::update() {
	//When quotientnum == 1 then try to display every frame
	if(quotientno < quotientnum - 1 || quotientnum == 1) {
		quotientno++;
	}
	else {
		quotientno = 0;
		//When fpsintervalnum == 0 then appfps is a multiple of avgfps
		if(fpsintervalno < fpsintervalnum || fpsintervalnum == 0) {
			fpsintervalno++;
			if(firstframe) {
					firstframe = false;
					return;
				}
				if(closed || isPaused || !couldopen) return;
				if(currentframe >= framecount) {
					close();
					return;
				}

				int result;
				int size_1, size_2;
				float *buffer_1, *buffer_2;
				while(true) {
					result = utils.read_frame();

					if(result == gipFFmpegUtils::RECEIVED_VIDEO) {
						utils.fetch_video_frame(&framedata, &pts);
						framebuffer->setData(framedata, true);
						currentframe++;
						break;
					}
					else if (result == gipFFmpegUtils::RECEIVED_NONE) {
						gLoge("Video:") << "Could not load the video frame";
						return;
					}
					//	Audio received, (result = number of samples)
					utils.getAudio()->getRingBuffer()->writeWait(result * utils.getState()->num_channels, &size_1, &buffer_1, &size_2, &buffer_2);
					utils.fetch_audio_frame(size_1, buffer_1, size_2, buffer_2);
					utils.getAudio()->getRingBuffer()->writeAdvance(result * utils.getState()->num_channels);
				}
				//gLogi("ffmpeg video drawn");
			}

		else {
			//gLogi("ffmpeg video not drawn");
			fpsintervalno = 0;
		}

	}

}

void gipFFmpegVideo::draw() {
	draw(0, 0);
}

void gipFFmpegVideo::draw(int x, int y) {
	draw(x, y, width, height);
}

void gipFFmpegVideo::draw(int x, int y, int w, int h) {
	if(closed || isPaused || !couldopen) return;
	framebuffer->draw(x, y, w, h);

}

void gipFFmpegVideo::play() {
	if(!couldopen) return;

    isplaying = true;
}

void gipFFmpegVideo::stop() {
	close();

	couldopen = utils.openVideo(filepath.c_str(), &width, &height, &framecount, &duration, &time_base);

	isplaying = false;
}

void gipFFmpegVideo::setPaused(bool isPaused) {
	this->isPaused = isPaused;
}

void gipFFmpegVideo::close() {
	utils.close();
	closed = true;
}

int64_t gipFFmpegVideo::getPosition() {
	return ptsec;
}

int64_t gipFFmpegVideo::getDuration() {
	return duration / time_base.den;
}

int gipFFmpegVideo::getWidth() {
	return width;
}

int gipFFmpegVideo::getHeight() {
	return height;
}

void gipFFmpegVideo::setSpeed(float speed) {
	this->speed = speed;
}

void gipFFmpegVideo::setVolume(float volume) {
	this->volume = volume;
}

float gipFFmpegVideo::getSpeed() {
	return speed;
}

float gipFFmpegVideo::getVolume() {
	return volume;
}

bool gipFFmpegVideo::isPlaying() {
	return isplaying;
}

void gipFFmpegVideo::setAppFps(int appfps) {
	this->appfps = appfps;
}
