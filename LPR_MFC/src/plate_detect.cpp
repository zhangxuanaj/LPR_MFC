#include "stdafx.h"
#include "../include/plate_detect.h"
#include "plate.h"
#include "chars_recognise.h"

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) if ((p)) {delete (p);  (p) = NULL; }
#endif

namespace lpr {

	CPlateDetect::CPlateDetect() {
		
		m_plateLocate = new CPlateLocate();
		m_plateJudge = new CPlateJudge();

		// Ĭ��EasyPR��һ��ͼ�ж�λ���10������
		m_maxPlates = 10;

		isColorSobelafterMser.swap(vector<RotatedRect>());   //�������
		m_accumulator = 0;  //�ۼ�������
	}

	CPlateDetect::~CPlateDetect() 
	{
		SAFE_RELEASE(m_plateLocate);
		SAFE_RELEASE(m_plateJudge);
	}

	void CPlateDetect::LoadSVM(string s) 
	{ m_plateJudge->LoadModel(s.c_str()); }


	Mat CPlateDetect::plateDetect(Mat src, vector<CPlate>& resultVec, bool showDetectArea, int index)
	{
		vector<Mat> resultPlates;

		vector<CPlate> mser_Plates;
		vector<CPlate> mser_result_Plates;
		vector<CPlate> mser_result_Plates_tmp;
		vector<CPlate> color_Plates;
		vector<CPlate> sobel_Plates;
		vector<CPlate> color_result_Plates;
		vector<CPlate> color_result_Plates_tmp;
		vector<CPlate> sobel_result_Plates;
		vector<CPlate> sobel_result_Plates_tmp;

		vector<CPlate> all_result_Plates;
		vector<CPlate> all_result_Plates_tmp;//��ʱ����

		//���mser�����ҵ�m_maxPlates�����ϣ�����m_maxPlates�����ĳ��ƣ��Ͳ��ٽ�����ɫ�����ˡ�
		const int mser_find_max = m_maxPlates;

		Mat result;
		src.copyTo(result);

		//���svm���ŶȱȽϹ������õ�����������
	    vector<int>before;
		vector<int>after;
		vector<int>mser;
		vector<int>color;
		vector<int>sobel;
		//vector<int>all;
		//��������MSER�������ֶ�λ
		//Mser�������ܲ�����������������ȡ������������ɫ���ܶ�λ��׼������Ŀǰɸѡ���ԣ�����˳��Mser>��ɫ>Sobel��������Ҫ�޸�
		//EasyPR�зǼ���ֵ���ƿ����ܽ���������

		//Mser��λ
		m_plateLocate->plateMserLocate(src, mser_Plates, index);
		m_plateJudge->plateJudge(mser_Plates, mser_result_Plates);
		for (int i = 0; i < mser_result_Plates.size();i++)
		{
			//cout << "���Ŷȣ� " << mser_result_Plates[i].m_svmScore << endl;
		}
		//��mser���������svm���Ŷ��ж�
		for (size_t i = 0; i < mser_result_Plates.size(); i++)
		{
			for (size_t j = 0; j < mser_result_Plates.size(); j++)
			{
				if (j == i)
					continue;
				else
				{
					if (abs(mser_result_Plates[i].getPlatePos().center.x - mser_result_Plates[j].getPlatePos().center.x) < 150 && abs(mser_result_Plates[i].getPlatePos().center.y - mser_result_Plates[j].getPlatePos().center.y) < 150)
					{
						if (mser_result_Plates[i].m_svmScore < mser_result_Plates[j].m_svmScore)
						{
							//�����Ż����Կ��Ǵ����¼����������
							//1.���svmѵ��ģ�;���
							//2.��mser��λǿ���Ӹ�����Ϊ�ж�
                            //3.��mser������������С��Ϊ�ж�
						/*	vector<string>plateLicense_tmp;
							CCharsRecognise cRecognise_tmp;
							string pLicense_tmp;

							cRecognise_tmp.charsRecognise(mser_result_Plates[i], pLicense_tmp);
							plateLicense_tmp.push_back(pLicense_tmp);*/

							mser.push_back(j);//sobel�д�����Ҫ��sobel_result_Plates��ɾ��������
							//after.push_back(i); //after�д�����Ҫ��all_result_Plates����ӵ�����
						}
					}
				}

			}
		}

		for (int i = 0; i < mser_result_Plates.size(); i++)
		{
			int flag = 0;
			for (int j = 0; j < mser.size(); j++)
			{
				if (i == mser[j])
					flag = 1;
			}
			if (flag == 0)
			{
				mser_result_Plates_tmp.push_back(mser_result_Plates[i]);
			}
		}
		mser_result_Plates.swap(vector<CPlate>());
		mser_result_Plates = mser_result_Plates_tmp;



		//std::cout << "Mser��λ�ĳ������ǣ� " << mser_result_Plates.size() << endl;

		for (size_t i = 0; i < mser_result_Plates.size(); i++)
		{
			CPlate plate = mser_result_Plates[i];
			cout << "Mser��λ����SVM���Ŷ�score�ǣ� " << plate.m_svmScore << endl;

			//RotatedRect minRect = plate.getPlatePos();
			//Point2f rect_points[4];
			//minRect.points(rect_points);  //��minRect�ĸ�����������rect_points��

			//isColorSobelafterMser.push_back(minRect);  //��RotateRect��ĳ����������vector��

			//for (int j = 0; j < 4; j++)
			//line(result, rect_points[j], rect_points[(j + 1) % 4], Scalar(0, 255, 0), 2, 8);
			//imshow("���ƶ�λ", result);  //��ʾ�ñ���߿�ѡ�ĳ�������
			//plate.setPlateLocateType(lpr::CMSER);


			all_result_Plates.push_back(plate);
		}


		/*
		//��ɫ��λ
		m_plateLocate->plateColorLocate(src, color_Plates, index);
		m_plateJudge->plateJudge(color_Plates, color_result_Plates);
		std::cout << "��ɫ��λ�ĳ������ǣ� " << color_result_Plates.size() << endl;

		//Sobel��λ
		m_plateLocate->plateSobelLocate(src, sobel_Plates, index);
		m_plateJudge->plateJudge(sobel_Plates, sobel_result_Plates);//svmģ���ж�sobel��λ�ĳ����ǲ��������ĳ���
		//��sobel���μ��������svm���Ŷ��ж�
		for (size_t i = 0; i < sobel_result_Plates.size(); i++)
		{
			for (size_t j = i + 1; j < sobel_result_Plates.size(); j++)
			{
				if (abs(sobel_result_Plates[i].getPlatePos().center.x - sobel_result_Plates[j].getPlatePos().center.x) < 150 && abs(sobel_result_Plates[i].getPlatePos().center.y - sobel_result_Plates[j].getPlatePos().center.y) < 150)
				{
					if (sobel_result_Plates[i].m_svmScore < sobel_result_Plates[j].m_svmScore)
					{
						sobel.push_back(j);//sobel�д�����Ҫ��sobel_result_Plates��ɾ��������
						//after.push_back(i); //after�д�����Ҫ��all_result_Plates����ӵ�����
					}
				}
			}
		}
		//����svm���ŶȾ��߽����ɾsobel_result_Plates��Ԫ��
		for (int i = 0; i < sobel.size(); i++)
		{
			//��ȡsobel_result_Plates��sobel[i]��Ԫ�ص�iterator
			vector<CPlate>::iterator   iter = sobel_result_Plates.begin() + (sobel[i]);
			//ɾ��sobel_result_Plates��sobel[i]��Ԫ��
			sobel_result_Plates.erase(iter);
		}
		//���Sobel��λ���ĳ��Ƹ���
		std::cout << "Sobel��λ���������ǣ� " << sobel_result_Plates.size() << endl;


		for (size_t i = 0; i < mser_result_Plates.size(); i++) 
		{
			CPlate plate = mser_result_Plates[i];
			cout << "Mser��λ����svm���Ŷ�score�ǣ� " << plate.m_svmScore << endl;
			
			//RotatedRect minRect = plate.getPlatePos();
			//Point2f rect_points[4];
			//minRect.points(rect_points);  //��minRect�ĸ�����������rect_points��

			//isColorSobelafterMser.push_back(minRect);  //��RotateRect��ĳ����������vector��

			//for (int j = 0; j < 4; j++)
				//line(result, rect_points[j], rect_points[(j + 1) % 4], Scalar(0, 255, 0), 2, 8);
			//imshow("���ƶ�λ", result);  //��ʾ�ñ���߿�ѡ�ĳ�������
			//plate.setPlateLocateType(lpr::CMSER);


			all_result_Plates.push_back(plate);
		}
		
		for (size_t i = 0; i < color_result_Plates.size();i++)
		{
			cout << "��ɫ��λ����svm���Ŷ�score�ǣ� " << color_result_Plates[i].m_svmScore << endl;
			all_result_Plates.push_back(color_result_Plates[i]);
		}

		for (size_t i = 0; i < sobel_result_Plates.size(); i++)
		{
			cout << "Sobel��λ����svm���Ŷ�score�ǣ� " << sobel_result_Plates[i].m_svmScore << endl;
			all_result_Plates.push_back(sobel_result_Plates[i]);
		}

		//��all_result_PlatesԪ�ؽ���svm���Ŷ��ж�
		for (size_t i = 0; i < all_result_Plates.size(); i++)
		{
			for (size_t j = i + 1; j < all_result_Plates.size(); j++)
			{
				if (abs(all_result_Plates[i].getPlatePos().center.x - all_result_Plates[j].getPlatePos().center.x) < 150 && abs(all_result_Plates[i].getPlatePos().center.y - all_result_Plates[j].getPlatePos().center.y) < 150)
				{
					if (all_result_Plates[i].m_svmScore < all_result_Plates[j].m_svmScore)
					{
						all.push_back(j);//all�д�����Ҫ��all_result_Plates��ɾ��������
						//after.push_back(i); //after�д�����Ҫ��all_result_Plates����ӵ�����
					}
				}
			}
		}
		//����svm���ŶȾ��߽����ɾall_result_Plates��Ԫ��
		for (int i = 0; i < all.size(); i++)
		{
			//��ȡall_result_Plates��all[i]��Ԫ�ص�iterator
			vector<CPlate>::iterator   iter = all_result_Plates.begin() + (all[i]);
			//ɾ��all_result_Plates��all[i]��Ԫ��
			all_result_Plates.erase(iter);
		}
		*/

		//���Mser������λ���ĳ�����
		//std::cout << "Mser������λ�ĳ������ǣ� " << isColorSobelafterMser.size() << endl;
		//���Mser��λ��������������
		//std::cout << isColorSobelafterMser[0].center.x << endl;
		//std::cout << isColorSobelafterMser[0].center.y << endl;

		//if (mser_result_Plates.size() >= mser_find_max)
		//{
		//	//���mser�����ҵ�mser_find_max�����ϣ�����mser_find_max�����ĳ��ƣ��Ͳ��ٽ�����ɫ�����ˡ�
		//}

		
		//else
		//{

/***************************************************************************************************************************************/
		
      //������ɫ��λ
			m_plateLocate->plateColorLocate(src, color_Plates, index);
			m_plateJudge->plateJudge(color_Plates, color_result_Plates);

			for (int i = 0; i < color_result_Plates.size(); i++)
			{
				//cout << "���Ŷȣ� " << color_result_Plates[i].m_svmScore << endl;
			}

			//����ɫ���������svm���Ŷ��ж�
			for (size_t i = 0; i < color_result_Plates.size(); i++)
			{
				for (size_t j = 0; j < color_result_Plates.size(); j++)
				{
					if (j == i)
						continue;
					else
					{
						if (abs(color_result_Plates[i].getPlatePos().center.x - color_result_Plates[j].getPlatePos().center.x) < 150 && abs(color_result_Plates[i].getPlatePos().center.y - color_result_Plates[j].getPlatePos().center.y) < 150)
						{
							if (color_result_Plates[i].m_svmScore < color_result_Plates[j].m_svmScore)
							{
								color.push_back(j);//color�д�����Ҫ��color_result_Plates��ɾ��������
								//after.push_back(i); //after�д�����Ҫ��all_result_Plates����ӵ�����
							}
						}
					}

				}
			}

			for (int i = 0; i < color_result_Plates.size(); i++)
			{
				int flag = 0;
				for (int j = 0; j < color.size(); j++)
				{
					if (i == color[j])
						flag = 1;
				}
				if (flag == 0)
				{
					color_result_Plates_tmp.push_back(color_result_Plates[i]);
				}
			}
			color_result_Plates.swap(vector<CPlate>());
			color_result_Plates = color_result_Plates_tmp;


			//�����ɫ��λ���ĳ��Ƹ���
			//std::cout << "��ɫ��λ�ĳ������ǣ� " << color_result_Plates.size() << endl;

			if (all_result_Plates.size())
			{
				//ʹ��svm���ŶȽ��о���
				for (size_t i = 0; i < color_result_Plates.size(); i++)
				{
					int flag = 0;
					cout << "HSV��ɫ��λ����SVM���Ŷ�score�ǣ� " << color_result_Plates[i].m_svmScore << endl;
					for (size_t j = 0; j < all_result_Plates.size(); j++)
					{
						if (abs(color_result_Plates[i].getPlatePos().center.x - all_result_Plates[j].getPlatePos().center.x)<150 && abs(color_result_Plates[i].getPlatePos().center.y - all_result_Plates[j].getPlatePos().center.y) < 150)
						{
							flag = 1;
							//���color��λ����svm���Ŷȸ���mser��λ����svm���Ŷ�(���ŶȾ�Ϊ����������ֵԽС��ʾ���Ŷ�Խ��)
							//����Ҫɾ��mser��ӦԪ�أ�����color��ӦԪ��
							//
							if (color_result_Plates[i].m_svmScore < all_result_Plates[j].m_svmScore)
							{
								before.push_back(j);//before�д�����Ҫ��all_result_Plates��ɾ��������
								after.push_back(i); //after�д�����Ҫ��all_result_Plates����ӵ�����
							}
						}
					}
					if (!flag)
					{
						after.push_back(i);
					}

				}

				//cout << all_result_Plates.size() << endl;
				//cout << before.size() << endl;
				/*for (int i = 0; i < before.size();i++)
				{
					cout << before[i] << endl;
				}*/
				//����svm���ŶȾ��߽����ɾall_result_Plates��Ԫ��
				//ɾ�����������Ԫ�����ŵ�����
				//���ø�ֵ�ķ���

				for (int i = 0; i<all_result_Plates.size();i++)
				{
					int flag=0;
					for (int j = 0; j<before.size();j++)
					{
						if (i == before[j])
							flag = 1;
					}
					if (flag==0)
					{
						all_result_Plates_tmp.push_back(all_result_Plates[i]);
					}
				}

				//for (int i = before.size()-1; i > -1; i--)
				//{
				//	//cout << "ssss" << endl;
				//	//��ȡall_result_Plates��before[i]��Ԫ�ص�iterator
				//	vector<CPlate>::iterator   iter = all_result_Plates.begin() + (before[i]);
				//	//ɾ��all_result_Plates��before[i]��Ԫ��
				//	all_result_Plates.erase(iter);
				//}
				for (int i = 0; i < after.size(); i++)
				{
					//��after�д洢������ֵ��Ӧ��color_result_Plates��Ԫ�ش���all_result_Plates��
					all_result_Plates_tmp.push_back(color_result_Plates[after[i]]);

				}
				
				//��all_result_Plates�������
				//��all_result_Plate_tmp���Ƹ�all_result_Plates
				all_result_Plates.swap(vector<CPlate>());
				all_result_Plates = all_result_Plates_tmp;

				//���before��after��all_result_Plates_tmp�������Ա��������ʹ��
				before.swap(vector<int>());
				after.swap(vector<int>());
				all_result_Plates_tmp.swap(vector<CPlate>());
			}
			else
			{
				for (size_t i = 0; i < color_result_Plates.size();i++)
				{
					all_result_Plates.push_back(color_result_Plates[i]);
					cout << "HSV��ɫ��λ����SVM���Ŷ�score�ǣ� " << color_result_Plates[i].m_svmScore << endl;
				}
			}
			//for (size_t i = 0; i < color_result_Plates.size(); i++)
			//{
			//	m_accumulator = 0;

			//	CPlate plate = color_result_Plates[i];

			//	RotatedRect minRect = plate.getPlatePos();
			//	Point2f rect_points[4];
			//	minRect.points(rect_points);

			//	//�����ɫ��λ������������
			//	//std::cout << minRect.center.x << endl;
			//	//std::cout << minRect.center.y << endl;
			//	for (int j = 0; j < isColorSobelafterMser.size(); j++)
			//	{

			//		//�����ɫ��mser���ͬһ�����ƣ����������ľ���Ͻ����趨150Ϊ��ֵ
			//		//��ֵѡȡ�ܵ��ۣ���ʱ��Mser��λ�ĳ��Ʋ���������������ɫ����sobel��λ�ĺ�����������Ŀǰ�Ĳ���ֻ������ѡ��Mser�ķ���
			//		//ע��Ҫ�þ���ֵ��
			//		if (abs(minRect.center.x - isColorSobelafterMser[j].center.x)>150|| abs(minRect.center.y - isColorSobelafterMser[j].center.y) > 150)
			//		{

			//			m_accumulator++;
			//		}

			//	}
			//	//����ۼ���ֵ
			//	//std::cout << "�ۼ���ֵ�� " << m_accumulator << endl;

			//	if (m_accumulator == isColorSobelafterMser.size())  //������ɫ��λ�ĳ����Ƿ���mser��λ�����غϣ����غϲŴ����������������
			//	{
			//		//isSobelafterSobel.push_back(minRect);
			//		//plate.bColored = false;
			//		plate.setPlateLocateType(lpr::COLOR);

			//		all_result_Plates.push_back(plate);

			//		for (int j = 0; j < 4; j++)
			//			line(result, rect_points[j], rect_points[(j + 1) % 4], Scalar(0, 0, 255), 2, 8);
			//	}

			//}

			//if (all_result_Plates.size() >= mser_find_max)
			//{
			//	//���mser+��ɫ�����ҵ�mser_find_max�����ϣ�����mser_find_max�����ĳ��ƣ��Ͳ��ٽ���sobel�����ˡ�
			//}
			
			//else
			//{

				//����Sobel��λ
				m_plateLocate->plateSobelLocate(src, sobel_Plates, index);
				m_plateJudge->plateJudge(sobel_Plates, sobel_result_Plates);//svmģ���ж�sobel��λ�ĳ����ǲ��������ĳ���

				
				//��sobel���μ��������svm���Ŷ��ж�
				for (size_t i = 0; i < sobel_result_Plates.size(); i++)
				{
					for (size_t j = 0; j < sobel_result_Plates.size(); j++)
					{
						if(j==i)
							continue;
						else
						{
							if (abs(sobel_result_Plates[i].getPlatePos().center.x - sobel_result_Plates[j].getPlatePos().center.x) < 150 && abs(sobel_result_Plates[i].getPlatePos().center.y - sobel_result_Plates[j].getPlatePos().center.y) < 150)
							{
								if (sobel_result_Plates[i].m_svmScore < sobel_result_Plates[j].m_svmScore)
								{
									sobel.push_back(j);//sobel�д�����Ҫ��sobel_result_Plates��ɾ��������
									//after.push_back(i); //after�д�����Ҫ��all_result_Plates����ӵ�����
								}
							}
						}
						
					}
				}

				for (int i = 0; i < sobel_result_Plates.size(); i++)
				{
					int flag = 0;
					for (int j = 0; j < sobel.size(); j++)
					{
						if (i == sobel[j])
							flag = 1;
					}
					if (flag == 0)
					{
						sobel_result_Plates_tmp.push_back(sobel_result_Plates[i]);
					}
				}
				sobel_result_Plates.swap(vector<CPlate>());
				sobel_result_Plates = sobel_result_Plates_tmp;

				////����svm���ŶȾ��߽����ɾsobel_result_Plates��Ԫ��
				//for (int i = sobel.size(); i < sobel.size(); i++)
				//{
				//	//��ȡsobel_result_Plates��sobel[i]��Ԫ�ص�iterator
				//	vector<CPlate>::iterator   iter = sobel_result_Plates.begin() + (sobel[i]);
				//	//ɾ��sobel_result_Plates��sobel[i]��Ԫ��
				//	sobel_result_Plates.erase(iter);
				//}
				
				//���Sobel��λ���ĳ��Ƹ���
				//std::cout << "Sobel��λ���������ǣ� " << sobel_result_Plates.size() << endl;

				if (all_result_Plates.size())
				{
					//ʹ��svm���ŶȽ��о���
					for (size_t i = 0; i < sobel_result_Plates.size(); i++)
					{
						int flag = 0;
						cout << "Sobel��λ����SVM���Ŷ�score�ǣ� " << sobel_result_Plates[i].m_svmScore << endl<<endl;
						for (size_t j = 0; j < all_result_Plates.size(); j++)
						{
							if (abs(sobel_result_Plates[i].getPlatePos().center.x - all_result_Plates[j].getPlatePos().center.x) < 150 && abs(sobel_result_Plates[i].getPlatePos().center.y - all_result_Plates[j].getPlatePos().center.y) < 150)
							{
								flag = 1;
								//���sobel��λ����svm���Ŷȸ���mser����ɫ��λ����svm���Ŷ�(���ŶȾ�Ϊ����������ֵԽС��ʾ���Ŷ�Խ��)
								//����Ҫɾ��mser��ӦԪ�أ�����sobel��ӦԪ��
								//
								if (sobel_result_Plates[i].m_svmScore < all_result_Plates[j].m_svmScore)
								{
									before.push_back(j);//before�д�����Ҫ��all_result_Plates��ɾ��������
									after.push_back(i); //after�д�����Ҫ��all_result_Plates����ӵ�����
								}
							}
						}
						if (!flag)
						{
							after.push_back(i);
						}
					}

					for (int i = 0; i < all_result_Plates.size(); i++)
					{
						int flag = 0;
						for (int j = 0; j < before.size(); j++)
						{
							if (i == before[j])
								flag = 1;
						}
						if (flag == 0)
						{
							all_result_Plates_tmp.push_back(all_result_Plates[i]);
						}
					}

					////����svm���ŶȾ��߽����ɾall_result_Plates��Ԫ��
					//for (int i = before.size()-1; i >-1; i--)
					//{
					//	//��ȡall_result_Plates��before[i]��Ԫ�ص�iterator
					//	vector<CPlate>::iterator   iter = all_result_Plates.begin() + (before[i]);
					//	//ɾ��all_result_Plates��before[i]��Ԫ��
					//	all_result_Plates.erase(iter);
					//}
					for (int i = 0; i < after.size(); i++)
					{
						//��after�д洢������ֵ��Ӧ��color_result_Plates��Ԫ�ش���all_result_Plates��
						all_result_Plates_tmp.push_back(sobel_result_Plates[after[i]]);

					}
					//��all_result_Plates�������
					//��all_result_Plate_tmp���Ƹ�all_result_Plates
					all_result_Plates.swap(vector<CPlate>());
					all_result_Plates = all_result_Plates_tmp;
				}
				else
				{
					for (size_t i = 0; i < sobel_result_Plates.size();i++)
					{
						all_result_Plates.push_back(sobel_result_Plates[i]);
						cout << "Sobel��λ����svm���Ŷ�score�ǣ� " << sobel_result_Plates[i].m_svmScore << endl;
					}
				}
				
/*************************************************************************************************************************************/
				//���Ʒ����ʾ���ƶ�λ����
				for (size_t i = 0; i < all_result_Plates.size(); i++)
				{
					CPlate plate = all_result_Plates[i];
					RotatedRect minRect = plate.getPlatePos();
					Point2f rect_points[4];
					minRect.points(rect_points);  //��minRect�ĸ�����������rect_points��

					if (plate.getPlateLocateType() == CMSER)
					{
						//�����Mser��λ�ĳ��ƣ������߻��Ʒ���
						for (int j = 0; j < 4; j++)
							line(result, rect_points[j], rect_points[(j + 1) % 4], Scalar(0, 255, 0), 2, 8);
					}
					else
					{
						if (plate.getPlateLocateType() == COLOR)
						{
							//�����Color��λ�ĳ��ƣ��ú��߻��Ʒ���
							for (int j = 0; j < 4; j++)
								line(result, rect_points[j], rect_points[(j + 1) % 4], Scalar(0, 0, 255), 2, 8);
						}
						else
						{
							//�����Sobel��λ�ĳ��ƣ��û��߻��Ʒ���
							for (int j = 0; j < 4; j++)
								line(result, rect_points[j], rect_points[(j + 1) % 4], Scalar(0, 255, 255), 2, 8);
						}
					}
				}


				

					//namedWindow("���ƶ�λ",WINDOW_NORMAL);
					//cv::imshow("���ƶ�λ", result);  //��ʾ�ñ���߿�ѡ�ĳ�������


					//imwrite("���ƶ�λ.jpg", result);
					for (size_t i = 0; i < all_result_Plates.size(); i++)
					{
						// �ѽ�ȡ�ĳ���ͼ�����ηŵ����Ͻ�
						CPlate plate = all_result_Plates[i];
						resultVec.push_back(plate);   //�ѽ������vector<CPlate>���resultVec��
					}
					return result;


		//		//�������,��mser��λ����ɫ��λ�����ս������������
		//		isColorSobelafterMser.swap(vector<RotatedRect>());
		//		for (size_t i = 0; i < all_result_Plates.size(); i++)
		//		{
		//			isColorSobelafterMser.push_back(all_result_Plates[i].getPlatePos());
		//		}

		//		//�������sobel���
		//		//����Sobel���ҿ���ͬһ�������ҵ����Σ����Բ�����Ҫ��mser+��ɫ��λ�Ľ��в��ش���
		//		//����Ҫ��ÿ��Sobel��λ�ĳ��ƽ��в���
		//		//����Ҫ�������������Ǹ�
		//		//********************************************************************************
		//		//����sobel��⵽��ҲӦ��ʹ��score���ŶȾ��ߣ�ѡ��score��С�ģ������Ƹ������ģ�
		//		//������ʱ���Ǳ����������
		//		//��Ϊ���ܵڶ���sobel���ڵ�һ�εĻ������ٽ��У����ܵڶ��ε���������ڵ�һ��
		//		m_plateLocate->plateSobelLocate(src, sobel_Plates, index);
		//		//cout << "Sobel�������ܳ��Ƹ����� " << sobel_Plates.size() << endl;
		//		m_plateJudge->plateJudge(sobel_Plates, sobel_result_Plates);//svmģ���ж�sobel��λ�ĳ����ǲ��������ĳ���

		//		//20170609���ӣ����ƺ�Ӧ�ôӶ���Sobel�㷨��Ѱ���α�����
		//		//Solel��λ����ڲ���⣬ȥ��ͬһ���Ƽ������ε����

		//		vector<int>p;//���sobel�ظ�����ͼ������

		//		for (size_t i = 0; i < sobel_result_Plates.size(); i++)
		//		{
		//			CPlate plate_i = sobel_result_Plates[i];

		//			for (size_t j = i + 1; j < sobel_result_Plates.size(); j++)
		//			{

		//				CPlate plate_j = sobel_result_Plates[j];

		//				//ͬһ����λ�ñ�������Σ���ζ������RotateRect��������x��y�������ܽӽ�������ȡ��ֵ150
		//				if ((plate_i.getPlatePos().center.x - plate_j.getPlatePos().center.x)<150
		//					&& (plate_i.getPlatePos().center.y - plate_j.getPlatePos().center.y)<150)
		//				{
		//					if (max(plate_i.getPlatePos().size.width, plate_i.getPlatePos().size.height) <
		//						max(plate_j.getPlatePos().size.width, plate_j.getPlatePos().size.height))
		//					{
		//						p.push_back(i);
		//					}
		//					else
		//						p.push_back(j);

		//				}

		//			}

		//		}
		//		//ɾ������p�м�¼������
		//		for (int i = 0; i < p.size(); i++)
		//		{
		//			//��ȡsobel_result_Plates��p[i]��Ԫ�ص�iterator
		//			vector<CPlate>::iterator   iter = sobel_result_Plates.begin() + (p[i]);
		//			//ɾ��sobel_result_Plates��p[i]��Ԫ��
		//			sobel_result_Plates.erase(iter);
		//		}

		//		//���Sobel��λ���ĳ��Ƹ���
		//		std::cout << "Sobel��λ���ĳ�����Ŀ�� " << sobel_result_Plates.size() << endl;
		//		for (size_t i = 0; i < sobel_result_Plates.size(); i++)
		//		{
		//			m_accumulator = 0;

		//			CPlate plate = sobel_result_Plates[i];

		//			RotatedRect minRect = plate.getPlatePos();
		//			Point2f rect_points[4];
		//			minRect.points(rect_points);

		//			for (int j = 0; j < isColorSobelafterMser.size(); j++)
		//			{
		//				//���mser+��ɫ��sobel���ͬһ�����ƣ����������ľ���Ͻ����趨150Ϊ��ֵ
		//				if (abs(minRect.center.x - isColorSobelafterMser[j].center.x)>150|| abs(minRect.center.y - isColorSobelafterMser[j].center.y)>150)
		//				{
		//					m_accumulator++;
		//				}

		//			}

		//			if (m_accumulator == isColorSobelafterMser.size())  //����sobel��λ�ĳ����Ƿ���mser+��ɫ��λ�����غϣ����غϲŴ����������������
		//			{
		//				plate.setPlateLocateType(lpr::SOBEL);

		//				all_result_Plates.push_back(plate);

		//				for (int j = 0; j < 4; j++)
		//					line(result, rect_points[j], rect_points[(j + 1) % 4], Scalar(0, 255, 255), 2, 8);
		//			}

		//		}
		//		
		//	}

		//}



			//for (size_t i = 0; i < color_result_Plates.size(); i++) 
			//{
			//	CPlate plate = color_result_Plates[i];

			//	RotatedRect minRect = plate.getPlatePos();
			//	Point2f rect_points[4];
			//	minRect.points(rect_points);  //��minRect�ĸ�����������rect_points��

			//	isSobelafterColor.push_back(minRect);  //��RotateRect��ĳ����������vector��

			//	for (int j = 0; j < 4; j++)

			//		line(result, rect_points[j], rect_points[(j + 1) % 4], Scalar(0, 255, 255), 2, 8);

			//	//imshow("���ƶ�λ", result);  //��ʾ�ñ���߿�ѡ�ĳ�������


			//	plate.setPlateLocateType(lpr::COLOR);
			//	all_result_Plates.push_back(plate);


			//}

		
	}




