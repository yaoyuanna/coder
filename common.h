#ifndef COMMON_H
#define COMMON_H

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/pixfmt.h"
#include "libswscale/swscale.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include"libswresample/swresample.h"
#include"libavutil/time.h"
}
#include<QDebug>
#include<QMessageBox>
#include<QTime>
//opencv 头文件
#include"highgui/highgui.hpp"
#include"imgproc/imgproc.hpp"
#include"core/core.hpp"
#include"opencv2\imgproc\types_c.h"
using namespace cv;
#define FRAME_RATE (15)
#define VIDEOBITRATE 2000000
#endif // COMMON_H
