#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <cassert>
#include "path.h"

using namespace std;
using namespace cv;
#define ABS(x) ((x) > 0 ? (x) : (-(x)))

IplImage * tmplWeiImg = 0;
IplImage * tmplShuImg = 0;
IplImage * tmplWuImg = 0;
IplImage * tmplQunImg = 0;

void init()
{
	tmplWeiImg = cvLoadImage(SGSTMPLPATH"wei.png", 1);
	tmplShuImg = cvLoadImage(SGSTMPLPATH"shu.png", 1);
	tmplWuImg = cvLoadImage(SGSTMPLPATH"wu.png", 1);
	tmplQunImg = cvLoadImage(SGSTMPLPATH"qun.png", 1);
	assert(tmplWeiImg->width == 19 && tmplWeiImg->height == 14 && tmplWeiImg->nChannels == 3);
	assert(tmplShuImg->width == 19 && tmplShuImg->height == 14 && tmplShuImg->nChannels == 3);
	assert(tmplWuImg->width == 19 && tmplWuImg->height == 14 && tmplWuImg->nChannels == 3);
	assert(tmplQunImg->width == 19 && tmplQunImg->height == 14 && tmplQunImg->nChannels == 3);
}
double imageDiff(IplImage * bigImage, IplImage * smallImage, int x0, int y0)
{
	int width = smallImage->width;
	int height = smallImage->height;
	int nchannels = smallImage->nChannels;
	assert(bigImage->nChannels == smallImage->nChannels);
	assert(bigImage->width >= smallImage->width && bigImage->height >= smallImage->height);
	double sumdiff = 0.0;
	for(int j = 0; j < height; j++)
	{
		for(int i = 0; i < width; i++)
		{
			int jj = j + y0;
			int ii = i + x0;
			for(int c = 0; c < nchannels; c++)
			{
				double val1 = CV_IMAGE_ELEM(smallImage, unsigned char, j, i*nchannels+c);
				double val2 = CV_IMAGE_ELEM(bigImage, unsigned char, jj, ii*nchannels+c);
				sumdiff += ABS(val1-val2);
			}
		}
	}
	return sumdiff;
}

int main(int argc, char ** argv)
{
	if(argc < 2)
	{
		cout<<"Usage: "<<argv[0]<<" <screenimg>"<<endl;
		return 0;
	}
	init();
	IplImage * screenImage = cvLoadImage(argv[1], 1);
	IplImage * drawImage = cvLoadImage(argv[1], 1);
	assert(screenImage->width == 1016 && screenImage->height == 638);
	int screenWidth = screenImage->width;
	int screenHeight = screenImage->height;
	int xposes[] = {706,667,667,498,337,176,5,5};
	int yposes[] = {500,278,102,51,51,51,102,278};
	int n = 8;
	for(int i = 0; i < n; i++)
	{
		int x = xposes[i];
		int y = yposes[i];
		if(imageDiff(screenImage, tmplWeiImg, x, y) == 0) cvRectangle(drawImage, cvPoint(x, y), cvPoint(x+19, y+14), CV_RGB(255,0,0),2);
		else if(imageDiff(screenImage, tmplShuImg, x, y) == 0) cvRectangle(drawImage, cvPoint(x, y), cvPoint(x+19, y+14), CV_RGB(0,255,0),2);
		else if(imageDiff(screenImage, tmplWuImg, x, y) == 0) cvRectangle(drawImage, cvPoint(x, y), cvPoint(x+19, y+14), CV_RGB(0,0,255),2);
		else if(imageDiff(screenImage, tmplQunImg, x, y) == 0) cvRectangle(drawImage, cvPoint(x, y), cvPoint(x+19, y+14), CV_RGB(255,255,0),2);
		else cvRectangle(drawImage, cvPoint(x, y), cvPoint(x+19, y+14), CV_RGB(255,0,255),2);
	}
	cvSaveImage("out.png", drawImage);
	cvReleaseImage(&screenImage);
	cvReleaseImage(&drawImage);
}
