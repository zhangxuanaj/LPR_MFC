
#ifndef __CHARS_SEGMENT_H__
#define __CHARS_SEGMENT_H__

#include "prep.h"
#include "plate.h"

using namespace cv;


namespace lpr {
	

class CCharsSegment
	{
	public:
		CCharsSegment();

		
		// find the best chinese binaranzation method
		void judgeChinese(Mat in, Mat& out, Color plateType);

		
		bool slideChineseWindow(Mat& image, Rect mr, Mat& newRoi, Color plateType, float slideLengthRatio, bool useAdapThreshold);

		
		Mat preprocessChar(Mat in, int char_size);

		// non-maximum suppression
		void NMStoCharacter(std::vector<CCharacter> &inVec, double overlap);

		float computeIOU(const Rect& rect1, const Rect& rect2);

		Rect interRect(const Rect& a, const Rect& b);

		
		Rect mergeRect(const Rect& a, const Rect& b);

		// ostu region
		void spatial_ostu(InputArray _src, int grid_x, int grid_y, Color type = BLUE);

		//! 获得车牌颜色
		Color getPlateType(const Mat& src, const bool adaptive_minsv);

		//! 判断一个车牌的颜色
		//! 输入车牌mat与颜色模板
		//! 返回true或fasle
		bool plateColorJudge(const Mat& src, const Color r, const bool adaptive_minsv,float& percent);

		


		Mat colorMatch(const Mat& src, Mat& match, const Color r,const bool adaptive_minsv);

		int ThresholdOtsu(Mat mat);

		//! 字符分割

		int charsSegment(Mat input, std::vector<Mat>& resultVec, Color color = BLUE);
		

		//! 字符尺寸验证
		bool verifySizes(Mat r);

		//! 字符预处理
		Mat preprocessChar(Mat in);

		//! 直方图均衡，为判断车牌颜色做准备
		//Mat histeq(Mat in);

		bool clearMaoDing(Mat& img);

		//! 根据特殊车牌来构造猜测中文字符的位置和大小
		Rect GetChineseRect(const Rect rectSpe);

		//! 找出指示城市的字符的Rect，例如苏A7003X，就是A的位置
		int GetSpecificRect(const vector<Rect>& vecRect);

		//! 这个函数做两个事情
		//  1.把特殊字符Rect左边的全部Rect去掉，后面再重建中文字符的位置。
		//  2.从特殊字符Rect开始，依次选择6个Rect，多余的舍去。
		int RebuildRect(const vector<Rect>& vecRect, vector<Rect>& outRect, int specIndex);

		//! 将Rect按位置从左到右进行排序
		//int SortRect(const vector<Rect>& vecRect, vector<Rect>& out);

		//! 设置变量
		inline void setMaoDingSize(int param){ m_MaoDingSize = param; }
		inline void setColorThreshold(int param){ m_ColorThreshold = param; }

		inline void setBluePercent(float param){ m_BluePercent = param; }
		inline float getBluePercent() const { return m_BluePercent; }
		inline void setWhitePercent(float param){ m_WhitePercent = param; }
		inline float getWhitePercent() const { return m_WhitePercent; }

		//! preprocessChar所用常量
		static const int CHAR_SIZE = 20;
		static const int HORIZONTAL = 1;
		static const int VERTICAL = 0;

		//! preprocessChar所用常量
		static const int DEFAULT_MaoDing_SIZE = 7;
		static const int DEFAULT_MAT_WIDTH = 136;
		static const int DEFAULT_COLORTHRESHOLD = 150;



	private:
		//！铆钉判断参数
		int m_MaoDingSize;

		//！车牌大小参数
		int m_theMatWidth;

		//！车牌颜色判断参数
		int m_ColorThreshold;
		float m_BluePercent;
		float m_WhitePercent;

	
	};

}

#endif
