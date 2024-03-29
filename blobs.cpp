#include <opencv2/highgui/highgui.hpp>

#include <iostream>
#include <set>
#include "blobs.h"

#ifndef BLOBTOTALCOUNT
#define BLOBTOTALCOUNT 6000
#endif

using namespace std;
using namespace cv;

#ifndef POINT_H
#define POINT_H
typedef struct
{
  int x;
  int y;
} point;
#endif

int border = 255, thresh = 80, minArea = 20, dilateCount = -2, erodeCount = -2;

vector<point> GetRegionDataArrayPoints(float RegionData[BLOBTOTALCOUNT][BLOBDATACOUNT])
{
	int ThisRegion;
	vector<point> pts;
	for(ThisRegion = 0; ThisRegion < BLOBTOTALCOUNT; ThisRegion++)
	{
		if(ThisRegion > 0 && RegionData[ThisRegion][0] < 0) break;
		if(RegionData[ThisRegion][BLOBAREA] <= 0) break;

    point newPoint;
    newPoint.x = (int) RegionData[ThisRegion][BLOBSUMX];
    newPoint.y = (int) RegionData[ThisRegion][BLOBSUMY];

    pts.push_back(newPoint);
	}
  return pts;
}
    
vector<point> identifyCells(IplImage* SampleImage)
{	
			IplImage* GrayImage = 0;
			IplImage* ThresholdedImage = 0;
			IplImage* LabeledImage = 0;

			int Cols = SampleImage->width;
			int Rows = SampleImage->height;

			// Make the sample picture into a gray image
			GrayImage = cvCreateImage(cvSize(Cols, Rows), IPL_DEPTH_8U, 1);
			cvCvtColor(SampleImage, GrayImage, CV_BGR2GRAY);

			// Make it into a binary image
			ThresholdedImage = cvCreateImage(cvSize(Cols, Rows), IPL_DEPTH_8U, 1);
			cvThreshold(GrayImage, ThresholdedImage, thresh, border, CV_THRESH_BINARY);

			// Make sure that there are no isolated spots in the image
			if(dilateCount > 0)
			{
				cvDilate(ThresholdedImage, ThresholdedImage, NULL, dilateCount);
				if(erodeCount > 0) { cvErode(ThresholdedImage, ThresholdedImage, NULL, erodeCount); }
			}
			else
			{
				if(dilateCount < 0)
				{
					cvErode(ThresholdedImage, ThresholdedImage, NULL, -erodeCount);
					cvDilate(ThresholdedImage, ThresholdedImage, NULL, -dilateCount);
				}
			}

			//------- Call Blob Analysis routine to analyze image ---------
			int HighRegionNum;

			HighRegionNum = BlobAnalysis(ThresholdedImage, WorkingStorage, RegionData, 0, 0, Cols, Rows, (uchar)border, minArea, LabeledImage);
			//------- Return from Blob Analysis routine ---------
	
	cvReleaseImage(&ThresholdedImage);
	cvReleaseImage(&GrayImage);
	cvReleaseImage(&LabeledImage);
	return GetRegionDataArrayPoints(RegionData);

} 