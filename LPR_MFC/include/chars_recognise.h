
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
		//! ����һ��ͼ������ɫģ���ȡ��Ӧ�Ķ�ֵͼ
		//! ����RGBͼ��, ��ɫģ�壨��ɫ����ɫ��
		//! ����Ҷ�ͼ��ֻ��0��255����ֵ��255����ƥ�䣬0����ƥ�䣩
		Mat colorMatch(const Mat& src, Mat& match, const Color r,const bool adaptive_minsv);

		//! ��ó�����ɫ
		Color getPlateType(const Mat& src, const bool adaptive_minsv);
		//! �ж�һ�����Ƶ���ɫ
		//		//! ���복��mat����ɫģ��
		//		//! ����true��fasle
		bool plateColorJudge(const Mat& src, const Color r, const bool adaptive_minsv,float& percent);

		inline std::string getPlateColor(cv::Mat input) //const
		{
			std::string color = "δ֪";
			Color result = getPlateType(input, true);
			if (BLUE == result) color = "����";
			if (YELLOW == result) color = "����";
			if (WHITE == result) color = "����";

			return color;
		}

		inline std::string getPlateColor(Color in) const 
		{
			std::string color = "δ֪";
			if (BLUE == in) color = "����";
			if (YELLOW == in) color = "����";
			if (WHITE == in) color = "����";

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
		//���ַ��ָ�

		CCharsSegment* m_charsSegment;
	};

} 

#endif 