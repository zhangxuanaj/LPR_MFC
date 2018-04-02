
#ifndef __CHARS_RECOGNISE_H__
#define __CHARS_RECOGNISE_H__

#include "prep.h"
#include "chars_segment.h"
#include "chars_identify.h"


namespace lpr 
{

	class CCharsRecognise 
	{
	public:
		CCharsRecognise();

		~CCharsRecognise();

		int charsRecognise(cv::Mat plate, std::string& plateLicense);
		int charsRecognise(CPlate& plate, std::string& plateLicense);
		//! 根据一幅图像与颜色模板获取对应的二值图
		//! 输入RGB图像, 颜色模板（蓝色、黄色）
		//! 输出灰度图（只有0和255两个值，255代表匹配，0代表不匹配）
		Mat colorMatch(const Mat& src, Mat& match, const Color r,const bool adaptive_minsv);

		//! 获得车牌颜色
		Color getPlateType(const Mat& src, const bool adaptive_minsv);
		//! 判断一个车牌的颜色
		//		//! 输入车牌mat与颜色模板
		//		//! 返回true或fasle
		bool plateColorJudge(const Mat& src, const Color r, const bool adaptive_minsv,float& percent);

		inline std::string getPlateColor(cv::Mat input) //const
		{
			std::string color = "未知";
			Color result = getPlateType(input, true);
			if (BLUE == result) color = "蓝牌";
			if (YELLOW == result) color = "黄牌";
			if (WHITE == result) color = "白牌";

			return color;
		}

		inline std::string getPlateColor(Color in) const 
		{
			std::string color = "未知";
			if (BLUE == in) color = "蓝牌";
			if (YELLOW == in) color = "黄牌";
			if (WHITE == in) color = "白牌";

			return color;
		}

		inline void setLiuDingSize(int param) 
		{
			m_charsSegment->setMaoDingSize(param);
		}
		inline void setColorThreshold(int param) 
		{
			m_charsSegment->setColorThreshold(param);
		}
		inline void setBluePercent(float param) 
		{
			m_charsSegment->setBluePercent(param);
		}
		inline float getBluePercent() const 
		{
			return m_charsSegment->getBluePercent();
		}
		inline void setWhitePercent(float param) 
		{
			m_charsSegment->setWhitePercent(param);
		}
		inline float getWhitePercent() const 
		{
			return m_charsSegment->getWhitePercent();
		}

	private:
		//！字符分割

		CCharsSegment* m_charsSegment;
	};

} 

#endif 