#include <opencv2/highgui/highgui.hpp>

#define CODE CPP
//#define CODE C

#include <iostream>
#include <set>
using namespace std;
using namespace cv;

#define DISPLAY_GRAY 0
#define DISPLAY_THRESHOLDED 0
#define DISPLAY_BOXES 1

// For demo of labeling
#define MAKE_LABELED 1

#include <sys/types.h>
#include <time.h>
time_t t1, t2, elapsed_time;

// Sample main program to demonstrate how Blob/Region Analysis works

void ProcessImage(IplImage*, char* imagePath);

// Optional defines may precede include of blobs.h
#define COLS 1440
#define ROWS 1296
#define BLOBTOTALCOUNT 6000

#include "blobs.h"

int main(int argc, char** argv)
{
	IplImage* SampleImage = 0;
  if (argc != 2) {
    printf("Input the image file as an argument.\n");
    return 1;
  }
	SampleImage = cvLoadImage(argv[1], CV_LOAD_IMAGE_UNCHANGED);
	ProcessImage(SampleImage, argv[1]);

	sleep(1);

	printf("ALL DONE\n");

  return 0;
}
    
void ProcessImage(IplImage* SampleImage, char* imagePath)
{
	// Display Sample image
	cvNamedWindow("SampleImage", CV_WINDOW_AUTOSIZE);
	cvShowImage("SampleImage", SampleImage);
	sleep(1);
	
	int Cols = SampleImage->width;
	int Rows = SampleImage->height;

  CvScalar pixel, pixelThr;

  for (int x = 0; x < Cols; x++) {
    for (int y = 0; y < Rows; y++) {
      pixel = cvGet2D(SampleImage, y, x);
      //printf("blue: %d   green: %d   red: %d\n", (int)pixel.val[0], (int)pixel.val[1], (int)pixel.val[2]);
      if ((int)pixel.val[0] > (int)pixel.val[1] - 40 || (int)pixel.val[2] > (int)pixel.val[1] - 40) {
        pixelThr.val[0] = 0.0;
        pixelThr.val[1] = 0.0;
        pixelThr.val[2] = 0.0;
        cvSet2D(SampleImage, y, x, pixelThr);
      }
      int blue = CV_IMAGE_ELEM(SampleImage, unsigned char, y, x * 1 + 0);
      int green = CV_IMAGE_ELEM(SampleImage, unsigned char, y, x * 1 + 1);
      int red = CV_IMAGE_ELEM(SampleImage, unsigned char, y, x * 1 + 2);
      //printf("blue: %d   green: %d   red: %d\n\n", blue, green, red);
    }
  }

  char imagePathGreen[255];
  
  int i = 0;
  while (imagePath[i] != '\0') {
    if (imagePath[i] == '.') {
      break;
    }
    imagePathGreen[i] = imagePath[i];
    i++;
  }
  strcat(imagePathGreen, "_green.png");
  cvSaveImage(imagePathGreen, SampleImage);
  //cvShowImage("GreenImage", SampleImage);
  //Redisplay Sample Image
  //sleep(1);
  //cvDestroyWindow("SampleImage");
  //cvNamedWindow("GreenImage", CV_WINDOW_AUTOSIZE);
  //cvMoveWindow("GreenImage", 0, 512);
  //IplImage* greenimg=0;
  //img = cvLoadImage(imagePathGreen);
  //cvShowImage("GreenImage", SampleImage);
  sleep(1);

	
} 


