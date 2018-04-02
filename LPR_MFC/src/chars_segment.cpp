#include "stdafx.h"
#include "../include/chars_segment.h"
#include "../include/chars_identify.h"

//�����SVM���ĳ��Ʒָ��һ�����ַ���

namespace lpr{

const float DEFAULT_BLUEPERCEMT = 0.3f; 
const float	DEFAULT_WHITEPERCEMT = 0.1f;

CCharsSegment::CCharsSegment()   //���캯��
{
	
	m_MaoDingSize = DEFAULT_MaoDing_SIZE;
	m_theMatWidth = DEFAULT_MAT_WIDTH; 

	//��������ɫ�жϲ���
	m_ColorThreshold = DEFAULT_COLORTHRESHOLD;
	m_BluePercent = DEFAULT_BLUEPERCEMT;
	m_WhitePercent = DEFAULT_WHITEPERCEMT;
}



//! �ַ��ߴ���֤
bool CCharsSegment::verifySizes(Mat r)
{
	//Char sizes 45x90
	float aspect=45.0f/90.0f;
	float charAspect= (float)r.cols/(float)r.rows;  //�����������ı�ֵ
	float error=0.7f;
	float minHeight=10.f;
	float maxHeight=35.f;
	//We have a different aspect ratio for number 1, and it can be ~0.2
	float minAspect=0.05f;
	float maxAspect=aspect+aspect*error;
	//area of pixels


	int area = countNonZero(r);   //�õ���ֵ��ͼ��r�������ص����
	

	//bb area

	int bbArea = r.cols*r.rows;

	//% of pixel in area

	int percPixels = area / bbArea;   //�������ص����ռ�ܵĶ�ֵ��ͼ��r���صı���
	//float percPixels=area/bbArea;   //�������ص����ռ�ܵĶ�ֵ��ͼ��r���صı���

	if(percPixels <= 1 && charAspect > minAspect && charAspect < maxAspect && r.rows >= minHeight && r.rows < maxHeight)
		return true;
	else
		return false;
}


//! �ַ�Ԥ����
Mat CCharsSegment::preprocessChar(Mat in)
{
	//Remap image
	int h=in.rows;
	int w=in.cols;
	int charSize=CHAR_SIZE;	//ͳһÿ���ַ��Ĵ�С20
	Mat transformMat=Mat::eye(2,3,CV_32F);  //eye����2��3�о���ǰ2��ǰ2���ǶԽ���ʣ�µ�ȫΪ0������任ʱ����ͼ������ת��Ϊ�����ʽ
	int m=max(w,h);


	transformMat.at<float>(0, 2) =float(m / 2 - w / 2);
	transformMat.at<float>(1, 2) = float(m / 2 - h / 2);


	Mat warpImage(m,m, in.type());
	warpAffine(in, warpImage, transformMat, warpImage.size(), INTER_LINEAR, BORDER_CONSTANT, Scalar(0) );
	//����任��ʹ�����ַ�����m��m��С�������ַ�����wrapImage���м�λ�ã�����任����ָ����ˣ�transformMat��

	Mat out;
	resize(warpImage, out, Size(charSize, charSize) ); 
	//�ѷ���任���ͼ��ߴ�����Ϊ20��20����ΪANN�ı�׼����

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



//���붨λ���ƣ�����ָ����ֿ�
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
	/*namedWindow("ԭʼ���ƻҶ�ͼ��", WINDOW_NORMAL);
	imshow("ԭʼ���ƻҶ�ͼ��", input_grey);*/

	//����Ӧ��ֵ��
	//Mat img_threshold_ = img_threshold.clone();
	//int blockSize = 9;//����ߴ�����
	//int constValue = 0;
	//cv::adaptiveThreshold(img_threshold_, img_threshold_, 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, blockSize, constValue);
	//namedWindow("����Ӧ��ֵ��", WINDOW_NORMAL);
	//imshow("����Ӧ��ֵ��", img_threshold_);

	spatial_ostu(img_threshold, 8, 2, plateType);
	/*namedWindow("OTSU�����ֵ��", WINDOW_NORMAL);
	imshow("OTSU�����ֵ��", img_threshold);*/
	

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

	Rect chineseRect;//�洢�����ֿ飬sortedRect[specIndex]�洢�������ҵ�һ���ֿ�
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
	//cout << "�ָ��ֿ����Ϊ�� " << resultVec.size() << endl;
	
	return 0;
}



//! ����һ��ͼ������ɫģ���ȡ��Ӧ�Ķ�ֵͼ
//! ����RGBͼ��, ��ɫģ�壨��ɫ����ɫ��
//! ����Ҷ�ͼ��ֻ��0��255����ֵ��255����ƥ�䣬0����ƥ�䣩
Mat CCharsSegment::colorMatch(const Mat& src, Mat& match, const Color r, const bool adaptive_minsv)
{
	// S��V����Сֵ��adaptive_minsv���boolֵ�ж�
	// ���Ϊtrue������Сֵȡ����Hֵ��������˥��
	// ���Ϊfalse����������Ӧ��ʹ�ù̶�����Сֵminabs_sv
	// Ĭ��Ϊfalse
	const float max_sv = 255;
	const float minref_sv = 64;

	const float minabs_sv = 95;

	// blue��H��Χ
	const int min_blue = 100;  // 100
	const int max_blue = 140;  // 140

	// yellow��H��Χ
	const int min_yellow = 15;  // 15
	const int max_yellow = 40;  // 40

	// white��H��Χ
	const int min_white = 0;   // 15
	const int max_white = 30;  // 40

	Mat src_hsv;
	// ת��HSV�ռ���д�����ɫ������Ҫʹ�õ���H����������ɫ���ɫ��ƥ�乤��
	cvtColor(src, src_hsv, CV_BGR2HSV);

	vector<Mat> hsvSplit;
	split(src_hsv, hsvSplit);
	equalizeHist(hsvSplit[2], hsvSplit[2]);
	merge(hsvSplit, src_hsv);

	//ƥ��ģ���ɫ,�л��Բ�����Ҫ�Ļ�ɫ
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
	//ͼ����������Ҫ����ͨ������Ӱ�죻
	int nCols = src_hsv.cols * channels;

	if (src_hsv.isContinuous())  //�����洢�����ݣ���һ�д���
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

				// S��V����Сֵ��adaptive_minsv���boolֵ�ж�
				// ���Ϊtrue������Сֵȡ����Hֵ��������˥��
				// ���Ϊfalse����������Ӧ��ʹ�ù̶�����Сֵminabs_sv
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

	// ��ȡ��ɫƥ���Ķ�ֵ�Ҷ�ͼ
	Mat src_grey;
	vector<Mat> hsvSplit_done;
	split(src_hsv, hsvSplit_done);
	src_grey = hsvSplit_done[2];

	match = src_grey;

	return src_grey;
}

//! �ж�һ�����Ƶ���ɫ
//! ���복��mat����ɫģ��
//! ����true��fasle
bool CCharsSegment::plateColorJudge(const Mat& src, const Color r, const bool adaptive_minsv, float& percent)
 {
		// �ж���ֵ
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
//�жϳ��Ƶ�����
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

		// �������һ�߶���������ֵ����ȡֵ�����
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


//ȥ�������Ϸ���ť��
//����ÿ��Ԫ�صĽ�Ծ�������С��X��Ϊ��������������ȫ����0��Ϳ�ڣ�
// X���Ƽ�ֵΪ���ɸ���ʵ�ʵ���
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
			//�����ַ�����һ����������
			iCount++;
		}
	}

	////�����Ĳ��ǳ���
	if (iCount * 1.0 / img.rows <= 0.40) {
		//�������������������ҲҪ��һ������ֵ��
		return false;
	}
	//�����㳵�Ƶ�����
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


//! �������⳵��������²������ַ���λ�úʹ�С
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


//! �ҳ�ָʾ���е��ַ���Rect��������A7003X������"A"��λ��
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

		//���һ���ַ���һ���Ĵ�С���������������Ƶ�1/7��2/7֮�䣬��������Ҫ�ҵ����⳵��
		//��ǰ�ַ����¸��ַ��ľ�����һ���ķ�Χ��
		if ((mr.width > maxWidth * 0.8 || mr.height > maxHeight * 0.8) &&
			(midx < int(m_theMatWidth / 7) * 2 && midx > int(m_theMatWidth / 7) * 1))
		{
			specIndex = i;
		}
	}

	return specIndex;
}


//! �����������������
//  1.�������ַ�Rect��ߵ�ȫ��Rectȥ�����������ؽ������ַ���λ�á�
//  2.�������ַ�Rect��ʼ������ѡ��6��Rect���������ȥ��
//1.3��
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
