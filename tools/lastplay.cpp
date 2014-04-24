#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <iostream>

#include "path.h"
using namespace std;
using namespace cv;
#define ABS(x) ((x) > 0 ? (x) : (-(x)))

IplImage * tmplHistBkgImg = 0;

IplImage * cropImage(IplImage * src, int x, int y, int width, int height)
{
	cvSetImageROI(src, cvRect(x, y, width , height));
	IplImage * dst = cvCreateImage(cvSize(width,height), IPL_DEPTH_8U , src->nChannels);
	cvCopy(src, dst, 0);
    cvResetImageROI(src);
	return dst;
}

void init()
{
	tmplHistBkgImg = cvLoadImage(SGSTMPLPATH"history_background.png",1);
	assert(tmplHistBkgImg != 0);
}

void drawSepLine(IplImage * image)
{
	int width = image->width;
	int height = image->height;
	int nchannels = image->nChannels;
	assert(nchannels == 3);
	vector<double> sumR(width, 0);
	vector<double> sumG(width, 0);
	vector<double> sumB(width, 0);
	vector<int> numVals(width, 0);
	for(int i = 0; i < width; i++)
	{
		for(int j = 0; j < height; j++)
		{
			int R = CV_IMAGE_ELEM(image, unsigned char, j, 3*i+2);
			int G = CV_IMAGE_ELEM(image, unsigned char, j, 3*i+1);
			int B = CV_IMAGE_ELEM(image, unsigned char, j, 3*i);
			if(R+G+B > 0)
			{
				numVals[i]++;
				sumR[i]+=R;
				sumG[i]+=G;
				sumB[i]+=B;
			}
		}
	}
	int left = 0, right = width - 1;
	while(numVals[left] == 0 && left < width) left++;
	while(numVals[right] == 0 && right >= 0) right--;
	if(left == width || right == -1) return;
	for(int i = left; i <= right; i++)
	{
		if(i < 12 || i >= width - 12) continue;
		double leftAvgR = 0.0;
		double leftAvgG = 0.0;
		double leftAvgB = 0.0;
		int leftNum = 0;
		double rightAvgR = 0.0;
		double rightAvgG = 0.0;
		double rightAvgB = 0.0;
		int rightNum = 0;
		for(int ii = i-12; ii < i; ii++)
		{
			leftAvgR += sumR[ii];
			leftAvgG += sumG[ii];
			leftAvgB += sumB[ii];
			leftNum += numVals[ii];
		}
		if(leftNum > 0) 
		{
			leftAvgR /= leftNum;
			leftAvgG /= leftNum;
			leftAvgB /= leftNum;
		}

		for(int ii = i; ii < i+12; ii++)
		{
			rightAvgR += sumR[ii];
			rightAvgG += sumG[ii];
			rightAvgB += sumB[ii];
			rightNum += numVals[ii];
		}
		if(rightNum > 0)
		{
			rightAvgR /= rightNum;
			rightAvgG /= rightNum;
			rightAvgB /= rightNum;
		}

		double diffVal = sqrt((rightAvgR - leftAvgR)*(rightAvgR - leftAvgR) + 
			(rightAvgG - leftAvgG)*(rightAvgG - leftAvgG) +
			(rightAvgB - leftAvgB)*(rightAvgB - leftAvgB));
		if(diffVal > 80)
		{
			cvLine(image, cvPoint(i, 0), cvPoint(i, height-1), CV_RGB(255, 0, 0), 1);
		}
	}
}

int main(int argc, char ** argv)
{
	if(argc == 1)
	{
		cout<<"Usage: "<<argv[0]<<" <screenimage>"<<endl;
		return 0;
	}
	init();
	IplImage * screenImg = cvLoadImage(argv[1], 1);
	IplImage * histBkgImg = cropImage(screenImg, 808, 120, 195, 223);
	assert(histBkgImg->width == tmplHistBkgImg->width);
	assert(histBkgImg->height == tmplHistBkgImg->height);
	assert(histBkgImg->nChannels == tmplHistBkgImg->nChannels);
	int width = histBkgImg->width;
	int height = histBkgImg->height;
	int nchannels = histBkgImg->nChannels;

	IplImage * diffImg = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, nchannels);
	vector<double> sumVals(height, 0);
	for(int j = 0; j < height; j++)
	{
		for(int i = 0; i < width; i++)
		{
			for(int c = 0; c < nchannels; c++)
			{
				int val0 = CV_IMAGE_ELEM(tmplHistBkgImg, unsigned char, j, i*nchannels + c);
				int val1 = CV_IMAGE_ELEM(histBkgImg, unsigned char, j, i*nchannels + c);
				int diffval = ABS(val0-val1);
				CV_IMAGE_ELEM(diffImg, unsigned char, j, i*nchannels+c) = diffval;
				sumVals[j] += diffval;
			}
		}
		//cout<<sumVals[j]<<endl;
	}
	int thresh = 50;
	for(int j = height - 1; j >= 0; j--)
	{
		if(sumVals[j] > thresh)
		{
			int y1 = j;
			while(j >= 0 && sumVals[j] > thresh) j--;
			int y0 = j+1;
			int count = y1 - y0 + 1;
			if(count >= 10 && count <= 12)
			{
				ostringstream oss;
				oss<<argv[1]<<".lastplay.png";
				IplImage * lineImg = cropImage(diffImg, 0, y0, width, 10);
				//drawSepLine(lineImg);
				cvSaveImage(oss.str().c_str(), lineImg);
				cvReleaseImage(&lineImg);
			}
			else if(count >= 20 && count <= 24)
			{
				ostringstream oss1;
				oss1<<argv[1]<<".lastplay1.png";
				IplImage * lineImg1 = cropImage(diffImg, 0, y0, width, 10);
				//drawSepLine(lineImg1);
				cvSaveImage(oss1.str().c_str(), lineImg1);
				cvReleaseImage(&lineImg1);

				ostringstream oss2;
				oss2<<argv[1]<<".lastplay2.png";
				IplImage * lineImg2 = cropImage(diffImg, 0, y0+12, width, 10);
				//drawSepLine(lineImg2);
				cvSaveImage(oss2.str().c_str(), lineImg2);
				cvReleaseImage(&lineImg2);
			}
			else cout<<argv[1]<<" invalid play"<<endl;
			break;
		}
	}
	//cvSaveImage("out.png", diffImg);
	cvReleaseImage(&screenImg);
	cvReleaseImage(&histBkgImg);
	cvReleaseImage(&tmplHistBkgImg);
	cvReleaseImage(&diffImg);
	return 0;
}
