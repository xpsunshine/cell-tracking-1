CC = g++
CFLAGS =
LDFLAGS = -I/usr/local/include/opencv -lm -lopencv_core -lopencv_highgui -lopencv_video -lopencv_imgproc

all:
	$(CC) $(LDFLAGS) -o main main.cpp
	$(CC) $(LDFLAGS) -o tracking tracking.cpp
	$(CC) $(LDFLAGS) -o togreen togreen.cpp

clean: main tracking togreen
	rm main
	rm tracking
	rm togreen
