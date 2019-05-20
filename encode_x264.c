#include "encode_x264.h"
#include "decode_sps.h" 

my_x264_encoder* init_x264_encoder()
{
    int ret;     
    my_x264_encoder *encoder=(my_x264_encoder *)malloc(sizeof(my_x264_encoder));  
    if(!encoder){  
        printf("cannot malloc my_x264_encoder !\n");  
        exit(EXIT_FAILURE); 
    }  
    CLEAR(*encoder);  
  
    strcpy(encoder->parameter_preset,ENCODER_PRESET);  
    strcpy(encoder->parameter_tune,ENCODER_TUNE);  
  
    encoder->x264_parameter=(x264_param_t *)malloc(sizeof(x264_param_t));  
    if(!encoder->x264_parameter){  
        printf("malloc x264_parameter error!\n");  
        exit(EXIT_FAILURE);  
    }  
    CLEAR(*(encoder->x264_parameter));  
    x264_param_default(encoder->x264_parameter);  
  
    if((ret=x264_param_default_preset(encoder->x264_parameter,encoder->parameter_preset,encoder->parameter_tune))<0){  
        printf("x264_param_default_preset error!\n");  
        exit(EXIT_FAILURE);  
    }  
  
    encoder->x264_parameter->i_fps_den       =1;  
    encoder->x264_parameter->i_fps_num       =25;  
    encoder->x264_parameter->i_width         =IMAGE_WIDTH;  
    encoder->x264_parameter->i_height        =IMAGE_HEIGHT;  
    encoder->x264_parameter->i_threads       =1;  
    encoder->x264_parameter->i_keyint_max    =25;  
    encoder->x264_parameter->b_intra_refresh =1;  
    encoder->x264_parameter->b_annexb        =1;      //如果设置了该项，则在每个NAL单元前加一个四字节的前缀符

	//对于推rtmp流的使用者来说，只生成单个slice是简单的。多个slice的可用性我还没有做实验。以后有机会可以试一试。
	encoder->x264_parameter->i_threads = 1;      //指定处理线程，如果不为1，slice设置将失效
	encoder->x264_parameter->i_slice_count = 1;  //依赖i_threads的设置，否则此处设置一个slice将失效
	encoder->x264_parameter->b_sliced_threads = 0; //关掉slicethread,该参数只有在使用了zerolatency的情况下才会被开启。在使用zerolatency的时候要注意是否会因为多slice导致处理逻辑有问题
/*
	encoder->x264_parameter->rc.i_bitrate = 1200;       // 设置码率,在ABR(平均码率)模式下才生效，且必须在设置ABR前先设置bitrate
	encoder->x264_parameter->rc.i_rc_method = X264_RC_ABR;  // 码率控制方法，CQP(恒定质量)，CRF(恒定码率,缺省值23)，ABR(平均码率)
	encoder->x264_parameter->rc.b_mb_tree = 0;    //这个不为0,将导致编码延时帧,在实时编码时,必须为0
	encoder->x264_parameter->rc.i_qp_constant=0;
	encoder->x264_parameter->rc.i_qp_max=0;
	encoder->x264_parameter->rc.i_qp_min=0;
	encoder->x264_parameter->i_log_level  = X264_LOG_DEBUG;  //LOG参数
	encoder->x264_parameter->i_frame_total = 0;   //编码的桢数，不知道用0
	encoder->x264_parameter->i_bframe  = 5;
	encoder->x264_parameter->i_bframe_pyramid = 0;
	encoder->x264_parameter->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
	encoder->x264_parameter->i_timebase_den = pParam->i_fps_num;
	encoder->x264_parameter->i_timebase_num = pParam->i_fps_den; 

	encoder->x264_parameter->b_repeat_headers = 1;//在每个关键帧I帧前添加sps和pps，实时视频传输需要enable
	encoder->x264_parameter->b_open_gop  = 0;
 */

    strcpy(encoder->parameter_profile,ENCODER_PROFILE);  
    if((ret=x264_param_apply_profile(encoder->x264_parameter,encoder->parameter_profile))<0){  
        printf("x264_param_apply_profile error!\n");  
        exit(EXIT_FAILURE);  
    }  
  
 
    encoder->x264_encoder=x264_encoder_open(encoder->x264_parameter);  
    encoder->colorspace=ENCODER_COLORSPACE;  
    encoder->yuv420p_picture=(x264_picture_t *)malloc(sizeof(x264_picture_t ));  
    if(!encoder->yuv420p_picture){  
        printf("malloc encoder->yuv420p_picture error!\n");  
        exit(EXIT_FAILURE);  
    } 
    if((ret=x264_picture_alloc(encoder->yuv420p_picture,encoder->colorspace,IMAGE_WIDTH,IMAGE_HEIGHT))<0){  
        printf("ret=%d\n",ret);  
        printf("x264_picture_alloc error!\n");  
        exit(EXIT_FAILURE);  
    }  

    //初始化yuv420_picture
    encoder->yuv420p_picture->img.i_csp=encoder->colorspace;  
    encoder->yuv420p_picture->img.i_plane=3;  
    encoder->yuv420p_picture->i_type=X264_TYPE_AUTO;   
  
    encoder->yuv=(uint8_t *)malloc(IMAGE_WIDTH*IMAGE_HEIGHT*3/2);  
    if(!encoder->yuv){  
        printf("malloc yuv error!\n");  
        exit(EXIT_FAILURE);  
    }  
    CLEAR(*(encoder->yuv));  

    ////建立yuv420_picture和yuv之间的关系。yuv存储具体的yuv数据，yuv420_picture在后面的编码中会用到
    encoder->yuv420p_picture->img.plane[0]=encoder->yuv;  
    encoder->yuv420p_picture->img.plane[1]=encoder->yuv+IMAGE_WIDTH*IMAGE_HEIGHT;  
    encoder->yuv420p_picture->img.plane[2]=encoder->yuv+IMAGE_WIDTH*IMAGE_HEIGHT+IMAGE_WIDTH*IMAGE_HEIGHT/4;  

    encoder->nal=(x264_nal_t *)malloc(sizeof(x264_nal_t ));  
    if(!encoder->nal){  
        printf("malloc x264_nal_t error!\n");  
        exit(EXIT_FAILURE);  
    }  
    CLEAR(*(encoder->nal)); 

    return encoder;
}

/*clean_up functions*/  
int free_x264_encoder(my_x264_encoder *encoder)
{

    free(encoder->yuv);  
    free(encoder->yuv420p_picture);  
    free(encoder->x264_parameter);  
    x264_encoder_close(encoder->x264_encoder);  
    free(encoder); 
    //free(encoder->nal);//???? confused conflict with x264_encoder_close(encoder->x264_encoder);  
    return 0;

}

 
 