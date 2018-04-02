#include "stdafx.h"
#include "../include/chars_segment.h"
#include "../include/chars_identify.h"

//该类把SVM检测的车牌分割成一个个字符块

namespace lpr{

const float DEFAULT_BLUEPERCEMT = 0.3f; 
const float	DEFAULT_WHITEPERCEMT = 0.1f;

CCharsSegment::CCharsSegment()   //构造函数
{
	
	m_MaoDingSize = DEFAULT_MaoDing_SIZE;
	m_theMatWidth = DEFAULT_MAT_WIDTH; 

	//！车牌颜色判断参数
	m_ColorThreshold = DEFAULT_COLORTHRESHOLD;
	m_BluePercent = DEFAULT_BLUEPERCEMT;
	m_WhitePercent = DEFAULT_WHITEPERCEMT;
}



//! 字符尺寸验证
bool CCharsSegment::verifySizes(Mat r)
{
	//Char sizes 45x90
	float aspect=45.0f/90.0f;
	float charAspect= (float)r.cols/(float)r.rows;  //列数与行数的比值
	float error=0.7f;
	float minHeight=10.f;
	float maxHeight=35.f;
	//We have a different aspect ratio for number 1, and it can be ~0.2
	float minAspect=0.05f;
	float maxAspect=aspect+aspect*error;
	//area of pixels


	int area = countNonZero(r);   //得到二值化图像r非零像素点个数
	

	//bb area

	int bbArea = r.cols*r.rows;

	//% of pixel in area

	int percPixels = area / bbArea;   //非零像素点个数占总的二值化图像r像素的比例
	//float percPixels=area/bbArea;   //非零像素点个数占总的二值化图像r像素的比例

	if(percPixels <= 1 && charAspect > minAspect && charAspect < maxAspect && r.rows >= minHeight && r.rows < maxHeight)
		return true;
	else
		return false;
}


//! 字符预处理
Mat CCharsSegment::preprocessChar(Mat in)
{
	//Remap image
	int h=in.rows;
	int w=in.cols;
	int charSize=CHAR_SIZE;	//统一每个字符的大小20
	Mat transformMat=Mat::eye(2,3,CV_32F);  //eye创建2行3列矩阵，前2行前2列是对角阵，剩下的全为0，仿射变换时输入图像坐标转化为齐次形式
	int m=max(w,h);


	transformMat.at<float>(0, 2) =float(m / 2 - w / 2);
	transformMat.at<float>(1, 2) = float(m / 2 - h / 2);


	Mat warpImage(m,m, in.type());
	warpAffine(in, warpImage, transformMat, warpImage.size(), INTER_LINEAR, BORDER_CONSTANT, Scalar(0) );
	//仿射变换，使输入字符块变成m×m大小，其中字符处于wrapImage的中间位置（仿射变换矩阵指定如此，transformMat）

	Mat out;
	resize(warpImage, out, Size(charSize, charSize) ); 
	//把放射变换后的图像尺寸扩大为20×20，作为ANN的标准输入

	return out;
}


//! choose the bese threshold method for chinese
void CCharsSegment::judgeChinese(Mat in, Mat& out, Color plateType) 
{

	Mat auxRoi = in;
	float valOstu = -1.f, valAdap = -1.f;
	Mat roiOstu, roiAdap;
	bool isChinese = true;
	if (1) 
	{
		if (BLUE == plateType) 
		{
			threshold(auxRoi, roiOstu, 0, 255, CV_THRESH_BINARY + CV_THRESH_OTSU);
		}
		else if (YELLOW == plateType) 
		{
			threshold(auxRoi, roiOstu, 0, 255, CV_THRESH_BINARY_INV + CV_THRESH_OTSU);
		}
		else if (WHITE == plateType) 
		{
			threshold(auxRoi, roiOstu, 0, 255, CV_THRESH_BINARY_INV + CV_THRESH_OTSU);
		}
		else {
			threshold(auxRoi, roiOstu, 0, 255, CV_THRESH_OTSU + CV_THRESH_BINARY);
		}
		roiOstu = preprocessChar(roiOstu);
		if (0) 
		{
			imshow("roiOstu", roiOstu);
			waitKey(0);
			destroyWindow("roiOstu");
		}
		auto character = CharsIdentify::instance()->identifyChinese(roiOstu, valOstu, isChinese);
	}
	if (1) 
	{
		if (BLUE == plateType)
		{
			adaptiveThreshold(auxRoi, roiAdap, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 3, 0);
		}
		else if (YELLOW == plateType) 
		{
			adaptiveThreshold(auxRoi, roiAdap, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY_INV, 3, 0);
		}
		else if (WHITE == plateType)
		{
			adaptiveThreshold(auxRoi, roiAdap, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY_INV, 3, 0);
		}
		else 
		{
			adaptiveThreshold(auxRoi, roiAdap, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 3, 0);
		}
		roiAdap = preprocessChar(roiAdap);
		auto character = CharsIdentify::instance()->identifyChinese(roiAdap, valAdap, isChinese);
	}



	if (valOstu >= valAdap) 
	{
		out = roiOstu;
	}
	else 
	{
		out = roiAdap;
	}

}



Mat CCharsSegment:: preprocessChar(Mat in, int char_size) 
{
	// Remap image
	int h = in.rows;
	int w = in.cols;

	int charSize = char_size;

	Mat transformMat = Mat::eye(2, 3, CV_32F);
	int m = max(w, h);
	transformMat.at<float>(0, 2) = float(m / 2 - w / 2);
	transformMat.at<float>(1, 2) = float(m / 2 - h / 2);

	Mat warpImage(m, m, in.type());
	warpAffine(in, warpImage, transformMat, warpImage.size(), INTER_LINEAR,
		BORDER_CONSTANT, Scalar(0));

	Mat out;
	cv::resize(warpImage, out, Size(charSize, charSize));

	return out;
}

Rect CCharsSegment::interRect(const Rect& a, const Rect& b) 
{
	Rect c;
	int x1 = a.x > b.x ? a.x : b.x;
	int y1 = a.y > b.y ? a.y : b.y;
	c.width = (a.x + a.width < b.x + b.width ? a.x + a.width : b.x + b.width) - x1;
	c.height = (a.y + a.height < b.y + b.height ? a.y + a.height : b.y + b.height) - y1;
	c.x = x1;
	c.y = y1;
	if (c.width <= 0 || c.height <= 0)
		c = Rect();
	return c;
}

Rect CCharsSegment::mergeRect(const Rect& a, const Rect& b) 
{
	Rect c;
	int x1 = a.x < b.x ? a.x : b.x;
	int y1 = a.y < b.y ? a.y : b.y;
	c.width = (a.x + a.width > b.x + b.width ? a.x + a.width : b.x + b.width) - x1;
	c.height = (a.y + a.height > b.y + b.height ? a.y + a.height : b.y + b.height) - y1;
	c.x = x1;
	c.y = y1;
	return c;
}


float CCharsSegment::computeIOU(const Rect& rect1, const Rect& rect2)
{

	Rect inter = interRect(rect1, rect2);
	Rect urect = mergeRect(rect1, rect2);

	float iou = (float)inter.area() / (float)urect.area();

	return iou;
}




//! non-maximum suppression
void CCharsSegment::NMStoCharacter(std::vector<CCharacter> &inVec, double overlap) 
{

	std::sort(inVec.begin(), inVec.end());

	std::vector<CCharacter>::iterator it = inVec.begin();
	for (; it != inVec.end(); ++it) 
	{
		CCharacter charSrc = *it;
		//std::cout << "plateScore:" << plateSrc.getPlateScore() << std::endl;
		Rect rectSrc = charSrc.getCharacterPos();

		std::vector<CCharacter>::iterator itc = it + 1;

		for (; itc != inVec.end();) 
		{
			CCharacter charComp = *itc;
			Rect rectComp = charComp.getCharacterPos();
			//Rect rectInter = rectSrc & rectComp;
			//Rect rectUnion = rectSrc | rectComp;
			//double r = double(rectInter.area()) / double(rectUnion.area());

			float iou = computeIOU(rectSrc, rectComp);

			if (iou > overlap) 
			{
				itc = inVec.erase(itc);
			}
			else 
			{
				++itc;
			}
		}
	}
}



bool CCharsSegment::slideChineseWindow(Mat& image, Rect mr, Mat& newRoi, Color plateType, float slideLengthRatio, bool useAdapThreshold) 
{
	std::vector<CCharacter> charCandidateVec;

	Rect maxrect = mr;
	Point tlPoint = mr.tl();

	bool isChinese = true;
	int slideLength = int(slideLengthRatio * maxrect.width);
	int slideStep = 1;
	int fromX = 0;
	fromX = tlPoint.x;

	for (int slideX = -slideLength; slideX < slideLength; slideX += slideStep) 
	{
		float x_slide = 0;

		x_slide = float(fromX + slideX);

		float y_slide = (float)tlPoint.y;
		Point2f p_slide(x_slide, y_slide);

		//cv::circle(image, p_slide, 2, Scalar(255), 1);

		int chineseWidth = int(maxrect.width);
		int chineseHeight = int(maxrect.height);

		Rect rect(Point2f(x_slide, y_slide), Size(chineseWidth, chineseHeight));

		if (rect.tl().x < 0 || rect.tl().y < 0 || rect.br().x >= image.cols || rect.br().y >= image.rows)
			continue;

		Mat auxRoi = image(rect);

		Mat roiOstu, roiAdap;
		if (1) 
		{
			if (BLUE == plateType) 
			{
				threshold(auxRoi, roiOstu, 0, 255, CV_THRESH_BINARY + CV_THRESH_OTSU);
			}
			else if (YELLOW == plateType) 
			{
				threshold(auxRoi, roiOstu, 0, 255, CV_THRESH_BINARY_INV + CV_THRESH_OTSU);
			}
			else if (WHITE == plateType) 
			{
				threshold(auxRoi, roiOstu, 0, 255, CV_THRESH_BINARY_INV + CV_THRESH_OTSU);
			}
			else 
			{
				threshold(auxRoi, roiOstu, 0, 255, CV_THRESH_OTSU + CV_THRESH_BINARY);
			}
			roiOstu = preprocessChar(roiOstu, kChineseSize);

			CCharacter charCandidateOstu;
			charCandidateOstu.setCharacterPos(rect);
			charCandidateOstu.setCharacterMat(roiOstu);
			charCandidateOstu.setIsChinese(isChinese);
			charCandidateVec.push_back(charCandidateOstu);
		}
		if (useAdapThreshold) 
		{
			if (BLUE == plateType)
			{
				adaptiveThreshold(auxRoi, roiAdap, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 3, 0);
			}
			else if (YELLOW == plateType) 
			{
				adaptiveThreshold(auxRoi, roiAdap, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY_INV, 3, 0);
			}
			else if (WHITE == plateType) 
			{
				adaptiveThreshold(auxRoi, roiAdap, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY_INV, 3, 0);
			}
			else
			{
				adaptiveThreshold(auxRoi, roiAdap, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 3, 0);
			}
			roiAdap = preprocessChar(roiAdap, kChineseSize);

			CCharacter charCandidateAdap;
			charCandidateAdap.setCharacterPos(rect);
			charCandidateAdap.setCharacterMat(roiAdap);
			charCandidateAdap.setIsChinese(isChinese);
			charCandidateVec.push_back(charCandidateAdap);
		}

	}

	CharsIdentify::instance()->classifyChinese(charCandidateVec);

	double overlapThresh = 0.1;
	NMStoCharacter(charCandidateVec, overlapThresh);

	if (charCandidateVec.size() >= 1) 
	{
		std::sort(charCandidateVec.begin(), charCandidateVec.end(),
			[](const CCharacter& r1, const CCharacter& r2) 
		{
			return r1.getCharacterScore() > r2.getCharacterScore();
		});
		newRoi = charCandidateVec.at(0).getCharacterMat();
		return true;
	}
	return false;
}


void CCharsSegment::spatial_ostu(InputArray _src, int grid_x, int grid_y, Color type) 
{
	Mat src = _src.getMat();

	int width = src.cols / grid_x;
	int height = src.rows / grid_y;

	// iterate through grid
	for (int i = 0; i < grid_y; i++) 
	{
		for (int j = 0; j < grid_x; j++) 
		{
			Mat src_cell = Mat(src, Range(i*height, (i + 1)*height), Range(j*width, (j + 1)*width));
			if (type == BLUE) 
			{
				cv::threshold(src_cell, src_cell, 0, 255, CV_THRESH_OTSU + CV_THRESH_BINARY);
			}
			else if (type == YELLOW)
			{
				cv::threshold(src_cell, src_cell, 0, 255, CV_THRESH_OTSU + CV_THRESH_BINARY_INV);
			}
			else if (type == WHITE) 
			{
				cv::threshold(src_cell, src_cell, 0, 255, CV_THRESH_OTSU + CV_THRESH_BINARY_INV);
			}
			else 
			{
				cv::threshold(src_cell, src_cell, 0, 255, CV_THRESH_OTSU + CV_THRESH_BINARY);
			}
		}
	}
}



//输入定位车牌，输出分割后的字块
int CCharsSegment::charsSegment(Mat input, vector<Mat>& resultVec, Color color)
{
	if (!input.data) return 0x01;

	Color plateType = color;

	Mat input_grey;
	cvtColor(input, input_grey, CV_BGR2GRAY);

	if (0) 
	{
		imshow("plate", input_grey);
		waitKey(0);
		destroyWindow("plate");
	}

	Mat img_threshold;

	img_threshold = input_grey.clone();
	/*namedWindow("原始车牌灰度图像", WINDOW_NORMAL);
	imshow("原始车牌灰度图像", input_grey);*/

	//自适应阈值法
	//Mat img_threshold_ = img_threshold.clone();
	//int blockSize = 9;//领域尺寸奇数
	//int constValue = 0;
	//cv::adaptiveThreshold(img_threshold_, img_threshold_, 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, blockSize, constValue);
	//namedWindow("自适应阈值化", WINDOW_NORMAL);
	//imshow("自适应阈值化", img_threshold_);

	spatial_ostu(img_threshold, 8, 2, plateType);
	/*namedWindow("OTSU大津阈值化", WINDOW_NORMAL);
	imshow("OTSU大津阈值化", img_threshold);*/
	

	if (0) 
	{
		imshow("plate", img_threshold);
		waitKey(0);
		destroyWindow("plate");
	}

	// remove liuding and hor lines
	// also judge weather is plate use jump count

	if (!clearMaoDing(img_threshold)) return 0x02;
	//clearLiuDing(img_threshold);


	Mat img_contours;
	img_threshold.copyTo(img_contours);

	vector<vector<Point> > contours;
	findContours(img_contours,
		contours,               // a vector of contours
		CV_RETR_EXTERNAL,       // retrieve the external contours
		CV_CHAIN_APPROX_NONE);  // all pixels of each contours

	vector<vector<Point> >::iterator itc = contours.begin();
	vector<Rect> vecRect;

	while (itc != contours.end()) 
	{
		Rect mr = boundingRect(Mat(*itc));
		Mat auxRoi(img_threshold, mr);

		if (verifySizes(auxRoi)) vecRect.push_back(mr);
		++itc;
	}


	if (vecRect.size() == 0) return 0x03;

	vector<Rect> sortedRect(vecRect);
	std::sort(sortedRect.begin(), sortedRect.end(),
		[](const Rect& r1, const Rect& r2) { return r1.x < r2.x; });

	size_t specIndex = 0;

	specIndex = GetSpecificRect(sortedRect);

	Rect chineseRect;//存储汉字字块，sortedRect[specIndex]存储汉字往右第一个字块
	if (specIndex < sortedRect.size())
		chineseRect = GetChineseRect(sortedRect[specIndex]);
	else
		return 0x04;

	if (0) 
	{
		rectangle(img_threshold, chineseRect, Scalar(255));
		imshow("plate", img_threshold);
		waitKey(0);
		destroyWindow("plate");
	}

	vector<Rect> newSortedRect;
	newSortedRect.push_back(chineseRect);
	RebuildRect(sortedRect, newSortedRect, specIndex);

	if (newSortedRect.size() == 0) return 0x05;

	bool useSlideWindow = true;
	bool useAdapThreshold = true;
	//bool useAdapThreshold = CParams::instance()->getParam1b();

	for (size_t i = 0; i < newSortedRect.size(); i++) 
	{
		Rect mr = newSortedRect[i];

		// Mat auxRoi(img_threshold, mr);
		Mat auxRoi(input_grey, mr);
		Mat newRoi;

		if (i == 0) 
		{
			if (useSlideWindow) 
			{
				float slideLengthRatio = 0.1f;
				//float slideLengthRatio = CParams::instance()->getParam1f();
				if (!slideChineseWindow(input_grey, mr, newRoi, plateType, slideLengthRatio, useAdapThreshold))
					judgeChinese(auxRoi, newRoi, plateType);
			}
			else
				judgeChinese(auxRoi, newRoi, plateType);
		}
		else 
		{
			if (BLUE == plateType) 
			{
				threshold(auxRoi, newRoi, 0, 255, CV_THRESH_BINARY + CV_THRESH_OTSU);
			}
			else if (YELLOW == plateType) 
			{
				threshold(auxRoi, newRoi, 0, 255, CV_THRESH_BINARY_INV + CV_THRESH_OTSU);
			}
			else if (WHITE == plateType) 
			{
				threshold(auxRoi, newRoi, 0, 255, CV_THRESH_OTSU + CV_THRESH_BINARY_INV);
			}
			else 
			{
				threshold(auxRoi, newRoi, 0, 255, CV_THRESH_OTSU + CV_THRESH_BINARY);
			}

			newRoi = preprocessChar(newRoi);
		}

		if (0) 
		{
			if (i == 0) 
			{
				imshow("input_grey", input_grey);
				waitKey(0);
				destroyWindow("input_grey");
			}
			if (i == 0)
			{
				imshow("newRoi", newRoi);
				waitKey(0);
				destroyWindow("newRoi");
			}
		}

		resultVec.push_back(newRoi);
	}
	//cout << "分割字块个数为： " << resultVec.size() << endl;
	
	return 0;
}



//! 根据一幅图像与颜色模板获取对应的二值图
//! 输入RGB图像, 颜色模板（蓝色、黄色）
//! 输出灰度图（只有0和255两个值，255代表匹配，0代表不匹配）
Mat CCharsSegment::colorMatch(const Mat& src, Mat& match, const Color r, const bool adaptive_minsv)
{
	// S和V的最小值由adaptive_minsv这个bool值判断
	// 如果为true，则最小值取决于H值，按比例衰减
	// 如果为false，则不再自适应，使用固定的最小值minabs_sv
	// 默认为false
	const float max_sv = 255;
	const float minref_sv = 64;

	const float minabs_sv = 95;

	// blue的H范围
	const int min_blue = 100;  // 100
	const int max_blue = 140;  // 140

	// yellow的H范围
	const int min_yellow = 15;  // 15
	const int max_yellow = 40;  // 40

	// white的H范围
	const int min_white = 0;   // 15
	const int max_white = 30;  // 40

	Mat src_hsv;
	// 转到HSV空间进行处理，颜色搜索主要使用的是H分量进行蓝色与黄色的匹配工作
	cvtColor(src, src_hsv, CV_BGR2HSV);

	vector<Mat> hsvSplit;
	split(src_hsv, hsvSplit);
	equalizeHist(hsvSplit[2], hsvSplit[2]);
	merge(hsvSplit, src_hsv);

	//匹配模板基色,切换以查找想要的基色
	int min_h = 0;
	int max_h = 0;
	switch (r) {
	case BLUE:
		min_h = min_blue;
		max_h = max_blue;
		break;
	case YELLOW:
		min_h = min_yellow;
		max_h = max_yellow;
		break;
	case WHITE:
		min_h = min_white;
		max_h = max_white;
		break;
	default:
		// Color::UNKNOWN
		break;
	}

	float diff_h = float((max_h - min_h) / 2);
	float avg_h = min_h + diff_h;

	int channels = src_hsv.channels();
	int nRows = src_hsv.rows;
	//图像数据列需要考虑通道数的影响；
	int nCols = src_hsv.cols * channels;

	if (src_hsv.isContinuous())  //连续存储的数据，按一行处理
	{
		nCols *= nRows;
		nRows = 1;
	}

	int i, j;
	uchar* p;
	float s_all = 0;
	float v_all = 0;
	float count = 0;
	for (i = 0; i < nRows; ++i) {
		p = src_hsv.ptr<uchar>(i);
		for (j = 0; j < nCols; j += 3) {
			int H = int(p[j]);      // 0-180
			int S = int(p[j + 1]);  // 0-255
			int V = int(p[j + 2]);  // 0-255

			s_all += S;
			v_all += V;
			count++;

			bool colorMatched = false;

			if (H > min_h && H < max_h) {
				float Hdiff = 0;
				if (H > avg_h)
					Hdiff = H - avg_h;
				else
					Hdiff = avg_h - H;

				float Hdiff_p = float(Hdiff) / diff_h;

				// S和V的最小值由adaptive_minsv这个bool值判断
				// 如果为true，则最小值取决于H值，按比例衰减
				// 如果为false，则不再自适应，使用固定的最小值minabs_sv
				float min_sv = 0;
				if (true == adaptive_minsv)
					min_sv =
					minref_sv -
					minref_sv / 2 *
					(1 - Hdiff_p);  // inref_sv - minref_sv / 2 * (1 - Hdiff_p)
				else
					min_sv = minabs_sv;  // add

				if ((S > min_sv && S < max_sv) && (V > min_sv && V < max_sv))
					colorMatched = true;
			}

			if (colorMatched == true) {
				p[j] = 0;
				p[j + 1] = 0;
				p[j + 2] = 255;
			}
			else {
				p[j] = 0;
				p[j + 1] = 0;
				p[j + 2] = 0;
			}
		}
	}
	// cout << "avg_s:" << s_all / count << endl;
	// cout << "avg_v:" << v_all / count << endl;

	// 获取颜色匹配后的二值灰度图
	Mat src_grey;
	vector<Mat> hsvSplit_done;
	split(src_hsv, hsvSplit_done);
	src_grey = hsvSplit_done[2];

	match = src_grey;

	return src_grey;
}

//! 判断一个车牌的颜色
//! 输入车牌mat与颜色模板
//! 返回true或fasle
bool CCharsSegment::plateColorJudge(const Mat& src, const Color r, const bool adaptive_minsv, float& percent)
 {
		// 判断阈值
		const float thresh = 0.45f;

		Mat src_gray;
		colorMatch(src, src_gray, r, adaptive_minsv);

		percent =
			float(countNonZero(src_gray)) / float(src_gray.rows * src_gray.cols);
		// cout << "percent:" << percent << endl;

		if (percent > thresh)
			return true;
		else
			return false;
}


// getPlateType
//判断车牌的类型
lpr::Color CCharsSegment::getPlateType(const Mat& src, const bool adaptive_minsv) 
{
	float max_percent = 0;
	Color max_color = UNKNOWN;

	float blue_percent = 0;
	float yellow_percent = 0;
	float white_percent = 0;

	if (plateColorJudge(src, BLUE, adaptive_minsv, blue_percent) == true) {
		// cout << "BLUE" << endl;
		return BLUE;
	}
	else if (plateColorJudge(src, YELLOW, adaptive_minsv, yellow_percent) ==
		true) {
		// cout << "YELLOW" << endl;
		return YELLOW;
	}
	else if (plateColorJudge(src, WHITE, adaptive_minsv, white_percent) ==
		true) {
		// cout << "WHITE" << endl;
		return WHITE;
	}
	else {
		// cout << "OTHER" << endl;

		// 如果任意一者都不大于阈值，则取值最大者
		max_percent = blue_percent > yellow_percent ? blue_percent : yellow_percent;
		max_color = blue_percent > yellow_percent ? BLUE : YELLOW;

		max_color = max_percent > white_percent ? max_color : WHITE;
		return max_color;
	}
}




int CCharsSegment::ThresholdOtsu(Mat mat) 
{
	int height = mat.rows;
	int width = mat.cols;

	// histogram
	float histogram[256] = { 0 };
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			unsigned char p = (unsigned char)((mat.data[i * mat.step[0] + j]));
			histogram[p]++;
		}
	}
	// normalize histogram
	int size = height * width;
	for (int i = 0; i < 256; i++) {
		histogram[i] = histogram[i] / size;
	}

	// average pixel value
	float avgValue = 0;
	for (int i = 0; i < 256; i++) {
		avgValue += i * histogram[i];
	}

	int thresholdV;
	float maxVariance = 0;
	float w = 0, u = 0;
	for (int i = 0; i < 256; i++) {
		w += histogram[i];
		u += i * histogram[i];

		float t = avgValue * w - u;
		float variance = t * t / (w * (1 - w));
		if (variance > maxVariance) {
			maxVariance = variance;
			thresholdV = i;
		}
	}

	return thresholdV;
}


