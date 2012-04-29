cell-tracking
=============

Usage:

1. Install OpenCV 2.2 or higher: http://opencv.willowgarage.com/wiki/InstallGuide.
2. Pull from our git repo: git@github.com:abliu/cell-tracking.git.
3. Run make.
4. To use the mouse tracking Kalman filter, run ./main.
5. To use the cell tracking Kalman filter, run
    ./tracking [-i input-file] [-o output-file] frame_1 frame_2 ... frame_n
where input-file is a csv file of m lines, with each line having 4 comma-delimited
integers xin_i, yin_i, xout_i, yout_i, which are the hand-annotated (x,y)
coordinates of the i-th cell in the first and last frames, respectively; where
output-file is the name of a csv which will contain the coordinates of each cell
in each frame; and where frame_1, frame_2, ..., frame_n are the n frames in which
cells are to be tracked.

For your sample usage, we have provided an input file (input.csv) and sample frames
(in the images file). The corresponding sample command would be
    ./tracking -i input.csv -o output.csv images/Pics_159.png images/Pics_160.png
images/Pics_161.png images/Pics_162.png

Code Architecture:

main.cpp - self-contained Kalman filter and UI for mouse tracking.
tracking.cpp - main for cell tracking in images; usage is as above.
blobs.cpp and blobs.h - conducts blob analysis to identify cells in a static frame.
correctness.cpp - provides functionality for scoring algorithm.
