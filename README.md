1.本程序再macOS平台下开发，需要x264库的支持。程序目录已经复制x264静态库和动态库文件。macOS的x264编译安装在路径-L/usr/local/Cellar/x264/r2854/lib下，所以gcc命令如下：
gcc -o encode_flv.out main.c encode_x264.c decode_sps.c flv.c -L/usr/local/Cellar/x264/r2854/lib -lx264.152
或者直接（lx264是lx264.152的一个快捷方式）
gcc -o encode_flv.out main.c encode_x264.c decode_sps.c flv.c -L/usr/local/Cellar/x264/r2854/lib -lx264

2.原始视频文件是akiyo_cif.yuv，宽352，高288。

3.下一步工作是rtmp推流