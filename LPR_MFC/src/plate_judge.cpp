
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

	//! ֱ��ͼ����
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


	//! �Ե���ͼ�����SVM�ж�
	//Ϊ��ʹ��svm���ŶȲ���score��ֵ������������ֵ��int��Ϊfloat
	float CPlateJudge::plateJudge(const Mat& inMat, int& result)
	{
		if (m_getFeatures == NULL) return -1;

		Mat features;
		m_getFeatures(inMat, features);

		//ͨ��ֱ��ͼ���⻯��Ĳ�ɫͼ����Ԥ��
		Mat p = features.reshape(1, 1);
		p.convertTo(p, CV_32FC1);

		float response = svm->predict(p);
		//response��ֵʱ1��0��1�����ƣ�0�����ǳ���
		//cout << "svmԤ������ " << response << endl;
		//scoreС��0˵���ǳ��ƣ�����0���ǳ���
		//��scoreС��0ʱ��scoreԽС�����ڳ��Ƹ���Խ��
		//scoreӦ����ԽСԽ�ã���scoreΪ��ֵʱҲӦ��ԽСԽ��
		float score = svm->predict(p, noArray(), cv::ml::StatModel::Flags::RAW_OUTPUT);
		//cout << "svm���Ŷȣ� " << score << endl;
		if (score < 0)
		{
			//cout << "svm���Ŷ�score�ǣ� " << score << endl;
		}


		//svm��predict��������Ķ���1��0���޷����������������ж�
		//mser��color��sobel����˳������޷�ͨ��svm���߽��
		//cout << "svm�������Ŷȣ� " << (float)response << endl;
		result = (int)response;

		return score;
	}




	//! �Զ��ͼ�����SVM�ж�
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

	//! �Զ�����ƽ���SVM�ж�

	//����plate_detect���ò�����֪��Ŀǰʹ�õ����������
	//ͨ��svm����resultVec�����svm���ŶȲ���score�����ڳ�Ա����m_svmScore��
	int CPlateJudge::plateJudge(const vector<CPlate>& inVec, vector<CPlate>& resultVec)
	{
		int num = inVec.size();
		for (int j = 0; j < num; j++)
		{
			CPlate inPlate = inVec[j];
			//cout << "score����ֵ�� " << inPlate.m_svmScore << endl;
			Mat inMat = inPlate.getPlateMat();

			int response = -1;
			//inVec[j].m_svmScore = plateJudge(inMat, response);
			inPlate.m_svmScore = plateJudge(inMat, response);
			//cout << "score����ֵ�� "<<inPlate.m_svmScore << endl;
			if (response == 1)
			{
				//����ͨ��svm�жϵģ��������ų����Ŷ�ֵ�ܵ͵ģ�����ȡ��ֵ0.08
				if (inPlate.m_svmScore < -0.08)
					resultVec.push_back(inPlate);
				//cout << "����ֵ�� " << resultVec[0].m_svmScore << endl;
				//CPlate plate(inPlate);
				//cout << "���ε���ֵ�� " << plate.m_svmScore << endl;
				//cout << "�ٴε���ֵ�� " << inPlate.m_svmScore << endl;
			}
			else
			{
				int w = inMat.cols;
				int h = inMat.rows;
				//��ȡ�м䲿���ж�һ�Σ�����м䲿�ֲ����ɵ�����Ӱ��ܴ�
				Mat tmpmat = inMat(Rect_<double>(w * 0.04, h * 0.1, w * 0.9, h * 0.8));
				//Mat tmpmat = inMat(Rect_<double>(w * 0.05, h * 0.1, w * 0.9, h * 0.8));
				Mat tmpDes = inMat.clone();
				resize(tmpmat, tmpDes, Size(inMat.size()));
				//��Ҫ���¼���svm���Ŷ�
				inPlate.m_svmScore = plateJudge(tmpDes, response);
				//plateJudge(tmpDes, response);

				if (response == 1)
				{
					if (inPlate.m_svmScore < -0.08)
						//inPlate.m_svmScore = -inPlate.m_svmScore;//������ԭ���ϴ����score��ֵԽСԽ��
						resultVec.push_back(inPlate);
				}
			}

		}
		return 0;
	}

}