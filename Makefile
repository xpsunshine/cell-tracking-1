CC = g++
CFLAGS =
LDFLAGS = -I/usr/local/include/opencv -lm -lopencv_core -lopencv_highgui -lopencv_video -lopencv_imgproc

all:
	$(CC) $(LDFLAGS) -o main main.cpp