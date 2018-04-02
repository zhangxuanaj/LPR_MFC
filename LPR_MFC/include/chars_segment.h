
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

		//! ��ó�����ɫ
		Color getPlateType(const Mat& src, const bool adaptive_minsv);

		//! �ж�һ�����Ƶ���ɫ
		//! ���복��mat����ɫģ��
		//! ����true��fasle
		bool plateColorJudge(const Mat& src, const Color r, const bool adaptive_minsv,float& percent);

		


		Mat colorMatch(const Mat& src, Mat& match, const Color r,const bool adaptive_minsv);

		int ThresholdOtsu(Mat mat);

		//! �ַ��ָ�

		int charsSegment(Mat input, std::vector<Mat>& resultVec, Color color = BLUE);
		

		//! �ַ��ߴ���֤
		bool verifySizes(Mat r);

		//! �ַ�Ԥ����
		Mat preprocessChar(Mat in);

		//! ֱ��ͼ���⣬Ϊ�жϳ�����ɫ��׼��
		//Mat histeq(Mat in);

		bool clearMaoDing(Mat& img);

		//! �������⳵��������²������ַ���λ�úʹ�С
		Rect GetChineseRect(const Rect rectSpe);

		//! �ҳ�ָʾ���е��ַ���Rect��������A7003X������A��λ��
		int GetSpecificRect(const vector<Rect>& vecRect);

		//! �����������������
		//  1.�������ַ�Rect��ߵ�ȫ��Rectȥ�����������ؽ������ַ���λ�á�
		//  2.�������ַ�Rect��ʼ������ѡ��6��Rect���������ȥ��
		int RebuildRect(const vector<Rect>& vecRect, vector<Rect>& outRect, int specIndex);

		//! ��Rect��λ�ô����ҽ�������
		//int SortRect(const vector<Rect>& vecRect, vector<Rect>& out);

		//! ���ñ���
		inline void setMaoDingSize(int param){ m_MaoDingSize = param; }
		inline void setColorThreshold(int param){ m_ColorThreshold = param; }

		inline void setBluePercent(float param){ m_BluePercent = param; }
		inline float getBluePercent() const { return m_BluePercent; }
		inline void setWhitePercent(float param){ m_WhitePercent = param; }
		inline float getWhitePercent() const { return m_WhitePercent; }

		//! preprocessChar���ó���
		static const int CHAR_SIZE = 20;
		static const int HORIZONTAL = 1;
		static const int VERTICAL = 0;

		//! preprocessChar���ó���
		static const int DEFAULT_MaoDing_SIZE = 7;
		static const int DEFAULT_MAT_WIDTH = 136;
		static const int DEFAULT_COLORTHRESHOLD = 150;



	private:
		//��í���жϲ���
		int m_MaoDingSize;

		//�����ƴ�С����
		int m_theMatWidth;

		//��������ɫ�жϲ���
		int m_ColorThreshold;
		float m_BluePercent;
		float m_WhitePercent;

	
	};

}

#endif
