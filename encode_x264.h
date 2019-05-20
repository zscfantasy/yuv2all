#include <stdint.h>  
#include <x264.h>  
#include <stdio.h>  
#include <unistd.h>  
#include <fcntl.h>  
#include <stdlib.h>  
#include <string.h>  
  
#ifndef ENCODE_X264_H_
#define ENCODE_X264_H_
  
#define CLEAR(x) (memset((&x),0,sizeof(x))) 
 
#define IMAGE_WIDTH   352    //原始yuv数据的宽 
#define IMAGE_HEIGHT  288    //原始yuv数据的高
#define ENCODER_PRESET "veryfast"  
#define ENCODER_TUNE   "zerolatency"  
#define ENCODER_PROFILE  "baseline"  
#define ENCODER_COLORSPACE X264_CSP_I420  
  
typedef struct my_x264_encoder{  
    x264_param_t  * x264_parameter;  
    char parameter_preset[20];  
    char parameter_tune[20];  
    char parameter_profile[20];  
    x264_t  * x264_encoder;  
    x264_picture_t * yuv420p_picture;  
    long colorspace;  
    unsigned char *yuv;  
    x264_nal_t * nal;  		//h264库已经定义，不需要自己另外去定义了
} my_x264_encoder;  


my_x264_encoder* init_x264_encoder();
int free_x264_encoder(my_x264_encoder *encoder);

#endif 