//去除车牌上方的钮钉
//计算每行元素的阶跃数，如果小于X认为是柳丁，将此行全部填0（涂黑）
// X的推荐值为，可根据实际调整
bool CCharsSegment::clearMaoDing(Mat& img) 
{
	vector<float> fJump;
	int whiteCount = 0;
	const int x = 7;
	Mat jump = Mat::zeros(1, img.rows, CV_32F);
	for (int i = 0; i < img.rows; i++) {
		int jumpCount = 0;

		for (int j = 0; j < img.cols - 1; j++) {
			if (img.at<char>(i, j) != img.at<char>(i, j + 1)) jumpCount++;

			if (img.at<uchar>(i, j) == 255) {
				whiteCount++;
			}
		}

		jump.at<float>(i) = (float)jumpCount;
	}

	int iCount = 0;
	for (int i = 0; i < img.rows; i++) {
		fJump.push_back(jump.at<float>(i));
		if (jump.at<float>(i) >= 16 && jump.at<float>(i) <= 45) {
			//车牌字符满足一定跳变条件
			iCount++;
		}
	}

	////这样的不是车牌
	if (iCount * 1.0 / img.rows <= 0.40) {
		//满足条件的跳变的行数也要在一定的阈值内
		return false;
	}
	//不满足车牌的条件
	if (whiteCount * 1.0 / (img.rows * img.cols) < 0.15 ||
		whiteCount * 1.0 / (img.rows * img.cols) > 0.50) {
		return false;
	}

	for (int i = 0; i < img.rows; i++) {
		if (jump.at<float>(i) <= x) {
			for (int j = 0; j < img.cols; j++) {
				img.at<char>(i, j) = 0;
			}
		}
	}
	return true;
}


