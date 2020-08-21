#pragma once

#include <stdio.h>
#include <cstdlib>
#include <queue>
#include "al.h"
#include "alc.h"

extern "C" {
#include "libavformat/avformat.h"	//封装格式
#include "libavcodec/avcodec.h"	//解码
#include "libswresample/swresample.h"
};


#define MAX_AUDIO_FARME_SIZE 48000 * 2
#define NUMBUFFERS (4)