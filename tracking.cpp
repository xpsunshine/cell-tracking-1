#include <opencv2/video/tracking.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <iostream>
#include <limits>
#include <vector>
#include <cmath>
#include <fstream>
#include "blobs.cpp"
#include "correctness.cpp"

using namespace std;
using namespace cv;

#ifndef dist(a, b)
#define dist(a, b) sqrt(pow((double)(a.x - b.x), 2) + pow((double)(a.y - b.y), 2))
#endif

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

double correctnessMetric(ifstream file, vector<point> ainp, vector<point> aout) {
	vector<point> linp, lout;
	string coord;
	int xin, yin, xout, yout;
	while(getline(file, coord, ',')) {
		xin = atoi(coord.c_str());
		getline(file, coord, ',');
		yin = atoi(coord.c_str());
		getline(file, coord, ',');
		xout = atoi(coord.c_str());
		getline(file, coord, ',');
		yout = atoi(coord.c_str());
		point pin = {xin, yin};
		linp.push_back(pin);
		point pout = {xout, yout};
		lout.push_back(pout);
	}
	
	return computeMatches(linp, lout, ainp, aout);
}

int main (int argc, char * args[]) {
	
	ifstream fin;
	ofstream fout;
	vector<char *> imageNames;
	
	for (int i = 1; i < argc; i++) {
		if (args[i][0] == '-') {
			if (args[i][1] == 'i') {
				if (i+1>argc) {
					cout << "Must have input csv following -i tag and space" << endl;
					exit(1);
				}
				fin.open(args[i+1]);
				i++;
			} else if (args[i][1] == 'o') {
				if (i+1>argc) {
					cout << "Must have output file name following -o tag and space" << endl;
					exit(1);
				}
				fout.open(args[i+1]);
				i++;
			}
		} else {
			imageNames.push_back(args[i]);
		}
	}
	
	IplImage *current = 0;
	
	// Mat is the object that encapsulates images (pixel matrices) in openCV.
	// Initializes a blank canvas of size frame_size * frame_size, with each pixel taking values
	// that are 8-bit unsigned characters, 3 channels (RGB).
	int num_frames = imageNames.size();
	if (num_frames == 0) {
		cout << "Must have at least 1 image" << endl;
		exit(1);
	}
	if (fin.is_open() && !fout.is_open()) {
		cout << "Must have output file with input file to write score out" << endl;
		exit(1);
	}
	
	current = cvLoadImage(imageNames.at(0), CV_LOAD_IMAGE_UNCHANGED);	
	vector<point> cells = identifyCells(current);
	
	int cell_count = cells.size();

	// OpenCV KalmanFilter, which implements 1) prediction and 2) correction calculations.
	// Initialized with 4-d state (x, y, velocity_x, velocity_y), 2-d measurement (only x and y
	// are observed by mouse).
	vector<KalmanFilter> kalmans;
	if (fout)
		fout << "Frame 0" << endl;
	for (int i = 0; i < cell_count; i++) {
		KalmanFilter kalman(4, 2, 0, CV_32FC1);
		kalmans.push_back(kalman);
		if (fout)
			fout << i << "," << cells.at(i).x << "," << cells.at(i).y << endl;
	}

	// Measurement vector (observed x, observed y) at each iteration
    Mat_<float> measurement(2,1); measurement.setTo(Scalar(0));

	CvFont font;
	double hScale=0.5;
	double vScale=0.5;
	int lineWidth=1;
	cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX|CV_FONT_ITALIC, hScale,vScale,0,lineWidth);
	
	cvNamedWindow("Frame 0");
	for (int i = 0; i < cell_count; i++) {
		cvPutText(current, stringOfInt(i).c_str(), cvPoint(cells.at(i).x, cells.at(i).y), &font, Scalar(255, 255, 255));
	}
	cvShowImage("Frame 0", current);
	
	sleep(2);
	
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
	
	vector<point> nextCells, finalPos;
	
	for (int i = 1; i < num_frames; i++) {
		current = cvLoadImage(imageNames.at(i), CV_LOAD_IMAGE_UNCHANGED);
		nextCells = identifyCells(current);
		if (fout)
			fout << "Frame " << i << endl;
		
		stringstream name;
		name << "Frame " << i;
		const char *title = name.str().c_str();
		cvNamedWindow(title);
		
		for (int j = 0; j < cell_count; j++) {
			
			// Prediction step
			Mat prediction = kalmans.at(j).predict();
			point predictPt = {(int)prediction.at<float>(0), (int)prediction.at<float>(1)};
			
			// Get measurement
			int minIndex = getIndexClosest(predictPt, nextCells);
			measurement(0) = nextCells.at(minIndex).x;
			measurement(1) = nextCells.at(minIndex).y;
			
			// Correct
			Mat estimate = kalmans.at(j).correct(measurement);
			/* kalmans.at(j).statePost.at<float>(0) = measurement(0);
			kalmans.at(j).statePost.at<float>(1) = measurement(1);
			cvPutText(current, stringOfInt(j).c_str(), cvPoint(measurement(0),
				measurement(1)), &font, Scalar(255, 255, 255));
			if (i == num_frames - 1) {
				point location = {measurement(0), measurement(1)};
				finalPos.push_back(location);
			}
			if (fout)
				fout << j << "," << measurement(0) << "," << measurement(1) << endl; */
			cvPutText(current, stringOfInt(j).c_str(), cvPoint((int)estimate.at<float>(0),
				(int)estimate.at<float>(1)), &font, Scalar(255, 255, 255));
			if (i == num_frames - 1) {
				point location = {(int)estimate.at<float>(0), (int)estimate.at<float>(1)};
				finalPos.push_back(location);
			}
			if (fout)
				fout << j << "," << (int)estimate.at<float>(0) << "," << (int)estimate.at<float>(1) << endl;
        }

		cvShowImage(title, current);
		sleep(2);
    }

	if (fin.is_open()) {
		vector<point> linp, lout;
		string coord;
		int xin, yin, xout, yout;
		while(getline(fin, coord, ',')) {
			xin = atoi(coord.c_str());
			getline(fin, coord, ',');
			yin = atoi(coord.c_str());
			getline(fin, coord, ',');
			xout = atoi(coord.c_str());
			getline(fin, coord);
			yout = atoi(coord.c_str());
			point pin = {xin, yin};
			linp.push_back(pin);
			point pout = {xout, yout};
			lout.push_back(pout);
		}
		double metric = computeMatches(linp, lout, cells, finalPos);
		fout << "Matches score: " << metric << endl;
	}
	
    return 0;
}