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
struct point curr_mouse = {-1,-1}, last_mouse;

/*
 * Updates last and current mouse positions.
 */
void on_mouse(int ev, int x_c, int y_c, int flag, void* param) {
	last_mouse = curr_mouse;
	curr_mouse.x = x_c;
	curr_mouse.y = y_c;
}

int main (int argc, char * args[]) {
	
	// Mat is the object that encapsulates images (pixel matrices) in openCV.
	// Initializes a blank canvas of size frame_size * frame_size, with each pixel taking values
	// that are 8-bit unsigned characters, 3 channels (RGB).
    Mat canvas(frame_size, frame_size, CV_8UC3);

	// OpenCV KalmanFilter, which implements 1) prediction and 2) correction calculations.
	// Initialized with 4-d state (x, y, velocity_x, velocity_y), 2-d measurement (only x and y
	// are observed by mouse).
    KalmanFilter kalman(4, 2, 0);

	// Current (x, y, velocity_x, velocity_y) vector at each iteration.
    Mat_<float> state_velocity(4, 1);

	// Process 
    Mat processNoise(4, 1, CV_32F);
    Mat_<float> measurement(2,1); measurement.setTo(Scalar(0));
    char code = (char)-1;
	
	namedWindow("Mouse Tracking");
	setMouseCallback("Mouse Tracking", on_mouse, 0);
	
    while (true) {
		if (curr_mouse.x < 0 || curr_mouse.y < 0) {
			imshow("Mouse Tracking", canvas);
			waitKey(30);
			continue;
		}
		kalman.statePre.at<float>(0) = curr_mouse.x;
 		kalman.statePre.at<float>(1) = curr_mouse.y;
		kalman.statePre.at<float>(2) = 0;
		kalman.statePre.at<float>(3) = 0;
		kalman.transitionMatrix = *(Mat_<float>(4, 4) << 1,0,0,0,   0,1,0,0,  0,0,1,0,  0,0,0,1);
		
        setIdentity(kalman.measurementMatrix);
        setIdentity(kalman.processNoiseCov, Scalar::all(1e-4));
        setIdentity(kalman.measurementNoiseCov, Scalar::all(1e-1));
        setIdentity(kalman.errorCovPost, Scalar::all(.1));
		
		mouse_coords.clear();
		kalman_coords.clear();
		
        while (true)
        {			
            Mat prediction = kalman.predict();
            Point predictPt(prediction.at<float>(0),prediction.at<float>(1));
			
            measurement(0) = curr_mouse.x;
			measurement(1) = curr_mouse.y;
			
			Point measPt(measurement(0),measurement(1));
			mouse_coords.push_back(measPt);

			Mat estimated = kalman.correct(measurement);
			Point state_velocity_pt(estimated.at<float>(0),estimated.at<float>(1));
			kalman_coords.push_back(state_velocity_pt);
			
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
			
            imshow( "Mouse Tracking", canvas );
            code = (char)waitKey(100);
			
            if( code == 'q' || code == 'Q' )
	            break;
        }
        if( code == 'q' || code == 'Q' )
            break;
    }
	
    return 0;
}