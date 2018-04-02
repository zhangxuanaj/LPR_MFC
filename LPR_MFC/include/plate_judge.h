

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

		//! 对多幅车牌进行SVM判断
		int plateJudge(const vector<CPlate>&, vector<CPlate>&);

		//! 车牌判断
		int plateJudge(const vector<Mat>&, vector<Mat>&);

		//! 车牌判断（一副图像）
		//为了使用svm置信度参数score的值，将函数返回值由int改为float
		float plateJudge(const Mat& inMat, int& result);

		//! 直方图均衡
		Mat histeq(Mat);

		//! 装载SVM模型
		void LoadModel();

		//! 装载SVM模型
		void LoadModel(string s);

		//! 设置与读取模型路径
		inline void setModelPath(string path){ m_path = path; }
		inline string getModelPath() const{ return m_path; }

	private:
		//！使用的SVM模型
		cv::Ptr<cv::ml::SVM> svm;

		// ! lpr的getFeatures回调函数
		// ！用于从车牌的image生成svm的训练特征features
		svmCallback m_getFeatures;

		//! 模型存储路径
		string m_path;
	};

}	

#endif 
