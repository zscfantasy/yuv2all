/*
 * @file flv-muxer.c
 * @author Akagi201
 * @date 2015/02/04
 *
 * @forked	by Steve Liu <steveliu121@163.com>
 * @date 2018/12/28
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flv.h"
#include "decode_sps.h" 

int64_t dts = 0;        //上一次时间戳
int64_t cts = 0;        //当前时间戳

/*大小端转换，flv数据都是大端存储*/
static uint8_t *ui08_to_bytes(uint8_t *buf, uint8_t val) {
	buf[0] = (val) & 0xff;
	return buf + 1;
}

static uint8_t *ui16_to_bytes(uint8_t *buf, uint16_t val) {
	buf[0] = (val >> 8) & 0xff;
	buf[1] = (val) & 0xff;

	return buf + 2;
}

static uint8_t *ui24_to_bytes(uint8_t *buf, uint32_t val) {
	buf[0] = (val >> 16) & 0xff;
	buf[1] = (val >> 8) & 0xff;
	buf[2] = (val) & 0xff;

	return buf + 3;
}

static uint8_t *ui32_to_bytes(uint8_t *buf, uint32_t val) {
	buf[0] = (val >> 24) & 0xff;
	buf[1] = (val >> 16) & 0xff;
	buf[2] = (val >> 8) & 0xff;
	buf[3] = (val) & 0xff;

	return buf + 4;
}

static uint8_t *ui64_to_bytes(uint8_t *buf, uint64_t val) {
	buf[0] = (val >> 56) & 0xff;
	buf[1] = (val >> 48) & 0xff;
	buf[2] = (val >> 40) & 0xff;
	buf[3] = (val >> 32) & 0xff;
	buf[4] = (val >> 24) & 0xff;
	buf[5] = (val >> 16) & 0xff;
	buf[6] = (val >> 8) & 0xff;
	buf[7] = (val) & 0xff;

	return buf + 8;
}

static uint8_t *double_to_bytes(uint8_t *buf, double val) {
	union {
	uint8_t dc[8];
	double dd;
	} d;

	uint8_t b[8];

	d.dd = val;

	b[0] = d.dc[7];
	b[1] = d.dc[6];
	b[2] = d.dc[5];
	b[3] = d.dc[4];
	b[4] = d.dc[3];
	b[5] = d.dc[2];
	b[6] = d.dc[1];
	b[7] = d.dc[0];

	memcpy(buf, b, 8);

	return buf + 8;
}

static uint8_t bytes_to_ui32(const uint8_t *buf) {
	return (((buf[0]) << 24) & 0xff000000)
		| (((buf[1]) << 16) & 0xff0000)
		| (((buf[2]) << 8) & 0xff00)
		| (((buf[3])) & 0xff);
}

static uint8_t *amf_string_to_bytes(uint8_t *buf, const char *str) {
	uint8_t *pbuf = buf;
	size_t len = strlen(str);

	pbuf = ui08_to_bytes(pbuf, AMF_DATA_TYPE_STRING);
	pbuf = ui16_to_bytes(pbuf, len);
	memcpy(pbuf, str, len);
	pbuf += len;

	return pbuf;
}

static uint8_t *amf_double_to_bytes(uint8_t *buf, double d) {
	uint8_t *pbuf = buf;

	pbuf = ui08_to_bytes(pbuf, AMF_DATA_TYPE_NUMBER);
	pbuf = double_to_bytes(pbuf, d);

	return pbuf;
}

static uint8_t *amf_bool_to_bytes(uint8_t *buf, int b) {
	uint8_t *pbuf = buf;

	pbuf = ui08_to_bytes(pbuf, AMF_DATA_TYPE_BOOL);
	pbuf = ui08_to_bytes(pbuf, !!b);

	return pbuf;
}

static uint8_t *amf_ecmaarray_to_bytes(uint8_t *buf, uint32_t size)
{
	uint8_t *pbuf = buf;

	pbuf = ui08_to_bytes(pbuf, AMF_DATA_TYPE_ECMAARRAY);
	pbuf = ui32_to_bytes(pbuf, size);

	return pbuf;
}

