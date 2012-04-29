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

// distance between two points
#ifndef dist(a, b)
#define dist(a, b) sqrt(pow((double)(a.x - b.x), 2) + pow((double)(a.y - b.y), 2))
#endif

/*
 * Get the index of closest point to point a among pts
 */
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

/*
 * Takes in an input file csv (of manually annotated ) and two vectors of points,
 * ainp (algorithmically calculated points in first frame) and aout (same for last frame).
 */
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

/*
 * Performs cell tracking over input set of images and (optionally) spits out point coordinates
 * and correctness score.
 */
int main (int argc, char * args[]) {
	
	// To spit out coordinates and correctness score
	ifstream fin;
	ofstream fout;
	vector<char *> imageNames;
	
	// Parse command line
	for (int i = 1; i < argc; i++) {
		if (args[i][0] == '-') {
			// input file flag
			if (args[i][1] == 'i') {
				if (i+1>argc) {
					cout << "Must have input csv following -i tag and space" << endl;
					exit(1);
				}
				fin.open(args[i+1]);
				i++;
			// output file flag
			} else if (args[i][1] == 'o') {
				if (i+1>argc) {
					cout << "Must have output file name following -o tag and space" << endl;
					exit(1);
				}
				fout.open(args[i+1]);
				i++;
			}
		// otherwise image name
		} else {
			imageNames.push_back(args[i]);
		}
	}
	
	// Stores current image frame that in which cells are being identified and tracked.
	IplImage *current = 0;
	
	// Must have at least one image frame, and must have output file if input file is given
	// (to write score to).
	int num_frames = imageNames.size();
	if (num_frames == 0) {
		cout << "Must have at least 1 image" << endl;
		exit(1);
	}
	if (fin.is_open() && !fout.is_open()) {
		cout << "Must have output file with input file to write score out" << endl;
		exit(1);
	}
	
	// Identify cells in first frame.
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

	// currPos vector (observed x, observed y) at each iteration
    Mat_<float> currPos(2,1); currPos.setTo(Scalar(0));

	// font for labeling cells in picture
	CvFont font;
	double hScale=0.5;
	double vScale=0.5;
	int lineWidth=1;
	cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX|CV_FONT_ITALIC, hScale,vScale,0,lineWidth);
	
	// Display frame 0 with identified cells from blob analysis
	cvNamedWindow("Frame 0");
	for (int i = 0; i < cell_count; i++) {
		cvPutText(current, stringOfInt(i).c_str(), cvPoint(cells.at(i).x, cells.at(i).y), &font, Scalar(255, 255, 255));
	}
	cvShowImage("Frame 0", current);
	sleep(2);
	
	// Initialize Kalman filter for mouse at mouse's on-screen position and with 0 velocity.
	// For simplicity's sake, set the Kalman matrices as follows (discovered after a bit of tuning).	
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
		
		// Display frame i
		current = cvLoadImage(imageNames.at(i), CV_LOAD_IMAGE_UNCHANGED);
		nextCells = identifyCells(current);
		if (fout)
			fout << "Frame " << i << endl;
		stringstream name;
		name << "Frame " << i;
		const char *title = name.str().c_str();
		cvNamedWindow(title);
		
		for (int j = 0; j < cell_count; j++) {
			
			// Predict next location of cell j
			Mat prediction = kalmans.at(j).predict();
			point predictPt = {(int)prediction.at<float>(0), (int)prediction.at<float>(1)};
			
			// Get closest cell that is actually identified (via blob analysis) and denote that
			// as the tracked cell
			int minIndex = getIndexClosest(predictPt, nextCells);
			currPos(0) = nextCells.at(minIndex).x;
			currPos(1) = nextCells.at(minIndex).y;
			
			// Don't take that cell as the 100 percent correct matching cell
			// Correct updated location based on trajectory
			Mat estimate = kalmans.at(j).correct(currPos);
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

	// Calculate correctness score - essentially the percentage of matches between first frame and last
	// fin gives hand-annotated input.
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