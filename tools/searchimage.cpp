#include <opencv/cv.h>
#include <opencv/highgui.h>

using namespace std;
using namespace cv;
#define ABS(x) ((x) > 0 ? (x) : (-(x)))

int main(int argc, char ** argv)
{
	if(argc < 3)
	{
		cout<<"Usage: "<<argv[0]<<" <smallimg> <bigimg>"<<endl;
		return 0;
	}
	IplImage * smallImage = cvLoadImage(argv[1], 1);
	IplImage * bigImage = cvLoadImage(argv[2], 1);
	IplImage * drawImage = cvLoadImage(argv[2], 1);
	assert(bigImage->width == 1016 && bigImage->height == 638);
	assert(bigImage->width >= smallImage->width && bigImage->height >= smallImage->height);
	assert(bigImage->nChannels == smallImage->nChannels);
	int smallWidth = smallImage->width;
	int smallHeight = smallImage->height;
	int bigWidth = bigImage->width;
	int bigHeight = bigImage->height;
	int nchannels = bigImage->nChannels;
	for(int j = 0; j <= bigHeight - smallHeight; j++)
	{
		for(int i = 0; i <= bigWidth - smallWidth; i++)
		{
			double sumdiff = 0;
			for(int y = 0; y < smallHeight; y++)
			{
				for(int x = 0; x < smallWidth; x++)
				{
					int by = j + y;
					int bx = i + x;
					for(int c = 0; c < nchannels; c++)
					{
						double val1 = CV_IMAGE_ELEM(smallImage, unsigned char, y, x*nchannels + c);
						double val2 = CV_IMAGE_ELEM(bigImage, unsigned char, by, bx*nchannels + c);
						sumdiff += ABS(val1-val2);
					}
				}
			}
			if(sumdiff == 0)
			{
				cout<<"("<<j<<","<<i<<")"<<endl;
				cvRectangle(drawImage, cvPoint(i, j), cvPoint(i+smallWidth, j+smallHeight), CV_RGB(255,0,0),2);
			}
		}
	}
	cvSaveImage("out.png", drawImage);
	cvReleaseImage(&smallImage);
	cvReleaseImage(&bigImage);
	cvReleaseImage(&drawImage);
}
