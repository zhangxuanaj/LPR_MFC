#ifndef __FEATURES_H__
#define __FEATURES_H__


namespace lpr 
{


	Rect GetCenterRect(Mat& in);


	Mat CutTheRect(Mat& in, Rect& rect);


	//! get character feature
	cv::Mat charFeatures(cv::Mat in, int sizeData);

//! ֱ��ͼ����
Mat histeq(Mat in);

// ����ȡ��ֱ��ˮƽ����ֱ��ͼ
Mat ProjectedHistogram(Mat img, int t);

//! ��ó��Ƶ�������
Mat getTheFeatures(Mat in);

// ! lpr��getFeatures�ص�����
// �����ڴӳ��Ƶ�image����svm��ѵ������features
typedef void(*svmCallback)(const Mat& image, Mat& features);

// ! lpr��getFeatures�ص�����
// ��������������ֱ��ͼ���������Ļص�����
void getHisteqFeatures(const Mat& image, Mat& features);

// ! lpr��getFeatures�ص�����
// ���������ǻ�ȡ��ֱ��ˮƽ��ֱ��ͼͼֵ
void getHistogramFeatures(const Mat& image, Mat& features);


//! LBP feature
//void getLBPFeatures(const cv::Mat& image, cv::Mat& features);


//! color feature and histom
void getHistomPlusColoFeatures(const cv::Mat& image, cv::Mat& features);

//! color feature
void getColorFeatures(const cv::Mat& src, cv::Mat& features);

//// ���������ǻ�ȡSIFT������
//void getSIFTFeatures(const Mat& image, Mat& features);
//
//// ���������ǻ�ȡHOG������
//void getHOGFeatures(const Mat& image, Mat& features);

}	

#endif
