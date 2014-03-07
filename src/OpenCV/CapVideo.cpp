#include "OpenCV/CapVideo.h"
#include <QDebug>

using namespace OpenCV;

OpenCV::CapVideo::CapVideo(int device_id, int width, int height)
{
	this->capture = cvCreateCameraCapture(device_id);
	this->device_id=device_id;
	cvSetCaptureProperty(this->capture, CV_CAP_PROP_FRAME_WIDTH, width);
	cvSetCaptureProperty(this->capture, CV_CAP_PROP_FRAME_HEIGHT, height);
}

OpenCV::CapVideo::~CapVideo()
{
	qDebug() << "CapVideo: dectructor: release capture";
	cvReleaseCapture( &this->capture );
}


cv::Mat OpenCV::CapVideo::queryFrame()
{
	this->frame = cvQueryFrame(this->capture);
	return this->frame;
}

cv::Mat CapVideo::getGrayframe()
{
	return this->grayframe;
}

cv::Mat CapVideo::getFrame()
{
	return this->frame;
}

void OpenCV::CapVideo::createGray()
{
	cvtColor(this->frame, this->grayframe, CV_BGR2GRAY);
}

CvCapture *OpenCV::CapVideo::getCapture()
{
	return this->capture;
}
