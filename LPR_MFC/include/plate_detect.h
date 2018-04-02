
#ifndef __PLATE_DETECT_H__
#define __PLATE_DETECT_H__

#include "prep.h"
#include "plate_locate.h"
#include "plate_judge.h"
#include "plate.h"


namespace lpr {

	class CPlateDetect {
	public:
		CPlateDetect();

		~CPlateDetect();

		//! ��ȳ��Ƽ�⣬ʹ����ɫ�����Sobel���ۺ�
		Mat plateDetect(Mat src, vector<CPlate>& resultVec, bool showDetectArea = true, int index = 0);

		//! չʾ�м�Ľ��
		int showResult(const Mat& result);

		//! װ��SVMģ��
		void LoadSVM(string s);

		//! ����ģʽ�빤ҵģʽ�л�
		inline void setPDLifemode(bool param) { m_plateLocate->setLifemode(param); }

		//! �Ƿ�������ģʽ
		inline void setPDDebug(bool param) { m_plateLocate->setDebug(param); }

		//! ��ȡ����ģʽ״̬
		inline bool getPDDebug() { return m_plateLocate->getDebug(); }

		//! �������ȡ����
		inline void setGaussianBlurSize(int param) {
			m_plateLocate->setGaussianBlurSize(param);
		}
		inline int getGaussianBlurSize() const {
			return m_plateLocate->getGaussianBlurSize();
		}

		inline void setMorphSizeWidth(int param) {
			m_plateLocate->setMorphSizeWidth(param);
		}
		inline int getMorphSizeWidth() const {
			return m_plateLocate->getMorphSizeWidth();
		}

		inline void setMorphSizeHeight(int param) {
			m_plateLocate->setMorphSizeHeight(param);
		}
		inline int getMorphSizeHeight() const {
			return m_plateLocate->getMorphSizeHeight();
		}

		inline void setVerifyError(float param) {
			m_plateLocate->setVerifyError(param);
		}
		inline float getVerifyError() const {
			return m_plateLocate->getVerifyError();
		}
		inline void setVerifyAspect(float param) {
			m_plateLocate->setVerifyAspect(param);
		}
		inline float getVerifyAspect() const {
			return m_plateLocate->getVerifyAspect();
		}

		inline void setVerifyMin(int param) { m_plateLocate->setVerifyMin(param); }
		inline void setVerifyMax(int param) { m_plateLocate->setVerifyMax(param); }

		inline void setJudgeAngle(int param) { m_plateLocate->setJudgeAngle(param); }

		inline void setMaxPlates(int param) { m_maxPlates = param; }
		inline int getMaxPlates() const { return m_maxPlates; }

	private:

		enum LocateType { SOBEL, COLOR,CMSER,OTHER };

		//! ����һ��ͼ������ж��ٳ���
		int m_maxPlates;

		//! ���ƶ�λ
		CPlateLocate* m_plateLocate;

		//! �����ж�
		CPlateJudge* m_plateJudge;

		//mser��λ������Ƿ������ɫ��λ
		vector<RotatedRect> isColorSobelafterMser;

		// ��ɫ��λ������Ƿ����Sobel��λ
		//vector<RotatedRect> isSobelafterColor;
		int m_accumulator;

		//����Sobel�������ܵ���ͬһ���Ƽ���������
		vector<RotatedRect> isSobelafterSobel;
	};

} 

#endif 