static uint8_t *amf_ecmaarray_add_key(uint8_t *buf, const char *str)
{
	uint8_t *pbuf = buf;
	uint16_t len = strlen(str);

	pbuf = ui16_to_bytes(pbuf, len);  //两字节为长度
	memcpy(pbuf, str, len);           //具体的key数据
	pbuf += len;

	return pbuf;
}

/*TODO*/
static uint8_t *amf_ecmaarray_add_double(uint8_t *buf, double value)
{
	uint8_t *pbuf = buf;

	pbuf = amf_double_to_bytes(pbuf, value);

	return pbuf;
}

static uint8_t *amf_objend(uint8_t *buf)
{
	uint8_t *pbuf = buf;

	pbuf = ui08_to_bytes(pbuf, 0);
	pbuf = ui08_to_bytes(pbuf, 0);
	pbuf = ui08_to_bytes(pbuf, 9);

	return pbuf;
}

//函数参数filename加const，表示函数内部不会也不可以修改穿进去的这个参数filename
FILE *flv_file_open(const char *filename)
{
	FILE *file_hd = NULL;

	if (NULL == filename){
		printf("FLV filename invalid\n");
		return NULL;
	}

	file_hd = fopen(filename, "wb");
	if (file_hd == NULL){
		printf("FLV file open failed\n");
		return NULL;	
	}

	return file_hd;

}

static void flv_write_file_header(FLVProfile flv_profile)
{
	uint8_t prev_size[4] = {0}; //前一个tag的长度
	char flv_file_header[] = "FLV\x1\x5\0\0\0\x9"; // have audio and have video

	if (flv_profile.has_video && flv_profile.has_audio)
		flv_file_header[4] = 0x05;
	else if (flv_profile.has_video && !flv_profile.has_audio)
		flv_file_header[4] = 0x04;
	else if (!flv_profile.has_video && flv_profile.has_audio)
		flv_file_header[4] = 0x01;
	else
		flv_file_header[4] = 0x00;
	fwrite(flv_file_header, 9, 1, flv_profile.flv_hd);

	//flv头之后就是第一个tag的previous的长度，一定是0
	ui32_to_bytes(prev_size, 0);
	fwrite(prev_size, 4, 1, flv_profile.flv_hd);

	return;
}

/*
 * @brief write flv tag
 * @param[in] buf:
 * @param[in] buf_len: flv tag body size
 * @param[in] timestamp: flv tag timestamp
 */
static void flv_write_flv_tag(FILE *file_hd,
					uint8_t *buf, uint32_t buf_len,
					uint32_t timestamp, int tag_type)
{
	uint8_t prev_size[4] = {0};

	FLVTagHeader flvtagheader;
	memset(&flvtagheader, 0, sizeof(flvtagheader));

	flvtagheader.type = tag_type;
	ui24_to_bytes(flvtagheader.data_size, buf_len);
	/*
	flvtagheader.timestamp_ex = (uint8_t) ((timestamp >> 24) & 0xff);
	flvtagheader.timestamp[0] = (uint8_t) ((timestamp >> 16) & 0xff);
	flvtagheader.timestamp[1] = (uint8_t) ((timestamp >> 8) & 0xff);
	flvtagheader.timestamp[2] = (uint8_t) ((timestamp) & 0xff);
	*/
	
	ui24_to_bytes(flvtagheader.timestamp, timestamp & 0x00ffffff);
	ui08_to_bytes(&(flvtagheader.timestamp_ex), (uint8_t) (timestamp >> 24));

	fwrite(&flvtagheader, sizeof(flvtagheader), 1, file_hd);
	fwrite(buf, 1, buf_len, file_hd);

	ui32_to_bytes(prev_size, buf_len + (uint32_t) sizeof(flvtagheader));
	fwrite(prev_size, 4, 1, file_hd);

	return;
}


