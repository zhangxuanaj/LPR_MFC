
// LPR_MFCDlg.cpp : 实现文件
//
#include "stdafx.h"

#include "LPR_MFC.h"
#include "LPR_MFCDlg.h"
#include "afxdialogex.h"
#include <string>
#include <opencv2/opencv.hpp>

#include "../include/plate_locate.h"
#include "../include/plate_judge.h"
#include "../include/chars_segment.h"
#include "../include/chars_identify.h"

#include "../include/plate_detect.h"
#include "../include/chars_recognise.h"




#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace lpr;

std::string imageOpenPath; //存储打开文件后的图片路径


//1.3版
vector<CPlate> resultVec;//全局变量，经过SVM判断的车牌图块集合
vector<string> plateLicense;//最终识别车牌号容器

//旧版
//vector<Mat> resultVec;//全局变量，经过SVM判断的车牌图块集合
//vector<string> plateLicense;//最终识别车牌号容器




// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CLPR_MFCDlg 对话框



CLPR_MFCDlg::CLPR_MFCDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CLPR_MFCDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CLPR_MFCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_Edit);
	DDX_Control(pDX, IDC_SHOWIMG, m_showImg);
	DDX_Control(pDX, IDC_EDIT2, m_locateTime);
	DDX_Control(pDX, IDC_EDIT3, m_recognizeTime);
}

