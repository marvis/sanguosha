#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <iostream>
#include <path.h>
using namespace std;
using namespace cv;
#define ABS(x) ((x) > 0 ? (x) : (-(x)))
#define DEBUG 0

string infile = "";
IplImage * cropImage(IplImage * src, int x, int y, int width, int height)
{
	cvSetImageROI(src, cvRect(x, y, width , height));
	IplImage * dst = cvCreateImage(cvSize(width,height), IPL_DEPTH_8U , src->nChannels);
	cvCopy(src, dst, 0);
    cvResetImageROI(src);
	return dst;
}

void lastplay(IplImage * screenImg, IplImage * & lastImg1, IplImage * &lastImg2)
{
	IplImage * tmplHistBkgImg = cvLoadImage(SGSTMPLPATH"history_background.png",1);
	IplImage * histBkgImg = cropImage(screenImg, 808, 120, 195, 223);
	assert(tmplHistBkgImg && histBkgImg);
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
	}
	bool isok = false;
	for(int j = height - 1; j >= 0; j--)
	{
		if(sumVals[j] != 0)
		{
			int y1 = j;
			while(j >= 0 && sumVals[j] != 0) j--;
			int y0 = j+1;
			int count = y1 - y0 + 1;
			if(count >= 11 && count <= 12)
			{
				lastImg1 = cropImage(diffImg, 0, y0, width, 10);
				lastImg2 = 0;
			}
			else if(count >= 22 && count <= 24)
			{
				lastImg1 = cropImage(diffImg, 0, y0, width, 10);
				lastImg2 = cropImage(diffImg, 0, y0+12, width, 10);
			}
			else 
			{
				lastImg1 = 0;
				lastImg2 = 0;
			}
			isok = true;
			break;
		}
	}
	if(!isok)
	{
		lastImg1 = 0;
		lastImg2 = 0;
	}
	cvReleaseImage(&histBkgImg);
	cvReleaseImage(&tmplHistBkgImg);
	cvReleaseImage(&diffImg);
}

bool isImageSame(IplImage * bigImage, int x0, int y0, int width, int height, IplImage * smallImage)
{
	assert(smallImage && bigImage && bigImage->width >= smallImage->width && bigImage->height >= smallImage->height && bigImage->nChannels == smallImage->nChannels);
	assert(smallImage->width == width && smallImage->height == height);
	assert(x0 >= 0 && y0 >= 0 && x0 + width <= bigImage->width && y0 + height <= bigImage->height);
	int nchannels = bigImage->nChannels;
	double sumdiff = 0;
	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			int by = y0 + y;
			int bx = x0 + x;
			for(int c = 0; c < nchannels; c++)
			{
				double val1 = CV_IMAGE_ELEM(smallImage, unsigned char, y, x*nchannels + c);
				double val2 = CV_IMAGE_ELEM(bigImage, unsigned char, by, bx*nchannels + c);
				sumdiff += ABS(val1-val2);
			}
		}
	}
	return (sumdiff/(width*height) < 5);
}

int find_right_boundary(IplImage * image)
{
	int width = image->width;
	int height = image->height;
	int nchannels = image->nChannels;
	vector<double> sumVals(width, 0);
	for(int i = 0; i < width; i++)
	{
		for(int j = 0; j < height; j++)
		{
			for(int c = 0; c < nchannels; c++)
			{
				sumVals[i] += CV_IMAGE_ELEM(image, unsigned char, j, i*nchannels+c);
			}
		}
	}
	int right = width - 1;
	while(sumVals[right] == 0 && right >= 0) right--;
	return right;
}

#define NUM_WID  6 // 数字宽度
#define CHN_WID  12 // 中文宽度
#define YOU_WID  19 // 中文宽度
class SGSRecg
{
	public:
		enum Status{FIRST_SHOW_HAO};

	public:
		int leftside;
		int rightside1;
		int rightside2;
		IplImage * img1;
		IplImage * img2;
		IplImage * curImg;
		int x0;
		int imgHei;
		string result;
		bool isImageChanged;

		SGSRecg(IplImage * _img1, IplImage * _img2)
		{
			img1 = _img1;
			img2 = _img2;
			leftside = 3;
			rightside1 = find_right_boundary(img1);
			if(img2) rightside2 = find_right_boundary(img2);
			imgHei = 10;
			assert(img1->height = imgHei);
			if(img2) assert(img2->height == imgHei);

			x0 = leftside;
			curImg = img1;
			isImageChanged = false;
		}