static void flv_write_onmetadata_tag(FLVProfile flv_profile)
{
	uint8_t *buf = NULL;
	uint8_t *pbuf = NULL;

	buf = (uint8_t *)malloc(200);   //200应该够用了
	if(NULL == buf){  
        printf("malloc temp buf error!\n");  
        exit(EXIT_FAILURE);  
    }
	pbuf = buf;

	pbuf = amf_string_to_bytes(pbuf, "onMetadata");//SCRIPTDATA tag name

	pbuf = amf_ecmaarray_to_bytes(pbuf, 3);//ECMAARRAY size
	/* ECMAARRAY properties */

#if 1
	pbuf = amf_ecmaarray_add_key(pbuf, "width");
	pbuf = amf_ecmaarray_add_double(pbuf, 640);

	pbuf = amf_ecmaarray_add_key(pbuf, "height");
	pbuf = amf_ecmaarray_add_double(pbuf, 480);

	pbuf = amf_ecmaarray_add_key(pbuf, "framerate");
	pbuf = amf_ecmaarray_add_double(pbuf, 25.00);
#endif

	pbuf = amf_objend(pbuf);

	flv_write_flv_tag(flv_profile.flv_hd, buf, (uint32_t)(pbuf - buf),
				0, FLV_TAG_TYPE_META);

	free(buf);

	return;
}


static void flv_write_sps_pps_tag(FLVProfile flv_profile)
{
	uint8_t *buf = NULL;
	uint8_t *pbuf = NULL;
	uint8_t prev_size[4] = {0};
	FLVTagData_AVC flvtagdata_avc;
	FLVTagHeader flvtagheader;
	uint32_t timestamp = 0;
	uint32_t sps_pps_data_len;

	//写flv文件头 11表示6字节AVCDecoderConfigurationRecord+2字节sps长度+1字节pps数量+2字节pps长度
	sps_pps_data_len = flv_profile.sps_len + flv_profile.pps_len + 11;
	memset(&flvtagheader, 0, sizeof(flvtagheader));
	flvtagheader.type = FLV_TAG_TYPE_VIDEO;
	ui24_to_bytes(flvtagheader.data_size, sps_pps_data_len + sizeof(flvtagdata_avc));	
	ui24_to_bytes(flvtagheader.timestamp, timestamp & 0x00ffffff);
	ui08_to_bytes(&(flvtagheader.timestamp_ex), (uint8_t) (timestamp >> 24));
	fwrite(&flvtagheader, sizeof(flvtagheader), 1, flv_profile.flv_hd);

	//sps和pps是特殊的视频帧
	memset(&flvtagdata_avc, 0, sizeof(flvtagdata_avc));
	flvtagdata_avc.FrameType = 1;	//sps和pps帧也属于1:keyframe (for AVC, a seekable frame)
	flvtagdata_avc.CodecID = 7;
	flvtagdata_avc.AVCPacketType = 0x00; // AVCPacketType: 0x00 - AVC sequence header; 0x01 - AVC NALU
	ui24_to_bytes(flvtagdata_avc.CompositionTime, 0);
	fwrite(&flvtagdata_avc, sizeof(flvtagdata_avc), 1, flv_profile.flv_hd);

	buf = (uint8_t *)malloc(flv_profile.sps_len+flv_profile.pps_len+20); 
	if(NULL == buf){  
        printf("malloc temp buf error!\n");  
        exit(EXIT_FAILURE);  
    }
	pbuf = buf;

	//SPS
	//先写AVCDecoderConfigurationRecord，一共是 6 Bytes
	pbuf = ui08_to_bytes(pbuf, 1);  				 //version
	pbuf = ui08_to_bytes(pbuf, flv_profile.sps[1]);  //profile  
	pbuf = ui08_to_bytes(pbuf, flv_profile.sps[2]);  //profile
	pbuf = ui08_to_bytes(pbuf, flv_profile.sps[3]);  //level
	pbuf = ui08_to_bytes(pbuf, 0xff );   // 6 bits reserved (111111) + 2 bits nal size length - 1 (11)
    pbuf = ui08_to_bytes(pbuf, 0xe1 );   // 3 bits reserved (111) + 5 bits number of sps (00001)
    //两个字节用于存放sps的长度
    pbuf = ui16_to_bytes(pbuf, (uint16_t)flv_profile.sps_len);//减去0x 00 00 00 01
    memcpy(pbuf, flv_profile.sps, flv_profile.sps_len);
	pbuf += flv_profile.sps_len;
    // PPS
    //1个字节存放pps的数量
    pbuf = ui08_to_bytes(pbuf, 1);   // number of pps
    //两个字节用于存放pps的长度
    pbuf = ui16_to_bytes(pbuf, (uint16_t)flv_profile.pps_len);//减去0x 00 00 00 01
	memcpy(pbuf, flv_profile.pps, flv_profile.pps_len);
	pbuf += flv_profile.pps_len;
	if((uint32_t)(pbuf - buf) != sps_pps_data_len){
		printf("memcpy buf error!\n");  
	    exit(EXIT_FAILURE);
	}
	fwrite(buf, sps_pps_data_len, 1, flv_profile.flv_hd);
	free(buf);

	//写previousTag大小信息
	ui32_to_bytes(prev_size, (uint32_t)sizeof(flvtagheader) + (uint32_t)sizeof(flvtagdata_avc) + sps_pps_data_len);
	fwrite(prev_size, 4, 1, flv_profile.flv_hd);

	return;

}


