TARGET = main
OBJECT = decode_sps.o flv.o encode_x264.o
all:$(TARGET) clean_o
$(TARGET):$(OBJECT)
	gcc $(OBJECT) main.c -o $(TARGET) -lx264.152
$(OBJECT):%.o:%.c
	gcc -c $< -o $@
clean_o:$(OBJECT)
	rm -rf $(OBJECT);