//! 根据特殊车牌来构造猜测中文字符的位置和大小
Rect CCharsSegment::GetChineseRect(const Rect rectSpe)
{
	int height = rectSpe.height;
	float newwidth = rectSpe.width * 1.15f;
	int x = rectSpe.x;
	int y = rectSpe.y;

	int newx = x - int (newwidth * 1.15);
	newx = newx > 0 ? newx : 0;

	Rect a(newx, y, int(newwidth), height);

	return a;
}


//! 找出指示城市的字符的Rect，例如苏A7003X，就是"A"的位置
int CCharsSegment::GetSpecificRect(const vector<Rect>& vecRect)
{
	vector<int> xpositions;
	int maxHeight = 0;
	int maxWidth = 0;

	for (size_t i = 0; i < vecRect.size(); i++)
	{
        xpositions.push_back(vecRect[i].x);

		if (vecRect[i].height > maxHeight)
		{
			maxHeight = vecRect[i].height;
		}
		if (vecRect[i].width > maxWidth)
		{
			maxWidth = vecRect[i].width;
		}
	}

	int specIndex = 0;
	for (size_t i = 0; i < vecRect.size(); i++)
	{
		Rect mr = vecRect[i];
		int midx = mr.x + mr.width/2;

		//如果一个字符有一定的大小，并且在整个车牌的1/7到2/7之间，则是我们要找的特殊车牌
		//当前字符和下个字符的距离在一定的范围内
		if ((mr.width > maxWidth * 0.8 || mr.height > maxHeight * 0.8) &&
			(midx < int(m_theMatWidth / 7) * 2 && midx > int(m_theMatWidth / 7) * 1))
		{
			specIndex = i;
		}
	}

	return specIndex;
}


//! 这个函数做两个事情
//  1.把特殊字符Rect左边的全部Rect去掉，后面再重建中文字符的位置。
//  2.从特殊字符Rect开始，依次选择6个Rect，多余的舍去。
//1.3版
int CCharsSegment::RebuildRect(const vector<Rect>& vecRect,vector<Rect>& outRect, int specIndex) 
{
	int count = 6;
	for (size_t i = specIndex; i < vecRect.size() && count; ++i, --count) 
	{
		outRect.push_back(vecRect[i]);
	}

	return 0;
}

}	