/*
 * @brief write video(H264/AVC) tag data
 *
 */
void flv_write_avc_data_tag(FLVProfile flv_profile, x264_nal_t *p_nalPacket)
{
	uint8_t *buf = NULL;
	uint8_t *pbuf = NULL;
	uint8_t video_info = 0;
	int64_t offset;
    uint8_t *data;
    uint32_t nalu_len;
	uint32_t start_code_len;
	uint32_t timestamp;
	uint8_t prev_size[4] = {0};
	FLVTagData_AVC flvtagdata_avc;
	FLVTagHeader flvtagheader;

	//重复的sps，pps以及sei信息不再写
	if ((p_nalPacket->i_type == NALU_TYPE_SEI) || (p_nalPacket->i_type == NALU_TYPE_SPS) || (p_nalPacket->i_type == NALU_TYPE_PPS))
	{
		return;
	}

	if(p_nalPacket->b_long_startcode)
		start_code_len = 4;
	else
		start_code_len = 3;
	nalu_len = p_nalPacket->i_payload - start_code_len;

	offset = 0x28;  //40mm
    timestamp = cts;
	//写flv文件头
	memset(&flvtagheader, 0, sizeof(flvtagheader));
	flvtagheader.type = FLV_TAG_TYPE_VIDEO;
	ui24_to_bytes(flvtagheader.data_size, nalu_len + 4 + (uint32_t)sizeof(flvtagdata_avc));	
	ui24_to_bytes(flvtagheader.timestamp, timestamp & 0x00ffffff);
	ui08_to_bytes(&(flvtagheader.timestamp_ex), (uint8_t) (timestamp >> 24));
	fwrite(&flvtagheader, sizeof(flvtagheader), 1, flv_profile.flv_hd);

	//写flv视频信息（1字节）和avcPacket信息（4字节）
	/*		一字节的视频信息
	前4位为帧类型Frame Type
	值 类型
	1 	keyframe (for AVC, a seekable frame)  
	2 	inter frame (for AVC, a non-seekable frame)
	3 	disposable inter frame (H.263 only)
	4 	generated keyframe (reserved for server use only)
	5 	video info/command frame

	后4位为CodeID    (CodecID)
	值	类型
	1	JPEG (currently unused)
	2	Sorenson H.263
	3 	Screen video
	4	On2 VP6
	5 	On2 VP6 with alpha channel
	6 	Screen video version 2
	7 	AVC(h.264)							*/
	memset(&flvtagdata_avc, 0, sizeof(flvtagdata_avc));
	if (p_nalPacket->i_type == NALU_TYPE_IDR)
		flvtagdata_avc.FrameType = 1;
	else
		flvtagdata_avc.FrameType = 2;

	flvtagdata_avc.CodecID = 7;
	flvtagdata_avc.AVCPacketType = 0x01;
	ui24_to_bytes(flvtagdata_avc.CompositionTime, offset & 0x00ffffff);
	fwrite(&flvtagdata_avc, sizeof(flvtagdata_avc), 1, flv_profile.flv_hd);

	//写nalu数据，先写nalu长度信息(4字节)，再写nalu数据
	buf = (uint8_t *) malloc(nalu_len + 4); //mac平台上需要所申请一点，否则报segment fault:11
	if(NULL == buf){  
        printf("malloc temp buf error!\n");  
        exit(EXIT_FAILURE);  
    }
    pbuf = buf;
	pbuf = ui32_to_bytes(pbuf, nalu_len); //nalu（除去0x00000001或者0x000001的长度）
	memcpy(pbuf, p_nalPacket->p_payload + start_code_len, nalu_len);
	pbuf += nalu_len;
	if((uint32_t)(pbuf - buf) != nalu_len + 4){
		printf("memcpy buf error!\n");  
	    exit(EXIT_FAILURE);
	}
	fwrite(buf, nalu_len + 4, 1, flv_profile.flv_hd);
	free(buf);

	//写previousTag大小信息
	ui32_to_bytes(prev_size, (uint32_t)sizeof(flvtagheader) + (uint32_t)sizeof(flvtagdata_avc) + nalu_len + 4);
	fwrite(prev_size, 4, 1, flv_profile.flv_hd);

	cts += offset;
    dts = cts - offset;

	return;
}

