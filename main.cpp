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

typedef struct _tFrame {
    void* data;
    int size;
    int samplerate;
}TFRAME, * PTFRAME;

int initAL();
int SoundCallback(ALuint& bufferID);
int Play();

std::queue<PTFRAME> queueData;
ALuint m_source;

int main() {
    AVFormatContext* pFormatCtx; //解封装
    AVCodecContext* pCodecCtx; //解码
    AVCodec* pCodec;
    AVFrame* pFrame, * pFrameYUV; //帧数据
    AVPacket* packet;	//解码前的压缩数据（包数据）
    int index;
    uint8_t* out_buffer;	//数据缓冲区
    int out_buffer_size;    //缓冲区大小
    SwrContext* swrCtx;

    av_register_all();	//注册库
    avformat_network_init();
    avcodec_register_all();
    pFormatCtx = avformat_alloc_context();

    char filepath[] = "F:/bupt/网研保研/audioPlayer/Debug/test.mp3";
    //char outputpath[] = "output.pcm";

    //打开视频文件，初始化pFormatCtx
    if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0) {
        printf("Couldn't open input stream.\n");
        return -1;
    }
    //获取文件信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        printf("Couldn't find stream information.\n");
        return -1;
    }
    //获取各个媒体流的编码器信息，找到对应的type所在的pFormatCtx->streams的索引位置，初始化编码器。播放音频时type是AUDIO
    index = -1;
    for (int i = 0; i<pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            index = i;
            break;
        }
    if (index == -1) {
        printf("Didn't find a video stream.\n");
        return -1;
    }
    //获取解码器
    pCodec = avcodec_find_decoder(pFormatCtx->streams[index]->codecpar->codec_id);
    if (pCodec == NULL) {
        printf("Codec not found.\n");
        return -1;
    }
    pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[index]->codecpar);
    pCodecCtx->pkt_timebase = pFormatCtx->streams[index]->time_base;
    //打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL)<0) {
        printf("Couldn't open codec.\n");
        return -1;
    }

    //内存分配
    packet = (AVPacket*)av_malloc(sizeof(AVPacket));
    pFrame = av_frame_alloc();

    swrCtx = swr_alloc();

    //设置采样参数 frame->16bit双声道 采样率44100 PCM采样格式
    //输入的采样格式
    enum AVSampleFormat in_sample_fmt = pCodecCtx->sample_fmt;
    //输出采样格式16bit PCM
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输入采样率
    int in_sample_rate = pCodecCtx->sample_rate;
    //输出采样率
    int out_sample_rate = in_sample_rate;
    //输入的声道布局
    uint64_t in_ch_layout = pCodecCtx->channel_layout;
    //输出的声道布局（立体声）
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    swr_alloc_set_opts(swrCtx,
        out_ch_layout, out_sample_fmt, out_sample_rate,
        in_ch_layout, in_sample_fmt, in_sample_rate,
        0, NULL);
    swr_init(swrCtx);


    //16bit 44100 PCM 数据 内存空间。

    //根据声道布局获取输出的声道个数
    int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);

    //FILE* fpOutput = fopen(outputpath, "wb");
    int ret;
    //int count = 0;
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == index) {
            ret = avcodec_send_packet(pCodecCtx, packet);
            if (ret < 0) {
                printf("avcodec_send_packet：%d\n", ret);
                continue;
            }
            while (ret >= 0) {
                ret = avcodec_receive_frame(pCodecCtx, pFrame); 
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    char* errbuf = (char*)av_malloc(100);
                    size_t bug_size = 100;
                    av_strerror(AVERROR(EAGAIN), errbuf, bug_size);
                    printf("%s\n", errbuf);
                    //printf("avcodec_receive_frame 1：%d\n", ret);
                    break;
                }
                else if (ret < 0) {
                    printf("avcodec_receive_frame：%d\n", AVERROR(ret));
                    return -1;
                }

                if (ret >= 0) {   //AVFrame->Audio 参数要仔细填对
                    out_buffer = (uint8_t*)av_malloc(MAX_AUDIO_FARME_SIZE);
                    swr_convert(swrCtx, &out_buffer, MAX_AUDIO_FARME_SIZE, (const uint8_t**)pFrame->data, pFrame->nb_samples);
                    //获取有多少有效的数据在out_buffer的内存上
                    out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb,
                        pFrame->nb_samples, out_sample_fmt, 1);
                    //写入pcm文件
                    //fwrite(out_buffer, 1, (size_t)out_buffer_size, fpOutput);
                    PTFRAME frame = new TFRAME;
                    frame->data = out_buffer;
                    frame->size = out_buffer_size;
                    frame->samplerate = out_sample_rate;
                    queueData.push(frame);
                }
            }
        }
        av_packet_unref(packet);
    }

    printf("Decoding ended.\n");

    initAL();

    ALuint m_buffers[NUMBUFFERS];
    alGenSources(1, &m_source); 
    if (alGetError() != AL_NO_ERROR) {
        printf("Error generating audio source.");
        return -1;
    }
    ALfloat SourcePos[] = { 0.0, 0.0, 0.0 };
    ALfloat SourceVel[] = { 0.0, 0.0, 0.0 };
    ALfloat ListenerPos[] = { 0.0, 0, 0 };
    ALfloat ListenerVel[] = { 0.0, 0.0, 0.0 };
    // first 3 elements are "at", second 3 are "up"
    ALfloat ListenerOri[] = { 0.0, 0.0, -1.0,  0.0, 1.0, 0.0 };
    //设置源属性
    alSourcef(m_source, AL_PITCH, 1.0);
    alSourcef(m_source, AL_GAIN, 1.0);
    alSourcefv(m_source, AL_POSITION, SourcePos);
    alSourcefv(m_source, AL_VELOCITY, SourceVel);
    alSourcef(m_source, AL_REFERENCE_DISTANCE, 50.0f);
    alSourcei(m_source, AL_LOOPING, AL_FALSE);
    alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
    alListener3f(AL_POSITION, 0, 0, 0);
    alGenBuffers(NUMBUFFERS, m_buffers); //创建缓冲区
    
    printf("Play...\n");

    ALint processed1 = 0;
    alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &processed1);
    for (int i = 0; i < NUMBUFFERS; i++) {
        SoundCallback(m_buffers[i]);
    }
    Play();

    while (!queueData.empty()) {  //队列为空后停止播放
        ALint processed = 0;
        alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &processed);
        while (processed > 0) {
            ALuint bufferID = 0;
            alSourceUnqueueBuffers(m_source, 1, &bufferID);
            SoundCallback(bufferID);
            processed--;
        }
        Play();
    }

    alSourceStop(m_source);

    printf("End.\n");

    //fclose(fpOutput);
    av_frame_free(&pFrame);
    av_free(out_buffer);
    swr_free(&swrCtx);


    ALCcontext* pCurContext = alcGetCurrentContext();
    ALCdevice* pCurDevice = alcGetContextsDevice(pCurContext);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(pCurContext);
    alcCloseDevice(pCurDevice);


    system("pause");

    return 0;
}

int initAL() {
    ALCdevice* pDevice;
    ALCcontext* pContext;

    pDevice = alcOpenDevice(NULL);
    pContext = alcCreateContext(pDevice, NULL);
    alcMakeContextCurrent(pContext);

    if (alcGetError(pDevice) != ALC_NO_ERROR)
        return AL_FALSE;

    return 0;
}

int SoundCallback(ALuint& bufferID) {
    PTFRAME frame = queueData.front();
    queueData.pop();
    if (frame == nullptr)
        return -1;
    //把数据写入buffer
    alBufferData(bufferID, AL_FORMAT_STEREO16, frame->data, frame->size, frame->samplerate);
    //将buffer放回缓冲区
    alSourceQueueBuffers(m_source, 1, &bufferID);
    //释放数据
    if (frame) {
        av_free(frame->data);
        delete frame;
    }
    return 0;
}

int Play() {
    int state;
    alGetSourcei(m_source, AL_SOURCE_STATE, &state);
    if (state == AL_STOPPED || state == AL_INITIAL) {
        alSourcePlay(m_source);
    }

    return 0;
}