BEGIN_MESSAGE_MAP(CLPR_MFCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
//	ON_BN_CLICKED(IDC_BUTTON1, &CLPR_MFCDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_OPEN_BUTTON, &CLPR_MFCDlg::OnBnClickedOpenButton)
	ON_BN_CLICKED(IDC_SELECT_BUTTON, &CLPR_MFCDlg::OnBnClickedSelectButton)
	ON_BN_CLICKED(IDC_RECOGNIZE_BUTTON, &CLPR_MFCDlg::OnBnClickedRecognizeButton)
	ON_BN_CLICKED(IDOK, &CLPR_MFCDlg::OnBnClickedOk)
	ON_EN_CHANGE(IDC_EDIT1, &CLPR_MFCDlg::OnEnChangeEdit1)
END_MESSAGE_MAP()


// CLPR_MFCDlg 消息处理程序

BOOL CLPR_MFCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码
	    namedWindow("原始图");
		HWND hWnd = (HWND)cvGetWindowHandle("原始图");
		HWND hParent = ::GetParent(hWnd);
		::SetParent(hWnd, GetDlgItem(IDC_SHOWIMG)->m_hWnd);
		::ShowWindow(hParent, SW_HIDE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CLPR_MFCDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CLPR_MFCDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CLPR_MFCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


//这段必须注释掉
//void CLPR_MFCDlg::OnBnClickedButton1()
//{
//	// TODO:  在此添加控件通知处理程序代码
//}


void CLPR_MFCDlg::OnBnClickedOpenButton() //打开图片并显示，将图片路径加载到全局变量imageOpenPath中
{
	//std::cout << "ddd" << std::endl;//用来调试，MFC工程同时开启了命令行窗口

	// TODO:  在此添加控件通知处理程序代码
	// 设置过滤器   
	//TCHAR szFilter[] = _T("所有文件(*.*) | *.* || ");
	// 构造打开文件对话框   

	m_Edit.SetWindowText(_T(" "));//清空编辑框
	m_locateTime.SetWindowText(_T(" "));//清空编辑框
	m_recognizeTime.SetWindowText(_T(" "));//清空编辑框

	CFileDialog fileDlg(TRUE, _T("*.*"), NULL, 0, NULL, this);  //不设置过滤器，NULL
	CString strFilePath;

	
	// 显示打开文件对话框   
	if (fileDlg.DoModal() == IDOK)
	{
		//清空图片控件
		//CStatic* pStatic = (CStatic*)GetDlgItem(IDC_SHOWIMG);
		//pStatic->SetBitmap(NULL);

		


		// 如果点击了文件对话框上的“打开”按钮，  
		strFilePath = fileDlg.GetPathName();
		strFilePath.Replace(_T("//"), _T("////"));  // imread打开文件路径是双斜杠，所以这里需要转换 
	
		//unicode字节工程下CString转string
		USES_CONVERSION;
		std::string imagePath(W2A(strFilePath));
		imageOpenPath = imagePath;
		
		cv::Mat src = cv::imread(imageOpenPath);

		//先清空图片
		//m_showImg.SetBitmap(NULL);
		//再刷新窗体
		//this->RedrawWindow();

		Mat src_show = src.clone();
		/*namedWindow("原始图");
		HWND hWnd = (HWND)cvGetWindowHandle("原始图");
		HWND hParent = ::GetParent(hWnd);
		::SetParent(hWnd, GetDlgItem(IDC_SHOWIMG)->m_hWnd);
		::ShowWindow(hParent, SW_HIDE);*/

		//以下操作获取图形控件尺寸并以此改变图片尺寸    
		CRect rect;
		GetDlgItem(IDC_SHOWIMG)->GetClientRect(&rect);
		Rect dst(rect.left, rect.top, rect.right, rect.bottom);
		resize(src_show, src_show, cv::Size(rect.Width(), rect.Height()));


	    cv:; imshow("原始图", src_show);

		//清空容器
		resultVec.swap(vector<CPlate>());   
		plateLicense.swap(vector<string>());   

		//imageOpenPath = strFilePath.GetBuffer(0); // CString类型转string类型，因为imread不能读取CString类,但注意要先将工程字符集修改为多字节 

		//20160606修改，打开按钮回调函数只加载图片路径,并显示原图
		//cv::Mat src = cv::imread(imageOpenPath);

		//cv::namedWindow("原始图");
	 //   cv:; imshow("原始图", src);

		////vector<Mat> resultVec;
		//matVec.swap(vector<Mat>());   //清空容器

		//CPlateLocate plate;

		//int result = plate.plateLocate(src, matVec);
		/*if (result == 0)
		{
		int num = matVec.size();
		for (int j = 0; j < num; j++)
		{
		Mat resultMat = matVec[j];
		imshow("plate_locate", resultMat);
		waitKey(0);
		}
		}*/
	}
}




void CLPR_MFCDlg::OnBnClickedSelectButton() //利用mser+颜色+Sobel方法定位车牌区域
{
	// TODO:  在此添加控件通知处理程序代码
	//计算程序运行时间
	m_locateTime.SetWindowText(_T(" "));//清空编辑框
	clock_t start, end;
	//string locateTime;
	start = clock();
	m_Edit.SetWindowText(_T(" "));//清空编辑框
	//1.3版
	//清空容器
	resultVec.swap(vector<CPlate>());
	plateLicense.swap(vector<string>());

	cv::Mat src = cv::imread(imageOpenPath);
	//cv::namedWindow("原始图");
	//cv:; imshow("原始图", src);
	Mat locateImg;

	CPlateDetect pDetect;
	locateImg= pDetect.plateDetect(src, resultVec, 0, 10);  //提取通过svm检测的车牌，输入原图src，输出图中的正确车牌，存入resultVec


	//以下操作获取图形控件尺寸并以此改变图片尺寸    
	CRect rect;
	GetDlgItem(IDC_SHOWIMG)->GetClientRect(&rect);
	Rect dst(rect.left, rect.top, rect.right, rect.bottom);
	resize(locateImg, locateImg, cv::Size(rect.Width(), rect.Height()));
	imshow("原始图", locateImg);

	if (resultVec.size() == 0)
	{
		// 显示消息对话框   
		INT_PTR nRes;
		nRes = MessageBox(_T("无法定位到车牌！\n请重新选择图片！"), _T("提示"), MB_ICONWARNING);
		std::cout << "没有发现车牌！" << std::endl;
		
	}
	else
	{
		
		std::cout << "发现 " << resultVec.size() << " 个车牌" << std::endl;
		
		int num = resultVec.size();
		for (int j = 0; j < num; j++)
		{
			Mat resultMat = resultVec[j].getPlateMat();

			//namedWindow("提取车牌", WINDOW_NORMAL);
			//HWND hWnd = (HWND)cvGetWindowHandle("提取车牌");
			//HWND hParent = ::GetParent(hWnd);
			//::SetParent(hWnd, GetDlgItem(IDC_SHOWIMG)->m_hWnd);
			//::ShowWindow(hParent, SW_HIDE);
			////以下操作获取图形控件尺寸并以此改变图片尺寸    
			//CRect rect;
			//GetDlgItem(IDC_SHOWIMG)->GetClientRect(&rect);
			//Rect dst(rect.left, rect.top, rect.right, rect.bottom);
			//resize(resultMat, resultMat, cv::Size(rect.Width(), rect.Height()));

			end = clock();
			//cout << "dd" << endl;

			//显示定位用时
			ostringstream buffer;
			buffer << (float)(end - start) / CLOCKS_PER_SEC;
			string locateTime = buffer.str();
			locateTime += " s";
			CString clocateTime(locateTime.c_str());
			m_locateTime.SetWindowText(clocateTime);//编辑框显示定位时间
			cout << "定位用时： " << (float)(end - start) / CLOCKS_PER_SEC << "s" << endl;

			//char locateTime_tmp[100];
			//sprintf_s(locateTime_tmp, "%f", float((end - start) / CLOCKS_PER_SEC));
			//string locateTime(locateTime_tmp);
			//locateTime = to_string(float((end - start) / CLOCKS_PER_SEC));
			
			

			//cv::imshow("提取车牌", resultMat);  


			//CBitmap bitmap;  // CBitmap对象，用于加载位图   
			//HBITMAP hBmp;    // 保存CBitmap加载的位图的句柄 
			//bitmap.LoadBitmap(resultMat);  // 将位图IDB_BITMAP1加载到bitmap   
			//hBmp = (HBITMAP)bitmap.GetSafeHandle();  // 获取bitmap加载位图的句柄   
			//m_showImg.SetBitmap(resultMat);


			//waitKey(0);
		}
	}
	
	


	//************************旧版*************************************************************
	//清空容器
	//resultVec.swap(vector<M>());   
	//plateLicense.swap(vector<string>());

	//cv::Mat src = cv::imread(imageOpenPath);
	////cv::namedWindow("原始图");
 //   //cv:; imshow("原始图", src);

	//CPlateDetect pDetect;
	//pDetect.plateDetectDeep(src, resultVec, 0, 10);  //提取通过svm检测的车牌，输入原图src，输出图中的正确车牌，存入resultVec


	//if (resultVec.size() == 0)
	//	std::cout << "没有发现车牌！" << std::endl;
	//else
	//{
	//	std::cout << "发现 " << resultVec.size() << " 个车牌" << std::endl;
	//	int num = resultVec.size();
	//	for (int j = 0; j < num; j++)
	//	{
	//		Mat resultMat = resultVec[j];
	//		cv::imshow("提取车牌", resultMat);   //
	//		waitKey(0);
	//	}
	//}

	////20160606修改，加入颜色定位
	///*
	//CPlateJudge pJudge;

	//pJudge.LoadModel("model/svm1.1.xml"); //加载不同svm模型   需要修改svm输入，因为1.0版直接输入图像就可，而后续版本需要提取特征，然后再输入

	//int result = pJudge.plateJudge(matVec, resultVec); 
	////if (0 != result)
	//	//std::cout << "没有发现车牌！" << std::endl;
	//if(resultVec.size()==0)
	//	std::cout << "没有发现车牌！" << std::endl;
	//else
	//{
	//	std::cout << "发现 " << resultVec.size() << " 个车牌" << std::endl;
	//	int num = resultVec.size();
	//	for (int j = 0; j < num;j++)
	//	{
	//		Mat resultMat = resultVec[j];
	//		cv::imshow("提取车牌", resultMat);   //不能显示多个车牌，后续再解决
	//		waitKey(0);
	//	}
	//}
	//*/
}




//字符识别1.0版效果极差，需要参考后续版本
//已经提升为1.5版
void CLPR_MFCDlg::OnBnClickedRecognizeButton()  //车牌字符识别的回调函数
{
	// TODO:  在此添加控件通知处理程序代码
	//m_Edit.SetWindowText(_T(" "));//清空编辑框
	//m_recognizeTime.SetWindowText(_T(" 11"));//清空编辑框
	clock_t start,end;
	string recogniseTime="";

	start = clock();
	plateLicense.swap(vector<string>());
	
	CCharsRecognise cRecognise;
	//vector<string> plateLicense;//最终识别车牌号容器

	
	//1.5使用参数为CPlate、string的函数charsRecognise
	for (size_t i = 0; i < resultVec.size(); i++)
	{
		string pLicense;
		cRecognise.charsRecognise(resultVec[i], pLicense);
		cout << pLicense << endl;
		plateLicense.push_back(pLicense);
		
	}

	string allpLicense = plateLicense.at(0)+"       ";
	

	for (int i = 1; i < plateLicense.size();i++)
	{
		allpLicense = allpLicense + plateLicense.at(i) + "       ";
	}
	
	CString callpLicense(allpLicense.c_str());
	
	m_Edit.SetWindowText(callpLicense);     
	
	end = clock();

	ostringstream buffer;
	buffer << (float)(end - start) / CLOCKS_PER_SEC;
	string recognizeTime = buffer.str();
	recognizeTime += " s";
	CString crecognizeTime(recognizeTime.c_str());
	m_recognizeTime.SetWindowText(crecognizeTime);//编辑框显示识别时间

	//recogniseTime = to_string(float((end - start) / CLOCKS_PER_SEC));
	//recogniseTime += " s";
	//CString crecogniseTime(recogniseTime.c_str());
	//m_recognizeTime.SetWindowText(crecogniseTime);//编辑框显示定位时间

	cout << "定位用时： " << (float)(end - start) / CLOCKS_PER_SEC << "s" << endl;


	//1.3版本
	/*for (size_t i = 0; i < resultVec.size();i++)
	{
		string pLicense;
		cRecognise.charsRecognise(resultVec[i].getPlateMat(), pLicense);
		cout << pLicense << endl;
		plateLicense.push_back(pLicense);
	}*/
	
}


void CLPR_MFCDlg::OnBnClickedOk()
{
	// TODO:  在此添加控件通知处理程序代码
	CDialogEx::OnOK();
}


void CLPR_MFCDlg::OnEnChangeEdit1()
{

	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}
