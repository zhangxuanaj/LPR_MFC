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

		// 默认EasyPR在一幅图中定位最多10个车牌
		m_maxPlates = 10;

		isColorSobelafterMser.swap(vector<RotatedRect>());   //清空容器
		m_accumulator = 0;  //累加器置零
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
		vector<CPlate> all_result_Plates_tmp;//临时容器

		//如果mser查找找到m_maxPlates个以上（包含m_maxPlates个）的车牌，就不再进行颜色查找了。
		const int mser_find_max = m_maxPlates;

		Mat result;
		src.copyTo(result);

		//存放svm置信度比较过程中用到的容器索引
	    vector<int>before;
		vector<int>after;
		vector<int>mser;
		vector<int>color;
		vector<int>sobel;
		//vector<int>all;
		//首先利用MSER进行文字定位
		//Mser方法可能不能完整车牌区域提取出来，但是颜色可能定位更准，所以目前筛选策略（优先顺序：Mser>颜色>Sobel）可能需要修改
		//EasyPR中非极大值抑制可能能解决这个问题

		//Mser定位
		m_plateLocate->plateMserLocate(src, mser_Plates, index);
		m_plateJudge->plateJudge(mser_Plates, mser_result_Plates);
		for (int i = 0; i < mser_result_Plates.size();i++)
		{
			//cout << "置信度： " << mser_result_Plates[i].m_svmScore << endl;
		}
		//对mser检测结果进行svm置信度判断
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
							//后续优化可以考虑从以下几个方面进行
							//1.提高svm训练模型精度
							//2.以mser定位强种子个数作为判断
                            //3.以mser检出车牌区域大小作为判断
						/*	vector<string>plateLicense_tmp;
							CCharsRecognise cRecognise_tmp;
							string pLicense_tmp;

							cRecognise_tmp.charsRecognise(mser_result_Plates[i], pLicense_tmp);
							plateLicense_tmp.push_back(pLicense_tmp);*/

							mser.push_back(j);//sobel中存着需要从sobel_result_Plates中删除的索引
							//after.push_back(i); //after中存着需要往all_result_Plates中添加的索引
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



		//std::cout << "Mser定位的车牌数是： " << mser_result_Plates.size() << endl;

		for (size_t i = 0; i < mser_result_Plates.size(); i++)
		{
			CPlate plate = mser_result_Plates[i];
			cout << "Mser定位车牌SVM置信度score是： " << plate.m_svmScore << endl;

			//RotatedRect minRect = plate.getPlatePos();
			//Point2f rect_points[4];
			//minRect.points(rect_points);  //将minRect四个点的坐标存入rect_points中

			//isColorSobelafterMser.push_back(minRect);  //将RotateRect类的车牌区域存入vector中

			//for (int j = 0; j < 4; j++)
			//line(result, rect_points[j], rect_points[(j + 1) % 4], Scalar(0, 255, 0), 2, 8);
			//imshow("车牌定位", result);  //显示用标记线框选的车牌区域
			//plate.setPlateLocateType(lpr::CMSER);


			all_result_Plates.push_back(plate);
		}


		/*
		//颜色定位
		m_plateLocate->plateColorLocate(src, color_Plates, index);
		m_plateJudge->plateJudge(color_Plates, color_result_Plates);
		std::cout << "颜色定位的车牌数是： " << color_result_Plates.size() << endl;

		//Sobel定位
		m_plateLocate->plateSobelLocate(src, sobel_Plates, index);
		m_plateJudge->plateJudge(sobel_Plates, sobel_result_Plates);//svm模型判断sobel定位的车牌是不是真正的车牌
		//对sobel二次检测结果进行svm置信度判断
		for (size_t i = 0; i < sobel_result_Plates.size(); i++)
		{
			for (size_t j = i + 1; j < sobel_result_Plates.size(); j++)
			{
				if (abs(sobel_result_Plates[i].getPlatePos().center.x - sobel_result_Plates[j].getPlatePos().center.x) < 150 && abs(sobel_result_Plates[i].getPlatePos().center.y - sobel_result_Plates[j].getPlatePos().center.y) < 150)
				{
					if (sobel_result_Plates[i].m_svmScore < sobel_result_Plates[j].m_svmScore)
					{
						sobel.push_back(j);//sobel中存着需要从sobel_result_Plates中删除的索引
						//after.push_back(i); //after中存着需要往all_result_Plates中添加的索引
					}
				}
			}
		}
		//根据svm置信度决策结果增删sobel_result_Plates中元素
		for (int i = 0; i < sobel.size(); i++)
		{
			//获取sobel_result_Plates第sobel[i]个元素的iterator
			vector<CPlate>::iterator   iter = sobel_result_Plates.begin() + (sobel[i]);
			//删除sobel_result_Plates第sobel[i]个元素
			sobel_result_Plates.erase(iter);
		}
		//输出Sobel定位到的车牌个数
		std::cout << "Sobel定位出车牌数是： " << sobel_result_Plates.size() << endl;


		for (size_t i = 0; i < mser_result_Plates.size(); i++) 
		{
			CPlate plate = mser_result_Plates[i];
			cout << "Mser定位车牌svm置信度score是： " << plate.m_svmScore << endl;
			
			//RotatedRect minRect = plate.getPlatePos();
			//Point2f rect_points[4];
			//minRect.points(rect_points);  //将minRect四个点的坐标存入rect_points中

			//isColorSobelafterMser.push_back(minRect);  //将RotateRect类的车牌区域存入vector中

			//for (int j = 0; j < 4; j++)
				//line(result, rect_points[j], rect_points[(j + 1) % 4], Scalar(0, 255, 0), 2, 8);
			//imshow("车牌定位", result);  //显示用标记线框选的车牌区域
			//plate.setPlateLocateType(lpr::CMSER);


			all_result_Plates.push_back(plate);
		}
		
		for (size_t i = 0; i < color_result_Plates.size();i++)
		{
			cout << "颜色定位车牌svm置信度score是： " << color_result_Plates[i].m_svmScore << endl;
			all_result_Plates.push_back(color_result_Plates[i]);
		}

		for (size_t i = 0; i < sobel_result_Plates.size(); i++)
		{
			cout << "Sobel定位车牌svm置信度score是： " << sobel_result_Plates[i].m_svmScore << endl;
			all_result_Plates.push_back(sobel_result_Plates[i]);
		}

		//对all_result_Plates元素进行svm置信度判断
		for (size_t i = 0; i < all_result_Plates.size(); i++)
		{
			for (size_t j = i + 1; j < all_result_Plates.size(); j++)
			{
				if (abs(all_result_Plates[i].getPlatePos().center.x - all_result_Plates[j].getPlatePos().center.x) < 150 && abs(all_result_Plates[i].getPlatePos().center.y - all_result_Plates[j].getPlatePos().center.y) < 150)
				{
					if (all_result_Plates[i].m_svmScore < all_result_Plates[j].m_svmScore)
					{
						all.push_back(j);//all中存着需要从all_result_Plates中删除的索引
						//after.push_back(i); //after中存着需要往all_result_Plates中添加的索引
					}
				}
			}
		}
		//根据svm置信度决策结果增删all_result_Plates中元素
		for (int i = 0; i < all.size(); i++)
		{
			//获取all_result_Plates第all[i]个元素的iterator
			vector<CPlate>::iterator   iter = all_result_Plates.begin() + (all[i]);
			//删除all_result_Plates第all[i]个元素
			all_result_Plates.erase(iter);
		}
		*/

		//输出Mser方法定位到的车牌数
		//std::cout << "Mser方法定位的车牌数是： " << isColorSobelafterMser.size() << endl;
		//输出Mser定位到车牌中心坐标
		//std::cout << isColorSobelafterMser[0].center.x << endl;
		//std::cout << isColorSobelafterMser[0].center.y << endl;

		//if (mser_result_Plates.size() >= mser_find_max)
		//{
		//	//如果mser查找找到mser_find_max个以上（包含mser_find_max个）的车牌，就不再进行颜色查找了。
		//}

		
		//else
		//{

