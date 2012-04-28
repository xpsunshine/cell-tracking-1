#include <opencv2/video/tracking.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <iostream>
#include <limits>
#include <vector>
#include <cmath>
#include "blobs.cpp"

using namespace std;
using namespace cv;

#define dist(a, b) sqrt(pow((double)(a.x - b.x), 2) + pow((double)(a.y - b.y), 2))

int getIndexClosest(point a, vector<point> pts) {
	int minIndex = -1, len = pts.size();
	double minDist = numeric_limits<double>::max( );
	double currDist;
	for (int i = 0; i < len; i++) {
		currDist = dist(a, pts.at(i));
		if (currDist < minDist) {
			minDist = currDist;
			minIndex = i;
		}
	}
	return minIndex;
}

string stringOfInt(int x) {
	stringstream ss;
	ss << x;
	return ss.str();
}

int main (int argc, char * args[]) {
	
	if (argc < 2) {
		cout << "Need multiple images in sequence" << endl;
		exit(1);
	}
		
	IplImage *current = 0;
	
	// Mat is the object that encapsulates images (pixel matrices) in openCV.
	// Initializes a blank canvas of size frame_size * frame_size, with each pixel taking values
	// that are 8-bit unsigned characters, 3 channels (RGB).
	int num_frames = argc-1;
	
	current = cvLoadImage(args[1], CV_LOAD_IMAGE_UNCHANGED);	
	vector<point> cells = identifyCells(current);
	
	int cell_count = cells.size();

	// OpenCV KalmanFilter, which implements 1) prediction and 2) correction calculations.
	// Initialized with 4-d state (x, y, velocity_x, velocity_y), 2-d measurement (only x and y
	// are observed by mouse).
	vector<KalmanFilter> kalmans;
	for (int i = 0; i < cell_count; i++) {
		KalmanFilter kalman(4, 2, 0, CV_32FC1);
		kalmans.push_back(kalman);
	}

	// Current (x, y, velocity_x, velocity_y) vector at each iteration.
	/* vector<Mat_<float> > states;
	for (int i = 0; i < cell_count; i++) {
		Mat_<float> state(4, 1);
		states.push_back(state);
	} */

	// Measurement vector (observed x, observed y) at each iteration
    Mat_<float> measurement(2,1); measurement.setTo(Scalar(0));

    char code = (char)-1;

	CvFont font;
	double hScale=0.5;
	double vScale=0.5;
	int lineWidth=1;
	cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX|CV_FONT_ITALIC, hScale,vScale,0,lineWidth);
	
	cvNamedWindow("Frame 1");
	for (int i = 0; i < cell_count; i++) {
		cvPutText(current, stringOfInt(i).c_str(), cvPoint(cells.at(i).x, cells.at(i).y), &font, Scalar(255, 255, 255));
	}
	cvShowImage("Frame 1", current);
	
	sleep(5);
	
	// Initialize each cell's Kalman filter
	for (int i = 0; i < cell_count; i++) {
		kalmans.at(i).statePost.at<float>(0) = cells.at(i).x;
		kalmans.at(i).statePost.at<float>(1) = cells.at(i).y;
		kalmans.at(i).statePost.at<float>(2) = 0;
		kalmans.at(i).statePost.at<float>(3) = 0;
		kalmans.at(i).transitionMatrix = *(Mat_<float>(4, 4) << 1,0,0,0,   0,1,0,0,  0,0,1,0,  0,0,0,1);
		
		setIdentity(kalmans.at(i).measurementMatrix);
		setIdentity(kalmans.at(i).processNoiseCov, Scalar::all(1e-4));
		setIdentity(kalmans.at(i).measurementNoiseCov, Scalar::all(1e-1));
		setIdentity(kalmans.at(i).errorCovPost, Scalar::all(.1));
	}
	
	for (int i = 2; i <= num_frames; i++) {
		current = cvLoadImage(args[i], CV_LOAD_IMAGE_UNCHANGED);
		cells = identifyCells(current);
		
		// cout << i << "th Frame:" << endl;
		
		cvNamedWindow("Frame " + i);
		
		for (int j = 0; j < cell_count; j++) {
			
			// Prediction step
			Mat prediction = kalmans.at(j).predict();
			point predictPt = {(int)prediction.at<float>(0), (int)prediction.at<float>(1)};
			// cout << predictPt.x << ", " << predictPt.y << endl;
			
			// Get measurement
			int minIndex = getIndexClosest(predictPt, cells);
			// cout << minIndex << endl;
			measurement(0) = cells.at(minIndex).x;
			measurement(1) = cells.at(minIndex).y;
						
			// Correct
			/* KalmanFilter temp = kalmans.at(j);
			gemm(temp.measurementMatrix * temp.errorCovPre, temp.measurementMatrix, 1, temp.measurementNoiseCov, 1, temp3, GEMM_2_T); */
			Mat estimate = kalmans.at(j).correct(measurement);
						
			for (int k = 0; k < cell_count; k++) {
				cvPutText(current, stringOfInt(j).c_str(), cvPoint((int)estimate.at<float>(0), (int)estimate.at<float>(1)), &font, Scalar(255, 255, 255));
			}
			stringstream name;
			name << "Frame " << i;
			cvShowImage(name.str().c_str(), current);
			
            // Mat prediction = kalman.predict();
            // Point predictPt(prediction.at<float>(0),prediction.at<float>(1));
/*
			
#define drawCross( center, color, d )                                 \
line( canvas, Point( center.x - d, center.y - d ),                \
Point( center.x + d, center.y + d ), color, 2, CV_AA, 0); \
line( canvas, Point( center.x + d, center.y - d ),                \
Point( center.x - d, center.y + d ), color, 2, CV_AA, 0 )

            canvas = Scalar::all(0);
            drawCross( state_velocity_pt, Scalar(255,255,255), 5 );
            drawCross( measPt, Scalar(0,0,255), 5 );
			
			for (int i = 0; i < mouse_coords.size()-1; i++) {
				line(canvas, mouse_coords[i], mouse_coords[i+1], Scalar(255,255,0), 1);
			}
			for (int i = 0; i < kalman_coords.size()-1; i++) {
				line(canvas, kalman_coords[i], kalman_coords[i+1], Scalar(0,255,0), 1);
			}
			
            imshow( "Cell Tracking", canvas ); */
            code = (char)waitKey(100);
			
            if( code == 'q' || code == 'Q' )
	            break;
        }
        if( code == 'q' || code == 'Q' )
            break;
		sleep(5);
    }
	
    return 0;
}