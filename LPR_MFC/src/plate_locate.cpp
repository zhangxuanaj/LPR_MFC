

#include "stdafx.h"
#include "../include/mser2.hpp"
#include "../include/plate_locate.h"
#include "../include/character.hpp"
#include "../include/plate.h"
#include "../include/chars_identify.h"
#include <omp.h> // OpenMP�����Ҫ������ͷ�ļ�  

namespace lpr{

	const float DEFAULT_ERROR = 0.6;//0.6
	const float DEFAULT_ASPECT = 3.75;

	CPlateLocate::CPlateLocate()
	{
		//cout << "CPlateLocate" << endl;
		m_GaussianBlurSize = DEFAULT_GAUSSIANBLUR_SIZE;
		m_MorphSizeWidth = DEFAULT_MORPH_SIZE_WIDTH;
		m_MorphSizeHeight = DEFAULT_MORPH_SIZE_HEIGHT;

		m_error = DEFAULT_ERROR;
		m_aspect = DEFAULT_ASPECT;
		m_verifyMin = DEFAULT_VERIFY_MIN;
		m_verifyMax = DEFAULT_VERIFY_MAX;

		m_angle = DEFAULT_ANGLE;

		m_debug = DEFAULT_DEBUG;
	}

	//! ����ģʽ�빤ҵģʽ�л�
	//! ���Ϊ�棬�����ø������Ϊ��λ�������Ƭ����ٶ�ͼƬ���Ĳ���������ָ�Ĭ��ֵ��
	void CPlateLocate::setLifemode(bool param)
	{
		if (param == true)
		{
			setGaussianBlurSize(5);
			setMorphSizeWidth(17);
			setMorphSizeHeight(4);
			setVerifyError(0.75);
			setVerifyAspect(4.0);
			setVerifyMin(1);
			setVerifyMax(200);
		}
		else
		{
			setGaussianBlurSize(DEFAULT_GAUSSIANBLUR_SIZE);
			setMorphSizeWidth(DEFAULT_MORPH_SIZE_WIDTH);
			setMorphSizeHeight(DEFAULT_MORPH_SIZE_HEIGHT);
			setVerifyError(DEFAULT_ERROR);
			setVerifyAspect(DEFAULT_ASPECT);
			setVerifyMin(DEFAULT_VERIFY_MIN);
			setVerifyMax(DEFAULT_VERIFY_MAX);
		}
	}


	//! mser search method
	int CPlateLocate::mserSearch(const Mat &src, vector<Mat> &out,vector<vector<CPlate>>& out_plateVec, 
		bool usePlateMser, vector<vector<RotatedRect>>& out_plateRRect,
		int img_index, bool showDebug) 
	{
		vector<Mat> match_grey;

		vector<CPlate> plateVec_blue;
		plateVec_blue.reserve(16);
		vector<RotatedRect> plateRRect_blue;
		plateRRect_blue.reserve(16);

		vector<CPlate> plateVec_yellow;
		plateVec_yellow.reserve(16);

		vector<RotatedRect> plateRRect_yellow;
		plateRRect_yellow.reserve(16);
		
		mserCharMatch(src, match_grey, plateVec_blue, plateVec_yellow, usePlateMser, plateRRect_blue, plateRRect_yellow, img_index, showDebug);

		out_plateVec.push_back(plateVec_blue);
		out_plateVec.push_back(plateVec_yellow);

		out_plateRRect.push_back(plateRRect_blue);
		out_plateRRect.push_back(plateRRect_yellow);

		out = match_grey;

		return 0;
	}

	
	//����OpenMP���д���Ҫ����Щ���������ຯ���������޷�ʶ��
	bool verifyCharSizes(Rect r) 
	{
		// Char sizes 45x90
		float aspect = 45.0f / 90.0f;
		float charAspect = (float)r.width / (float)r.height;
		float error = 0.35f;
		float minHeight = 25.f;
		float maxHeight = 50.f;
		// We have a different aspect ratio for number 1, and it can be ~0.2
		float minAspect = 0.05f;
		float maxAspect = aspect + aspect * error;

		// bb area
		int bbArea = r.width * r.height;

		if (charAspect > minAspect && charAspect < maxAspect /*&&
															 r.rows >= minHeight && r.rows < maxHeight*/)
															 return true;
		else
			return false;
	}



	bool mat_valid_position(const Mat& mat, int row, int col) 
	{
		return row >= 0 && col >= 0 && row < mat.rows && col < mat.cols;
	}



	template<class T>
	static void mat_set_invoke(Mat& mat, int row, int col, const Scalar& value) 
	{
		if (1 == mat.channels()) 
		{
			mat.at<T>(row, col) = (T)value.val[0];
		}
		else if (3 == mat.channels()) 
		{
			T* ptr_src = mat.ptr<T>(row, col);
			*ptr_src++ = (T)value.val[0];
			*ptr_src++ = (T)value.val[1];
			*ptr_src = (T)value.val[2];
		}
		else if (4 == mat.channels()) 
		{
			T* ptr_src = mat.ptr<T>(row, col);
			*ptr_src++ = (T)value.val[0];
			*ptr_src++ = (T)value.val[1];
			*ptr_src++ = (T)value.val[2];
			*ptr_src = (T)value.val[3];
		}
	}


	void setPoint(Mat& mat, int row, int col, const Scalar& value) 
	{
		if (CV_8U == mat.depth()) {
			mat_set_invoke<uchar>(mat, row, col, value);
		}
		else if (CV_8S == mat.depth()) {
			mat_set_invoke<char>(mat, row, col, value);
		}
		else if (CV_16U == mat.depth()) {
			mat_set_invoke<ushort>(mat, row, col, value);
		}
		else if (CV_16S == mat.depth()) {
			mat_set_invoke<short>(mat, row, col, value);
		}
		else if (CV_32S == mat.depth()) {
			mat_set_invoke<int>(mat, row, col, value);
		}
		else if (CV_32F == mat.depth()) {
			mat_set_invoke<float>(mat, row, col, value);
		}
		else if (CV_64F == mat.depth()) {
			mat_set_invoke<double>(mat, row, col, value);
		}
	}


	
	Mat adaptive_image_from_points(const std::vector<Point>& points,
		const Rect& rect, const Size& size, const Scalar& backgroundColor /* = ml_color_white */,
		const Scalar& forgroundColor /* = ml_color_black */, bool gray /* = true */) 
	{
		int expendHeight = 0;
		int expendWidth = 0;

		if (rect.width > rect.height) {
			expendHeight = (rect.width - rect.height) / 2;
		}
		else if (rect.height > rect.width) {
			expendWidth = (rect.height - rect.width) / 2;
		}

		Mat image(rect.height + expendHeight * 2, rect.width + expendWidth * 2, gray ? CV_8UC1 : CV_8UC3, backgroundColor);//�����ú�ɫ����

		for (int i = 0; i < (int)points.size(); ++i) {
			Point point = points[i];
			Point currentPt(point.x - rect.tl().x + expendWidth, point.y - rect.tl().y + expendHeight);
			if (mat_valid_position(image, currentPt.y, currentPt.x)) {
				setPoint(image, currentPt.y, currentPt.x, forgroundColor);  //�㼯�ð�ɫ����
			}
		}

		Mat result;
		cv::resize(image, result, size, 0, 0, INTER_NEAREST);

		return result;
	}


	bool verifyRotatedPlateSizes(RotatedRect mr, bool showDebug) 
	{
		float error = 0.65f;
		// Spain car plate size: 52x11 aspect 4,7272
		// China car plate size: 440mm*140mm��aspect 3.142857

		// Real car plate size: 136 * 32, aspect 4
		float aspect = 3.75f;

		// Set a min and max area. All other patchs are discarded
		// int min= 1*aspect*1; // minimum area
		// int max= 2000*aspect*2000; // maximum area
		//int min = 34 * 8 * 1;  // minimum area
		//int max = 34 * 8 * 200;  // maximum area

		// Get only patchs that match to a respect ratio.
		float aspect_min = aspect - aspect * error;
		float aspect_max = aspect + aspect * error;

		float width_max = 600.f;
		float width_min = 30.f;

		float min = float(width_min * width_min / aspect_max);  // minimum area
		float max = float(width_max * width_max / aspect_min);  // maximum area

		float width = mr.size.width;
		float height = mr.size.height;
		float area = width * height;

		float ratio = width / height;
		float angle = mr.angle;
		if (ratio < 1) {
			swap(width, height);
			ratio = width / height;

			angle = 90.f + angle;
			//std::cout << "angle:" << angle << std::endl;
		}

		float angle_min = -60.f;
		float angle_max = 60.f;

		//std::cout << "aspect_min:" << aspect_min << std::endl;
		//std::cout << "aspect_max:" << aspect_max << std::endl;

		if (area < min || area > max) {
			if (0 && showDebug) {
				std::cout << "area < min || area > max: " << area << std::endl;
			}

			return false;
		}
		else if (ratio < aspect_min || ratio > aspect_max) {
			if (0 && showDebug) {
				std::cout << "ratio < aspect_min || ratio > aspect_max: " << ratio << std::endl;
			}

			return false;
		}
		else if (angle < angle_min || angle > angle_max) {
			if (0 && showDebug) {
				std::cout << "angle < angle_min || angle > angle_max: " << angle << std::endl;
			}

			return false;
		}
		else if (width < width_min || width > width_max) {
			if (0 && showDebug) {
				std::cout << "width < width_min || width > width_max: " << width << std::endl;
			}

			return false;
		}
		else {
			return true;
		}

		return true;
	}


	//����������������Ľ�����
	Rect interRect(const Rect& a, const Rect& b)
	{
		Rect c;
		int x1 = a.x > b.x ? a.x : b.x;
		int y1 = a.y > b.y ? a.y : b.y;
		c.width = (a.x + a.width < b.x + b.width ? a.x + a.width : b.x + b.width) - x1;
		c.height = (a.y + a.height < b.y + b.height ? a.y + a.height : b.y + b.height) - y1;
		c.x = x1;
		c.y = y1;
		if (c.width <= 0 || c.height <= 0)
			c = Rect();//������߳��ָ�ֵ��������������û���ص����򷵻ؿվ��Σ��վ������Ͻ����ꡢ��ߡ������Ϊ0
		return c;
	}

	//����������������Ĳ�����
	Rect mergeRect(const Rect& a, const Rect& b)
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


	bool computeIOU(const Rect& rect1, const Rect& rect2, const float thresh, float& result) 
	{

		Rect inter = interRect(rect1, rect2);
		Rect urect = mergeRect(rect1, rect2);

		float iou = (float)inter.area() / (float)urect.area();
		result = iou;

		if (iou > thresh) {
			return true;
		}

		return false;
	}

	//����������������Ľ����κͲ����ε������
	float computeIOU(const Rect& rect1, const Rect& rect2)
	{

		Rect inter = interRect(rect1, rect2);
		Rect urect = mergeRect(rect1, rect2);

		float iou = (float)inter.area() / (float)urect.area();

		return iou;
	}

	Mat preprocessChar(Mat in, int char_size) 
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


	Rect adaptive_charrect_from_rect(const Rect& rect, int maxwidth, int maxheight) 
	{
		int expendWidth = 0;

		if (rect.height > 3 * rect.width) {
			expendWidth = (rect.height / 2 - rect.width) / 2;
		}

		//Rect resultRect(rect.tl().x - expendWidth, rect.tl().y, 
		//  rect.width + expendWidth * 2, rect.height);

		int tlx = rect.tl().x - expendWidth > 0 ? rect.tl().x - expendWidth : 0;
		int tly = rect.tl().y;

		int brx = rect.br().x + expendWidth < maxwidth ? rect.br().x + expendWidth : maxwidth;
		int bry = rect.br().y;

		Rect resultRect(tlx, tly, brx - tlx, bry - tly);
		return resultRect;
	}



