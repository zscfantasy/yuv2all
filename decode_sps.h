#include <stdio.h>  
#include <stdint.h>  
#include <string.h> 
#include <stdlib.h> 
#include <stdbool.h> 
   
typedef  unsigned int UINT;  
typedef  unsigned char BYTE;  
typedef  unsigned long DWORD;  
  
UINT Ue(BYTE *pBuff, UINT nLen, UINT *p_nStartBit);
  
int Se(BYTE *pBuff, UINT nLen, UINT *p_nStartBit);
  
DWORD u(UINT BitCount,BYTE * buf,UINT *p_nStartBit); 
  
/** 
 * H264的NAL起始码防竞争机制 
 * 
 * @param buf SPS数据内容 
 * 
 * @无返回值 
 */  
void de_emulation_prevention(BYTE* buf,unsigned int* buf_size); 
  
/** 
 * 解码SPS,获取视频图像宽、高和帧率信息 
 * 
 * @param buf SPS数据内容 
 * @param nLen SPS数据的长度 
 * @param width 图像宽度 
 * @param height 图像高度 
 
 * @成功则返回true , 失败则返回false 
 */  
bool h264_decode_sps(BYTE * buf,unsigned int nLen,int *p_width,int *p_height,int *p_fps);
