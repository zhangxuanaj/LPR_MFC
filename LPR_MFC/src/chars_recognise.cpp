#include "stdafx.h"

#include "../include/chars_recognise.h"

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) if ((p)) {delete (p);  (p) = NULL; }
#endif


namespace lpr
{

	CCharsRecognise::CCharsRecognise() { m_charsSegment = new CCharsSegment(); }

	CCharsRecognise::~CCharsRecognise() { SAFE_RELEASE(m_charsSegment); }

	// getPlateType
	//�жϳ��Ƶ�����
	Color CCharsRecognise::getPlateType(const Mat& src, const bool adaptive_minsv)
	{
		float max_percent = 0;
		Color max_color = UNKNOWN;
	
		float blue_percent = 0;
		float yellow_percent = 0;
		float white_percent = 0;
	
		if (plateColorJudge(src, BLUE, adaptive_minsv, blue_percent) == true) 
		{
			// cout << "BLUE" << endl;
			return BLUE;
		}
		else if (plateColorJudge(src, YELLOW, adaptive_minsv, yellow_percent) ==true) 
		{
			// cout << "YELLOW" << endl;
			return YELLOW;
		}
		else if (plateColorJudge(src, WHITE, adaptive_minsv, white_percent) ==true) 
		{
			// cout << "WHITE" << endl;
			return WHITE;
		}
		else {
			// cout << "OTHER" << endl;
	
			// �������һ�߶���������ֵ����ȡֵ�����
			max_percent = blue_percent > yellow_percent ? blue_percent : yellow_percent;
			max_color = blue_percent > yellow_percent ? BLUE : YELLOW;
	
			max_color = max_percent > white_percent ? max_color : WHITE;
			return max_color;
		}
	}


	//! �ж�һ�����Ƶ���ɫ
	//! ���복��mat����ɫģ��
	//! ����true��fasle
	bool CCharsRecognise::plateColorJudge(const Mat& src, const Color r, const bool adaptive_minsv,float& percent) 
	{
		// �ж���ֵ
		const float thresh = 0.45f;
	
		Mat src_gray;
		colorMatch(src, src_gray, r, adaptive_minsv);
	
		percent =
			float(countNonZero(src_gray)) / float(src_gray.rows * src_gray.cols);
		// cout << "percent:" << percent << endl;
	
		if (percent > thresh)
			return true;
		else
			return false;
	}


	//! ����һ��ͼ������ɫģ���ȡ��Ӧ�Ķ�ֵͼ
	//! ����RGBͼ��, ��ɫģ�壨��ɫ����ɫ��
	//! ����Ҷ�ͼ��ֻ��0��255����ֵ��255����ƥ�䣬0����ƥ�䣩
	Mat CCharsRecognise::colorMatch(const Mat& src, Mat& match, const Color r,const bool adaptive_minsv) 
	{
		// S��V����Сֵ��adaptive_minsv���boolֵ�ж�
		// ���Ϊtrue������Сֵȡ����Hֵ��������˥��
		// ���Ϊfalse����������Ӧ��ʹ�ù̶�����Сֵminabs_sv
		// Ĭ��Ϊfalse
		const float max_sv = 255;
		const float minref_sv = 64;
	
		const float minabs_sv = 95;
	
		// blue��H��Χ
		const int min_blue = 100;  // 100
		const int max_blue = 140;  // 140
	
		// yellow��H��Χ
		const int min_yellow = 15;  // 15
		const int max_yellow = 40;  // 40
	
		// white��H��Χ
		const int min_white = 0;   // 15
		const int max_white = 30;  // 40
	
		Mat src_hsv;
		// ת��HSV�ռ���д�����ɫ������Ҫʹ�õ���H����������ɫ���ɫ��ƥ�乤��
		cvtColor(src, src_hsv, CV_BGR2HSV);
	
		vector<Mat> hsvSplit;
		split(src_hsv, hsvSplit);
		equalizeHist(hsvSplit[2], hsvSplit[2]);
		merge(hsvSplit, src_hsv);
	
		//ƥ��ģ���ɫ,�л��Բ�����Ҫ�Ļ�ɫ
		int min_h = 0;
		int max_h = 0;
		switch (r) {
		case BLUE:
			min_h = min_blue;
			max_h = max_blue;
			break;
		case YELLOW:
			min_h = min_yellow;
			max_h = max_yellow;
			break;
		case WHITE:
			min_h = min_white;
			max_h = max_white;
			break;
		default:
			// Color::UNKNOWN
			break;
		}
	
		float diff_h = float((max_h - min_h) / 2);
		float avg_h = min_h + diff_h;
	
		int channels = src_hsv.channels();
		int nRows = src_hsv.rows;
		//ͼ����������Ҫ����ͨ������Ӱ�죻
		int nCols = src_hsv.cols * channels;
	
		if (src_hsv.isContinuous())  //�����洢�����ݣ���һ�д���
		{
			nCols *= nRows;
			nRows = 1;
		}
	
		int i, j;
		uchar* p;
		float s_all = 0;
		float v_all = 0;
		float count = 0;
		for (i = 0; i < nRows; ++i) {
			p = src_hsv.ptr<uchar>(i);
			for (j = 0; j < nCols; j += 3) {
				int H = int(p[j]);      // 0-180
				int S = int(p[j + 1]);  // 0-255
				int V = int(p[j + 2]);  // 0-255
	
				s_all += S;
				v_all += V;
				count++;
	
				bool colorMatched = false;
	
				if (H > min_h && H < max_h) {
					float Hdiff = 0;
					if (H > avg_h)
						Hdiff = H - avg_h;
					else
						Hdiff = avg_h - H;
	
					float Hdiff_p = float(Hdiff) / diff_h;
	
					// S��V����Сֵ��adaptive_minsv���boolֵ�ж�
					// ���Ϊtrue������Сֵȡ����Hֵ��������˥��
					// ���Ϊfalse����������Ӧ��ʹ�ù̶�����Сֵminabs_sv
					float min_sv = 0;
					if (true == adaptive_minsv)
						min_sv =
						minref_sv -
						minref_sv / 2 *
						(1 - Hdiff_p);  // inref_sv - minref_sv / 2 * (1 - Hdiff_p)
					else
						min_sv = minabs_sv;  // add
	
					if ((S > min_sv && S < max_sv) && (V > min_sv && V < max_sv))
						colorMatched = true;
				}
	
				if (colorMatched == true) {
					p[j] = 0;
					p[j + 1] = 0;
					p[j + 2] = 255;
				}
				else {
					p[j] = 0;
					p[j + 1] = 0;
					p[j + 2] = 0;
				}
			}
		}

	
		// ��ȡ��ɫƥ���Ķ�ֵ�Ҷ�ͼ
		Mat src_grey;
		vector<Mat> hsvSplit_done;
		split(src_hsv, hsvSplit_done);
		src_grey = hsvSplit_done[2];
	
		match = src_grey;
	
		return src_grey;
	}


