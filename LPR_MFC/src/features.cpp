// 这个文件定义了lpr里所有特征生成的函数
// 所属命名空间为lpr

#include "stdafx.h"
#include "../include/prep.h"
#include "../include/features.h"
//#include "../include/lbp.hpp"

namespace lpr {


	Rect GetCenterRect(Mat &in)
	{
		Rect _rect;

		int top = 0;
		int bottom = in.rows - 1;

		// find the center rect

		for (int i = 0; i < in.rows; ++i) {
			bool bFind = false;
			for (int j = 0; j < in.cols; ++j) {
				if (in.data[i * in.step[0] + j] > 20) {
					top = i;
					bFind = true;
					break;
				}
			}
			if (bFind) {
				break;
			}

		}
		for (int i = in.rows - 1;
			i >= 0;
			--i) {
			bool bFind = false;
			for (int j = 0; j < in.cols; ++j) {
				if (in.data[i * in.step[0] + j] > 20) {
					bottom = i;
					bFind = true;
					break;
				}
			}
			if (bFind) {
				break;
			}

		}


		int left = 0;
		int right = in.cols - 1;
		for (int j = 0; j < in.cols; ++j) {
			bool bFind = false;
			for (int i = 0; i < in.rows; ++i) {
				if (in.data[i * in.step[0] + j] > 20) {
					left = j;
					bFind = true;
					break;
				}
			}
			if (bFind) {
				break;
			}

		}
		for (int j = in.cols - 1;
			j >= 0;
			--j) {
			bool bFind = false;
			for (int i = 0; i < in.rows; ++i) {
				if (in.data[i * in.step[0] + j] > 20) {
					right = j;
					bFind = true;

					break;
				}
			}
			if (bFind) {
				break;
			}
		}

		_rect.x = left;
		_rect.y = top;
		_rect.width = right - left + 1;
		_rect.height = bottom - top + 1;

		return _rect;
	}


	Mat CutTheRect(Mat &in, Rect &rect) 
	{
		int size = in.cols;  // (rect.width>rect.height)?rect.width:rect.height;
		Mat dstMat(size, size, CV_8UC1);
		dstMat.setTo(Scalar(0, 0, 0));

		int x = (int)floor((float)(size - rect.width) / 2.0f);
		int y = (int)floor((float)(size - rect.height) / 2.0f);

		for (int i = 0; i < rect.height; ++i) {

			for (int j = 0; j < rect.width; ++j) 
			{
				dstMat.data[dstMat.step[0] * (i + y) + j + x] =
					in.data[in.step[0] * (i + rect.y) + j + rect.x];
			}
		}

		return dstMat;
	}


