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

double avgImgDiff(IplImage * img1, IplImage * img2)
{
	assert(img1->width == img2->width && img1->height == img2->height && img1->nChannels == img2->nChannels);
	int width = img1->width;
	int height = img1->height;
	int nchannels = img1->nChannels;
	double sumdiff = 0;
	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			for(int c = 0; c < nchannels; c++)
			{
				double val1 = CV_IMAGE_ELEM(img1, unsigned char, y, x*nchannels + c);
				double val2 = CV_IMAGE_ELEM(img2, unsigned char, y, x*nchannels + c);
				sumdiff += ABS(val1-val2);
			}
		}
	}
	return (sumdiff/(width*height));
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

// width and height here is used for size check
bool isImageSame(IplImage * bigImage, int x0, int y0, int width, int height, IplImage * smallImage, double thresh = 15)
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
	return (sumdiff/(width*height) < thresh);
}

// assume y0 = 0
bool isImageSame(IplImage * bigImage, int x0, string filename, double thresh = 15)
{
	IplImage * smallImage = cvLoadImage((SGSTMPLPATH + filename).c_str(), 1);
	if(!smallImage)
		cerr<<filename<<" does not exist"<<endl;
	assert(smallImage);
	bool result = isImageSame(bigImage, x0, 0, smallImage->width, smallImage->height, smallImage, thresh);
	cvReleaseImage(&smallImage);
	return result;
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

#define J_WID  4 // J宽度
#define NUM_WID  6 // 数字宽度
#define QKA_WID  8 // J宽度
#define CHN_WID  12 // 中文宽度
#define YOU_WID  19 // 中文宽度
#define CARD_TYPE_WID 9 // 桃杏梅方宽度
#define QUOTE_WID 5 // 桃杏梅方宽度
class SGSRecg
{
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
		bool isfinished;

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
			isfinished = false;
		}

		bool isFirstShow()
		{
			IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_first_show_hao.png", 1);
			bool result = isImageSame(img1, leftside + NUM_WID, 0, CHN_WID, imgHei, tmpImg);
			cvReleaseImage(&tmpImg);
			return result;
		}
		int FirstShow_who()
		{
			int y0 = 0;
			int height = imgHei;
			string filenames[] = {"sgs_digit_1.png", "sgs_digit_2.png", "sgs_digit_3.png", "sgs_digit_4.png",
			"sgs_digit_5.png", "sgs_digit_6.png", "sgs_digit_7.png", "sgs_digit_8.png"};
			int id = 0;
			for(int i = 0; i < 8; i++)
			{
				IplImage * tmpImg = cvLoadImage(string(SGSTMPLPATH + filenames[i]).c_str(), 1);
				bool result = isImageSame(curImg, x0, y0, NUM_WID, height, tmpImg, 10);
				cvReleaseImage(&tmpImg);
				if(result) 
				{
					id = i+1;
					break;
				}
			}
			if(id == 0)
			{
				cerr<<"unknow player"<<endl;
				saveImage(NUM_WID, infile+".unknown_player_id.png");
			}
			assert(refresh_x0_and_curImg(NUM_WID));
			return id;
		}
		string FirstShow_recog()
		{
			int x0 = leftside;
			IplImage * curImg = img1;
			int who = FirstShow_who();
			ostringstream oss;
			oss<<who<<"号位首先明置武将牌, 摸2张牌作为奖励";
			return oss.str();
		}
		bool refresh_x0_and_curImg(int width, int niter)
		{
			for(int i = 0; i < niter; i++)
			{
				if(!refresh_x0_and_curImg(width))
					return false;
			}
			return true;
		}
		bool refresh_x0_and_curImg(int width)
		{
			assert(width > 0);
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
				else if(!isfinished)
				{
					isfinished = true;
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
				else if(!isfinished)
				{
					isfinished = true;
					return true;
				}
				else return false;
			}
			return false;
		}
		int recog_green_num()
		{
			int y0 = 0;
			int width = NUM_WID;
			int height = imgHei;
			string filenames[] = {"sgs_digit_green0.png", "sgs_digit_green1.png", "sgs_digit_green2.png", "sgs_digit_green3.png", "sgs_digit_green4.png",
			"sgs_digit_green5.png", "sgs_digit_green6.png", "sgs_digit_green7.png", "sgs_digit_green8.png", "sgs_digit_green9.png"};

			int num = -1;
			for(int i = 0; i < 10; i++)
			{
				IplImage * tmpImg = cvLoadImage(string(SGSTMPLPATH + filenames[i]).c_str(), 1);
				bool result = isImageSame(curImg, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				if(result)
				{
					num = i;
					break;
				}
			}
			if(num == -1)
			{
				cerr<<"unknown number"<<endl;
				saveImage(NUM_WID, infile+".unknown_number.png");
			}
			assert(refresh_x0_and_curImg(NUM_WID));
			return num;

		}
		int recog_red_num()
		{
			int y0 = 0;
			int width = NUM_WID;
			int height = imgHei;
			string filenames[] = {"sgs_digit_red0.png", "sgs_digit_red1.png", "sgs_digit_red2.png", "sgs_digit_red3.png", "sgs_digit_red4.png",
			"sgs_digit_red5.png", "sgs_digit_red6.png", "sgs_digit_red7.png", "sgs_digit_red8.png", "sgs_digit_red9.png"};

			int num = -1;
			for(int i = 0; i < 10; i++)
			{
				IplImage * tmpImg = cvLoadImage(string(SGSTMPLPATH + filenames[i]).c_str(), 1);
				bool result = isImageSame(curImg, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				if(result)
				{
					num = i;
					break;
				}
			}
			if(num == -1)
			{
				cerr<<"unknown number"<<endl;
				saveImage(NUM_WID, infile+".unknown_number.png");
			}
			assert(refresh_x0_and_curImg(NUM_WID));
			return num;

		}
		int recog_num()
		{
			int y0 = 0;
			int width = NUM_WID;
			int height = imgHei;
			string filenames[] = {"sgs_digit_0.png", "sgs_digit_1.png", "sgs_digit_2.png", "sgs_digit_3.png", "sgs_digit_4.png",
			"sgs_digit_5.png", "sgs_digit_6.png", "sgs_digit_7.png", "sgs_digit_8.png", "sgs_digit_9.png"};

			int num = -1;
			for(int i = 0; i < 10; i++)
			{
				IplImage * tmpImg = cvLoadImage(string(SGSTMPLPATH + filenames[i]).c_str(), 1);
				bool result = isImageSame(curImg, x0, y0, width, height, tmpImg);
				cvReleaseImage(&tmpImg);
				if(result)
				{
					num = i;
					break;
				}
			}
			if(num == -1)
			{
				cerr<<"unknown number"<<endl;
				saveImage(NUM_WID, infile+".unknown_number.png");
			}
			assert(refresh_x0_and_curImg(NUM_WID));
			return num;
		}
		string recog_who()
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
				bool result = isImageSame(curImg, x0, y0, width, height, tmpImg);
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
				
			return out;
		}
		void saveImage(int width, string filename)
		{
			if(x0 + width > curImg->width) width = curImg->width - x0;
			int nchannels = curImg->nChannels;
			IplImage * tmpImg = cvCreateImage(cvSize(width, imgHei), IPL_DEPTH_8U, nchannels);
			for(int j = 0; j < imgHei; j++)
			{
				for(int i = 0; i < width; i++)
				{
					for(int c = 0; c < nchannels; c++)
					{
						CV_IMAGE_ELEM(tmpImg, unsigned char, j, i*nchannels+c) = CV_IMAGE_ELEM(curImg, unsigned char, j, (i+x0)*nchannels + c);
					}
				}
			}
			cvSaveImage(filename.c_str(), tmpImg);
			cvReleaseImage(&tmpImg);
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
			else if(type == "SGS_CHN_SHOU_RED")
			{
				int width = CHN_WID;
				IplImage * tmpImg = cvLoadImage(SGSTMPLPATH"sgs_chinese_shou_red.png", 1);
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
	
		string recog_sill(string filename)
		{
			string skillname;
			assert(refresh_x0_and_curImg(QUOTE_WID));
		}
		string recog_card(string filename)
		{
			string cardname;
			assert(refresh_x0_and_curImg(CHN_WID));
			// one word
			if(isImageSame(curImg, x0, "sgs_card_name_sha.png"))
			{
				cardname = "杀";
				assert(refresh_x0_and_curImg(CHN_WID));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_shan.png"))
			{
				cardname = "闪";
				assert(refresh_x0_and_curImg(CHN_WID));
				if(isImageSame(curImg, x0, "sgs_card_name_dian.png"))
				{
					cardname = "闪电";
					assert(refresh_x0_and_curImg(CHN_WID));
				}
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_tao.png"))
			{
				cardname = "桃";
				assert(refresh_x0_and_curImg(CHN_WID));
				if(isImageSame(curImg, x0, "sgs_card_name_tyuan.png"))
				{
					cardname = "桃园结义";
					assert(refresh_x0_and_curImg(CHN_WID, 3));
				}
			}
			/*else if(isImageSame(curImg, x0, "sgs_card_name_jiu.png"))
			{
				cardname = "酒";
				assert(refresh_x0_and_curImg(CHN_WID));
				if(isImageSame(curImg, x0, "sgs_card_name_sha.png"))
				{
					cardname = "酒杀";
					assert(refresh_x0_and_curImg(CHN_WID));
				}
			}*/
			// two word
			else if(isImageSame(curImg, x0, "sgs_card_name_lei.png"))
			{
				cardname = "雷杀";
				assert(refresh_x0_and_curImg(CHN_WID,2));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_di.png"))
			{
				cardname = "的卢";
				assert(refresh_x0_and_curImg(CHN_WID,2));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_huo.png"))
			{
				assert(refresh_x0_and_curImg(CHN_WID));
				if(isImageSame(curImg, x0, "sgs_card_name_sha.png"))
				{
					cardname = "火杀";
					assert(refresh_x0_and_curImg(CHN_WID));
				}
				else if(isImageSame(curImg, x0, "sgs_card_name_gong.png"))
				{
					cardname = "火攻";
					assert(refresh_x0_and_curImg(CHN_WID));
				}
				else
				{
					cerr<<"Invalid card after huo"<<endl;
				}
			}
			/*else if(isImageSame(curImg, x0, "sgs_card_name_jue_dou.png"))
			{
				cardname = "决斗";
				assert(refresh_x0_and_curImg(CHN_WID,2));
			}*/
			else if(isImageSame(curImg, x0, "sgs_card_name_jue_ying.png"))
			{
				cardname = "绝影";
				assert(refresh_x0_and_curImg(CHN_WID,2));
			}
			// Three words
			else if(isImageSame(curImg, x0, "sgs_card_name_wu_liu.png"))
			{
				cardname = "吴六剑";
				assert(refresh_x0_and_curImg(CHN_WID, 3));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_qing.png"))
			{
				cardname = "青虹剑";
				assert(refresh_x0_and_curImg(CHN_WID, 3));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_guan.png"))
			{
				cardname = "贯石斧";
				assert(refresh_x0_and_curImg(CHN_WID, 3));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_qi.png"))
			{
				cardname = "麒麟弓";
				assert(refresh_x0_and_curImg(CHN_WID, 3));
			}
			// Four words
			else if(isImageSame(curImg, x0, "sgs_card_name_wan.png"))
			{
				cardname = "万箭齐发";
				assert(refresh_x0_and_curImg(CHN_WID, 4));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_yi.png"))
			{
				cardname = "以逸待劳";
				assert(refresh_x0_and_curImg(CHN_WID, 4));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_yuan.png"))
			{
				cardname = "远交近攻";
				assert(refresh_x0_and_curImg(CHN_WID, 4));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_zhi.png"))
			{
				cardname = "知己知彼";
				assert(refresh_x0_and_curImg(CHN_WID, 4));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_zhu_ge.png"))
			{
				cardname = "诸葛连弩";
				assert(refresh_x0_and_curImg(CHN_WID, 4));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_zhu_que.png"))
			{
				cardname = "朱雀羽扇";
				assert(refresh_x0_and_curImg(CHN_WID, 4));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_zhua.png"))
			{
				cardname = "爪黄飞电";
				assert(refresh_x0_and_curImg(CHN_WID, 4));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_guo.png"))
			{
				cardname = "过河拆桥";
				assert(refresh_x0_and_curImg(CHN_WID, 4));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_jie.png"))
			{
				cardname = "借刀杀人";
				assert(refresh_x0_and_curImg(CHN_WID, 4));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_wu_gu.png"))
			{
				cardname = "五谷丰登";
				assert(refresh_x0_and_curImg(CHN_WID, 4));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_shun.png"))
			{
				cardname = "顺手牵羊";
				assert(refresh_x0_and_curImg(CHN_WID, 4));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_bing.png"))
			{
				cardname = "兵粮寸断";
				assert(refresh_x0_and_curImg(CHN_WID, 4));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_nan.png"))
			{
				cardname = "南蛮入侵";
				assert(refresh_x0_and_curImg(CHN_WID, 4));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_le.png"))
			{
				cardname = "乐不思蜀";
				assert(refresh_x0_and_curImg(CHN_WID, 4));
			}
			else if(isImageSame(curImg, x0, "sgs_card_name_wu.png"))
			{
				assert(refresh_x0_and_curImg(CHN_WID));
				if(isImageSame(curImg, x0, "sgs_card_name_zhong.png"))
				{
					cardname = "无中生有";
					assert(refresh_x0_and_curImg(CHN_WID, 3));
				}
				else if(isImageSame(curImg, x0, "sgs_card_name_xie.png"))
				{
					cardname = "无懈可击";
					assert(refresh_x0_and_curImg(CHN_WID, 3));
					if(isImageSame(curImg, x0, "sgs_special_dot.png"))
					{
						cardname = "无懈可击.国";
						assert(refresh_x0_and_curImg(3+12));
					}
				}

			}
			// Five words
			else if(isImageSame(curImg, x0, "sgs_card_name_san.png"))
			{
				cardname = "三尖两刃刀";
				assert(refresh_x0_and_curImg(CHN_WID, 5));
			}
			else 
			{
				saveImage(CHN_WID, filename);
				return cardname;
				cerr<<"Invalid card name"<<endl;
			}

			string cardtype = "";
			if(isImageSame(curImg, x0, "sgs_card_type_spade.png"))
			{
				cardtype = "黑桃";
				assert(refresh_x0_and_curImg(CARD_TYPE_WID));
			}
			else if(isImageSame(curImg, x0, "sgs_card_type_heart.png"))
			{
				cardtype = "红桃";
				assert(refresh_x0_and_curImg(CARD_TYPE_WID));
			}
			else if(isImageSame(curImg, x0, "sgs_card_type_club.png"))
			{
				cardtype = "梅花";
				assert(refresh_x0_and_curImg(CARD_TYPE_WID));
			}
			else if(isImageSame(curImg, x0, "sgs_card_type_diamond.png"))
			{
				cardtype = "方片";
				assert(refresh_x0_and_curImg(CARD_TYPE_WID));
			}
			else
			{
				cerr<<"Invalid card type"<<endl;
				saveImage(CARD_TYPE_WID, infile+".invalid_card_type.png");
				assert(refresh_x0_and_curImg(CARD_TYPE_WID));
			}

			ostringstream oss;
			oss<<cardname<<"("<<cardtype;

			int card_num_steps[] = {6, 8, 4, 12};
			for(int i = 0; i < 4; i++)
			{
				int x1 = x0 + card_num_steps[i];
				IplImage * curImg1 = curImg;
				if(x1 >= rightside1)
				{
					x1 = leftside;
					curImg1 = img2;
				}
				if(!curImg1) continue;
				if(isImageSame(curImg1, x1, "sgs_chinese_right_bracket.png"))
				{
					string color = "";
					if(cardtype == "黑桃" || cardtype == "梅花") color = "black";
					else color = "red";

					if(card_num_steps[i] == 6)
					{
						if(isImageSame(curImg, x0, "sgs_card_num_" + color + "_2.png"))
							oss<<"2)";
						else if(isImageSame(curImg, x0, "sgs_card_num_" + color + "_3.png"))
							oss<<"3)";
						else if(isImageSame(curImg, x0, "sgs_card_num_" + color + "_4.png"))
							oss<<"4)";
						else if(isImageSame(curImg, x0, "sgs_card_num_" + color + "_5.png"))
							oss<<"5)";
						else if(isImageSame(curImg, x0, "sgs_card_num_" + color + "_6.png"))
							oss<<"6)";
						else if(isImageSame(curImg, x0, "sgs_card_num_" + color + "_7.png"))
							oss<<"7)";
						else if(isImageSame(curImg, x0, "sgs_card_num_" + color + "_8.png"))
							oss<<"8)";
						else if(isImageSame(curImg, x0, "sgs_card_num_" + color + "_9.png"))
							oss<<"9)";
						else
						{
							cerr<<"unknown card number"<<endl;
							saveImage(6, filename + ".num.png");
							oss<<")";
						}
						assert(refresh_x0_and_curImg(6));
					}
					else if(card_num_steps[i] == 8)
					{
						if(isImageSame(curImg, x0, "sgs_card_num_" + color + "_A.png"))
							oss<<"A)";
						else if(isImageSame(curImg, x0, "sgs_card_num_" + color + "_Q.png"))
							oss<<"Q)";
						else if(isImageSame(curImg, x0, "sgs_card_num_" + color + "_K.png"))
							oss<<"K)";
						else
						{
							oss<<")";
							saveImage(8, filename + ".QKA.png");
						}
						assert(refresh_x0_and_curImg(8));
					}
					else if(card_num_steps[i] == 4)
					{
						oss<<"J)";
						assert(refresh_x0_and_curImg(4));
					}
					else if(card_num_steps[i] == 12)
					{
						oss<<"10)";
						assert(refresh_x0_and_curImg(12));
					}
					assert(refresh_x0_and_curImg(CHN_WID));
					break;
				}
			}
			return "[" + oss.str() +"]";
		}

		string recog()
		{
			if(isFirstShow()) return FirstShow_recog();
			x0 = leftside;
			curImg = img1;

			string who = recog_who(); // x0 and curImg is set in this function
			string next = "";
			if(who == "??号位") next = "";
			else next = " what";
			if(isWord(curImg, x0, "SGS_CHN_CONG"))
			{
				next = "从牌堆里摸了";
				assert(refresh_x0_and_curImg(CHN_WID, 6));
				int ncards = recog_num();
				ostringstream oss;
				oss<<next<<ncards<<"张卡牌";
				next = oss.str();
				assert(refresh_x0_and_curImg(CHN_WID, 2));
				assert(refresh_x0_and_curImg(CHN_WID, 1));
				if(!isfinished)
				{
					next += recog_card(infile+".card1.png");
					if(ncards>= 2) next += recog_card(infile+".card2.png");
					if(ncards>= 3) next += recog_card(infile+".card3.png");
				}
			}
			else if(isWord(curImg, x0, "SGS_CHN_ZHUANG"))
			{
				next = "装备了";
				assert(refresh_x0_and_curImg(CHN_WID, 3));
				next += recog_card(infile+".card.png");
			}
			else if(isWord(curImg, x0, "SGS_CHN_SHI"))
			{
				next = "使用了卡牌";
				assert(refresh_x0_and_curImg(CHN_WID, 5));
				next += recog_card(infile+".card.png");
			}
			else if(isWord(curImg, x0, "SGS_CHN_SHOU"))
			{
				next = "受到火攻, 展示了一张卡牌";
				assert(refresh_x0_and_curImg(CHN_WID, 12));
				next += recog_card(infile+".card.png");
			}
			else if(isWord(curImg, x0, "SGS_CHN_SHOU_RED"))
			{
				ostringstream oss;
				oss << "受到";
				assert(refresh_x0_and_curImg(CHN_WID, 2));
				int num1 = recog_red_num();
				oss << num1 << "点";//"伤害, 体力值为";
				assert(refresh_x0_and_curImg(CHN_WID));
				if(isImageSame(curImg, x0, "sgs_chinese_red_lei.png"))
				{
					oss<<"雷属性伤害, 体力值为";
					assert(refresh_x0_and_curImg(CHN_WID, 10));
				}
				else if(isImageSame(curImg, x0, "sgs_chinese_red_shang.png"))
				{
					oss<<"伤害, 体力值为";
					assert(refresh_x0_and_curImg(CHN_WID, 7));
				}
				else
				{
					cerr<<"unknow damage type"<<endl;
					saveImage(CHN_WID, infile + ".unknown_damage_type.png");
					oss<<"??属性伤害, 体力值为";
					assert(refresh_x0_and_curImg(CHN_WID, 10));
				}
				int num2 = recog_red_num();
				oss<<num2;
				next = oss.str();
			}
			else if(isWord(curImg, x0, "SGS_CHN_DA"))
			{
				next = "打出卡牌";
				assert(refresh_x0_and_curImg(CHN_WID, 4));
				next += recog_card(infile+".card.png");
			}
			else if(isWord(curImg, x0, "SGS_CHN_DE"))
			{
				next = "的";
			}
			else if(isWord(curImg, x0, "SGS_CHN_QI"))
			{
				ostringstream oss;
				oss<<"弃掉";
				assert(refresh_x0_and_curImg(CHN_WID, 2));
				int ncards = recog_num();
				oss<<ncards<<"张手牌";
				assert(refresh_x0_and_curImg(CHN_WID, 3)); // 张手牌
				for(int i = 0; i < ncards; i++)
				{
					ostringstream oss2;
					oss2<<infile<<".card"<<i+1<<".png";
					oss<<recog_card(oss2.str());
				}
				next = oss.str();
			}
			else if(isWord(curImg, x0, "SGS_CHN_FA"))
			{
				next = "发动了武将技能";
				assert(refresh_x0_and_curImg(CHN_WID, 7));
			}
			else if(isWord(curImg, x0, "SGS_CHN_HUO"))
			{
				next = "获得";
				assert(refresh_x0_and_curImg(CHN_WID, 2));
				string whom = recog_who();
				next += whom;
				next += "的";
				assert(refresh_x0_and_curImg(CHN_WID));
				if(isImageSame(curImg, x0, "sgs_chinese_zhuang.png"))
				{
					next += "装备";
					assert(refresh_x0_and_curImg(CHN_WID, 2));
					next += recog_card(infile + ".card.png");
				}
				else if(isImageSame(curImg, x0, "sgs_chinese_shou_pai.png"))
				{
					next += "手牌";
					assert(refresh_x0_and_curImg(CHN_WID, 2));
					next += recog_card(infile + ".card.png");
				}
				else
				{
					int ncards = recog_num();
					ostringstream oss;
					oss<<next<<ncards<<"张卡牌";
					assert(refresh_x0_and_curImg(CHN_WID, 3));
					next = oss.str();
				}
			}
			else if(isWord(curImg, x0, "SGS_CHN_DUI"))
			{
				next = "对";
				assert(refresh_x0_and_curImg(CHN_WID, 1));
				next += recog_who();
				if(isImageSame(curImg, x0, "sgs_chinese_shi.png"))
				{
					next += "使用了卡牌";
					assert(refresh_x0_and_curImg(CHN_WID, 5));
					next += recog_card(infile + ".card.png");
				}
				else if(isImageSame(curImg, x0, "sgs_chinese_fa.png"))
				{
					next += "发动了武将技能";
					assert(refresh_x0_and_curImg(CHN_WID, 7));
					next += recog_skill(infile + ".skill.png");
				}
				else
				{
					saveImage(CHN_WID, infile + ".unknow_dui.png");
				}
			}
			else if(isWord(curImg, x0, "SGS_CHN_HUI"))
			{
				ostringstream oss;
				oss << "恢复了";
				assert(refresh_x0_and_curImg(CHN_WID, 3));
				int num1 = recog_green_num();
				oss << num1 << "点体力, 体力值为";
				assert(refresh_x0_and_curImg(CHN_WID, 8));
				int num2 = recog_green_num();
				oss << num2;
				next = oss.str();
			}
			else if(isWord(curImg, x0, "SGS_CHN_YU"))
			{
				next = "与";
				assert(refresh_x0_and_curImg(CHN_WID, 1));
				string whom = recog_who();
				next += whom;
			}
			else if(isWord(curImg, x0, "SGS_CHN_XIANG"))
			{
				next = "向";
				assert(refresh_x0_and_curImg(CHN_WID, 1));
				string whom = recog_who();
				next += whom;
			}
			else if(isWord(curImg, x0, "SGS_CHN_GUAN"))
			{
				next = "观看了";
				assert(refresh_x0_and_curImg(CHN_WID, 3));
				string whom = recog_who();
				next += whom;
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