		bool isFirstShow()
		{
			IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_first_show_hao.png", 1);
			bool result = isImageSame(img1, leftside + NUM_WID, 0, CHN_WID, imgHei, tmpImg);
			cvReleaseImage(&tmpImg);
			return result;
		}
		int FirstShow_who(IplImage * image, int x0, int width)
		{
			int y0 = 0;
			int height = imgHei;
			string filenames[] = {"sgs_first_show_1.png", "sgs_first_show_2.png", "sgs_first_show_3.png", "sgs_first_show_4.png",
			"sgs_first_show_5.png", "sgs_first_show_6.png", "sgs_first_show_7.png", "sgs_first_show_8.png"};
			for(int i = 0; i < 8; i++)
			{
				IplImage * tmpImg = cvLoadImage(string(SGSTMPLPATH + filenames[i]).c_str(), 1);
				bool result = isImageSame(image, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				if(result) return i+1;
			}
			return 0;
		}
		string FirstShow_recog()
		{
			int x0 = leftside;
			IplImage * curImg = img1;
			int who = FirstShow_who(curImg, x0, NUM_WID); x0 += NUM_WID;
			ostringstream oss;
			oss<<who<<"号位首先明置武将牌, 摸2张牌作为奖励";
			return oss.str();
		}
		bool refresh_x0_and_curImg(int width)
		{
			if(!isImageChanged)
			{
				if(x0 + width < rightside1)
				{
					x0 = x0 + width;
					return true;
				}
				else if(img2)
				{
					x0 = leftside;
					curImg = img2;
					isImageChanged = true;
					return true;
				}
				else return false;
			}
			else
			{
				if(x0 + width < rightside2)
				{
					x0 = x0 + width;
					return true;
				}
				else return false;
			}
			return false;
		}
		string recog_who(IplImage * image)
		{
			int y0 = 0;
			int width = CHN_WID;
			int height = imgHei;
			string filenames[] = {"sgs_chinese_one.png", "sgs_chinese_two.png", "sgs_chinese_three.png", "sgs_chinese_four.png",
			"sgs_chinese_five.png", "sgs_chinese_six.png", "sgs_chinese_seven.png", "sgs_chinese_eight.png"};
			string names[] = {"一", "二", "三", "四", "五", "六", "七", "八"};
			string out = "??";
			for(int i = 0; i < 8; i++)
			{
				IplImage * tmpImg = cvLoadImage(string(SGSTMPLPATH + filenames[i]).c_str(), 1);
				bool result = isImageSame(image, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				if(result)
				{
					out = names[i];
					break;
				}
			}
			assert(refresh_x0_and_curImg(CHN_WID));
			out += "号";	
			assert(refresh_x0_and_curImg(CHN_WID));
			out += "位";	
			assert(refresh_x0_and_curImg(CHN_WID));
			if(isWord(curImg, x0, "SGS_YOU"))
			{
				out += "(你)";
				assert(refresh_x0_and_curImg(YOU_WID));
			}

#if DEBUG
			int nchannels = curImg->nChannels;
			IplImage * tmpImg = cvCreateImage(cvSize(CHN_WID, imgHei), IPL_DEPTH_8U, nchannels);
			for(int j = 0; j < imgHei; j++)
			{
				for(int i = 0; i < CHN_WID; i++)
				{
					for(int c = 0; c < nchannels; c++)
					{
						CV_IMAGE_ELEM(tmpImg, unsigned char, j, i*nchannels+c) = CV_IMAGE_ELEM(curImg, unsigned char, j, (i+x0)*nchannels + c);
					}
				}
			}
			cvSaveImage((infile+".out.png").c_str(), tmpImg);
			cvReleaseImage(&tmpImg);
#endif
				
			return out;
		}
		bool isWord(IplImage * image, int x0, string type)
		{
			int y0 = 0;
			int height = imgHei;
			if(type == "SGS_YOU")
			{
				int width = YOU_WID;
				IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_special_you.png", 1);
				bool result = isImageSame(image, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				return result;
			}
			else if(type == "SGS_CHN_CONG")
			{
				int width = CHN_WID;
				IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_chinese_cong.png", 1);
				bool result = isImageSame(image, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				return result;
			}
			else if(type == "SGS_CHN_ZHUANG")
			{
				int width = CHN_WID;
				IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_chinese_zhuang.png", 1);
				bool result = isImageSame(image, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				return result;
			}
			else if(type == "SGS_CHN_SHI")
			{
				int width = CHN_WID;
				IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_chinese_shi.png", 1);
				bool result = isImageSame(image, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				return result;
			}
			else if(type == "SGS_CHN_SHOU")
			{
				int width = CHN_WID;
				IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_chinese_shou.png", 1);
				bool result = isImageSame(image, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				return result;
			}
			else if(type == "SGS_CHN_DA")
			{
				int width = CHN_WID;
				IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_chinese_da.png", 1);
				bool result = isImageSame(image, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				return result;
			}
			else if(type == "SGS_CHN_DE")
			{
				int width = CHN_WID;
				IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_chinese_de.png", 1);
				bool result = isImageSame(image, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				return result;
			}
			else if(type == "SGS_CHN_QI")
			{
				int width = CHN_WID;
				IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_chinese_qi.png", 1);
				bool result = isImageSame(image, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				return result;
			}
			else if(type == "SGS_CHN_FA")
			{
				int width = CHN_WID;
				IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_chinese_fa.png", 1);
				bool result = isImageSame(image, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				return result;
			}
			else if(type == "SGS_CHN_HUO")
			{
				int width = CHN_WID;
				IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_chinese_huo.png", 1);
				bool result = isImageSame(image, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				return result;
			}
			else if(type == "SGS_CHN_DUI")
			{
				int width = CHN_WID;
				IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_chinese_dui.png", 1);
				bool result = isImageSame(image, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				return result;
			}
			else if(type == "SGS_CHN_HUI")
			{
				int width = CHN_WID;
				IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_chinese_hui.png", 1);
				bool result = isImageSame(image, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				return result;
			}
			else if(type == "SGS_CHN_YU")
			{
				int width = CHN_WID;
				IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_chinese_yu.png", 1);
				bool result = isImageSame(image, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				return result;
			}
			else if(type == "SGS_CHN_XIANG")
			{
				int width = CHN_WID;
				IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_chinese_xiang.png", 1);
				bool result = isImageSame(image, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				return result;
			}
			else if(type == "SGS_CHN_GUAN")
			{
				int width = CHN_WID;
				IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_chinese_guan.png", 1);
				bool result = isImageSame(image, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				return result;
			}
			return false;
		}

		string recog()
		{
			if(isFirstShow()) return FirstShow_recog();
			x0 = leftside;
			curImg = img1;

			string who = recog_who(curImg); // x0 and curImg is set in this function
			string next = "";
			if(who == "??号位") next = "";
			else next = " what";
			if(isWord(curImg, x0, "SGS_CHN_CONG"))
			{
				next = "从牌堆里摸了";
			}
			else if(isWord(curImg, x0, "SGS_CHN_ZHUANG"))
			{
				next = "装备了";
			}
			else if(isWord(curImg, x0, "SGS_CHN_SHI"))
			{
				next = "使用了";
			}
			else if(isWord(curImg, x0, "SGS_CHN_SHOU"))
			{
				next = "受到";
			}
			else if(isWord(curImg, x0, "SGS_CHN_DA"))
			{
				next = "打出卡牌";
			}
			else if(isWord(curImg, x0, "SGS_CHN_DE"))
			{
				next = "的";
			}
			else if(isWord(curImg, x0, "SGS_CHN_QI"))
			{
				next = "弃掉";
			}
			else if(isWord(curImg, x0, "SGS_CHN_FA"))
			{
				next = "发动了武将技能";
			}
			else if(isWord(curImg, x0, "SGS_CHN_HUO"))
			{
				next = "获得";
			}
			else if(isWord(curImg, x0, "SGS_CHN_DUI"))
			{
				next = "对";
			}
			else if(isWord(curImg, x0, "SGS_CHN_HUI"))
			{
				next = "恢复了";
			}
			else if(isWord(curImg, x0, "SGS_CHN_YU"))
			{
				next = "与";
			}
			else if(isWord(curImg, x0, "SGS_CHN_XIANG"))
			{
				next = "向";
			}
			else if(isWord(curImg, x0, "SGS_CHN_GUAN"))
			{
				next = "观看了";
			}
			ostringstream oss;
			oss<<who<<next;
			return oss.str();
		}
};

int main(int argc, char ** argv)
{
	if(argc == 1)
	{
		cout<<"Usage: "<<argv[0]<<" <screen_image>"<<endl;
		return 0;
	}
	infile = argv[1];
	IplImage * screenImg = cvLoadImage(argv[1], 1);
	IplImage * lastImg1 = 0, * lastImg2 = 0;
	lastplay(screenImg, lastImg1, lastImg2);
	if(lastImg1 == 0 && lastImg2 == 0)
	{
		cout<<"Invalid data"<<endl;
		return 0;
	}
	SGSRecg sgs(lastImg1, lastImg2);
	string str = sgs.recog();
	cout<<str<<endl;

	cvSaveImage("out1.png", lastImg1);
	if(lastImg2) cvSaveImage("out2.png", lastImg2);
	cvReleaseImage(&screenImg);
	cvReleaseImage(&lastImg1);
	if(lastImg2) cvReleaseImage(&lastImg2);
}
