


#ifndef __PLATE_LOCATE_H__
#define __PLATE_LOCATE_H__

#include "prep.h"
#include "plate.h"


namespace lpr {

	enum CharSearchDirection { LEFT, RIGHT };


	bool computeIOU(const Rect& rect1, const Rect& rect2, const float thresh, float& result);
	float computeIOU(const Rect& rect1, const Rect& rect2);

	
	// computer the insert over union about two rrect
	bool computeIOU(const RotatedRect& rrect1, const RotatedRect& rrect2, const int width, const int height, const float thresh, float& result);
	float computeIOU(const RotatedRect& rrect1, const RotatedRect& rrect2, const int width, const int height);

		// Scale back RotatedRect
	RotatedRect scaleBackRRect(const RotatedRect& rr, const float scale_ratio);

	void mserCharMatch(const Mat &src, std::vector<Mat> &match, std::vector<CPlate>& out_plateVec_blue,
		std::vector<CPlate>& out_plateVec_yellow, bool usePlateMser, std::vector<RotatedRect>& out_plateRRect_blue,
		std::vector<RotatedRect>& out_plateRRect_yellow, int index = 0, bool showDebug = false);


	Mat adaptive_image_from_points(const std::vector<Point>& points,
		const Rect& rect, const Size& size, const Scalar& backgroundColor = Scalar(0, 0, 0),
		const Scalar& forgroundColor = Scalar(255, 255, 255), bool gray = true);


	bool judegMDOratio2(const Mat& image, const Rect& rect, std::vector<Point>& contour, Mat& result);


	class CPlateLocate
	{
	public:
		CPlateLocate();

		enum LocateType { SOBEL, COLOR, CMSER, OTHER };

		//���mser��λ
		int plateMserLocate(Mat src, std::vector<CPlate>& candPlates, int index = 0);

		int mserSearch(const Mat &src, vector<Mat>& out,
			vector<vector<CPlate>>& out_plateVec, bool usePlateMser, vector<vector<RotatedRect>>& out_plateRRect,
			int img_index = 0, bool showDebug = false);
	


		void deleteNotArea(Mat &inmat, Color color = UNKNOWN);


	
		void mergeCharToGroup(std::vector<CCharacter> vecRect,std::vector<std::vector<CCharacter>>& charGroupVec);

	
		// Scale to small image (for the purpose of comput mser in large image)
	    Mat scaleImage(const Mat& image, const Size& maxSize, double& scale_ratio);

		bool compareCharRect(const CCharacter& character1, const CCharacter& character2);


		void removeRightOutliers(std::vector<CCharacter>& charGroup, std::vector<CCharacter>& out_charGroup, double thresh1, double thresh2, Mat result);


		void searchWeakSeed(const std::vector<CCharacter>& charVec, std::vector<CCharacter>& mserCharacter, double thresh1, double thresh2,
			const Vec4f& line, Point& boundaryPoint, const Rect& maxrect, Rect& plateResult, Mat result, CharSearchDirection searchDirection);

		
		void rotatedRectangle(InputOutputArray img, RotatedRect rect,
			const Scalar& color, int thickness = 1,
			int lineType = LINE_8, int shift = 0);


		//! Sobel��һ������
		//! �����ƴ�С����״����ȡ��BoundRect������һ��
		int sobelFrtSearch(const Mat& src, vector<Rect_<float>>& outRects);

		//! Sobel�ڶ�������
		//! �Դ�С����״�����ƣ����ɲο�����
		//1.3���һ���������������const
		int sobelSecSearch(Mat& bound, Point2f refpoint,vector<RotatedRect>& outRects);
	

	
		int sobelSecSearchPart(Mat& bound, Point2f refpoint,vector<RotatedRect>& outRects);

		int deskew(const Mat& src, const Mat& src_b,
			std::vector<RotatedRect>& inRects, std::vector<CPlate>& outPlates,
			bool useDeteleArea = true, Color color = UNKNOWN);


	
		//! ��Ťб����
		//int deskew(const Mat& src, const Mat& src_b, vector<RotatedRect>& inRects, vector<CPlate>& outPlates);

		//! �Ƿ�ƫб
		//! �����ֵ��ͼ������жϽ��
		bool isdeflection(const Mat& in, const double angle, double& slope);

		//! Sobel����
		//! �����ɫͼ�������ֵ��ͼ��
		int sobelOper(const Mat& in, Mat& out, int blurSize, int morphW, int morphH);

		//! ����һ����ȫ��Rect
	
		bool calcSafeRect(const RotatedRect& roi_rect, const Mat& src,Rect_<float>& safeBoundRect);
		bool calcSafeRect(const RotatedRect &roi_rect, const int width, const int height,Rect_<float> &safeBoundRect);
	

		bool verifyCharSizes(Rect r);

	
		bool mat_valid_position(const Mat& mat, int row, int col);


		void setPoint(Mat& mat, int row, int col, const Scalar& value);


		Mat preprocessChar(Mat in, int char_size);

		

		Rect adaptive_charrect_from_rect(const Rect& rect, int maxwidth, int maxheight);