	int CCharsRecognise::charsRecognise(Mat plate, std::string& plateLicense) 
	{
		std::vector<Mat> matChars;

		int result = m_charsSegment->charsSegment(plate, matChars);


		if (result == 0) 
		{
			
			int num = matChars.size();
			for (int j = 0; j < num; j++)
			{
				Mat charMat = matChars.at(j);
				bool isChinses = false;
				float maxVal = 0;
				if (j == 0) 
				{
					bool judge = true;
					isChinses = true;
					auto character = CharsIdentify::instance()->identifyChinese(charMat, maxVal, judge);
					plateLicense.append(character.second);
				}
				else 
				{
					isChinses = false;
					auto character = CharsIdentify::instance()->identify(charMat, isChinses);
					plateLicense.append(character.second);
				}
			}

		}
		if (plateLicense.size() < 7) 
		{
			return -1;
		}

		return result;
	}

	int CCharsRecognise::charsRecognise(CPlate& plate, std::string& plateLicense) 
	{
		std::vector<Mat> matChars;
		

		Mat plateMat = plate.getPlateMat();

		Color color;
		if (plate.getPlateLocateType() == CMSER)
		{
			color = plate.getPlateColor();
		}
		else
		{
			int w = plateMat.cols;
			int h = plateMat.rows;
			Mat tmpMat = plateMat(Rect_<double>(w * 0.1, h * 0.1, w * 0.8, h * 0.8));
			color = getPlateType(tmpMat, true);
		}

		//���복�ƣ�����ֿ�matChars
		int result = m_charsSegment->charsSegment(plateMat, matChars, color);

		//��֤���Ʒָ��Ƿ���ȷ
		/*for (int i = 0; i < matChars.size();i++)
		{
			cv::imshow("�ָ���", matChars[i]);
			waitKey(1000);
		}*/

		
		if (result == 0) 
		{
			//for (auto block : matChars) {
			//  auto character = CharsIdentify::instance()->identify(block);
			//  plateLicense.append(character.second);
			//}
			int num = matChars.size();
			for (int j = 0; j < num; j++)
			{
				Mat charMat = matChars.at(j);
				bool isChinses = false;
				//if (j == 0)
				//  isChinses = true;
				//auto character = CharsIdentify::instance()->identify(charMat, isChinses);
				//plateLicense.append(character.second);
				std::pair<std::string, std::string> character;
				float maxVal;
				if (j == 0) 
				{
					//j=0����ʾ��һ���ֿ�
					isChinses = true;
					bool judge = true;
					//���ֿ�����Ƿ��Ǻ����ж�
					character = CharsIdentify::instance()->identifyChinese(charMat, maxVal, judge);
					
					//��������ֿ�ƥ����
					//cout << "�����ֿ�ƥ�����ǣ� "<<character.first <<"  "<<character.second<< endl;

					plateLicense.append(character.second);
				}
				else 
				{
					isChinses = false;
					character = CharsIdentify::instance()->identify(charMat, isChinses);

					//��������ֿ�ƥ����
					//cout <<"�����ֿ�ƥ�����ǣ� " <<character.first << "  " << character.second << endl;

					plateLicense.append(character.second);
				}

				CCharacter charResult;
				charResult.setCharacterMat(charMat);
				charResult.setCharacterStr(character.second);

				plate.addReutCharacter(charResult);
			}
			if (plateLicense.size() < 7) 
			{
				return -1;
			}
		}

		return result;
	}


}