	//对输入的in，计算其文字特征，返回文字特征类Mat
	//这个文字特征就是每行每列灰度值为1的像素总数以及低分辨率下的图像像素总数
	Mat charFeatures(Mat in, int sizeData) 
	{
		const int VERTICAL = 0;
		const int HORIZONTAL = 1;

		// cut the cetner, will afect 5% perices.
		Rect _rect = GetCenterRect(in);
		Mat tmpIn = CutTheRect(in, _rect);
		//Mat tmpIn = in.clone();

		// Low data feature
		Mat lowData;
		resize(tmpIn, lowData, Size(sizeData, sizeData));

		// Histogram features
		Mat vhist = ProjectedHistogram(lowData, VERTICAL);
		Mat hhist = ProjectedHistogram(lowData, HORIZONTAL);

		// Last 10 is the number of moments components
		int numCols = vhist.cols + hhist.cols + lowData.cols * lowData.cols;

		Mat out = Mat::zeros(1, numCols, CV_32F);
		// Asign values to

		int j = 0;
		for (int i = 0; i < vhist.cols; i++) {
			out.at<float>(j) = vhist.at<float>(i);
			j++;
		}
		for (int i = 0; i < hhist.cols; i++) {
			out.at<float>(j) = hhist.at<float>(i);
			j++;
		}
		for (int x = 0; x < lowData.cols; x++) {
			for (int y = 0; y < lowData.rows; y++) {
				out.at<float>(j) += (float)lowData.at <unsigned char>(x, y);
				j++;
			}
		}

		//std::cout << out << std::endl;

		return out;
	}

//! 直方图均衡
Mat histeq(Mat in)
{
	Mat out(in.size(), in.type());
	if(in.channels()==3)
	{
		Mat hsv;
		vector<Mat> hsvSplit;
		cvtColor(in, hsv, CV_BGR2HSV);
		split(hsv, hsvSplit);
		equalizeHist(hsvSplit[2], hsvSplit[2]);
		merge(hsvSplit, hsv);
		cvtColor(hsv, out, CV_HSV2BGR);
	}
	else if(in.channels()==1)
	{
		equalizeHist(in, out);
	}
	return out;
}

////! LBP feature
//void getLBPFeatures(const Mat& image, Mat& features) 
//{
//
//	Mat grayImage;
//	cvtColor(image, grayImage, CV_RGB2GRAY);
//
//	//if (1) {
//	//  imshow("grayImage", grayImage);
//	//  waitKey(0);
//	//  destroyWindow("grayImage");
//	//}
//
//	//spatial_ostu(grayImage, 8, 2);
//
//	//if (1) {
//	//  imshow("grayImage", grayImage);
//	//  waitKey(0);
//	//  destroyWindow("grayImage");
//	//}
//
//	Mat lbpimage;
//	lbpimage = libfacerec::olbp(grayImage);
//	Mat lbp_hist = libfacerec::spatial_histogram(lbpimage, 32, 4, 4);
//
//	features = lbp_hist;
//}


void getHistomPlusColoFeatures(const Mat& image, Mat& features)
{
	// TODO
	Mat feature1, feature2;
	getHistogramFeatures(image, feature1);
	getColorFeatures(image, feature2);
	hconcat(feature1.reshape(1, 1), feature2.reshape(1, 1), features);
}

// compute color histom
void getColorFeatures(const Mat& src, Mat& features) 
{
	Mat src_hsv;

	//grayImage = histeq(grayImage);
	cvtColor(src, src_hsv, CV_BGR2HSV);
	int channels = src_hsv.channels();
	int nRows = src_hsv.rows;

	// consider multi channel image
	int nCols = src_hsv.cols * channels;
	if (src_hsv.isContinuous()) 
	{
		nCols *= nRows;
		nRows = 1;
	}

	const int sz = 180;
	int h[sz] = { 0 };

	uchar* p;
	for (int i = 0; i < nRows; ++i) 
	{
		p = src_hsv.ptr<uchar>(i);
		for (int j = 0; j < nCols; j += 3) 
		{
			int H = int(p[j]);      // 0-180
			if (H > sz - 1) H = sz - 1;
			if (H < 0) H = 0;
			h[H]++;
		}
	}

	Mat mhist = Mat::zeros(1, sz, CV_32F);
	for (int j = 0; j < sz; j++) 
	{
		mhist.at<float>(j) = (float)h[j];
	}

	// Normalize histogram
	double min, max;
	minMaxLoc(mhist, &min, &max);

	if (max > 0)
		mhist.convertTo(mhist, -1, 1.0f / max, 0);

	features = mhist;
}



// ！获取垂直和水平方向直方图
Mat ProjectedHistogram(Mat img, int t)
{
	int sz=(t)?img.rows:img.cols;
	Mat mhist=Mat::zeros(1,sz,CV_32F);

	for(int j=0; j<sz; j++){
		Mat data=(t)?img.row(j):img.col(j);
		mhist.at<float>(j)=countNonZero(data);	//统计这一行或一列中，非零元素的个数，并保存到mhist中
	}

	//Normalize histogram
	double min, max;
	minMaxLoc(mhist, &min, &max);

	if(max>0)
		mhist.convertTo(mhist,-1 , 1.0f/max, 0);//用mhist直方图中的最大值，归一化直方图

	return mhist;
}


//! 获得车牌的特征数
Mat getTheFeatures(Mat in)
{
	const int VERTICAL = 0;
	const int HORIZONTAL = 1;

	//Histogram features
	Mat vhist=ProjectedHistogram(in, VERTICAL);
	Mat hhist=ProjectedHistogram(in, HORIZONTAL);

	//Last 10 is the number of moments components
	int numCols = vhist.cols + hhist.cols;

	Mat out = Mat::zeros(1, numCols, CV_32F);

	//Asign values to feature,样本特征为水平、垂直直方图
	int j=0;
	for(int i=0; i<vhist.cols; i++)
	{
		out.at<float>(j)=vhist.at<float>(i);
		j++;
	}
	for(int i=0; i<hhist.cols; i++)
	{
		out.at<float>(j)=hhist.at<float>(i);
		j++;
	}

	return out;
}

// ! lpr的getFeatures回调函数
// ！本函数是生成直方图均衡特征的回调函数
void getHisteqFeatures(const Mat& image, Mat& features)
{
	features = histeq(image);
}

// ! lpr的getFeatures回调函数
// ！本函数是获取垂直和水平的直方图图值
void getHistogramFeatures(const Mat& image, Mat& features)
{
	Mat grayImage;
	cvtColor(image, grayImage, CV_RGB2GRAY);
	//grayImage = histeq(grayImage);
	Mat img_threshold;
	threshold(grayImage, img_threshold, 0, 255, CV_THRESH_OTSU+CV_THRESH_BINARY);
	features = getTheFeatures(img_threshold);
}


// ! lpr的getFeatures回调函数
// ！本函数是获取SITF特征子
// ! 
//void getSIFTFeatures(const Mat& image, Mat& features)
//{
//	//待完善
//}
//
//
//// ! lpr的getFeatures回调函数
//// ！本函数是获取HOG特征子
//void getHOGFeatures(const Mat& image, Mat& features)
//{
//	//待完善
//}

}	