	bool judegMDOratio2(const Mat& image, const Rect& rect, std::vector<Point>& contour, Mat& result)
	{

		Mat mser = image(rect);
		Mat mser_mat;
		cv::threshold(mser, mser_mat, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

		Rect normalRect = adaptive_charrect_from_rect(rect, image.cols, image.rows);
		Mat region = image(normalRect);
		Mat thresh_mat;
		cv::threshold(region, thresh_mat, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

		// count mser diff ratio
		int countdiff = countNonZero(thresh_mat) - countNonZero(mser_mat);

		float MserDiffOstuRatio = float(countdiff) / float(rect.area());

		if (MserDiffOstuRatio > 1) {
			/*std::cout << "MserDiffOstuRatio:" << MserDiffOstuRatio << std::endl;
			imshow("tmpMat", mser_mat);
			waitKey(0);
			imshow("tmpMat", thresh_mat);
			waitKey(0);*/

			cv::rectangle(result, rect, Scalar(0, 0, 0), 2);
			return false;
		}

		return true;
	}


	void NMStoCharacter(std::vector<CCharacter> &inVec, double overlap) 
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



	bool compareCharRect(const CCharacter& character1, const CCharacter& character2)
	{
		Rect rect1 = character1.getCharacterPos();
		Rect rect2 = character2.getCharacterPos();

		// the character in plate are similar height
		float width_1 = float(rect1.width);
		float height_1 = float(rect1.height);

		float width_2 = float(rect2.width);
		float height_2 = float(rect2.height);

		float height_diff = abs(height_1 - height_2);
		double height_diff_ratio = height_diff / min(height_1, height_2);

		if (height_diff_ratio > 0.25)
			return false;

		// the character in plate are similar in the y-axis
		float y_1 = float(rect1.tl().y);
		float y_2 = float(rect2.tl().y);

		float y_diff = abs(y_1 - y_2);
		double y_diff_ratio = y_diff / min(height_1, height_2);

		if (y_diff_ratio > 0.5)
			return false;

		// the character center in plate are not to near in the x-axis
		float x_1 = float(rect1.tl().x + rect1.width / 2);
		float x_2 = float(rect2.tl().x + rect2.width / 2);

		float x_diff = abs(x_1 - x_2);
		double x_diff_ratio = x_diff / min(height_1, height_2);

		if (x_diff_ratio < 0.25)
			return false;

		// the character in plate are near in the x-axis but not very near
		float x_margin_left = float(min(rect1.br().x, rect2.br().x));
		float x_margin_right = float(max(rect1.tl().x, rect2.tl().x));

		float x_margin_diff = abs(x_margin_left - x_margin_right);
		double x_margin_diff_ratio = x_margin_diff / min(height_1, height_2);

		if (x_margin_diff_ratio > 1.0)
			return false;

		return true;
	}


	void mergeCharToGroup(std::vector<CCharacter> vecRect,std::vector<std::vector<CCharacter>>& charGroupVec) 
	{

		std::vector<int> labels;

		int numbers = 0;
		if (vecRect.size() > 0)
			numbers = partition(vecRect, labels, &compareCharRect);

		for (size_t j = 0; j < size_t(numbers); j++) {
			std::vector<CCharacter> charGroup;

			for (size_t t = 0; t < vecRect.size(); t++) {
				int label = labels[t];

				if (label == j)
					charGroup.push_back(vecRect[t]);
			}

			if (charGroup.size() < 2)
				continue;

			charGroupVec.push_back(charGroup);
		}
	}


 
	// the slope are nealy the same along the line
	// if one slope is much different others, it should be outliers
	// this function to remove it
	void removeRightOutliers(std::vector<CCharacter>& charGroup, std::vector<CCharacter>& out_charGroup, double thresh1, double thresh2, Mat result)
	{
		std::sort(charGroup.begin(), charGroup.end(),
			[](const CCharacter& r1, const CCharacter& r2) 
		{
			return r1.getCenterPoint().x < r2.getCenterPoint().x;
		});

		std::vector<float> slopeVec;
		float slope_last = 0;
		for (size_t charGroup_i = 0; charGroup_i + 1 < charGroup.size(); charGroup_i++) {
			// line_between_two_points
			Vec4f line_btp;
			CCharacter leftChar = charGroup.at(charGroup_i);
			CCharacter rightChar = charGroup.at(charGroup_i + 1);
			std::vector<Point> two_points;
			two_points.push_back(leftChar.getCenterPoint());
			two_points.push_back(rightChar.getCenterPoint());
			fitLine(Mat(two_points), line_btp, CV_DIST_L2, 0, 0.01, 0.01);
			float slope = line_btp[1] / line_btp[0];
			slopeVec.push_back(slope);

			if (0) {
				cv::line(result, leftChar.getCenterPoint(), rightChar.getCenterPoint(), Scalar(0, 0, 255));
			}
		}

		int uniformity_count = 0;
		int outlier_index = -1;
		for (size_t slopeVec_i = 0; slopeVec_i + 1 < slopeVec.size(); slopeVec_i++) {
			float slope_1 = slopeVec.at(slopeVec_i);
			float slope_2 = slopeVec.at(slopeVec_i + 1);
			float slope_diff = abs(slope_1 - slope_2);
			if (0) {
				std::cout << "slope_diff:" << slope_diff << std::endl;
			}
			if (slope_diff <= thresh1) {
				uniformity_count++;
			}
			if (0) {
				std::cout << "slope_1:" << slope_1 << std::endl;
				std::cout << "slope_2:" << slope_2 << std::endl;
			}
			if (1/*(slope_1 <= 0 && slope_2 >= 0) || (slope_1 >= 0 && slope_2 <= 0)*/) {
				if (uniformity_count >= 2 && slope_diff >= thresh2) {
					outlier_index = slopeVec_i + 2;
					break;
				}
			}
		}
		if (0) {
			std::cout << "uniformity_count:" << uniformity_count << std::endl;
			std::cout << "outlier_index:" << outlier_index << std::endl;
		}

		for (int charGroup_i = 0; charGroup_i < (int)charGroup.size(); charGroup_i++) {
			if (charGroup_i != outlier_index) {
				CCharacter theChar = charGroup.at(charGroup_i);
				out_charGroup.push_back(theChar);
			}
		}

		if (0) {
			std::cout << "end:" << std::endl;
		}
	}


	void searchWeakSeed(const std::vector<CCharacter>& charVec, std::vector<CCharacter>& mserCharacter, double thresh1, double thresh2,
		const Vec4f& line, Point& boundaryPoint, const Rect& maxrect, Rect& plateResult, Mat result, CharSearchDirection searchDirection)
	{

		float k = line[1] / line[0];
		float x_1 = line[2];
		float y_1 = line[3];

		std::vector<CCharacter> searchWeakSeedVec;
		searchWeakSeedVec.reserve(8);

		for (auto weakSeed : charVec) {
			Rect weakRect = weakSeed.getCharacterPos();

			//cv::rectangle(result, weakRect, Scalar(255, 0, 255));

			Point weakCenter(weakRect.tl().x + weakRect.width / 2, weakRect.tl().y + weakRect.height / 2);
			float x_2 = (float)weakCenter.x;

			if (searchDirection == CharSearchDirection::LEFT) {
				if (weakCenter.x + weakRect.width / 2 > boundaryPoint.x) {
					continue;
				}
			}
			else if (searchDirection == CharSearchDirection::RIGHT) {
				if (weakCenter.x - weakRect.width / 2 < boundaryPoint.x) {
					continue;
				}
			}

			float y_2l = k * (x_2 - x_1) + y_1;
			float y_2 = (float)weakCenter.y;

			float y_diff_ratio = abs(y_2l - y_2) / maxrect.height;

			if (y_diff_ratio < thresh1) {
				float width_1 = float(maxrect.width);
				float height_1 = float(maxrect.height);

				float width_2 = float(weakRect.width);
				float height_2 = float(weakRect.height);

				float height_diff = abs(height_1 - height_2);
				double height_diff_ratio = height_diff / min(height_1, height_2);

				float width_diff = abs(width_1 - width_2);
				double width_diff_ratio = width_diff / maxrect.width;

				if (height_diff_ratio < thresh1 && width_diff_ratio < 0.5) {
					//std::cout << "h" << height_diff_ratio << std::endl;
					//std::cout << "w" << width_diff_ratio << std::endl;
					searchWeakSeedVec.push_back(weakSeed);
				}
				else {

				}
			}
		}

		// form right to left to split
		if (searchWeakSeedVec.size() != 0) {
			if (searchDirection == CharSearchDirection::LEFT) {
				std::sort(searchWeakSeedVec.begin(), searchWeakSeedVec.end(),
					[](const CCharacter& r1, const CCharacter& r2) {
					return r1.getCharacterPos().tl().x > r2.getCharacterPos().tl().x;
				});
			}
			else if (searchDirection == CharSearchDirection::RIGHT) {
				std::sort(searchWeakSeedVec.begin(), searchWeakSeedVec.end(),
					[](const CCharacter& r1, const CCharacter& r2) {
					return r1.getCharacterPos().tl().x < r2.getCharacterPos().tl().x;
				});
			}

			CCharacter firstWeakSeed = searchWeakSeedVec.at(0);
			Rect firstWeakRect = firstWeakSeed.getCharacterPos();
			Point firstWeakCenter(firstWeakRect.tl().x + firstWeakRect.width / 2,
				firstWeakRect.tl().y + firstWeakRect.height / 2);

			float ratio = (float)abs(firstWeakCenter.x - boundaryPoint.x) / (float)maxrect.height;
			if (ratio > thresh2) {
				if (0) {
					std::cout << "search seed ratio:" << ratio << std::endl;
				}
				return;
			}

			mserCharacter.push_back(firstWeakSeed);
			plateResult |= firstWeakRect;
			boundaryPoint = firstWeakCenter;

			for (size_t weakSeedIndex = 0; weakSeedIndex + 1 < searchWeakSeedVec.size(); weakSeedIndex++) {
				CCharacter weakSeed = searchWeakSeedVec[weakSeedIndex];
				CCharacter weakSeedCompare = searchWeakSeedVec[weakSeedIndex + 1];

				Rect rect1 = weakSeed.getCharacterPos();
				Rect rect2 = weakSeedCompare.getCharacterPos();

				Rect weakRect = rect2;
				Point weakCenter(weakRect.tl().x + weakRect.width / 2, weakRect.tl().y + weakRect.height / 2);

				// the character in plate are similar height
				float width_1 = float(rect1.width);
				float height_1 = float(rect1.height);

				float width_2 = float(rect2.width);
				float height_2 = float(rect2.height);

				// the character in plate are near in the x-axis but not very near
				float x_margin_left = float(min(rect1.br().x, rect2.br().x));
				float x_margin_right = float(max(rect1.tl().x, rect2.tl().x));

				float x_margin_diff = abs(x_margin_left - x_margin_right);
				double x_margin_diff_ratio = x_margin_diff / min(height_1, height_2);

				if (x_margin_diff_ratio > thresh2) {
					if (0) {
						std::cout << "search seed x_margin_diff_ratio:" << x_margin_diff_ratio << std::endl;
					}
					break;
				}
				else {
					//::rectangle(result, weakRect, Scalar(255, 0, 0), 1);
					mserCharacter.push_back(weakSeedCompare);
					plateResult |= weakRect;
					if (searchDirection == CharSearchDirection::LEFT) {
						if (weakCenter.x < boundaryPoint.x) {
							boundaryPoint = weakCenter;
						}
					}
					else if (searchDirection == CharSearchDirection::RIGHT) {
						if (weakCenter.x > boundaryPoint.x) {
							boundaryPoint = weakCenter;
						}
					}
				}
			}
		}
	}


	Rect getSafeRect(Point2f center, float width, float height, Mat image) 
	{
		int rows = image.rows;
		int cols = image.cols;

		float x = center.x;
		float y = center.y;

		float x_tl = (x - width / 2.f);
		float y_tl = (y - height / 2.f);

		float x_br = (x + width / 2.f);
		float y_br = (y + height / 2.f);

		x_tl = x_tl > 0.f ? x_tl : 0.f;
		y_tl = y_tl > 0.f ? y_tl : 0.f;
		x_br = x_br < (float)image.cols ? x_br : (float)image.cols;
		y_br = y_br < (float)image.rows ? y_br : (float)image.rows;

		Rect rect(Point((int)x_tl, int(y_tl)), Point((int)x_br, int(y_br)));
		return rect;
	}


	// based on the assumptions: distance beween two nearby characters in plate are the same.
	// add not found rect and combine two small and near rect.
	void reFoundAndCombineRect(std::vector<CCharacter>& mserCharacter, float min_thresh, float max_thresh,
		Vec2i dist, Rect maxrect, Mat result)
	{
		if (mserCharacter.size() == 0) {
			return;
		}

		std::sort(mserCharacter.begin(), mserCharacter.end(),
			[](const CCharacter& r1, const CCharacter& r2) {
			return r1.getCenterPoint().x < r2.getCenterPoint().x;
		});

		int comparDist = dist[0] * dist[0] + dist[1] * dist[1];
		if (0) {
			std::cout << "comparDist:" << comparDist << std::endl;
		}

		std::vector<CCharacter> reCharacters;

		size_t mserCharacter_i = 0;
		for (; mserCharacter_i + 1 < mserCharacter.size(); mserCharacter_i++) {
			CCharacter leftChar = mserCharacter.at(mserCharacter_i);
			CCharacter rightChar = mserCharacter.at(mserCharacter_i + 1);

			Point leftCenter = leftChar.getCenterPoint();
			Point rightCenter = rightChar.getCenterPoint();

			int x_diff = leftCenter.x - rightCenter.x;
			int y_diff = leftCenter.y - rightCenter.y;

			// distance between two centers
			int distance2 = x_diff * x_diff + y_diff * y_diff;

			if (0) {
				std::cout << "distance2:" << distance2 << std::endl;
			}

			float ratio = (float)distance2 / (float)comparDist;
			if (ratio > max_thresh) {
				float x_add = (float)(leftCenter.x + rightCenter.x) / 2.f;
				float y_add = (float)(leftCenter.y + rightCenter.y) / 2.f;

				float width = (float)maxrect.width;
				float height = (float)maxrect.height;

				float x_tl = (x_add - width / 2.f);
				float y_tl = (y_add - height / 2.f);

				//Rect rect_add((int)x_tl, (int)y_tl, (int)width, (int)height);
				Rect rect_add = getSafeRect(Point2f(x_add, y_add), width, height, result);

				reCharacters.push_back(leftChar);

				CCharacter charAdd;
				charAdd.setCenterPoint(Point((int)x_add, (int)y_add));
				charAdd.setCharacterPos(rect_add);
				reCharacters.push_back(charAdd);

				if (1) {
					cv::rectangle(result, rect_add, Scalar(0, 128, 255));
				}
			}
			else if (ratio < min_thresh) {
				Rect rect_union = leftChar.getCharacterPos() | rightChar.getCharacterPos();
				/*float x_add = (float)(leftCenter.x + rightCenter.x) / 2.f;
				float y_add = (float)(leftCenter.y + rightCenter.y) / 2.f;*/
				int x_add = rect_union.tl().x + rect_union.width / 2;
				int y_add = rect_union.tl().y + rect_union.height / 2;

				CCharacter charAdd;
				charAdd.setCenterPoint(Point(x_add, y_add));
				charAdd.setCharacterPos(rect_union);
				reCharacters.push_back(charAdd);
				if (1) {
					cv::rectangle(result, rect_union, Scalar(0, 128, 255));
				}

				mserCharacter_i++;
			}
			else {
				reCharacters.push_back(leftChar);
			}
		}

		if (mserCharacter_i + 1 == mserCharacter.size()) {
			reCharacters.push_back(mserCharacter.at(mserCharacter_i));
		}

		mserCharacter = reCharacters;
	}



	void slideWindowSearch(const Mat &image, std::vector<CCharacter>& slideCharacter, const Vec4f& line,
		Point& fromPoint, const Vec2i& dist, double ostu_level, float ratioWindow, float threshIsCharacter, const Rect& maxrect, Rect& plateResult,
		CharSearchDirection searchDirection, bool isChinese, Mat& result) 
	{
		float k = line[1] / line[0];
		float x_1 = line[2];
		float y_1 = line[3];

		int slideLength = int(ratioWindow * maxrect.width);
		int slideStep = 1;
		int fromX = 0;
		if (searchDirection == CharSearchDirection::LEFT) 
		{
			fromX = fromPoint.x - dist[0];
		}
		else if (searchDirection == CharSearchDirection::RIGHT)
		{
			fromX = fromPoint.x + dist[0];
		}

		std::vector<CCharacter> charCandidateVec;
		for (int slideX = -slideLength; slideX < slideLength; slideX += slideStep)
		{
			float x_slide = 0;

			if (searchDirection == CharSearchDirection::LEFT) 
			{
				x_slide = float(fromX - slideX);
			}
			else if (searchDirection == CharSearchDirection::RIGHT) 
			{
				x_slide = float(fromX + slideX);
			}

			float y_slide = k * (x_slide - x_1) + y_1;
			Point2f p_slide(x_slide, y_slide);
			cv::circle(result, p_slide, 2, Scalar(255, 255, 255), 1);

			int chineseWidth = int(maxrect.width * 1.05);
			int chineseHeight = int(maxrect.height * 1.05);

			Rect rect(Point2f(x_slide - chineseWidth / 2, y_slide - chineseHeight / 2), Size(chineseWidth, chineseHeight));

			if (rect.tl().x < 0 || rect.tl().y < 0 || rect.br().x >= image.cols || rect.br().y >= image.rows)
				continue;

			Mat region = image(rect);
			Mat binary_region;

			cv::threshold(region, binary_region, ostu_level, 255, CV_THRESH_BINARY);
			//double ostu_level = threshold(region, binary_region, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
			//std::cout << "ostu_level:" << ostu_level << std::endl;*/

			Mat charInput = preprocessChar(binary_region, 20);

			if (0) {
				imshow("charInput", charInput);
				waitKey(0);
				destroyWindow("charInput");
			}

			CCharacter charCandidate;
			charCandidate.setCharacterPos(rect);
			charCandidate.setCharacterMat(charInput);
			charCandidate.setIsChinese(isChinese);
			charCandidateVec.push_back(charCandidate);
		}

		if (isChinese)
		{
			CharsIdentify::instance()->classifyChinese(charCandidateVec);
		}
		else 
		{
			CharsIdentify::instance()->classify(charCandidateVec);
		}

		double overlapThresh = 0.1;
		NMStoCharacter(charCandidateVec, overlapThresh);

		for (auto character : charCandidateVec) 
		{
			Rect rect = character.getCharacterPos();
			Point center(rect.tl().x + rect.width / 2, rect.tl().y + rect.height / 2);

			if (character.getCharacterScore() > threshIsCharacter && character.getCharacterStr() != "1")
			{
				//cv::rectangle(result, rect, Scalar(255, 255, 255), 1);
				plateResult |= rect;
				slideCharacter.push_back(character);
				fromPoint = center;

				if (0) 
				{
					std::cout << "label:" << character.getCharacterStr();
					std::cout << "__score:" << character.getCharacterScore() << std::endl;
				}
			}
		}
	}



	void rotatedRectangle(InputOutputArray image, RotatedRect rrect, const Scalar& color, int thickness, int lineType, int shift) 
	{
		Point2f rect_points[4];
		rrect.points(rect_points);
		for (int j = 0; j < 4; j++) {
			cv::line(image, rect_points[j], rect_points[(j + 1) % 4], color, thickness, lineType, shift);
		}
	}



	//mser�㷨��Ϊmser+��mser-����Ϊ�Ҷ�ֵ�仯�Ǵ�0�䵽255����˵���mser���ֻ�ܼ�����ɫ���򣬼�mser+
	//�����ڰ�ɫ����255����Ϊ��254�䵽255�Ѿ������ˣ�û����255��������м�⣩���޷����
	//�����Ҫ��ͼ����лҶȷ�ת��Ȼ���ٽ���һ��mser��������mser-
	//mser+��mser-�õ��Ķ�������ȶ���ֵ����ĵ㼯��һ�����Ϊ�˱�����ʾ������Щ�㼯�ԻҶ�ֵ255��ʾ
	//�������ֵ�ȶ�������ԻҶ�ֵ0��ʾ�������ڰ�ͼ���а�ɫ����������ֵ�ȶ�����
	//Ȼ���mser+��mser-���а�λ��������������յõ��İ�ɫ�������mser+��mser-������Ľ���
	//����û�н��а�λ�����
	//! use verify size to first generate char candidates
	void mserCharMatch(const Mat &src, std::vector<Mat> &match, std::vector<CPlate>& out_plateVec_blue, std::vector<CPlate>& out_plateVec_yellow,
		bool usePlateMser, std::vector<RotatedRect>& out_plateRRect_blue, std::vector<RotatedRect>& out_plateRRect_yellow, int img_index,
		bool showDebug) 
	{
		Mat image = src;

		std::vector<std::vector<std::vector<Point>>> all_contours;//�洢���ֵ�ȶ�����㼯
		std::vector<std::vector<Rect>> all_boxes;  //�洢mser�����box��ʵ���Ͼ��ǵ㼯����Ӿ��Σ�������㼯������ͬ
		//ע��resize��reserve������
		//reserveֻ��Ԥ���ռ䣬����û�з����ڴ棬��Ҫ������������ܣ����ⲻ��Ҫ�������ڴ����
		//resizeֱ�ӽ�������С����Ϊn�����Ҹ����ڴ�
		//�����棬all_contours�Ĵ�С������resize����Ϊ����Ҫֱ���÷��ʵ���ʽ��at�����±�[]���������︳ֵ
		//all_contours.reserve(2);//д�������������������λ��detectRegion��
		all_contours.resize(2);   //����all_contours������СΪ2���ֱ���mser+��mser-�ĵ㼯����
		all_contours.at(0).reserve(1024);//����all_contours�����е�һ����������СΪ1024��Ҳ����˵mser+�����ĵ㼯����Ϊ1024������1024������ȶ���ֵ����mser-ͬ��
		all_contours.at(1).reserve(1024);
		all_boxes.resize(2);  //����all_boxes������СΪ2���ֱ���mser+��mser-�е㼯����Ӿ���
		all_boxes.at(0).reserve(1024);  //����all_boxes��������������СΪ1024�������mser+������1024���㼯��1024����Ӿ���
		all_boxes.at(1).reserve(1024);

		match.resize(2);  //match������С����Ϊ2

		std::vector<Color> flags;  //�洢0��1
		flags.push_back(BLUE);
		flags.push_back(YELLOW);

		const int imageArea = image.rows * image.cols;
		const int delta = 1; //mser�㷨�Ҷ�ֵ�仯��delta��������Ϊ1
		//const int delta = CParams::instance()->getParam2i();;
		const int minArea = 30;   //��⵽����������СֵΪ30
		const double maxAreaRatio = 0.05;  //��⵽�����������ֵΪͼ��ߴ��0.05��

		Ptr<MSER2> mser;  //����MSER2ָ�����mser
		mser = MSER2::create(delta, minArea, int(maxAreaRatio * imageArea));  //����mser���󣬲�������Ϊ�Ҷȱ仯���������С��������������
		//����mser��⣬all_contours.at(0)���mser-���������ɫ����Ҳ�������Ƶİ���
		//����mser��⣬all_contours.at(1)���mser+���������ɫ����Ҳ���ǻ��Ƶĺ���
		mser->detectRegions(image, all_contours.at(0), all_boxes.at(0), all_contours.at(1), all_boxes.at(1));
		
		//����mser-���Ч��ͼ������src������ǻҶ�ͼ
		//Mat mser_ = Mat::zeros(image.size(), CV_8UC1);
		//for (int i = (int)all_contours.at(0).size() - 1; i >= 0;i--)  //ʹ��size_t�����
		//{
		//	//���ݼ�����������mser-���
		//	const vector<Point>&r = all_contours.at(0)[i];
		//	for (int j = 0; j < (int)r.size();j++)
		//	{
		//		Point pt = r[j];
		//		mser_.at<uchar>(pt) = 255;
		//	}
		//}
		//imshow("MSER-�����", mser_);
		//waitKey(0);
		////�ղ���
		////���ú�
		//Mat element = getStructuringElement(MORPH_RECT, Size(17, 3));
		//morphologyEx(mser_, mser_, MORPH_CLOSE, element);
		//imshow("MSER-�ղ���", mser_);
		//waitKey(0);
		////����mser+���Ч��ͼ������src������ǻҶ�ͼ
		//Mat _mser = Mat::zeros(image.size(), CV_8UC1);
		//for (int i = (int)all_contours.at(1).size() - 1; i >= 0; i--)
		//{
		//	//���ݼ�����������mser-���
		//	const vector<Point>&r = all_contours.at(1)[i];
		//	for (int j = 0; j < (int)r.size(); j++)
		//	{
		//		Point pt = r[j];
		//		_mser.at<uchar>(pt) = 255;
		//	}
		//}
		//imshow("MSER+�����", _mser);
		//waitKey(200000);
		////�ղ���
		////Mat element = getStructuringElement(MORPH_RECT, Size(17, 3));
		//morphologyEx(_mser,_mser, MORPH_CLOSE, element);
		//imshow("MSER+�ղ���", _mser);
		//waitKey(0);
		//// mser+��mser-λ�����
		///*Mat mserRes;
		//mserRes = mser_ & _mser;
		//imshow("MSER+��MSER-��λ��", _mser);
		//waitKey(0);*/


		//��֤mser�����Ƿ��������ҷ��ֵ㼯����=boxes����������ж�all_boxes���Ǽ����ĵ㼯����Ӿ��Σ�
		//cout <<"��ɫ����㼯����Ϊ�� " <<all_contours[0].size() << endl;
		//cout <<"��ɫ����boxes����Ϊ��" << all_boxes.at(0).size() << endl;
		// mser detect 
		// color_index = 0 : mser-, detect white characters, which is in blue plate.
		// color_index = 1 : mser+, detect dark characters, which is in yellow plate.

#pragma omp parallel for//OpenMP���̼߳���
		for (int color_index = 0; color_index < 2; color_index++) //�ֱ��mser+�㼯��mser-�㼯������֤
		{
			Color the_color = flags.at(color_index);

			std::vector<CCharacter> charVec; //����ֿ飨�����ֿ飬���ߴ��Ϸ����ֿ�Ҫ�󣩵�����
			charVec.reserve(128);//�����ֿ�����Ԥ����СΪ128

			match.at(color_index) = Mat::zeros(image.rows, image.cols, image.type());

			Mat result = image.clone();
			cvtColor(result, result, COLOR_GRAY2BGR);

			size_t size = all_contours.at(color_index).size(); //mser-�㼯��������һѭ����mser+�㼯������

			int char_index = 0;
			int char_size = 20;

			// Chinese plate has max 7 characters.
			const int char_max_count = 7;

			//��mser-��mser+���㼯�е�ÿ�����boxes������֤���㼯����=boxes����
			//�������ǽ������ֿ�ߴ�Ҫ������boxes��Ӧ���������charVec������
			// verify char size and output to rects;
			for (size_t index = 0; index < size; index++) 
			{
				Rect rect = all_boxes.at(color_index)[index];//rect�ǵ����㼯�����boxes
				std::vector<Point>& contour = all_contours.at(color_index)[index];

				// sometimes a plate could be a mser rect, so we could
				// also use mser algorithm to find plate
				if (usePlateMser) 
				{
					RotatedRect rrect = minAreaRect(Mat(contour));
					if (verifyRotatedPlateSizes(rrect,0)) 
					{
						//cout << "mserֱ�Ӽ�⵽�������� " << endl;//ͨ����һ�п������Կ��������˶��̣߳���Ϊ��������˷ֶ�
						//rotatedRectangle(result, rrect, Scalar(255, 0, 0), 2);
						if (the_color == BLUE) out_plateRRect_blue.push_back(rrect);
						if (the_color == YELLOW) out_plateRRect_yellow.push_back(rrect);
					}
				}

				// find character
				if (verifyCharSizes(rect)) 
				{
					
					Mat mserMat = adaptive_image_from_points(contour, rect, Size(char_size, char_size));//��һ�����֮ǰ˽�Ը��������������ֵ������mser����ʧЧ
					//ͷ�ļ��ú�������ʱ�����������������Ĭ��ֵ������֮����ò��ô���������Ĭ��ֵ
					Mat charInput = preprocessChar(mserMat, char_size);
					Rect charRect = rect;

					Point center(charRect.tl().x + charRect.width / 2, charRect.tl().y + charRect.height / 2);
					Mat tmpMat;
					double ostu_level = cv::threshold(image(charRect), tmpMat, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

					//cv::circle(result, center, 3, Scalar(0, 0, 255), 2);

					// use judegMDOratio2 function to
					// remove the small lines in character like "zh-cuan"
					if (judegMDOratio2(image, rect, contour, result)) 
					{
						CCharacter charCandidate;
						charCandidate.setCharacterPos(charRect);
						charCandidate.setCharacterMat(charInput);
						charCandidate.setOstuLevel(ostu_level);
						charCandidate.setCenterPoint(center);
						charCandidate.setIsChinese(false);
						charVec.push_back(charCandidate);//������Ҫ����ֿ����charVec��
					}
				}
				
			}
			//��֤�Ƿ��з��ϳߴ�Ҫ���ֿ������Ԥ��128�����ǿ��Գ���
			//cout << "�����ֿ�ߴ�Ҫ������������ " << charVec.size() << endl;

			//ʹ�÷Ǽ���ֵ�����ֶΣ�����ѡ�����ָ��ʸ���0.9��ǿ���ӽ��оۺ�
			// improtant, use matrix multiplication to acclerate the 
			// classification of many samples. use the character 
			// score, we can use non-maximum superssion (nms) to 
			// reduce the characters which are not likely to be true
			// charaters, and use the score to select the strong seed
			// of which the score is larger than 0.9
			//classify����Ϊ���ã���ζ����classify����������charVec������޸�
			//Ҳ����˵�������ÿһ���ֿ飨CCharacter�ࣩ�����Զ�������޸ģ���Ҫ�Ĳ����������m_score�ᱻ����
			//������classify��ÿ���ֿ�ı���m_score�������������ֵĸ��ʣ�����0.9λǿ���ӣ��Դ�����
			CharsIdentify::instance()->classify(charVec);

			// use nms to remove the character are not likely to be true.
			double overlapThresh = 0.6;
			//double overlapThresh = CParams::instance()->getParam1f();
			//�Բ�����NMStoCharacter����charVec���ֿ����ɾ��
			NMStoCharacter(charVec, overlapThresh);
			charVec.shrink_to_fit();//����������������sizeƥ��

			std::vector<CCharacter> strongSeedVec;//ǿ������������ɫ����
			strongSeedVec.reserve(64);
			std::vector<CCharacter> weakSeedVec;//��������������ɫ����
			weakSeedVec.reserve(64);
			std::vector<CCharacter> littleSeedVec;//����������������ɫ����
			littleSeedVec.reserve(64);

			//size_t charCan_size = charVec.size();
			for (auto charCandidate : charVec) 
			{
				//CCharacter& charCandidate = charVec[char_index];
				Rect rect = charCandidate.getCharacterPos();
				double score = charCandidate.getCharacterScore();
				//����ֿ�m_scoreֵ����0.9������ֿ����ǿ��������
				if (charCandidate.getIsStrong()) 
				{
					strongSeedVec.push_back(charCandidate);
				}
				//����ֿ�m_scoreֵ����0.5С��0.9��������Ӵ�������������
				else if (charCandidate.getIsWeak()) 
				{
					weakSeedVec.push_back(charCandidate);
					//cv::rectangle(result, rect, Scalar(255, 0, 255));
				}
				//����ֿ�m_scoreֵС��0.5��������Ӵ��벻����������
				else if (charCandidate.getIsLittle()) 
				{
					littleSeedVec.push_back(charCandidate);
					//cv::rectangle(result, rect, Scalar(255, 0, 255));
				}
			}

			std::vector<CCharacter> searchCandidate = charVec;
			//cout << "ǿ���Ӹ����� " << strongSeedVec.size() <<endl;
			// nms to srong seed, only leave the strongest one
		    // ��ǿ���ӽ���nms��ɾ����̫ǿ�ģ�ֻ������ǿ��
			overlapThresh = 0.3;
			NMStoCharacter(strongSeedVec, overlapThresh);

			//cout << "nms��ǿ���Ӹ����� " << strongSeedVec.size() <<endl;

			// merge chars to group
			//ǿ���Ӿۺ�
			std::vector<std::vector<CCharacter>> charGroupVec;
			charGroupVec.reserve(64);
			mergeCharToGroup(strongSeedVec, charGroupVec);

			//ǿ���Ӿۺ������ߣ������߿�����Ϊ�ǳ���������
			// genenrate the line of the group
			// based on the assumptions , the mser rects which are 
			// given high socre by character classifier could be no doubtly
			// be the characters in one plate, and we can use these characeters
			// to fit a line which is the middle line of the plate.
			std::vector<CPlate> plateVec;
			plateVec.reserve(16);
			for (auto charGroup : charGroupVec) 
			{
				Rect plateResult = charGroup[0].getCharacterPos();//ǿ���Ӿۺ���ÿ���ֿ��Rect
				std::vector<Point> points;
				points.reserve(32);

				Vec4f line;
				int maxarea = 0;
				Rect maxrect;
				double ostu_level_sum = 0;

				int leftx = image.cols;
				Point leftPoint(leftx, 0);
				int rightx = 0;
				Point rightPoint(rightx, 0);

				std::vector<CCharacter> mserCharVec;
				mserCharVec.reserve(32);

				// remove outlier CharGroup
				std::vector<CCharacter> roCharGroup;
				roCharGroup.reserve(32);

				removeRightOutliers(charGroup, roCharGroup, 0.2, 0.5, result);
				//roCharGroup = charGroup;

				for (auto character : roCharGroup) 
				{
					Rect charRect = character.getCharacterPos();
					cv::rectangle(result, charRect, Scalar(0, 255, 0), 1);
					plateResult |= charRect;

					Point center(charRect.tl().x + charRect.width / 2, charRect.tl().y + charRect.height / 2);
					points.push_back(center);
					mserCharVec.push_back(character);
					//cv::circle(result, center, 3, Scalar(0, 255, 0), 2);

					ostu_level_sum += character.getOstuLevel();


					if (charRect.area() > maxarea) {
						maxrect = charRect;
						maxarea = charRect.area();
					}
					if (center.x < leftPoint.x) 
					{
						leftPoint = center;
					}
					if (center.x > rightPoint.x) 
					{
						rightPoint = center;
					}
				}

				double ostu_level_avg = ostu_level_sum / (double)roCharGroup.size();
				if (1 && showDebug) 
				{
					std::cout << "ostu_level_avg:" << ostu_level_avg << std::endl;
				}
				float ratio_maxrect = (float)maxrect.width / (float)maxrect.height;

				if (points.size() >= 2 && ratio_maxrect >= 0.3) 
				{
					fitLine(Mat(points), line, CV_DIST_L2, 0, 0.01, 0.01);

					float k = line[1] / line[0];
					//float angle = atan(k) * 180 / (float)CV_PI;
					//std::cout << "k:" << k << std::endl;
					//std::cout << "angle:" << angle << std::endl;
					//std::cout << "cos:" << 0.3 * cos(k) << std::endl;
					//std::cout << "ratio_maxrect:" << ratio_maxrect << std::endl;

					std::sort(mserCharVec.begin(), mserCharVec.end(),
						[](const CCharacter& r1, const CCharacter& r2) 
					{
						return r1.getCharacterPos().tl().x < r2.getCharacterPos().tl().x;
					});

					CCharacter midChar = mserCharVec.at(int(mserCharVec.size() / 2.f));
					Rect midRect = midChar.getCharacterPos();
					Point midCenter(midRect.tl().x + midRect.width / 2, midRect.tl().y + midRect.height / 2);

					int mindist = 7 * maxrect.width;
					std::vector<Vec2i> distVecVec;
					distVecVec.reserve(32);

					Vec2i mindistVec;
					Vec2i avgdistVec;

					//ͨ��ǿ���Ӿۺ������ֿ���㳵�������ֿ�ľ���
					//��������������ָ����������������������Χ
					// computer the dist which is the distacne between 
					// two near characters in the plate, use dist we can
					// judege how to computer the max search range, and choose the
					// best location of the sliding window in the next steps.
					for (size_t mser_i = 0; mser_i + 1 < mserCharVec.size(); mser_i++) 
					{
						Rect charRect = mserCharVec.at(mser_i).getCharacterPos();
						Point center(charRect.tl().x + charRect.width / 2, charRect.tl().y + charRect.height / 2);

						Rect charRectCompare = mserCharVec.at(mser_i + 1).getCharacterPos();
						Point centerCompare(charRectCompare.tl().x + charRectCompare.width / 2,
							charRectCompare.tl().y + charRectCompare.height / 2);

						int dist = charRectCompare.x - charRect.x;
						Vec2i distVec(charRectCompare.x - charRect.x, charRectCompare.y - charRect.y);
						distVecVec.push_back(distVec);

						//if (dist < mindist) 
						//{
						//  mindist = dist;
						//  mindistVec = distVec;
						//}
					}

					std::sort(distVecVec.begin(), distVecVec.end(),
						[](const Vec2i& r1, const Vec2i& r2) 
					{
						return r1[0] < r2[0];
					});

					avgdistVec = distVecVec.at(int((distVecVec.size() - 1) / 2.f));

					//float step = 10.f * (float)maxrect.width;
					//float step = (float)mindistVec[0];
					float step = (float)avgdistVec[0];

					//cv::line(result, Point2f(line[2] - step, line[3] - k*step), Point2f(line[2] + step, k*step + line[3]), Scalar(255, 255, 255));
					cv::line(result, Point2f(midCenter.x - step, midCenter.y - k*step), Point2f(midCenter.x + step, k*step + midCenter.y), Scalar(255, 255, 255));
					//cv::circle(result, leftPoint, 3, Scalar(0, 0, 255), 2);

					CPlate plate;
					plate.setPlateLeftPoint(leftPoint);
					plate.setPlateRightPoint(rightPoint);

					plate.setPlateLine(line);
					plate.setPlatDistVec(avgdistVec);
					plate.setOstuLevel(ostu_level_avg);

					plate.setPlateMergeCharRect(plateResult);
					plate.setPlateMaxCharRect(maxrect);
					plate.setMserCharacter(mserCharVec);
					plateVec.push_back(plate);
				}
			}

			//ǿ���Ӿۺ������ߺ������������������߶������ӽ�������
			//ֻ��λ�������߸����������Ӳſ�������ȷ���ֿ�
			// use strong seed to construct the first shape of the plate,
			// then we need to find characters which are the weak seed.
			// because we use strong seed to build the middle lines of the plate,
			// we can simply use this to consider weak seeds only lie in the 
			// near place of the middle line
			for (auto plate : plateVec)
			{
				Vec4f line = plate.getPlateLine();
				Point leftPoint = plate.getPlateLeftPoint();
				Point rightPoint = plate.getPlateRightPoint();

				Rect plateResult = plate.getPlateMergeCharRect();
				Rect maxrect = plate.getPlateMaxCharRect();
				Vec2i dist = plate.getPlateDistVec();
				double ostu_level = plate.getOstuLevel();

				std::vector<CCharacter> mserCharacter = plate.getCopyOfMserCharacters();
				mserCharacter.reserve(16);

				float k = line[1] / line[0];
				float x_1 = line[2];
				float y_1 = line[3];

				std::vector<CCharacter> searchWeakSeedVec;
				searchWeakSeedVec.reserve(16);

				std::vector<CCharacter> searchRightWeakSeed;
				searchRightWeakSeed.reserve(8);
				std::vector<CCharacter> searchLeftWeakSeed;
				searchLeftWeakSeed.reserve(8);

				std::vector<CCharacter> slideRightWindow;
				slideRightWindow.reserve(8);
				std::vector<CCharacter> slideLeftWindow;
				slideLeftWindow.reserve(8);

				// draw weak seed and little seed from line;
				// search for mser rect
				if (1 && showDebug) 
				{
					std::cout << "search for mser rect:" << std::endl;
				}

				if (0 && showDebug) 
				{
					std::stringstream ss(std::stringstream::in | std::stringstream::out);
					ss << "resources/image/tmp/" << img_index << "_1_" << "searcgMserRect.jpg";
					imwrite(ss.str(), result);
				}
				if (1 && showDebug)
				{
					std::cout << "mserCharacter:" << mserCharacter.size() << std::endl;
				}

				//���ǿ���Ӹ����Ѿ����������ַ�������7���������Ѿ�����Ҫ���������������Լ���������
				//������������ǿ������֮������Լ��ֿ�Rect�ߴ��С���ƶ�
				// if the count of strong seed is larger than max count, we dont need 
				// all the next steps, if not, we first need to search the weak seed in 
				// the same line as the strong seed. The judge condition contains the distance 
				// between strong seed and weak seed , and the rect simily of each other to improve
				// the roubustnedd of the seed growing algorithm.
				if (mserCharacter.size() < char_max_count)
				{
					double thresh1 = 0.15;
					double thresh2 = 2.0;
					searchWeakSeed(searchCandidate, searchRightWeakSeed, thresh1, thresh2, line, rightPoint,
						maxrect, plateResult, result, CharSearchDirection::RIGHT);
					if (1 && showDebug) 
					{
						std::cout << "searchRightWeakSeed:" << searchRightWeakSeed.size() << std::endl;
					}
					for (auto seed : searchRightWeakSeed) 
					{
						cv::rectangle(result, seed.getCharacterPos(), Scalar(255, 0, 0), 1);
						mserCharacter.push_back(seed);
					}

					searchWeakSeed(searchCandidate, searchLeftWeakSeed, thresh1, thresh2, line, leftPoint,
						maxrect, plateResult, result, CharSearchDirection::LEFT);
					if (1 && showDebug) 
					{
						std::cout << "searchLeftWeakSeed:" << searchLeftWeakSeed.size() << std::endl;
					}
					for (auto seed : searchLeftWeakSeed)
					{
						cv::rectangle(result, seed.getCharacterPos(), Scalar(255, 0, 0), 1);
						mserCharacter.push_back(seed);
					}
				}

				//��Щ����������Ӵ���ǿ����֮�䣬������ʱ��ǿ����ֻ��ͬһ���ַ�����������
				//�����������Ҫ��һ��������е�ǿ���Ӻ������ӣ���ͬһ���ַ���������������ϲ�Ϊһ������
				//���������󣬲��������ж��Ƿ���Ҫ���û���������ʣ���ַ�������
				// sometimes the weak seed is in the middle of the strong seed.
				// and sometimes two strong seed are actually the two parts of one character.
				// because we only consider the weak seed in the left and right direction of strong seed.
				// now we examine all the strong seed and weak seed. not only to find the seed in the middle,
				// but also to combine two seed which are parts of one character to one seed.
				// only by this process, we could use the seed count as the condition to judge if or not to use slide window.
				float min_thresh = 0.3f;
				float max_thresh = 2.5f;
				reFoundAndCombineRect(mserCharacter, min_thresh, max_thresh, dist, maxrect, result);

				//������Ӻϲ���������Ӹ���С�ڳ����ַ���������Ҫ�û������ҵ�δ���������ַ���
				// if the characters count is less than max count
				// this means the mser rect in the lines are not enough.
				// sometimes there are still some characters could not be captured by mser algorithm,
				// such as blur, low light ,and some chinese characters like zh-cuan.
				// to handle this ,we use a simple slide window method to find them.
				if (mserCharacter.size() < char_max_count) 
				{
					if (1 && showDebug) 
					{
						std::cout << "search chinese:" << std::endl;
						std::cout << "judege the left is chinese:" << std::endl;
					}

					//�������ߵ����Ӿ��Ǻ��֣�����������ֻ��Ҫ��������δ������ֿ飬����������
					// if the left most character is chinese, this means
					// that must be the first character in chinese plate,
					// and we need not to do a slide window to left. So,
					// the first thing is to judge the left charcater is 
					// or not the chinese.
					bool leftIsChinese = false;
					if (1)
					{
						std::sort(mserCharacter.begin(), mserCharacter.end(),
							[](const CCharacter& r1, const CCharacter& r2) 
						{
							return r1.getCharacterPos().tl().x < r2.getCharacterPos().tl().x;
						});

						CCharacter leftChar = mserCharacter[0];

						//Rect theRect = adaptive_charrect_from_rect(leftChar.getCharacterPos(), image.cols, image.rows);
						Rect theRect = leftChar.getCharacterPos();
						//cv::rectangle(result, theRect, Scalar(255, 0, 0), 1);

						Mat region = image(theRect);
						Mat binary_region;

						ostu_level = cv::threshold(region, binary_region, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
						if (1 && showDebug) 
						{
							std::cout << "left : ostu_level:" << ostu_level << std::endl;
						}
						//plate.setOstuLevel(ostu_level);

						Mat charInput = preprocessChar(binary_region, char_size);
						if (0 /*&& showDebug*/) 
						{
							imshow("charInput", charInput);
							waitKey(0);
							destroyWindow("charInput");
						}

						std::string label = "";
						float maxVal = -2.f;
						leftIsChinese = CharsIdentify::instance()->isCharacter(charInput, label, maxVal, true);
						//auto character = CharsIdentify::instance()->identifyChinese(charInput, maxVal, leftIsChinese);
						//label = character.second;
						if (0 /* && showDebug*/) 
						{
							std::cout << "isChinese:" << leftIsChinese << std::endl;
							std::cout << "chinese:" << label;
							std::cout << "__score:" << maxVal << std::endl;
						}
					}

					//�������ߵ��ַ����Ǻ��֣�����Ҫ���û�������������
					//����֮ǰmser���ֵĺ��ֽ���һ������Ӧ�����ɻ������������벻����ɵ�
					//Ŀǰ�޷���֤��������С�Ƿ��뺺�ֽ�һ���ԭ���йأ���ຯ��������������
					// if the left most character is not a chinese,
					// this means we meed to slide a window to find the missed mser rect.
					// search for sliding window
					float ratioWindow = 0.4f;
					//float ratioWindow = CParams::instance()->getParam3f();
					float threshIsCharacter = 0.8f;
					//float threshIsCharacter = CParams::instance()->getParam3f();
					if (!leftIsChinese) 
					{
						slideWindowSearch(image, slideLeftWindow, line, leftPoint, dist, ostu_level, ratioWindow, threshIsCharacter,
							maxrect, plateResult, CharSearchDirection::LEFT, true, result);
						if (1 && showDebug)
						{
							std::cout << "slideLeftWindow:" << slideLeftWindow.size() << std::endl;
						}

						for (auto window : slideLeftWindow) 
						{
							cv::rectangle(result, window.getCharacterPos(), Scalar(0, 0, 255), 1);
							mserCharacter.push_back(window);
						}
					}
				}

				//�����������Ȼ����������Ҫ������������
				// if we still have less than max count characters,
				// we need to slide a window to right to search for the missed mser rect.
				if (mserCharacter.size() < char_max_count) 
				{
					// change ostu_level
					float ratioWindow = 0.4f;
					//float ratioWindow = CParams::instance()->getParam3f();
					float threshIsCharacter = 0.8f;
					//float threshIsCharacter = CParams::instance()->getParam3f();
					slideWindowSearch(image, slideRightWindow, line, rightPoint, dist, plate.getOstuLevel(), ratioWindow, threshIsCharacter,
						maxrect, plateResult, CharSearchDirection::RIGHT, false, result);
					if (1 && showDebug) 
					{
						std::cout << "slideRightWindow:" << slideRightWindow.size() << std::endl;
					}
					for (auto window : slideRightWindow) 
					{
						cv::rectangle(result, window.getCharacterPos(), Scalar(0, 0, 255), 1);
						mserCharacter.push_back(window);
					}
				}

				// computer the plate angle
				float angle = atan(k) * 180 / (float)CV_PI;
				if (1 && showDebug) 
				{
					std::cout << "k:" << k << std::endl;
					std::cout << "angle:" << angle << std::endl;
				}

				// the plateResult rect need to be enlarge to contains all the plate,
				// not only the character area.
				float widthEnlargeRatio = 1.15f;
				float heightEnlargeRatio = 1.25f;
				RotatedRect platePos(Point2f((float)plateResult.x + plateResult.width / 2.f, (float)plateResult.y + plateResult.height / 2.f),
					Size2f(plateResult.width * widthEnlargeRatio, maxrect.height * heightEnlargeRatio), angle);

				// justify the size is likely to be a plate size.
				if (verifyRotatedPlateSizes(platePos,false)) 
				{
					rotatedRectangle(result, platePos, Scalar(0, 0, 255), 1,LINE_8,0);

					plate.setPlatePos(platePos);
					plate.setPlateColor(the_color);
					plate.setPlateLocateType(CMSER);

					if (the_color == BLUE) out_plateVec_blue.push_back(plate);
					if (the_color == YELLOW) out_plateVec_yellow.push_back(plate);
				}

				// use deskew to rotate the image, so we need the binary image.
				if (1) 
				{
					for (auto mserChar : mserCharacter)
					{
						Rect rect = mserChar.getCharacterPos();
						match.at(color_index)(rect) = 255;
					}
					cv::line(match.at(color_index), rightPoint, leftPoint, Scalar(255));
				}
			}

			if (0 /*&& showDebug*/) 
			{
				imshow("result", result);
				waitKey(0);
				destroyWindow("result");
			}

			if (0)
			{
				imshow("match", match.at(color_index));
				waitKey(0);
				destroyWindow("match");
			}

			if (1) 
			{
				std::stringstream ss(std::stringstream::in | std::stringstream::out);
				ss << "resources/image/tmp/plateDetect/plate_" << img_index << "_" << the_color << ".jpg";
				imwrite(ss.str(), result);
			}
		}


	}


	Mat CPlateLocate::scaleImage(const Mat& image, const Size& maxSize, double& scale_ratio) 
	{
		Mat ret;

		if (image.cols > maxSize.width || image.rows > maxSize.height)
		{
			double widthRatio = image.cols / (double)maxSize.width;
			double heightRatio = image.rows / (double)maxSize.height;
			double m_real_to_scaled_ratio = max(widthRatio, heightRatio);

			int newWidth = int(image.cols / m_real_to_scaled_ratio);
			int newHeight = int(image.rows / m_real_to_scaled_ratio);

			cv::resize(image, ret, Size(newWidth, newHeight), 0, 0);
			scale_ratio = m_real_to_scaled_ratio;
		}
		else {
			ret = image;
			scale_ratio = 1.0;
		}

		return ret;
	}


	// Scale back RotatedRect
	RotatedRect scaleBackRRect(const RotatedRect& rr, const float scale_ratio) 
	{
		float width = rr.size.width * scale_ratio;
		float height = rr.size.height * scale_ratio;
		float x = rr.center.x * scale_ratio;
		float y = rr.center.y * scale_ratio;
		RotatedRect mserRect(Point2f(x, y), Size2f(width, height), rr.angle);

		return mserRect;
	}


	bool CPlateLocate::calcSafeRect(const RotatedRect &roi_rect, const Mat &src,Rect_<float> &safeBoundRect) 
	{
		Rect_<float> boudRect = roi_rect.boundingRect();

		float tl_x = boudRect.x > 0 ? boudRect.x : 0;
		float tl_y = boudRect.y > 0 ? boudRect.y : 0;

		float br_x = boudRect.x + boudRect.width < src.cols
			? boudRect.x + boudRect.width - 1
			: src.cols - 1;
		float br_y = boudRect.y + boudRect.height < src.rows
			? boudRect.y + boudRect.height - 1
			: src.rows - 1;

		float roi_width = br_x - tl_x;
		float roi_height = br_y - tl_y;

		if (roi_width <= 0 || roi_height <= 0) return false;

		//  a new rect not out the range of mat

		safeBoundRect = Rect_<float>(tl_x, tl_y, roi_width, roi_height);

		return true;
	}
	

	bool calcSafeRect(const RotatedRect &roi_rect, const int width, const int height,
		Rect_<float> &safeBoundRect) {
		Rect_<float> boudRect = roi_rect.boundingRect();

		float tl_x = boudRect.x > 0 ? boudRect.x : 0;
		float tl_y = boudRect.y > 0 ? boudRect.y : 0;

		float br_x = boudRect.x + boudRect.width < width
			? boudRect.x + boudRect.width - 1
			: width - 1;
		float br_y = boudRect.y + boudRect.height < height
			? boudRect.y + boudRect.height - 1
			: height - 1;

		float roi_width = br_x - tl_x;
		float roi_height = br_y - tl_y;

		if (roi_width <= 0 || roi_height <= 0) return false;

		//  a new rect not out the range of mat

		safeBoundRect = Rect_<float>(tl_x, tl_y, roi_width, roi_height);

		return true;
	}

	


	bool computeIOU(const RotatedRect& rrect1, const RotatedRect& rrect2, const int width, const int height, const float thresh, float& result) 
	{
		Rect_<float> safe_rect1;
		calcSafeRect(rrect1, width, height, safe_rect1);

		Rect_<float> safe_rect2;
		calcSafeRect(rrect2, width, height, safe_rect2);

		Rect inter = interRect(safe_rect1, safe_rect2);
		Rect urect = mergeRect(safe_rect1, safe_rect2);

		float iou = (float)inter.area() / (float)urect.area();

		result = iou;

		if (iou > thresh) 
		{
			return true;
		}

		return false;
	}

	//! MSER plate locate
	//����Դͼsrc,���candPlates
	//�ȵ��ú���mserSearch����Դͼsrc��ͼ�񣨻Ҷȴ����ߴ�任�����룬����mserSearch�����platesVec
	//mserSearch�����ٵ���mserCharMatch����������src������mserCharMatch�����out_plateVec_blue��out_plateVec_yellow
	//����Ϊ��
	//plateMserLocate��src��mserSearch��src��mserCharMatch
	//mserCharMatch���out_plateVec_blue��out_plateVec_yellow��mserSearch�����out_plateVec��plateMserLocate�����candPlates
	int CPlateLocate::plateMserLocate(Mat src, vector<CPlate> &candPlates, int img_index) 
	{
		std::vector<Mat> channelImages;
		std::vector<Color> flags;
		flags.push_back(BLUE);
		flags.push_back(YELLOW);

		bool usePlateMser = true;
		int scale_size = 1000;
		//int scale_size = CParams::instance()->getParam1i();
		double scale_ratio = 1;

		// only conside blue plate
		if (1) 
		{
			Mat grayImage;
			cvtColor(src, grayImage, COLOR_BGR2GRAY);
			channelImages.push_back(grayImage);
			
			//Mat singleChannelImage;
			//extractChannel(src, singleChannelImage, 2);
			//channelImages.push_back(singleChannelImage);
			//flags.push_back(BLUE);

			//channelImages.push_back(255 - grayImage);
			//flags.push_back(YELLOW);
		}

		for (size_t i = 0; i < channelImages.size(); ++i) 
		{
			vector<vector<RotatedRect>> plateRRectsVec;
			vector<vector<CPlate>> platesVec;
			vector<Mat> src_b_vec;
			
			Mat channelImage = channelImages.at(i);
			Mat image = scaleImage(channelImage, Size(scale_size, scale_size), scale_ratio);

			// vector<RotatedRect> rects;
			mserSearch(image, src_b_vec, platesVec, usePlateMser, plateRRectsVec, img_index, false);
			
			for (size_t j = 0; j < flags.size(); j++) 
			{
				vector<CPlate>& plates = platesVec.at(j);
				Mat& src_b = src_b_vec.at(j);
				Color color = flags.at(j);
				
				vector<RotatedRect> rects_mser;
				rects_mser.reserve(64);
				std::vector<CPlate> deskewPlate;
				deskewPlate.reserve(64);
				std::vector<CPlate> mserPlate;
				mserPlate.reserve(64);

				// deskew for rotation and slope image
				for (auto plate : plates) 
				{
					RotatedRect rrect = plate.getPlatePos();
					RotatedRect scaleRect = scaleBackRRect(rrect, (float)scale_ratio);
					plate.setPlatePos(scaleRect);
					plate.setPlateColor(color);

					rects_mser.push_back(scaleRect);
					mserPlate.push_back(plate);
					//all_plates.push_back(plate);
				}

				Mat resize_src_b;
				resize(src_b, resize_src_b, Size(channelImage.cols, channelImage.rows));

				//src_b_vec.push_back(resize_src_b);

				deskew(src, resize_src_b, rects_mser, deskewPlate, false, color);

				for (auto dplate : deskewPlate) 
				{
					RotatedRect drect = dplate.getPlatePos();
					Mat dmat = dplate.getPlateMat();

					for (auto splate : mserPlate) 
					{
						RotatedRect srect = splate.getPlatePos();
						float iou = 0.f;
						bool isSimilar = computeIOU(drect, srect, src.cols, src.rows, 0.95f, iou);
						
						if (isSimilar) 
						{
							splate.setPlateMat(dmat);
							//Ϊ�˱����жϳ����������ַ�����λ�ģ�������Ӷ�λ��������
							splate.setPlateLocateType(lpr::CMSER);
							candPlates.push_back(splate);
							break;
						}
					}
				}
				
			}

		}

		//if (usePlateMser) {
		//  std::vector<RotatedRect> plateRRect_B;
		//  std::vector<RotatedRect> plateRRect_Y;

		//  for (auto rrect : all_plateRRect) {
		//    RotatedRect theRect = scaleBackRRect(rrect, (float)scale_ratio);
		//    //rotatedRectangle(src, theRect, Scalar(255, 0, 0));
		//    for (auto plate : all_plates) {
		//      RotatedRect plateRect = plate.getPlatePos();
		//      //rotatedRectangle(src, plateRect, Scalar(0, 255, 0));
		//      bool isSimilar = computeIOU(theRect, plateRect, src, 0.8f);
		//      if (isSimilar) {
		//        //rotatedRectangle(src, theRect, Scalar(0, 0, 255));
		//        Color color = plate.getPlateColor();
		//        if (color == BLUE) plateRRect_B.push_back(theRect);
		//        if (color == YELLOW) plateRRect_Y.push_back(theRect);
		//      }
		//    }
		//  }

		//  for (size_t i = 0; i < channelImages.size(); ++i) {
		//    Color color = flags.at(i);
		//    Mat resize_src_b = src_b_vec.at(i);

		//    std::vector<CPlate> deskewMserPlate;
		//    if (color == BLUE)
		//      deskew(src, resize_src_b, plateRRect_B, deskewMserPlate, false, color);
		//    if (color == YELLOW)
		//      deskew(src, resize_src_b, plateRRect_Y, deskewMserPlate, false, color);

		//    for (auto plate : deskewMserPlate) {
		//      candPlates.push_back(plate);
		//    }
		//  }
		//}

		
		if (0) 
		{
			imshow("src", src);
			waitKey(0);
			destroyWindow("src");
		}

		return 0;
	}


	//! ��minAreaRect��õ���С��Ӿ��Σ����ݺ�Ƚ����ж�
	bool CPlateLocate::verifySizes(RotatedRect mr)
	{
		float error = m_error;
		//Spain car plate size: 52x11 aspect 4,7272
		//China car plate size: 440mm*140mm��aspect 3.142857

		//Real car plate size: 136 * 32, aspect 4
		float aspect = m_aspect;

		//Set a min and max area. All other patchs are discarded
		//int min= 1*aspect*1; // minimum area
		//int max= 2000*aspect*2000; // maximum area
		int min = 34 * 8 * m_verifyMin; // minimum area
		int max = 34 * 8 * m_verifyMax; // maximum area

		//Get only patchs that match to a respect ratio.
		float rmin = aspect - aspect*error;
		float rmax = aspect + aspect*error;

		int area = mr.size.height * mr.size.width;
		float r = (float)mr.size.width / (float)mr.size.height;
		if (r < 1)
			r = (float)mr.size.height / (float)mr.size.width;

		if ((area < min || area > max) || (r < rmin || r > rmax))
			return false;
		else
			return true;
	}



	//! ��ʾ�������ɵĳ���ͼ�񣬱����ж��Ƿ�ɹ���������ת��
	Mat CPlateLocate::showResultMat(Mat src, Size rect_size, Point2f center, int index)
	{
		Mat img_crop;

		getRectSubPix(src, rect_size, center, img_crop);

		if (m_debug)
		{
			stringstream ss(stringstream::in | stringstream::out);
			ss << "image/tmp/debug_crop_" << index << ".jpg";
			imwrite(ss.str(), img_crop);
		}

		Mat resultResized;
		resultResized.create(HEIGHT, WIDTH, TYPE);

		resize(img_crop, resultResized, resultResized.size(), 0, 0, INTER_CUBIC);

		if (m_debug)
		{
			stringstream ss(stringstream::in | stringstream::out);
			ss << "image/tmp/debug_resize_" << index << ".jpg";
			imwrite(ss.str(), resultResized);
		}

		return resultResized;
	}


	// !����HSV�ռ����ɫ��������

	int CPlateLocate::colorSearch(const Mat& src, const Color r, Mat& out,vector<RotatedRect>& outRects, int index) 
	{
		Mat match_grey;

		// widthֵ�����ս��Ӱ��ܴ󣬿��Կ��ǽ��ж��colorSerch��ÿ�β�ͬ��ֵ
		// ��һ�ֽ�����������ڽ�������SVM֮ǰ����������ǵ��پ���
		const int color_morph_width = 10;
		const int color_morph_height = 2;

		// ������ɫ����
		colorMatch(src, match_grey, r, false);
		//namedWindow("HSV��ɫ�ռ�����", WINDOW_NORMAL);
		//imshow("HSV��ɫ�ռ�����", match_grey);

		/*if (m_debug) {
			utils::imwrite("resources/image/tmp/match_grey.jpg", match_grey);
		}*/

		Mat src_threshold;
		threshold(match_grey, src_threshold, 0, 255,
			CV_THRESH_OTSU + CV_THRESH_BINARY);

		//namedWindow("HSV��ɫ�ռ�������ֵ��", WINDOW_NORMAL);
		//imshow("HSV��ɫ�ռ�������ֵ��", src_threshold);

		Mat element = getStructuringElement(MORPH_RECT, Size(color_morph_width, color_morph_height));
		morphologyEx(src_threshold, src_threshold, MORPH_CLOSE, element);

		//namedWindow("��HSV��ɫ�ռ��������бղ���", WINDOW_NORMAL);
		//imshow("��HSV��ɫ�ռ��������бղ���", src_threshold);


		/*if (m_debug) {
			utils::imwrite("resources/image/tmp/color.jpg", src_threshold);
		}*/

		src_threshold.copyTo(out);

		// ��������
		vector<vector<Point>> contours;

		// ע�⣬findContours��ı�src_threshold
		// ���Ҫ���src_threshold��������֮ǰʹ��copyTo����
		findContours(src_threshold,
			contours,               // a vector of contours
			CV_RETR_EXTERNAL,       // ��ȡ�ⲿ����
			CV_CHAIN_APPROX_NONE);  // all pixels of each contours

		vector<vector<Point>>::iterator itc = contours.begin();
		while (itc != contours.end()) {
			RotatedRect mr = minAreaRect(Mat(*itc));

			// ��Ҫ���д�С�ߴ��ж�
			if (!verifySizes(mr))
				itc = contours.erase(itc);
			else {
				++itc;
				outRects.push_back(mr);
			}
		}

		return 0;
	}


	Mat CPlateLocate::colorMatch(const Mat& src, Mat& match, const Color r,const bool adaptive_minsv) 
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


	
	//! Sobel��һ������
	//! �����ƴ�С����״����ȡ��BoundRect������һ��
	int CPlateLocate::sobelFrtSearch(const Mat& src, vector<Rect_<float>>& outRects)
	{
		Mat src_threshold;
        //Sobel�������õ���ֵͼ��
		sobelOper(src, src_threshold, m_GaussianBlurSize, m_MorphSizeWidth, m_MorphSizeHeight);

		if (0){
			imshow("Sobel��ֱ��Ե���", src_threshold);
			imwrite("sobel", src_threshold);
			waitKey(0);
			destroyWindow("Sobel��ֱ��Ե���");
		}

		vector< vector< Point> > contours;
		findContours(src_threshold,
			contours, // a vector of contours
			CV_RETR_EXTERNAL, // ��ȡ�ⲿ����
			CV_CHAIN_APPROX_NONE); // all pixels of each contours

		vector<vector<Point>>::iterator itc = contours.begin();

		vector<RotatedRect> first_rects;
		/*while (itc != contours.end())
		{
		RotatedRect mr = minAreaRect(Mat(*itc));
		first_rects.push_back(mr);
		++itc;
		}*/

		while (itc != contours.end())
		{
			RotatedRect mr = minAreaRect(Mat(*itc));


			// ��Ҫ���д�С�ߴ��ж�
			if (verifySizes(mr)) {
				first_rects.push_back(mr);

				float area = mr.size.height * mr.size.width;
				float r = (float)mr.size.width / (float)mr.size.height;
				if (r < 1) r = (float)mr.size.height / (float)mr.size.width;

				/*cout << "area:" << area << endl;
				cout << "r:" << r << endl;*/
			}

			++itc;
		}

		for (size_t i = 0; i < first_rects.size(); i++) {
			RotatedRect roi_rect = first_rects[i];

			Rect_<float> safeBoundRect;
			if (!calcSafeRect(roi_rect, src, safeBoundRect)) continue;

			outRects.push_back(safeBoundRect);
		}
		return 0;

	}


	//! Sobel�ڶ�������
	//! �Դ�С����״�����ƣ����ɲο�����

	int CPlateLocate::sobelSecSearch(Mat& bound, Point2f refpoint,vector<RotatedRect>& outRects) 
	{
		Mat bound_threshold;

		//!
		//�ڶ��β�����һ�ξ�ϸ������Ե��ǵõ�����Ӿ���֮���ͼ����sobel�õ���ֵͼ��
		sobelOper(bound, bound_threshold, 3, 10, 3);


		vector<vector<Point>> contours;
		findContours(bound_threshold,
			contours,               // a vector of contours
			CV_RETR_EXTERNAL,       // ��ȡ�ⲿ����
			CV_CHAIN_APPROX_NONE);  // all pixels of each contours

		vector<vector<Point>>::iterator itc = contours.begin();

		vector<RotatedRect> second_rects;
		while (itc != contours.end()) {
			RotatedRect mr = minAreaRect(Mat(*itc));
			second_rects.push_back(mr);
			++itc;
		}

		for (size_t i = 0; i < second_rects.size(); i++) {
			RotatedRect roi = second_rects[i];
			if (verifySizes(roi)) {
				Point2f refcenter = roi.center + refpoint;
				Size2f size = roi.size;
				float angle = roi.angle;

				RotatedRect refroi(refcenter, size, angle);
				outRects.push_back(refroi);
			}
		}

		return 0;
	}

	
	void CPlateLocate::clearMaoDingOnly(Mat& img) 
	{
		const int x = 7;
		Mat jump = Mat::zeros(1, img.rows, CV_32F);
		for (int i = 0; i < img.rows; i++) {
			int jumpCount = 0;
			int whiteCount = 0;
			for (int j = 0; j < img.cols - 1; j++) {
				if (img.at<char>(i, j) != img.at<char>(i, j + 1)) jumpCount++;

				if (img.at<uchar>(i, j) == 255) {
					whiteCount++;
				}
			}

			jump.at<float>(i) = (float)jumpCount;
		}

		for (int i = 0; i < img.rows; i++) {
			if (jump.at<float>(i) <= x) {
				for (int j = 0; j < img.cols; j++) {
					img.at<char>(i, j) = 0;
				}
			}
		}
	}

	bool CPlateLocate::bFindLeftRightBound(Mat& bound_threshold, int& posLeft, int& posRight) {
		//������Ѱ�ұ߽�
		float span = bound_threshold.rows * 0.2f;
		//��߽���
		for (int i = 0; i < bound_threshold.cols - span - 1; i += 2) {
			int whiteCount = 0;
			for (int k = 0; k < bound_threshold.rows; k++) {
				for (int l = i; l < i + span; l++) {
					if (bound_threshold.data[k * bound_threshold.step[0] + l] == 255) {
						whiteCount++;
					}
				}
			}
			if (whiteCount * 1.0 / (span * bound_threshold.rows) > 0.36) {
				posLeft = i;
				break;
			}
		}
		span = bound_threshold.rows * 0.2f;
		//�ұ߽���
		for (int i = bound_threshold.cols - 1; i > span; i -= 2) {
			int whiteCount = 0;
			for (int k = 0; k < bound_threshold.rows; k++) {
				for (int l = i; l > i - span; l--) {
					if (bound_threshold.data[k * bound_threshold.step[0] + l] == 255) {
						whiteCount++;
					}
				}
			}

			if (whiteCount * 1.0 / (span * bound_threshold.rows) > 0.26) {
				posRight = i;
				break;
			}
		}

		if (posLeft < posRight) {
			return true;
		}
		return false;
	}



	//! Sobel�ڶ�������,�Զ��ѵĲ��ֽ����ٴεĴ���
	//! �Դ�С����״�����ƣ����ɲο�����
	int CPlateLocate::sobelSecSearchPart(Mat& bound, Point2f refpoint,vector<RotatedRect>& outRects)
	{
		Mat bound_threshold;

		////!
		///�ڶ��β�����һ�ξ�ϸ������Ե��ǵõ�����Ӿ���֮���ͼ����sobel�õ���ֵͼ��
		sobelOperT(bound, bound_threshold, 3, 6, 2);

		////��ֵ��ȥ�����ߵı߽�

		// Mat mat_gray;
		// cvtColor(bound,mat_gray,CV_BGR2GRAY);

		// bound_threshold = mat_gray.clone();
		////threshold(input_grey, img_threshold, 5, 255, CV_THRESH_OTSU +
		/// CV_THRESH_BINARY);
		// int w = mat_gray.cols;
		// int h = mat_gray.rows;
		// Mat tmp = mat_gray(Rect(w*0.15,h*0.2,w*0.6,h*0.6));
		// int threadHoldV = ThresholdOtsu(tmp);
		// threshold(mat_gray, bound_threshold,threadHoldV, 255, CV_THRESH_BINARY);

		Mat tempBoundThread = bound_threshold.clone();
		////
		clearMaoDingOnly(tempBoundThread);

		int posLeft = 0, posRight = 0;
		if (bFindLeftRightBound(tempBoundThread, posLeft, posRight)) {
			//�ҵ������߽����������޲�����
			if (posRight != 0 && posLeft != 0 && posLeft < posRight) {
				int posY = int(bound_threshold.rows * 0.5);
				for (int i = posLeft + (int)(bound_threshold.rows * 0.1);
					i < posRight - 4; i++) {
					bound_threshold.data[posY * bound_threshold.cols + i] = 255;
				}
			}

			//utils::imwrite("resources/image/tmp/repaireimg1.jpg", bound_threshold);

			//���ߵ�����Ҫ
			for (int i = 0; i < bound_threshold.rows; i++) {
				bound_threshold.data[i * bound_threshold.cols + posLeft] = 0;
				bound_threshold.data[i * bound_threshold.cols + posRight] = 0;
			}
			//utils::imwrite("resources/image/tmp/repaireimg2.jpg", bound_threshold);
		}

		vector<vector<Point>> contours;
		findContours(bound_threshold,
			contours,               // a vector of contours
			CV_RETR_EXTERNAL,       // ��ȡ�ⲿ����
			CV_CHAIN_APPROX_NONE);  // all pixels of each contours

		vector<vector<Point>>::iterator itc = contours.begin();

		vector<RotatedRect> second_rects;
		while (itc != contours.end()) {
			RotatedRect mr = minAreaRect(Mat(*itc));
			second_rects.push_back(mr);
			++itc;
		}

		for (size_t i = 0; i < second_rects.size(); i++) {
			RotatedRect roi = second_rects[i];
			if (verifySizes(roi)) {
				Point2f refcenter = roi.center + refpoint;
				Size2f size = roi.size;
				float angle = roi.angle;

				RotatedRect refroi(refcenter, size, angle);
				outRects.push_back(refroi);
			}
		}

		return 0;
	}



	//! Sobel����//��ͼ��ָ��ʴ�����͵Ĳ���
	//! �����ɫͼ�������ֵ��ͼ��

int CPlateLocate::sobelOper(const Mat& in, Mat& out, int blurSize, int morphW,int morphH)
{
	Mat mat_blur;
	mat_blur = in.clone();
	GaussianBlur(in, mat_blur, Size(blurSize, blurSize), 0, 0, BORDER_DEFAULT);

	Mat mat_gray;
	if (mat_blur.channels() == 3)
		cvtColor(mat_blur, mat_gray, CV_RGB2GRAY);
	else
		mat_gray = mat_blur;

	// equalizeHist(mat_gray, mat_gray);

	int scale = SOBEL_SCALE;
	int delta = SOBEL_DELTA;
	int ddepth = SOBEL_DDEPTH;

	Mat grad_x, grad_y;
	Mat abs_grad_x, abs_grad_y;
	// ��X  soble
	Sobel(mat_gray, grad_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT);
	convertScaleAbs(grad_x, abs_grad_x);
	// ��Y  soble
	// Sobel(mat_gray, grad_y, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT);
	// convertScaleAbs(grad_y, abs_grad_y);
	// ������Ȩֵ���
	// ��ΪY�����Ȩ����0������ڴ˾Ͳ��ټ���Y�����sobel��
	Mat grad;
	addWeighted(abs_grad_x, SOBEL_X_WEIGHT, 0, 0, 0, grad);
	//imshow("Sobel��ֱ��Ե���", abs_grad_x);
	//waitKey(100000);
	//imwrite("sobel", abs_grad_x);
	/*Mat element_sobel = getStructuringElement(MORPH_RECT, Size(27, 3));
	morphologyEx(abs_grad_x, abs_grad_x, MORPH_CLOSE, element_sobel);
	cout << "sobel�ղ���" << endl;
	imshow("Sobel�ղ���", abs_grad_x);
	waitKey(0);*/

	// �ָ�
	Mat mat_threshold;
	double otsu_thresh_val =
		threshold(grad, mat_threshold, 0, 255, CV_THRESH_OTSU + CV_THRESH_BINARY);
	// ��ʴ������
	Mat element = getStructuringElement(MORPH_RECT, Size(morphW, morphH));
	morphologyEx(mat_threshold, mat_threshold, MORPH_CLOSE, element);

	out = mat_threshold;

	if (0) {
		imshow("sobelOper", out);
		waitKey(0);
		destroyWindow("sobelOper");
	}

	return 0;
}

 
//!�ж�һ�����Ƶ���ɫ
//! ���복��mat����ɫģ��
//! ����true��fasle
bool CPlateLocate::plateColorJudge(const Mat& src, const Color r, const bool adaptive_minsv,float& percent) 
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



//getPlateType
//�жϳ��Ƶ�����
lpr::Color CPlateLocate::getPlateType(const Mat& src, const bool adaptive_minsv)
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


int CPlateLocate::ThresholdOtsu(Mat mat)
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


void CPlateLocate::clearMaoDing(Mat mask, int& top, int& bottom) {
	const int x = 7;

	for (int i = 0; i < mask.rows / 2; i++) {
		int whiteCount = 0;
		int jumpCount = 0;
		for (int j = 0; j < mask.cols - 1; j++) {
			if (mask.at<char>(i, j) != mask.at<char>(i, j + 1)) jumpCount++;

			if ((int)mask.at<uchar>(i, j) == 255) {
				whiteCount++;
			}
		}
		if ((jumpCount < x && whiteCount * 1.0 / mask.cols > 0.15) ||
			whiteCount < 4) {
			top = i;
		}
	}
	top -= 1;
	if (top < 0) {
		top = 0;
	}

	// ok,�ҵ����±߽�
	for (int i = mask.rows - 1; i >= mask.rows / 2; i--) {
		int jumpCount = 0;
		int whiteCount = 0;
		for (int j = 0; j < mask.cols - 1; j++) {
			if (mask.at<char>(i, j) != mask.at<char>(i, j + 1)) jumpCount++;
			if (mask.at<uchar>(i, j) == 255) {
				whiteCount++;
			}
		}
		if ((jumpCount < x && whiteCount * 1.0 / mask.cols > 0.15) ||
			whiteCount < 4) {
			bottom = i;
		}
	}
	bottom += 1;
	if (bottom >= mask.rows) {
		bottom = mask.rows - 1;
	}

	if (top >= bottom) {
		top = 0;
		bottom = mask.rows - 1;
	}
}


bool CPlateLocate::bFindLeftRightBound1(Mat& bound_threshold, int& posLeft, int& posRight) {
	//������Ѱ�ұ߽�
	float span = bound_threshold.rows * 0.2f;
	//��߽���
	for (int i = 0; i < bound_threshold.cols - span - 1; i += 3) {
		int whiteCount = 0;
		for (int k = 0; k < bound_threshold.rows; k++) {
			for (int l = i; l < i + span; l++) {
				if (bound_threshold.data[k * bound_threshold.step[0] + l] == 255) {
					whiteCount++;
				}
			}
		}
		if (whiteCount * 1.0 / (span * bound_threshold.rows) > 0.15) {
			posLeft = i;
			break;
		}
	}
	span = bound_threshold.rows * 0.2f;
	//�ұ߽���
	for (int i = bound_threshold.cols - 1; i > span; i -= 2) {
		int whiteCount = 0;
		for (int k = 0; k < bound_threshold.rows; k++) {
			for (int l = i; l > i - span; l--) {
				if (bound_threshold.data[k * bound_threshold.step[0] + l] == 255) {
					whiteCount++;
				}
			}
		}

		if (whiteCount * 1.0 / (span * bound_threshold.rows) > 0.06) {
			posRight = i;
			if (posRight + 5 < bound_threshold.cols) {
				posRight = posRight + 5;
			}
			else {
				posRight = bound_threshold.cols - 1;
			}

			break;
		}
	}

	if (posLeft < posRight) {
		return true;
	}
	return false;
}

void CPlateLocate::DeleteNotArea(Mat& inmat) {
	Mat input_grey;
	cvtColor(inmat, input_grey, CV_BGR2GRAY);

	int w = inmat.cols;
	int h = inmat.rows;

	Mat tmpMat = inmat(Rect_<double>(w * 0.15, h * 0.1, w * 0.7, h * 0.7));
	//�жϳ�����ɫ�Դ�ȷ��threshold����
	Color plateType = getPlateType(tmpMat, true);
	Mat img_threshold;
	if (BLUE == plateType) {
		img_threshold = input_grey.clone();
		Mat tmp = input_grey(Rect_<double>(w * 0.15, h * 0.15, w * 0.7, h * 0.7));
		int threadHoldV = ThresholdOtsu(tmp);

		threshold(input_grey, img_threshold, threadHoldV, 255, CV_THRESH_BINARY);
		// threshold(input_grey, img_threshold, 5, 255, CV_THRESH_OTSU +
		// CV_THRESH_BINARY);

		//utils::imwrite("resources/image/tmp/inputgray2.jpg", img_threshold);

	}
	else if (YELLOW == plateType) {
		img_threshold = input_grey.clone();
		Mat tmp = input_grey(Rect_<double>(w * 0.1, h * 0.1, w * 0.8, h * 0.8));
		int threadHoldV = ThresholdOtsu(tmp);

		threshold(input_grey, img_threshold, threadHoldV, 255,
			CV_THRESH_BINARY_INV);

		//utils::imwrite("resources/image/tmp/inputgray2.jpg", img_threshold);

		// threshold(input_grey, img_threshold, 10, 255, CV_THRESH_OTSU +
		// CV_THRESH_BINARY_INV);
	}
	else
		threshold(input_grey, img_threshold, 10, 255,
		CV_THRESH_OTSU + CV_THRESH_BINARY);

	int posLeft = 0;
	int posRight = 0;

	int top = 0;
	int bottom = img_threshold.rows - 1;
	clearMaoDing(img_threshold, top, bottom);
	if (bFindLeftRightBound1(img_threshold, posLeft, posRight)) {
		inmat = inmat(Rect(posLeft, top, w - posLeft, bottom - top));

		/*int posY = inmat.rows*0.5*inmat.cols;
		for (int i=posLeft;i<posRight;i++)
		{

		inmat.data[posY+i] = 255;
		}
		*/
		/*roiRect.x += posLeft;
		roiRect.width -=posLeft;*/
	}
}


void CPlateLocate::deleteNotArea(Mat &inmat, Color color )
{
	Mat input_grey;
	cvtColor(inmat, input_grey, CV_BGR2GRAY);

	int w = inmat.cols;
	int h = inmat.rows;

	Mat tmpMat = inmat(Rect_<double>(w * 0.15, h * 0.1, w * 0.7, h * 0.7));

	Color plateType;
	if (UNKNOWN == color) {
		plateType = getPlateType(tmpMat, true);
	}
	else {
		plateType = color;
	}

	Mat img_threshold;

	if (BLUE == plateType) {
		img_threshold = input_grey.clone();
		Mat tmp = input_grey(Rect_<double>(w * 0.15, h * 0.15, w * 0.7, h * 0.7));
		int threadHoldV = ThresholdOtsu(tmp);

		threshold(input_grey, img_threshold, threadHoldV, 255, CV_THRESH_BINARY);
		// threshold(input_grey, img_threshold, 5, 255, CV_THRESH_OTSU +
		// CV_THRESH_BINARY);

		//utils::imwrite("resources/image/tmp/inputgray2.jpg", img_threshold);

	}
	else if (YELLOW == plateType) {
		img_threshold = input_grey.clone();
		Mat tmp = input_grey(Rect_<double>(w * 0.1, h * 0.1, w * 0.8, h * 0.8));
		int threadHoldV = ThresholdOtsu(tmp);

		threshold(input_grey, img_threshold, threadHoldV, 255,
			CV_THRESH_BINARY_INV);

		//utils::imwrite("resources/image/tmp/inputgray2.jpg", img_threshold);

		// threshold(input_grey, img_threshold, 10, 255, CV_THRESH_OTSU +
		// CV_THRESH_BINARY_INV);
	}
	else
		threshold(input_grey, img_threshold, 10, 255,
		CV_THRESH_OTSU + CV_THRESH_BINARY);

	//img_threshold = input_grey.clone();
	//spatial_ostu(img_threshold, 8, 2, plateType);

	int posLeft = 0;
	int posRight = 0;

	int top = 0;
	int bottom = img_threshold.rows - 1;
	clearMaoDing(img_threshold, top, bottom);

	if (0) {
		imshow("inmat", inmat);
		waitKey(0);
		destroyWindow("inmat");
	}

	if (bFindLeftRightBound1(img_threshold, posLeft, posRight)) {
		inmat = inmat(Rect(posLeft, top, w - posLeft, bottom - top));
		if (0) {
			imshow("inmat", inmat);
			waitKey(0);
			destroyWindow("inmat");
		}
	}
}


	//! ��Ťб����

int CPlateLocate::deskew(const Mat &src, const Mat &src_b,vector<RotatedRect> &inRects,vector<CPlate> &outPlates, bool useDeteleArea, Color color) 
{
	Mat mat_debug;
	src.copyTo(mat_debug);

	for (size_t i = 0; i < inRects.size(); i++) 
	{
		RotatedRect roi_rect = inRects[i];

		

		float r = (float)roi_rect.size.width / (float)roi_rect.size.height;
		float roi_angle = roi_rect.angle;

		Size roi_rect_size = roi_rect.size;
		if (r < 1) 
		{
			roi_angle = 90 + roi_angle;
			swap(roi_rect_size.width, roi_rect_size.height);
		}

		if (m_debug) 
		{
			Point2f rect_points[4];
			roi_rect.points(rect_points);
			for (int j = 0; j < 4; j++)
				line(mat_debug, rect_points[j], rect_points[(j + 1) % 4],
				Scalar(0, 255, 255), 1, 8);
		}

		// changed
		// rotation = 90 - abs(roi_angle);
		// rotation < m_angel;

		// m_angle=60
		if (roi_angle - m_angle < 0 && roi_angle + m_angle > 0) 
		{
			Rect_<float> safeBoundRect;
			bool isFormRect = calcSafeRect(roi_rect, src, safeBoundRect);
			if (!isFormRect) continue;

			Mat bound_mat = src(safeBoundRect);

			/*namedWindow("��б����ͼ��", WINDOW_NORMAL);
			imshow("��б����ͼ��", bound_mat);
			waitKey(0);*/

			Mat bound_mat_b = src_b(safeBoundRect);

			if (0) 
			{
				imshow("bound_mat_b", bound_mat_b);
				waitKey(0);
				destroyWindow("bound_mat_b");
			}

			//std::cout << "roi_angle:" << roi_angle << std::endl;

			Point2f roi_ref_center = roi_rect.center - safeBoundRect.tl();

			Mat deskew_mat;
			if ((roi_angle - 5 < 0 && roi_angle + 5 > 0) || 90.0 == roi_angle ||-90.0 == roi_angle) 
			{
				
				deskew_mat = bound_mat;
			}
			else 
			{

				
				Mat rotated_mat;
				Mat rotated_mat_b;

				if (!rotation(bound_mat, rotated_mat, roi_rect_size, roi_ref_center,
					roi_angle))
					continue;

				if (!rotation(bound_mat_b, rotated_mat_b, roi_rect_size, roi_ref_center,
					roi_angle))
					continue;

				// we need affine for rotatioed image

				double roi_slope = 0;
				// imshow("1roated_mat",rotated_mat);
				// imshow("rotated_mat_b",rotated_mat_b);
				if (isdeflection(rotated_mat_b, roi_angle, roi_slope)) {
					affine(rotated_mat, deskew_mat, roi_slope);
				}
				else
					deskew_mat = rotated_mat;
			}
			
			

			Mat plate_mat;
			plate_mat.create(HEIGHT, WIDTH, TYPE);

			// haitungaga add��affect 25% to full recognition.
			if (useDeteleArea)
				deleteNotArea(deskew_mat, color);


			if (deskew_mat.cols * 1.0 / deskew_mat.rows > 2.3 &&
				deskew_mat.cols * 1.0 / deskew_mat.rows < 6) 
			{

				if (deskew_mat.cols >= WIDTH || deskew_mat.rows >= HEIGHT)
					resize(deskew_mat, plate_mat, plate_mat.size(), 0, 0, INTER_AREA);
				else
					resize(deskew_mat, plate_mat, plate_mat.size(), 0, 0, INTER_CUBIC);

				CPlate plate;
				plate.setPlatePos(roi_rect);
				plate.setPlateMat(plate_mat);
				if (color != UNKNOWN) plate.setPlateColor(color);

				outPlates.push_back(plate);
				
				//namedWindow("��������ͼ��", WINDOW_NORMAL);
				//imshow("��������ͼ��", plate.getPlateMat());
			}
		}
	}

	return 0;
}



	//! ��ת����

bool CPlateLocate::rotation(Mat& in, Mat& out, const Size rect_size,
	const Point2f center, const double angle) {
	Mat in_large;
	in_large.create(int(in.rows * 1.5), int(in.cols * 1.5), in.type());

	float x = in_large.cols / 2 - center.x > 0 ? in_large.cols / 2 - center.x : 0;
	float y = in_large.rows / 2 - center.y > 0 ? in_large.rows / 2 - center.y : 0;

	float width = x + in.cols < in_large.cols ? in.cols : in_large.cols - x;
	float height = y + in.rows < in_large.rows ? in.rows : in_large.rows - y;

	/*assert(width == in.cols);
	assert(height == in.rows);*/

	if (width != in.cols || height != in.rows) return false;

	Mat imageRoi = in_large(Rect_<float>(x, y, width, height));
	addWeighted(imageRoi, 0, in, 1, 0, imageRoi);

	Point2f center_diff(in.cols / 2.f, in.rows / 2.f);
	Point2f new_center(in_large.cols / 2.f, in_large.rows / 2.f);

	Mat rot_mat = getRotationMatrix2D(new_center, angle, 1);

	/*imshow("in_copy", in_large);
	waitKey(0);*/

	Mat mat_rotated;
	warpAffine(in_large, mat_rotated, rot_mat, Size(in_large.cols, in_large.rows),
		CV_INTER_CUBIC);

	/*imshow("mat_rotated", mat_rotated);
	waitKey(0);*/

	Mat img_crop;
	getRectSubPix(mat_rotated, Size(rect_size.width, rect_size.height),
		new_center, img_crop);

	out = img_crop;

	/*imshow("img_crop", img_crop);
	waitKey(0);*/

	return true;
}

   

	//! �Ƿ�ƫб
	//! �����ֵ��ͼ������жϽ��
	bool CPlateLocate::isdeflection(const Mat& in, const double angle, double& slope)
	{
		int nRows = in.rows;
		int nCols = in.cols;

		assert(in.channels() == 1);

		int comp_index[3];
		int len[3];

		comp_index[0] = nRows / 4;
		comp_index[1] = nRows / 4 * 2;
		comp_index[2] = nRows / 4 * 3;

		const uchar* p;

		for (int i = 0; i < 3; i++)
		{
			int index = comp_index[i];
			p = in.ptr<uchar>(index);

			int j = 0;
			int value = 0;
			while (0 == value && j < nCols)
				value = int(p[j++]);

			len[i] = j;
		}

		//cout << "len[0]:" << len[0] << endl;
		//cout << "len[1]:" << len[1] << endl;
		//cout << "len[2]:" << len[2] << endl;

		double maxlen = max(len[2], len[0]);
		double minlen = min(len[2], len[0]);
		double difflen = abs(len[2] - len[0]);
		//cout << "nCols:" << nCols << endl;

		double PI = 3.14159265;
		double g = tan(angle * PI / 180.0);

		if (maxlen - len[1] > nCols / 32 || len[1] - minlen > nCols / 32) {
			// ���б��Ϊ������ײ����£���֮����
			double slope_can_1 = double(len[2] - len[0]) / double(comp_index[1]);
			double slope_can_2 = double(len[1] - len[0]) / double(comp_index[0]);
			double slope_can_3 = double(len[2] - len[1]) / double(comp_index[0]);

			/*cout << "slope_can_1:" << slope_can_1 << endl;
			cout << "slope_can_2:" << slope_can_2 << endl;
			cout << "slope_can_3:" << slope_can_3 << endl;*/

			slope = abs(slope_can_1 - g) <= abs(slope_can_2 - g) ? slope_can_1 : slope_can_2;

			/*slope = max(  double(len[2] - len[0]) / double(comp_index[1]),
			double(len[1] - len[0]) / double(comp_index[0]));*/

			//cout << "slope:" << slope << endl;
			return true;
		}
		else {
			slope = 0;
		}

		return false;
	}


	//! Ť�����

	void CPlateLocate::affine(const Mat& in, Mat& out, const double slope) {
		// imshow("in", in);
		// waitKey(0);
		//�����slope��ͨ���ж��Ƿ���б�ó�������б��
		Point2f dstTri[3];
		Point2f plTri[3];

		float height = (float)in.rows;  //��
		float width = (float)in.cols;   //��
		float xiff = (float)abs(slope) * height;

		if (slope > 0) {
			//��ƫ�ͣ����������ϵ��xiff/2λ��
			plTri[0] = Point2f(0, 0);
			plTri[1] = Point2f(width - xiff - 1, 0);
			plTri[2] = Point2f(0 + xiff, height - 1);

			dstTri[0] = Point2f(xiff / 2, 0);
			dstTri[1] = Point2f(width - 1 - xiff / 2, 0);
			dstTri[2] = Point2f(xiff / 2, height - 1);
		}
		else {
			//��ƫ�ͣ����������ϵ�� -xiff/2λ��
			plTri[0] = Point2f(0 + xiff, 0);
			plTri[1] = Point2f(width - 1, 0);
			plTri[2] = Point2f(0, height - 1);

			dstTri[0] = Point2f(xiff / 2, 0);
			dstTri[1] = Point2f(width - 1 - xiff + xiff / 2, 0);
			dstTri[2] = Point2f(xiff / 2, height - 1);
		}

		Mat warp_mat = getAffineTransform(plTri, dstTri);

		Mat affine_mat;
		affine_mat.create((int)height, (int)width, TYPE);

		if (in.rows > HEIGHT || in.cols > WIDTH)
			warpAffine(in, affine_mat, warp_mat, affine_mat.size(),
			CV_INTER_AREA);  //����任
		else
			warpAffine(in, affine_mat, warp_mat, affine_mat.size(), CV_INTER_CUBIC);

		out = affine_mat;

		// imshow("out", out);
		// waitKey(0);
	}



	

	// !������ɫ��Ϣ�ĳ��ƶ�λ

int CPlateLocate::plateColorLocate(Mat src, vector<CPlate>& candPlates,
	int index) 
{
	vector<RotatedRect> rects_color_blue;
	vector<RotatedRect> rects_color_yellow;
	vector<CPlate> plates;
	Mat src_b;

	// ������ɫ����
	// ������ɫƥ�䳵��
	colorSearch(src, BLUE, src_b, rects_color_blue, index);
	
	// ���п�Ťб����
	deskew(src, src_b, rects_color_blue, plates);

	// ���һ�ɫ����
	colorSearch(src, YELLOW, src_b, rects_color_yellow, index);
	deskew(src, src_b, rects_color_yellow, plates);

	for (size_t i = 0; i < plates.size(); i++) 
	{
		//Ϊ�˱����жϳ����������ַ�����λ�ģ�������Ӷ�λ��������
		plates[i].setPlateLocateType(lpr::COLOR);
		candPlates.push_back(plates[i]);
	}
	return 0;
}


//! Sobel����
//! �����ɫͼ�������ֵ��ͼ��
int CPlateLocate::sobelOperT(const Mat& in, Mat& out, int blurSize, int morphW,
	int morphH) {
	Mat mat_blur;
	mat_blur = in.clone();
	GaussianBlur(in, mat_blur, Size(blurSize, blurSize), 0, 0, BORDER_DEFAULT);

	Mat mat_gray;
	if (mat_blur.channels() == 3)
		cvtColor(mat_blur, mat_gray, CV_BGR2GRAY);
	else
		mat_gray = mat_blur;

	//utils::imwrite("resources/image/tmp/grayblure.jpg", mat_gray);

	// equalizeHist(mat_gray, mat_gray);

	int scale = SOBEL_SCALE;
	int delta = SOBEL_DELTA;
	int ddepth = SOBEL_DDEPTH;

	Mat grad_x, grad_y;
	Mat abs_grad_x, abs_grad_y;

	Sobel(mat_gray, grad_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT);
	convertScaleAbs(grad_x, abs_grad_x);

	//��ΪY�����Ȩ����0������ڴ˾Ͳ��ټ���Y�����sobel��
	Mat grad;
	addWeighted(abs_grad_x, 1, 0, 0, 0, grad);

	//utils::imwrite("resources/image/tmp/graygrad.jpg", grad);

	Mat mat_threshold;
	double otsu_thresh_val =
		threshold(grad, mat_threshold, 0, 255, CV_THRESH_OTSU + CV_THRESH_BINARY);

	//utils::imwrite("resources/image/tmp/grayBINARY.jpg", mat_threshold);

	Mat element = getStructuringElement(MORPH_RECT, Size(morphW, morphH));
	morphologyEx(mat_threshold, mat_threshold, MORPH_CLOSE, element);

	//utils::imwrite("resources/image/tmp/phologyEx.jpg", mat_threshold);

	out = mat_threshold;

	return 0;
}



int CPlateLocate::plateSobelLocate(Mat src, vector<CPlate>& candPlates,int index) 
{
	vector<RotatedRect> rects_sobel;
	vector<RotatedRect> rects_sobel_sel;
	vector<CPlate> plates;

	vector<Rect_<float>> bound_rects;

	// Sobel��һ�δ�������
	sobelFrtSearch(src, bound_rects);

	vector<Rect_<float>> bound_rects_part;

	//�Բ�����Ҫ������������չ
	for (size_t i = 0; i < bound_rects.size(); i++) {
		float fRatio = bound_rects[i].width * 1.0f / bound_rects[i].height;
		if (fRatio < 3.0 && fRatio > 1.0 && bound_rects[i].height < 120) {
			Rect_<float> itemRect = bound_rects[i];
			//��ȹ�С��������չ
			itemRect.x = itemRect.x - itemRect.height * (4 - fRatio);
			if (itemRect.x < 0) {
				itemRect.x = 0;
			}
			itemRect.width = itemRect.width + itemRect.height * 2 * (4 - fRatio);
			if (itemRect.width + itemRect.x >= src.cols) {
				itemRect.width = src.cols - itemRect.x;
			}

			itemRect.y = itemRect.y - itemRect.height * 0.08f;
			itemRect.height = itemRect.height * 1.16f;

			bound_rects_part.push_back(itemRect);
		}
	}
	//�Զ��ѵĲ��ֽ��ж��δ���
	for (size_t i = 0; i < bound_rects_part.size(); i++) {
		Rect_<float> bound_rect = bound_rects_part[i];
		Point2f refpoint(bound_rect.x, bound_rect.y);

		float x = bound_rect.x > 0 ? bound_rect.x : 0;
		float y = bound_rect.y > 0 ? bound_rect.y : 0;

		float width =
			x + bound_rect.width < src.cols ? bound_rect.width : src.cols - x;
		float height =
			y + bound_rect.height < src.rows ? bound_rect.height : src.rows - y;

		Rect_<float> safe_bound_rect(x, y, width, height);
		Mat bound_mat = src(safe_bound_rect);

		// Sobel�ڶ��ξ�ϸ����(����)
		sobelSecSearchPart(bound_mat, refpoint, rects_sobel);
	}

	for (size_t i = 0; i < bound_rects.size(); i++) {
		Rect_<float> bound_rect = bound_rects[i];
		Point2f refpoint(bound_rect.x, bound_rect.y);

		float x = bound_rect.x > 0 ? bound_rect.x : 0;
		float y = bound_rect.y > 0 ? bound_rect.y : 0;

		float width =
			x + bound_rect.width < src.cols ? bound_rect.width : src.cols - x;
		float height =
			y + bound_rect.height < src.rows ? bound_rect.height : src.rows - y;

		Rect_<float> safe_bound_rect(x, y, width, height);
		Mat bound_mat = src(safe_bound_rect);

		// Sobel�ڶ��ξ�ϸ����
		sobelSecSearch(bound_mat, refpoint, rects_sobel);
		// sobelSecSearchPart(bound_mat, refpoint, rects_sobel);
	}

	Mat src_b;
	sobelOper(src, src_b, 3, 10, 3);

	// ���п�Ťб����
	deskew(src, src_b, rects_sobel, plates);

	for (size_t i = 0; i < plates.size(); i++)
	{
		//Ϊ�˱����жϳ����������ַ�����λ�ģ�������Ӷ�λ��������
		plates[i].setPlateLocateType(lpr::SOBEL);
		candPlates.push_back(plates[i]);
	}

	return 0;
}



//1.5�棬����mser����
int CPlateLocate::plateLocate(Mat src, vector<Mat> &resultVec, int index) 
{
	vector<CPlate> all_result_Plates;

	plateColorLocate(src, all_result_Plates, index);
	plateSobelLocate(src, all_result_Plates, index);
	plateMserLocate(src, all_result_Plates, index);

	for (size_t i = 0; i < all_result_Plates.size(); i++) 
	{
		
		CPlate plate = all_result_Plates[i];
		resultVec.push_back(plate.getPlateMat());
	}

	return 0;
}


}