	int CPlateDetect::showResult(const Mat& result) {
		namedWindow("EasyPR", CV_WINDOW_AUTOSIZE);

		const int RESULTWIDTH = 640;   // 640 930
		const int RESULTHEIGHT = 540;  // 540 710

		Mat img_window;
		img_window.create(RESULTHEIGHT, RESULTWIDTH, CV_8UC3);

		int nRows = result.rows;
		int nCols = result.cols;

		Mat result_resize;
		if (nCols <= img_window.cols && nRows <= img_window.rows) {
			result_resize = result;

		}
		else if (nCols > img_window.cols && nRows <= img_window.rows) {
			float scale = float(img_window.cols) / float(nCols);
			resize(result, result_resize, Size(), scale, scale, CV_INTER_AREA);

		}
		else if (nCols <= img_window.cols && nRows > img_window.rows) {
			float scale = float(img_window.rows) / float(nRows);
			resize(result, result_resize, Size(), scale, scale, CV_INTER_AREA);

		}
		else if (nCols > img_window.cols && nRows > img_window.rows) {
			Mat result_middle;
			float scale = float(img_window.cols) / float(nCols);
			resize(result, result_middle, Size(), scale, scale, CV_INTER_AREA);

			if (result_middle.rows > img_window.rows) {
				float scale = float(img_window.rows) / float(result_middle.rows);
				resize(result_middle, result_resize, Size(), scale, scale, CV_INTER_AREA);

			}
			else {
				result_resize = result_middle;
			}
		}
		else {
			result_resize = result;
		}

		Mat imageRoi = img_window(Rect((RESULTWIDTH - result_resize.cols) / 2,
			(RESULTHEIGHT - result_resize.rows) / 2,
			result_resize.cols, result_resize.rows));
		addWeighted(imageRoi, 0, result_resize, 1, 0, imageRoi);

		imshow("EasyPR", img_window);
		waitKey();

		destroyWindow("EasyPR");

		return 0;
	}

} 

