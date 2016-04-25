#include "opencv/cv.h"
#include "opencv/highgui.h"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
 
using namespace std;
using namespace cv;

int main(int argc, char *argv[])
{

	int cnt = 0;

    cv::VideoCapture capture(0);
	IplImage *frame;
	IplImage *red;
    cv::Mat ref;
    cv::Mat  blackbox;

	int width, height;	// Ã¢ÀÇ ³ÐÀÌ, ³ôÀÌ
	int i, j, index;
	unsigned char R, G, B;

	CvCapture* capture1 = cvCaptureFromCAM(0);
	frame = cvQueryFrame(capture1);


    cv::Mat tpl = cv::imread("/home/odroid/Desktop/lena.jpg");

    cv::Mat gref, gtpl;

    if( !capture.isOpened() ) {
        std::cerr << "Could not open camera" << std::endl;
        return 0;
    }
 
    while (true) {
        bool frame_valid = true;
 
            capture >> ref; // get a new frame from webcam
																																frame = new IplImage(ref);

																																width = frame->width;
																																height = frame->height;
																																cvResizeWindow("Red Tracking", width, height);
																																red = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 3);
																															for (i = 0; i < (frame->height); i++) {
																																		for (j = 0; j < (frame->widthStep); j += (frame->nChannels)) {
																																			index = i * frame->widthStep + j;
																															
																																			R = frame->imageData[index + 2];	// R¿µ¿ª
																																			G = frame->imageData[index + 1];	// G¿µ¿ª
																																			B = frame->imageData[index + 0];	// B¿µ¿ª
																															
																																			// ºÓÀº»öÀž·Î ¿¹»óµÇŽÂ ¹üÀ§ Œ³Á€
																																			if (R > 100 && G < 200 && B < 200) {
																																				// Èò»öÀž·Î
																																				red->imageData[(i*frame->widthStep) + j + 0] = 255;
																																				red->imageData[(i*frame->widthStep) + j + 1] = 255;
																																				red->imageData[(i*frame->widthStep) + j + 2] = 255;
																																			}
																																			else {
																																				// °ËÁ€»öÀž·Î
																																				red->imageData[(i*frame->widthStep) + j + 0] = 0;
																																				red->imageData[(i*frame->widthStep) + j + 1] = 0;
																																				red->imageData[(i*frame->widthStep) + j + 2] = 0;
																																			}
																																		}
																																	}

																																	//cvShowImage("Red Tracking", red);
																																	//cv::Mat blackbox(red);
																																	blackbox = cvarrToMat(red);
																																	//cvReleaseImage(&red);

									    cv::cvtColor(ref, gref, CV_BGR2GRAY);
									    cv::cvtColor(tpl, gtpl, CV_BGR2GRAY);
									
									    cv::Mat res(ref.rows-tpl.rows+1, ref.cols-tpl.cols+1, CV_32FC1);
									    cv::matchTemplate(gref, gtpl, res, CV_TM_CCOEFF_NORMED);
									    cv::threshold(res, res, 0.8, 1., CV_THRESH_TOZERO);
 
        if (frame_valid) {
            try {

									while (true) 
									    {
									        double minval, maxval, threshold = 0.8;
									        cv::Point minloc, maxloc;
									        cv::minMaxLoc(res, &minval, &maxval, &minloc, &maxloc);
									
									        if (maxval >= threshold)
									        {
									            cv::rectangle(
									                blackbox, 
									                maxloc, 
									                cv::Point(maxloc.x + tpl.cols, maxloc.y + tpl.rows), 
									                CV_RGB(0,255,0), 2
									            );
									            cv::floodFill(res, maxloc, cv::Scalar(0), 0, cv::Scalar(.1), cv::Scalar(1.));
												cnt++;
									        }
									        else{
											cout << cnt << endl;
											cnt = 0;
									            break; 

										}
									    }
    cv::imshow("reference", blackbox);
 
            } catch(cv::Exception& e) {
                std::cerr << "Exception occurred. Ignoring frame... " << e.err
                          << std::endl;
            }
        }
        if (cv::waitKey(30) >= 0) break;
    }
 
    // VideoCapture automatically deallocate camera object
    return 0;
}