void creat_video_flv_basic(FLVProfile flv_profile)
{
	//写flv文件头
	flv_write_file_header(flv_profile);
	//printf("write file header success\n");
	//写script tag
	flv_write_onmetadata_tag(flv_profile);
	//printf("write script tag success\n");
	//写sps和pps的第一个视频tag
	flv_write_sps_pps_tag(flv_profile);
	//printf("write sps pps success\n");

}

FLVProfile init_flv_profile(const char *filename, x264_t *x264_encoder, bool has_video, bool has_audio)
{
	int ret;
    int n_nal;
    x264_nal_t *p_nal;
	uint8_t *sps_buf;
	uint8_t *pps_buf;
	uint16_t sps_len;
	uint16_t pps_len;
	uint32_t start_code_len;
	int width,height,fps;

	ret = x264_encoder_headers(x264_encoder, &p_nal, &n_nal);
	for (int i = 0; i < n_nal; ++i)
	{
		switch (p_nal[i].i_type)//也可以判断上述枚举中的其它相关信息；
		{
			case NALU_TYPE_SPS:
				if(p_nal[i].b_long_startcode)
					start_code_len = 4;
				else
					start_code_len = 3;
				//test_print(&p_nal[i]);
				sps_len =  p_nal[i].i_payload - start_code_len;
				sps_buf = (uint8_t*)malloc(sps_len);
				if(NULL == sps_buf){  
			        printf("malloc sps_buf error!\n");  
			        exit(EXIT_FAILURE);  
			    } 
				memcpy(sps_buf, p_nal[i].p_payload + start_code_len, sps_len);
				break;
			case NALU_TYPE_PPS:
				if(p_nal[i].b_long_startcode)
					start_code_len = 4;
				else
					start_code_len = 3;
				//test_print(&p_nal[i]);
				pps_len =  p_nal[i].i_payload - start_code_len;
				pps_buf = (uint8_t*)malloc(pps_len);
				if(NULL == pps_buf){  
			        printf("malloc pps_buf error!\n");  
			        exit(EXIT_FAILURE);  
			    }
				memcpy(pps_buf, p_nal[i].p_payload + start_code_len, pps_len);
				break;
			default:
				break;
		}
	}

	//获得sps_buf和pps_buf及其长度之后就可以完整赋值flv_profile
	FLVProfile flv_profile = {
		//.name = filename,
		.flv_hd = flv_file_open(filename),
		.has_video = has_video,
		.has_audio = has_audio,
		.sample_rate = AUDIO_SAMPLERATE,
		.channels = AUDIO_CHANNELS,
		.sps = sps_buf,
		.pps = pps_buf,
		.sps_len = sps_len,
		.pps_len = pps_len
	};
	/*
	//解包sps测试
	h264_decode_sps(sps_buf, sps_len,&width,&height,&fps) ;
	printf("width = %d,height = %d,fps = %d\n", width,height,fps); 
	*/
	return flv_profile;
}

void free_flv_profile_buf(FLVProfile flv_profile)
{
	if(NULL!= flv_profile.sps)
		free(flv_profile.sps);
	if(NULL!= flv_profile.pps)
		free(flv_profile.pps);

	fclose(flv_profile.flv_hd);
}
