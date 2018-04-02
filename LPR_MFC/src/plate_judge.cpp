
#include "stdafx.h"
#include "../include/plate_judge.h"



namespace lpr{

	CPlateJudge::CPlateJudge()
	{
		m_path = "model/svm.xml";
		m_getFeatures = getHistogramFeatures;



		LoadModel();
	}

	void CPlateJudge::LoadModel()
	{
		//svm.clear();
		//svm.load(m_path.c_str(), "svm");
		svm = Algorithm::load<cv::ml::SVM>(m_path);
	}

	void CPlateJudge::LoadModel(string s)
	{
		//svm.clear();
		//svm.load(s.c_str(), "svm");
		svm = Algorithm::load<cv::ml::SVM>(s);
	}

	//! 直方图均衡
	Mat CPlateJudge::histeq(Mat in)
	{
		Mat out(in.size(), in.type());
		if (in.channels() == 3)
		{
			Mat hsv;
			vector<Mat> hsvSplit;
			cvtColor(in, hsv, CV_BGR2HSV);
			split(hsv, hsvSplit);
			equalizeHist(hsvSplit[2], hsvSplit[2]);
			merge(hsvSplit, hsv);
			cvtColor(hsv, out, CV_HSV2BGR);
		}
		else if (in.channels() == 1)
		{
			equalizeHist(in, out);
		}
		return out;
	}


	//! 对单幅图像进行SVM判断
	//为了使用svm置信度参数score的值，将函数返回值由int改为float
	float CPlateJudge::plateJudge(const Mat& inMat, int& result)
	{
		if (m_getFeatures == NULL) return -1;

		Mat features;
		m_getFeatures(inMat, features);

		//通过直方图均衡化后的彩色图进行预测
		Mat p = features.reshape(1, 1);
		p.convertTo(p, CV_32FC1);

		float response = svm->predict(p);
		//response的值时1或0，1代表车牌，0代表不是车牌
		//cout << "svm预测结果： " << response << endl;
		//score小于0说明是车牌，大于0不是车牌
		//当score小于0时，score越小，属于车牌概率越大
		//score应该是越小越好，当score为正值时也应该越小越好
		float score = svm->predict(p, noArray(), cv::ml::StatModel::Flags::RAW_OUTPUT);
		//cout << "svm置信度： " << score << endl;
		if (score < 0)
		{
			//cout << "svm置信度score是： " << score << endl;
		}


		//svm的predict方法输出的都是1或0，无法利用这个结果进行判断
		//mser、color、sobel优先顺序决策无法通过svm决策解决
		//cout << "svm决策置信度： " << (float)response << endl;
		result = (int)response;

		return score;
	}




	//! 对多幅图像进行SVM判断
	int CPlateJudge::plateJudge(const vector<Mat>& inVec, vector<Mat>& resultVec)
	{
		int num = inVec.size();
		for (int j = 0; j < num; j++)
		{
			Mat inMat = inVec[j];

			int response = -1;
			plateJudge(inMat, response);

			if (response == 1)
				resultVec.push_back(inMat);
		}
		return 0;
	}

	//! 对多幅车牌进行SVM判断

	//根据plate_detect调用参数可知，目前使用的是这个函数
	//通过svm检测的resultVec里包含svm置信度参数score：存在成员变量m_svmScore中
	int CPlateJudge::plateJudge(const vector<CPlate>& inVec, vector<CPlate>& resultVec)
	{
		int num = inVec.size();
		for (int j = 0; j < num; j++)
		{
			CPlate inPlate = inVec[j];
			//cout << "score调试值： " << inPlate.m_svmScore << endl;
			Mat inMat = inPlate.getPlateMat();

			int response = -1;
			//inVec[j].m_svmScore = plateJudge(inMat, response);
			inPlate.m_svmScore = plateJudge(inMat, response);
			//cout << "score调试值： "<<inPlate.m_svmScore << endl;
			if (response == 1)
			{
				//对于通过svm判断的，紧接着排除置信度值很低的，这里取阈值0.08
				if (inPlate.m_svmScore < -0.08)
					resultVec.push_back(inPlate);
				//cout << "调试值： " << resultVec[0].m_svmScore << endl;
				//CPlate plate(inPlate);
				//cout << "二次调试值： " << plate.m_svmScore << endl;
				//cout << "再次调试值： " << inPlate.m_svmScore << endl;
			}
			else
			{
				int w = inMat.cols;
				int h = inMat.rows;
				//再取中间部分判断一次，这个中间部分参数可调，且影响很大
				Mat tmpmat = inMat(Rect_<double>(w * 0.04, h * 0.1, w * 0.9, h * 0.8));
				//Mat tmpmat = inMat(Rect_<double>(w * 0.05, h * 0.1, w * 0.9, h * 0.8));
				Mat tmpDes = inMat.clone();
				resize(tmpmat, tmpDes, Size(inMat.size()));
				//需要重新计算svm置信度
				inPlate.m_svmScore = plateJudge(tmpDes, response);
				//plateJudge(tmpDes, response);

				if (response == 1)
				{
					if (inPlate.m_svmScore < -0.08)
						//inPlate.m_svmScore = -inPlate.m_svmScore;//这样是原理上错误的score数值越小越好
						resultVec.push_back(inPlate);
				}
			}

		}
		return 0;
	}

}