		void NMStoCharacter(std::vector<CCharacter> &inVec, double overlap);

		//! ��ת����
		bool rotation(Mat& in, Mat& out, const Size rect_size, const Point2f center, const double angle);

		//! Ť�����
		void affine(const Mat& in, Mat& out, const double slope);

		//! ��ɫ��λ��
		int plateColorLocate(Mat src, vector<CPlate>& candPlates, int index = 0);

		//! Sobel��λ��
		int plateSobelLocate(Mat src, vector<CPlate>& candPlates, int index = 0);


		int sobelOperT(const Mat& in, Mat& out, int blurSize, int morphW, int morphH);
	
		//! Color����

		Mat colorMatch(const Mat& src, Mat& match, const Color r,const bool adaptive_minsv);
		
		int colorSearch(const Mat& src, const Color r, Mat& out,vector<RotatedRect>& outRects, int index = 0);
	

		bool charJudge(Mat roi);
		bool sobelJudge(Mat roi);
		int colorJudge(const Mat& src, const Color r, vector<RotatedRect>& rects);

		int deskewOld(Mat src, vector<RotatedRect>& inRects, vector<RotatedRect>& outRects, vector<Mat>& outMats, LocateType locateType);

		bool verifyCharSizes(Mat r);
		int getPlateType(Mat src);
		bool plateColorJudge(Mat src, const Color r);
		Mat clearMaoDing(Mat img);


		//! ���ƶ�λ
		int plateLocate(Mat, vector<Mat>&, int = 0);

		//! ���Ƶĳߴ���֤
		bool verifySizes(RotatedRect mr);

		//! ���������ʾ
		Mat showResultMat(Mat src, Size rect_size, Point2f center, int index);


		void clearMaoDingOnly(Mat& img);

	  
		bool bFindLeftRightBound(Mat& bound_threshold, int& posLeft, int& posRight);


		void DeleteNotArea(Mat& inmat);


		Color getPlateType(const Mat& src, const bool adaptive_minsv);

		bool plateColorJudge(const Mat& src, const Color r, const bool adaptive_minsv,float& percent);


		int ThresholdOtsu(Mat mat);


		void clearMaoDing(Mat mask, int& top, int& bottom);


		bool bFindLeftRightBound1(Mat& bound_threshold, int& posLeft, int& posRight);


		void setLifemode(bool param);

		//! �������ȡ����
		inline void setGaussianBlurSize(int param){ m_GaussianBlurSize = param; }
		inline int getGaussianBlurSize() const{ return m_GaussianBlurSize; }

		inline void setMorphSizeWidth(int param){ m_MorphSizeWidth = param; }
		inline int getMorphSizeWidth() const{ return m_MorphSizeWidth; }

		inline void setMorphSizeHeight(int param){ m_MorphSizeHeight = param; }
		inline int getMorphSizeHeight() const{ return m_MorphSizeHeight; }

		inline void setVerifyError(float param){ m_error = param; }
		inline float getVerifyError() const { return m_error; }
		inline void setVerifyAspect(float param){ m_aspect = param; }
		inline float getVerifyAspect() const { return m_aspect; }

		inline void setVerifyMin(int param){ m_verifyMin = param; }
		inline void setVerifyMax(int param){ m_verifyMax = param; }

		inline void setJudgeAngle(int param){ m_angle = param; }

		//! �Ƿ�������ģʽ
		inline void setDebug(int param){ m_debug = param; }

		//! ��ȡ����ģʽ״̬
		inline int getDebug(){ return m_debug; }

		//! PlateLocate���ó���
		static const int DEFAULT_GAUSSIANBLUR_SIZE = 5;
		static const int SOBEL_SCALE = 1;
		static const int SOBEL_DELTA = 0;
		static const int SOBEL_DDEPTH = CV_16S;
		static const int SOBEL_X_WEIGHT = 1;
		static const int SOBEL_Y_WEIGHT = 0;
		static const int DEFAULT_MORPH_SIZE_WIDTH = 17;//17
		static const int DEFAULT_MORPH_SIZE_HEIGHT = 3;//3

		//! showResultMat���ó���
		static const int WIDTH = 136;
		static const int HEIGHT = 36;
		static const int TYPE = CV_8UC3;

		//! verifySize���ó���
		static const int DEFAULT_VERIFY_MIN = 1;//3
		static const int DEFAULT_VERIFY_MAX = 24;//20

		//! �Ƕ��ж����ó���
		static const int DEFAULT_ANGLE = 60;//30

		//! �Ƿ�������ģʽ������Ĭ��0����ر�
		static const int DEFAULT_DEBUG = 0;

	protected:
		//! ��˹ģ�����ñ���
		int m_GaussianBlurSize;

		//! ���Ӳ������ñ���
		int m_MorphSizeWidth;
		int m_MorphSizeHeight;

		//! verifySize���ñ���
		float m_error;
		float m_aspect;
		int m_verifyMin;
		int m_verifyMax;

		//! �Ƕ��ж����ñ���
		int m_angle;

		//! �Ƿ�������ģʽ��0�رգ���0����
		int m_debug;
	};

}	


#endif 

