
#include <stdint.h>  
#include <x264.h>  
#include <stdio.h>  
#include <unistd.h>  
#include <fcntl.h>  
#include <stdlib.h>  
#include <string.h>  


#include "encode_x264.h"
#include "flv.h"


const char *yuv_filename = "akiyo_cif.yuv";  
const char *h264_filename = "akiyo_h264.h264"; 
char *flv_filename = "akiyo.flv";

void test_print(x264_nal_t *p_test_nal)
{
	int i;
	//printf("\n");
	for (int i = 0; i < p_test_nal->i_payload; ++i)
	{
		printf("0x%02x ",p_test_nal->p_payload[i]);		
	}
}


int main(int argc ,char **argv)
{  
    my_x264_encoder *encoder;
    x264_nal_t *my_nal;  /*for循环中，用于获取nal数据的临时存储变量*/
    int n_nal;  /*控制每次编码获得的nal的数量，保证了安全性，防止自定义函数调用溢出*/
    x264_picture_t pic_out; 
    unsigned int length = 0; 
    int ret; 
    int fd_read, fd_write;//, fd_flv;


    if((fd_read=open(yuv_filename,O_RDONLY))<0){  
    printf("cannot open input file!\n");  
    exit(EXIT_FAILURE);  
    }  
  
    if((fd_write=open(h264_filename,O_WRONLY | O_APPEND | O_CREAT,0777))<0){  
        printf("cannot open output file!\n");  
        exit(EXIT_FAILURE);  
    }  

    encoder = init_x264_encoder();
    //x264_picture_init(&pic_out);  //zsc add 
  
    //的同时也转一份flv，编码的时候已经设置了一帧就是一个nal，不会切割成多个slice，便于存储flv，也便于rtmp推流
	FLVProfile flv_profile = init_flv_profile(flv_filename, encoder->x264_encoder, true, false);
	//创建flv文件，包括写flv头，metadata和第一帧的sps和pps数据
	creat_video_flv_basic(flv_profile);

    while(read(fd_read,encoder->yuv,IMAGE_WIDTH*IMAGE_HEIGHT*3/2)>0)
    {  
        encoder->yuv420p_picture->i_pts++;  
        if((ret = x264_encoder_encode(encoder->x264_encoder,&encoder->nal,&n_nal,encoder->yuv420p_picture,&pic_out))<0){  
            printf("x264_encoder_encode error!\n");  
            exit(EXIT_FAILURE);  
        }

        for(my_nal=encoder->nal;my_nal<encoder->nal+n_nal;++my_nal)
        {  
        	//写h264文件
            write(fd_write,my_nal->p_payload,my_nal->i_payload);
            length+=my_nal->i_payload;
            //写视频帧到flv文件
			flv_write_avc_data_tag(flv_profile, my_nal);
			
		}  
        //printf("Current nal's length=%d Bytes\n",length);
        
    } 
    //清理flv_profile中的sps和pps
    free_flv_profile_buf(flv_profile); 
  
    //x264_picture_clean(encoder->yuv420p_picture);  
    
	free_x264_encoder(encoder);
	close(fd_read);  
    close(fd_write); 

    return 0;  
}


