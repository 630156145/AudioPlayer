#pragma once

#include <stdio.h>
#include <cstdlib>
#include <queue>
#include "al.h"
#include "alc.h"

extern "C" {
#include "libavformat/avformat.h"	//��װ��ʽ
#include "libavcodec/avcodec.h"	//����
#include "libswresample/swresample.h"
};


#define MAX_AUDIO_FARME_SIZE 48000 * 2
#define NUMBUFFERS (4)