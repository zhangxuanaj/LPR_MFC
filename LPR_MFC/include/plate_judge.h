

#ifndef __PLATE_JUDGE_H__
#define __PLATE_JUDGE_H__

#include "prep.h"
#include "features.h"
#include "plate.h"


namespace lpr {

	class CPlateJudge
	{
	public:
		CPlateJudge();

		//! �Զ�����ƽ���SVM�ж�
		int plateJudge(const vector<CPlate>&, vector<CPlate>&);

		//! �����ж�
		int plateJudge(const vector<Mat>&, vector<Mat>&);

		//! �����жϣ�һ��ͼ��
		//Ϊ��ʹ��svm���ŶȲ���score��ֵ������������ֵ��int��Ϊfloat
		float plateJudge(const Mat& inMat, int& result);

		//! ֱ��ͼ����
		Mat histeq(Mat);

		//! װ��SVMģ��
		void LoadModel();

		//! װ��SVMģ��
		void LoadModel(string s);

		//! �������ȡģ��·��
		inline void setModelPath(string path){ m_path = path; }
		inline string getModelPath() const{ return m_path; }

	private:
		//��ʹ�õ�SVMģ��
		cv::Ptr<cv::ml::SVM> svm;

		// ! lpr��getFeatures�ص�����
		// �����ڴӳ��Ƶ�image����svm��ѵ������features
		svmCallback m_getFeatures;

		//! ģ�ʹ洢·��
		string m_path;
	};

}	

#endif 