/***************************************************************************************************************************************/
		
      //进行颜色定位
			m_plateLocate->plateColorLocate(src, color_Plates, index);
			m_plateJudge->plateJudge(color_Plates, color_result_Plates);

			for (int i = 0; i < color_result_Plates.size(); i++)
			{
				//cout << "置信度： " << color_result_Plates[i].m_svmScore << endl;
			}

			//对颜色检测结果进行svm置信度判断
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
								color.push_back(j);//color中存着需要从color_result_Plates中删除的索引
								//after.push_back(i); //after中存着需要往all_result_Plates中添加的索引
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


			//输出颜色定位到的车牌个数
			//std::cout << "颜色定位的车牌数是： " << color_result_Plates.size() << endl;

			if (all_result_Plates.size())
			{
				//使用svm置信度进行决策
				for (size_t i = 0; i < color_result_Plates.size(); i++)
				{
					int flag = 0;
					cout << "HSV颜色定位车牌SVM置信度score是： " << color_result_Plates[i].m_svmScore << endl;
					for (size_t j = 0; j < all_result_Plates.size(); j++)
					{
						if (abs(color_result_Plates[i].getPlatePos().center.x - all_result_Plates[j].getPlatePos().center.x)<150 && abs(color_result_Plates[i].getPlatePos().center.y - all_result_Plates[j].getPlatePos().center.y) < 150)
						{
							flag = 1;
							//如果color定位方法svm置信度高于mser定位方法svm置信度(置信度均为负数，且数值越小表示置信度越高)
							//则需要删除mser对应元素，存入color对应元素
							//
							if (color_result_Plates[i].m_svmScore < all_result_Plates[j].m_svmScore)
							{
								before.push_back(j);//before中存着需要从all_result_Plates中删除的索引
								after.push_back(i); //after中存着需要往all_result_Plates中添加的索引
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
				//根据svm置信度决策结果增删all_result_Plates中元素
				//删除容器会造成元素重排的问题
				//采用赋值的方法

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
				//	//获取all_result_Plates第before[i]个元素的iterator
				//	vector<CPlate>::iterator   iter = all_result_Plates.begin() + (before[i]);
				//	//删除all_result_Plates第before[i]个元素
				//	all_result_Plates.erase(iter);
				//}
				for (int i = 0; i < after.size(); i++)
				{
					//将after中存储的索引值对应的color_result_Plates中元素存入all_result_Plates中
					all_result_Plates_tmp.push_back(color_result_Plates[after[i]]);

				}
				
				//将all_result_Plates容器清空
				//将all_result_Plate_tmp复制给all_result_Plates
				all_result_Plates.swap(vector<CPlate>());
				all_result_Plates = all_result_Plates_tmp;

				//清空before、after、all_result_Plates_tmp容器，以便后续重新使用
				before.swap(vector<int>());
				after.swap(vector<int>());
				all_result_Plates_tmp.swap(vector<CPlate>());
			}
			else
			{
				for (size_t i = 0; i < color_result_Plates.size();i++)
				{
					all_result_Plates.push_back(color_result_Plates[i]);
					cout << "HSV颜色定位车牌SVM置信度score是： " << color_result_Plates[i].m_svmScore << endl;
				}
			}
			//for (size_t i = 0; i < color_result_Plates.size(); i++)
			//{
			//	m_accumulator = 0;

			//	CPlate plate = color_result_Plates[i];

			//	RotatedRect minRect = plate.getPlatePos();
			//	Point2f rect_points[4];
			//	minRect.points(rect_points);

			//	//输出颜色定位车牌中心坐标
			//	//std::cout << minRect.center.x << endl;
			//	//std::cout << minRect.center.y << endl;
			//	for (int j = 0; j < isColorSobelafterMser.size(); j++)
			//	{

			//		//如果颜色与mser检出同一个车牌，则两者中心距离较近，设定150为阈值
			//		//阈值选取很蛋疼，有时候Mser定位的车牌不够完整，但是颜色或者sobel定位的很完整，但是目前的策略只能优先选择Mser的方法
			//		//注意要用绝对值！
			//		if (abs(minRect.center.x - isColorSobelafterMser[j].center.x)>150|| abs(minRect.center.y - isColorSobelafterMser[j].center.y) > 150)
			//		{

			//			m_accumulator++;
			//		}

			//	}
			//	//输出累加器值
			//	//std::cout << "累加器值： " << m_accumulator << endl;

			//	if (m_accumulator == isColorSobelafterMser.size())  //决策颜色定位的车牌是否与mser定位的相重合，不重合才存入最终输出容器中
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
			//	//如果mser+颜色查找找到mser_find_max个以上（包含mser_find_max个）的车牌，就不再进行sobel查找了。
			//}
			
			//else
			//{

				//进行Sobel定位
				m_plateLocate->plateSobelLocate(src, sobel_Plates, index);
				m_plateJudge->plateJudge(sobel_Plates, sobel_result_Plates);//svm模型判断sobel定位的车牌是不是真正的车牌

				
				//对sobel二次检测结果进行svm置信度判断
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
									sobel.push_back(j);//sobel中存着需要从sobel_result_Plates中删除的索引
									//after.push_back(i); //after中存着需要往all_result_Plates中添加的索引
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

				////根据svm置信度决策结果增删sobel_result_Plates中元素
				//for (int i = sobel.size(); i < sobel.size(); i++)
				//{
				//	//获取sobel_result_Plates第sobel[i]个元素的iterator
				//	vector<CPlate>::iterator   iter = sobel_result_Plates.begin() + (sobel[i]);
				//	//删除sobel_result_Plates第sobel[i]个元素
				//	sobel_result_Plates.erase(iter);
				//}
				
				//输出Sobel定位到的车牌个数
				//std::cout << "Sobel定位出车牌数是： " << sobel_result_Plates.size() << endl;

				if (all_result_Plates.size())
				{
					//使用svm置信度进行决策
					for (size_t i = 0; i < sobel_result_Plates.size(); i++)
					{
						int flag = 0;
						cout << "Sobel定位车牌SVM置信度score是： " << sobel_result_Plates[i].m_svmScore << endl<<endl;
						for (size_t j = 0; j < all_result_Plates.size(); j++)
						{
							if (abs(sobel_result_Plates[i].getPlatePos().center.x - all_result_Plates[j].getPlatePos().center.x) < 150 && abs(sobel_result_Plates[i].getPlatePos().center.y - all_result_Plates[j].getPlatePos().center.y) < 150)
							{
								flag = 1;
								//如果sobel定位方法svm置信度高于mser和颜色定位方法svm置信度(置信度均为负数，且数值越小表示置信度越高)
								//则需要删除mser对应元素，存入sobel对应元素
								//
								if (sobel_result_Plates[i].m_svmScore < all_result_Plates[j].m_svmScore)
								{
									before.push_back(j);//before中存着需要从all_result_Plates中删除的索引
									after.push_back(i); //after中存着需要往all_result_Plates中添加的索引
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

					////根据svm置信度决策结果增删all_result_Plates中元素
					//for (int i = before.size()-1; i >-1; i--)
					//{
					//	//获取all_result_Plates第before[i]个元素的iterator
					//	vector<CPlate>::iterator   iter = all_result_Plates.begin() + (before[i]);
					//	//删除all_result_Plates第before[i]个元素
					//	all_result_Plates.erase(iter);
					//}
					for (int i = 0; i < after.size(); i++)
					{
						//将after中存储的索引值对应的color_result_Plates中元素存入all_result_Plates中
						all_result_Plates_tmp.push_back(sobel_result_Plates[after[i]]);

					}
					//将all_result_Plates容器清空
					//将all_result_Plate_tmp复制给all_result_Plates
					all_result_Plates.swap(vector<CPlate>());
					all_result_Plates = all_result_Plates_tmp;
				}
				else
				{
					for (size_t i = 0; i < sobel_result_Plates.size();i++)
					{
						all_result_Plates.push_back(sobel_result_Plates[i]);
						cout << "Sobel定位车牌svm置信度score是： " << sobel_result_Plates[i].m_svmScore << endl;
					}
				}
				
/*************************************************************************************************************************************/
				//绘制方框标示车牌定位区域
				for (size_t i = 0; i < all_result_Plates.size(); i++)
				{
					CPlate plate = all_result_Plates[i];
					RotatedRect minRect = plate.getPlatePos();
					Point2f rect_points[4];
					minRect.points(rect_points);  //将minRect四个点的坐标存入rect_points中

					if (plate.getPlateLocateType() == CMSER)
					{
						//如果是Mser定位的车牌，用绿线绘制方框
						for (int j = 0; j < 4; j++)
							line(result, rect_points[j], rect_points[(j + 1) % 4], Scalar(0, 255, 0), 2, 8);
					}
					else
					{
						if (plate.getPlateLocateType() == COLOR)
						{
							//如果是Color定位的车牌，用红线绘制方框
							for (int j = 0; j < 4; j++)
								line(result, rect_points[j], rect_points[(j + 1) % 4], Scalar(0, 0, 255), 2, 8);
						}
						else
						{
							//如果是Sobel定位的车牌，用黄线绘制方框
							for (int j = 0; j < 4; j++)
								line(result, rect_points[j], rect_points[(j + 1) % 4], Scalar(0, 255, 255), 2, 8);
						}
					}
				}


				

					//namedWindow("车牌定位",WINDOW_NORMAL);
					//cv::imshow("车牌定位", result);  //显示用标记线框选的车牌区域


					//imwrite("车牌定位.jpg", result);
					for (size_t i = 0; i < all_result_Plates.size(); i++)
					{
						// 把截取的车牌图像依次放到左上角
						CPlate plate = all_result_Plates[i];
						resultVec.push_back(plate);   //把结果存入vector<CPlate>类的resultVec中
					}
					return result;


		//		//清空容器,将mser定位和颜色定位的最终结果存入容器中
		//		isColorSobelafterMser.swap(vector<RotatedRect>());
		//		for (size_t i = 0; i < all_result_Plates.size(); i++)
		//		{
		//			isColorSobelafterMser.push_back(all_result_Plates[i].getPlatePos());
		//		}

		//		//额外进行sobel检测
		//		//二次Sobel查找可能同一个车牌找到两次，所以不仅需要与mser+颜色定位的进行查重处理
		//		//还需要与每个Sobel定位的车牌进行查重
		//		//但是要保留方框最大的那个
		//		//********************************************************************************
		//		//两次sobel检测到的也应该使用score置信度决策，选择score最小的（即车牌概率最大的）
		//		//这里暂时还是保留方框最大
		//		//因为可能第二次sobel是在第一次的基础上再进行，可能第二次的区域包含于第一次
		//		m_plateLocate->plateSobelLocate(src, sobel_Plates, index);
		//		//cout << "Sobel检测出可能车牌个数： " << sobel_Plates.size() << endl;
		//		m_plateJudge->plateJudge(sobel_Plates, sobel_result_Plates);//svm模型判断sobel定位的车牌是不是真正的车牌

		//		//20170609增加，但似乎应该从二次Sobel算法中寻找治本方法
		//		//Solel定位结果内部检测，去除同一车牌检索两次的情况

		//		vector<int>p;//存放sobel重复检索图块索引

		//		for (size_t i = 0; i < sobel_result_Plates.size(); i++)
		//		{
		//			CPlate plate_i = sobel_result_Plates[i];

		//			for (size_t j = i + 1; j < sobel_result_Plates.size(); j++)
		//			{

		//				CPlate plate_j = sobel_result_Plates[j];

		//				//同一车牌位置被检测两次，意味着两个RotateRect中心坐标x和y分量都很接近，这里取阈值150
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
		//		//删除容器p中记录的索引
		//		for (int i = 0; i < p.size(); i++)
		//		{
		//			//获取sobel_result_Plates第p[i]个元素的iterator
		//			vector<CPlate>::iterator   iter = sobel_result_Plates.begin() + (p[i]);
		//			//删除sobel_result_Plates第p[i]个元素
		//			sobel_result_Plates.erase(iter);
		//		}

		//		//输出Sobel定位到的车牌个数
		//		std::cout << "Sobel定位出的车牌数目： " << sobel_result_Plates.size() << endl;
		//		for (size_t i = 0; i < sobel_result_Plates.size(); i++)
		//		{
		//			m_accumulator = 0;

		//			CPlate plate = sobel_result_Plates[i];

		//			RotatedRect minRect = plate.getPlatePos();
		//			Point2f rect_points[4];
		//			minRect.points(rect_points);

		//			for (int j = 0; j < isColorSobelafterMser.size(); j++)
		//			{
		//				//如果mser+颜色与sobel检出同一个车牌，则两者中心距离较近，设定150为阈值
		//				if (abs(minRect.center.x - isColorSobelafterMser[j].center.x)>150|| abs(minRect.center.y - isColorSobelafterMser[j].center.y)>150)
		//				{
		//					m_accumulator++;
		//				}

		//			}

		//			if (m_accumulator == isColorSobelafterMser.size())  //决策sobel定位的车牌是否与mser+颜色定位的相重合，不重合才存入最终输出容器中
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
			//	minRect.points(rect_points);  //将minRect四个点的坐标存入rect_points中

			//	isSobelafterColor.push_back(minRect);  //将RotateRect类的车牌区域存入vector中

			//	for (int j = 0; j < 4; j++)

			//		line(result, rect_points[j], rect_points[(j + 1) % 4], Scalar(0, 255, 255), 2, 8);

			//	//imshow("车牌定位", result);  //显示用标记线框选的车牌区域


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

