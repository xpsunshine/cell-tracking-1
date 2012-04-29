#include <opencv2/video/tracking.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <iostream>
#include <vector>

using namespace std;
using namespace cv;

// will keep history of cursor and Kalman prediction positions
vector<Point> mouse_coords;
vector<Point> kalman_coords;

// size of canvas
int frame_size = 500;

struct point { int x,y; };
// will track last and current mouse position at each loop iteration
struct point curr_mouse = {-5,-5}, last_mouse;

/*
 * Updates last and current mouse positions.
 */
void mouse_update(int ev, int x_c, int y_c, int flag, void* param) {
	last_mouse = curr_mouse;
	curr_mouse.x = x_c;
	curr_mouse.y = y_c;
}

void drawX(Mat canvas, Point p, CvScalar color, int len) {
	line(canvas, Point( p.x - len, p.y - len),
	Point(p.x + len, p.y + len), color, 2, CV_AA, 0);
	line(canvas, Point( p.x + len, p.y - len),
	Point(p.x - len, p.y + len), color, 2, CV_AA, 0);
}

/*
 * Opens frame upon which user can hover mouse, and Kalman filter will begin tracking cursor.
 */
int main (int argc, char * args[]) {
	
	// Mat is the object that encapsulates images (pixel matrices) in openCV.
	// Initializes a blank canvas of size frame_size * frame_size, with each pixel taking values
	// that are 8-bit unsigned characters, 3 channels (RGB).
    Mat canvas(frame_size, frame_size, CV_8UC3);

	// OpenCV KalmanFilter, which implements 1) prediction and 2) correction calculations.
	// Initialized with 4-d state (x, y, velocity_x, velocity_y), 2-d measurement (only x and y
	// are observed by mouse).
    KalmanFilter kalman(4, 2, 0);

	// Records current position of cursor.
	Mat_<float> currPos(2,1); currPos.setTo(Scalar(0));
	
	// Used for keyboard input to quit program.
    char code = (char)-1;
	namedWindow("Black and Yellow");
	setMouseCallback("Black and Yellow", mouse_update, 0);
	
    while (true) {
		if (curr_mouse.x < 0 || curr_mouse.y < 0) {
			imshow("Black and Yellow", canvas);
			waitKey(30);
			continue;
		}
		
		// Initialize Kalman filter for mouse at mouse's on-screen position and with 0 velocity.
		kalman.statePre.at<float>(0) = curr_mouse.x;
 		kalman.statePre.at<float>(1) = curr_mouse.y;
		kalman.statePre.at<float>(2) = 0;
		kalman.statePre.at<float>(3) = 0;
		
		// For simplicity's sake, set the Kalman matrices as follows (discovered after a bit of tuning).
		kalman.transitionMatrix = *(Mat_<float>(4, 4) << 1,0,0,0,   0,1,0,0,  0,0,1,0,  0,0,0,1);
        setIdentity(kalman.measurementMatrix);
        setIdentity(kalman.processNoiseCov, Scalar::all(1e-4));
        setIdentity(kalman.measurementNoiseCov, Scalar::all(1e-1));
        setIdentity(kalman.errorCovPost, Scalar::all(.1));
		
		// Make sure history of mouse and Kalman coordinates is cleared everytime mouse moves back
		// on-screen.
		mouse_coords.clear();
		kalman_coords.clear();
		
        while (true) {
	
			// Record curent mouse cursor position.
	        currPos(0) = curr_mouse.x;
			currPos(1) = curr_mouse.y;
			Point measPt(currPos(0),currPos(1));
			mouse_coords.push_back(measPt);
			
			// Perform Kalman filter algorithm. First, predict next position of "particle."
			// Then, correct it based on current mouse position.
            Mat prediction = kalman.predict();
            Point predictPt(prediction.at<float>(0),prediction.at<float>(1));
			Mat estimated = kalman.correct(currPos);
			Point estimated_pt(estimated.at<float>(0),estimated.at<float>(1));
			kalman_coords.push_back(estimated_pt);
			
			// Update UI: redraw new mouse cursor point and Kalman (corrected) point.
            canvas = Scalar::all(0);
            drawX(canvas, estimated_pt, Scalar(0,255,255), 5);
            drawX(canvas, measPt, Scalar(0,100,100), 5);
			for (int i = 0; i < mouse_coords.size()-1; i++) {
				line(canvas, mouse_coords[i], mouse_coords[i+1], Scalar(0,100,100), 1);
			}
			for (int i = 0; i < kalman_coords.size()-1; i++) {
				line(canvas, kalman_coords[i], kalman_coords[i+1], Scalar(0,255,255), 1);
			}
            imshow( "Black and Yellow", canvas );

			// If user wants to quit, then break.
            code = (char)waitKey(100);
            if( code == 'q' || code == 'Q' )
	            break;
        }
        if( code == 'q' || code == 'Q' )
            break;
    }
	
    return 0;
}