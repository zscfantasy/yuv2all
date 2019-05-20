/*
 * @file flvmuxer.h
 * @author Akagi201
 * @date 2015/02/04
 *
 * @forked  by Steve Liu <steveliu121@163.com>
 * @date 2018/12/28
 */

#ifndef FLV_MUXER_H_
#define FLV_MUXER_H_

#include <stdbool.h>
#include <stdint.h>   //c99中定义uint8_t,uint16_t,uint32_t,uint64_t等类型的头文件
#include <x264.h> 

#define RES_720P   //720P专用
#ifdef RES_720P
#define RESOLUTION_720P MY_VIDEO_RES_720P
#define RES_WIDTH	1280
#define RES_HEIGHT	720
#endif

#define VIDEO_FPS		25
#define VIDEO_TIME_SCALE	90000
#define VIDEO_SAMPLE_DURATION	(VIDEO_TIME_SCALE / VIDEO_FPS)

#define AUDIO_SAMPLERATE	8000
#define AUDIO_CHANNELS		1
#define AUDIO_TIME_SCALE	(AUDIO_SAMPLERATE * AUDIO_CHANNELS)
/* (AUDIO_TIME_SCALE / AUDIO_FPS)audio_period_time = 64ms, fps = 15.625 */
#define AUDIO_SAMPLE_DURATION	512
#define AAC_BITRATE		16000


typedef struct {
	uint8_t type;
	uint8_t data_size[3];
	uint8_t timestamp[3];
	uint8_t timestamp_ex;
	uint8_t streamid[3];
} __attribute__((__packed__)) FLVTagHeader;

/*相比较于其他视频压缩格式，如果CodecID是AVC（H.264），VideoTagHeader会多出4个字节的信息，AVCPacketType 和CompositionTime[3]*/
typedef struct{

	/*BYTE1 因为转flv需要大小端转换，所以这里需要把CodecID放到前面，FrameType放到后面！*/
	uint8_t CodecID:4;
	uint8_t FrameType:4;
	/*BYTE2 AVCPacketType*/
	uint8_t AVCPacketType;
	/*BYTE3，4，5 CompositionTime*/
	uint8_t CompositionTime[3];
}__attribute__((__packed__)) FLVTagData_AVC;

typedef struct {
	//char *name;
	FILE *flv_hd;
	bool has_video;
	bool has_audio;
	int sample_rate;
	int channels;
	uint8_t *sps;
	uint8_t *pps;
	uint32_t sps_len;
	uint32_t pps_len;
}FLVProfile;

typedef enum {

	NALU_TYPE_SLICE    = 1,
	NALU_TYPE_DPA      = 2,
	NALU_TYPE_DPB      = 3,
	NALU_TYPE_DPC      = 4,
	NALU_TYPE_IDR      = 5,
	NALU_TYPE_SEI      = 6,
	NALU_TYPE_SPS      = 7,
	NALU_TYPE_PPS      = 8,
	NALU_TYPE_AUD      = 9,
	NALU_TYPE_EOSEQ    = 10,
	NALU_TYPE_EOSTREAM = 11,
	NALU_TYPE_FILL     = 12,
	
} NaluType;

typedef enum {
    AMF_DATA_TYPE_NUMBER      = 0x00,
    AMF_DATA_TYPE_BOOL        = 0x01,
    AMF_DATA_TYPE_STRING      = 0x02,
    AMF_DATA_TYPE_OBJECT      = 0x03,
    AMF_DATA_TYPE_NULL        = 0x05,
    AMF_DATA_TYPE_UNDEFINED   = 0x06,
    AMF_DATA_TYPE_REFERENCE   = 0x07,
    AMF_DATA_TYPE_ECMAARRAY   = 0x08,
    AMF_DATA_TYPE_OBJECT_END  = 0x09,
    AMF_DATA_TYPE_ARRAY       = 0x0a,
    AMF_DATA_TYPE_DATE        = 0x0b,
    AMF_DATA_TYPE_LONG_STRING = 0x0c,
    AMF_DATA_TYPE_UNSUPPORTED = 0x0d,
}FlvAMFDataType;

enum {
    FLV_TAG_TYPE_AUDIO = 0x08,
    FLV_TAG_TYPE_VIDEO = 0x09,
    FLV_TAG_TYPE_META  = 0x12,
};


FILE *flv_file_open(const char *filename);

void flv_write_avc_data_tag(FLVProfile flv_profile, x264_nal_t *p_nalPacket);

void creat_video_flv_basic(FLVProfile flv_profile);

FLVProfile init_flv_profile(const char *name, x264_t *x264_encoder, bool has_video, bool has_audio);

void free_flv_profile_buf(FLVProfile flv_profile);

#endif // FLV_MUXER_H_
