
// LPR_MFCDlg.cpp : ʵ���ļ�
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

std::string imageOpenPath; //�洢���ļ����ͼƬ·��


//1.3��
vector<CPlate> resultVec;//ȫ�ֱ���������SVM�жϵĳ���ͼ�鼯��
vector<string> plateLicense;//����ʶ���ƺ�����

//�ɰ�
//vector<Mat> resultVec;//ȫ�ֱ���������SVM�жϵĳ���ͼ�鼯��
//vector<string> plateLicense;//����ʶ���ƺ�����




// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CLPR_MFCDlg �Ի���



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


// CLPR_MFCDlg ��Ϣ�������

BOOL CLPR_MFCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO:  �ڴ���Ӷ���ĳ�ʼ������
	    namedWindow("ԭʼͼ");
		HWND hWnd = (HWND)cvGetWindowHandle("ԭʼͼ");
		HWND hParent = ::GetParent(hWnd);
		::SetParent(hWnd, GetDlgItem(IDC_SHOWIMG)->m_hWnd);
		::ShowWindow(hParent, SW_HIDE);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CLPR_MFCDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CLPR_MFCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


//��α���ע�͵�
//void CLPR_MFCDlg::OnBnClickedButton1()
//{
//	// TODO:  �ڴ���ӿؼ�֪ͨ����������
//}


void CLPR_MFCDlg::OnBnClickedOpenButton() //��ͼƬ����ʾ����ͼƬ·�����ص�ȫ�ֱ���imageOpenPath��
{
	//std::cout << "ddd" << std::endl;//�������ԣ�MFC����ͬʱ�����������д���

	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	// ���ù�����   
	//TCHAR szFilter[] = _T("�����ļ�(*.*) | *.* || ");
	// ������ļ��Ի���   

	m_Edit.SetWindowText(_T(" "));//��ձ༭��
	m_locateTime.SetWindowText(_T(" "));//��ձ༭��
	m_recognizeTime.SetWindowText(_T(" "));//��ձ༭��

	CFileDialog fileDlg(TRUE, _T("*.*"), NULL, 0, NULL, this);  //�����ù�������NULL
	CString strFilePath;

	
	// ��ʾ���ļ��Ի���   
	if (fileDlg.DoModal() == IDOK)
	{
		//���ͼƬ�ؼ�
		//CStatic* pStatic = (CStatic*)GetDlgItem(IDC_SHOWIMG);
		//pStatic->SetBitmap(NULL);

		


		// ���������ļ��Ի����ϵġ��򿪡���ť��  
		strFilePath = fileDlg.GetPathName();
		strFilePath.Replace(_T("//"), _T("////"));  // imread���ļ�·����˫б�ܣ�����������Ҫת�� 
	
		//unicode�ֽڹ�����CStringתstring
		USES_CONVERSION;
		std::string imagePath(W2A(strFilePath));
		imageOpenPath = imagePath;
		
		cv::Mat src = cv::imread(imageOpenPath);

		//�����ͼƬ
		//m_showImg.SetBitmap(NULL);
		//��ˢ�´���
		//this->RedrawWindow();

		Mat src_show = src.clone();
		/*namedWindow("ԭʼͼ");
		HWND hWnd = (HWND)cvGetWindowHandle("ԭʼͼ");
		HWND hParent = ::GetParent(hWnd);
		::SetParent(hWnd, GetDlgItem(IDC_SHOWIMG)->m_hWnd);
		::ShowWindow(hParent, SW_HIDE);*/

		//���²�����ȡͼ�οؼ��ߴ粢�Դ˸ı�ͼƬ�ߴ�    
		CRect rect;
		GetDlgItem(IDC_SHOWIMG)->GetClientRect(&rect);
		Rect dst(rect.left, rect.top, rect.right, rect.bottom);
		resize(src_show, src_show, cv::Size(rect.Width(), rect.Height()));


	    cv:; imshow("ԭʼͼ", src_show);

		//�������
		resultVec.swap(vector<CPlate>());   
		plateLicense.swap(vector<string>());   

		//imageOpenPath = strFilePath.GetBuffer(0); // CString����תstring���ͣ���Ϊimread���ܶ�ȡCString��,��ע��Ҫ�Ƚ������ַ����޸�Ϊ���ֽ� 

		//20160606�޸ģ��򿪰�ť�ص�����ֻ����ͼƬ·��,����ʾԭͼ
		//cv::Mat src = cv::imread(imageOpenPath);

		//cv::namedWindow("ԭʼͼ");
	 //   cv:; imshow("ԭʼͼ", src);

		////vector<Mat> resultVec;
		//matVec.swap(vector<Mat>());   //�������

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




void CLPR_MFCDlg::OnBnClickedSelectButton() //����mser+��ɫ+Sobel������λ��������
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	//�����������ʱ��
	m_locateTime.SetWindowText(_T(" "));//��ձ༭��
	clock_t start, end;
	//string locateTime;
	start = clock();
	m_Edit.SetWindowText(_T(" "));//��ձ༭��
	//1.3��
	//�������
	resultVec.swap(vector<CPlate>());
	plateLicense.swap(vector<string>());

	cv::Mat src = cv::imread(imageOpenPath);
	//cv::namedWindow("ԭʼͼ");
	//cv:; imshow("ԭʼͼ", src);
	Mat locateImg;

	CPlateDetect pDetect;
	locateImg= pDetect.plateDetect(src, resultVec, 0, 10);  //��ȡͨ��svm���ĳ��ƣ�����ԭͼsrc�����ͼ�е���ȷ���ƣ�����resultVec


	//���²�����ȡͼ�οؼ��ߴ粢�Դ˸ı�ͼƬ�ߴ�    
	CRect rect;
	GetDlgItem(IDC_SHOWIMG)->GetClientRect(&rect);
	Rect dst(rect.left, rect.top, rect.right, rect.bottom);
	resize(locateImg, locateImg, cv::Size(rect.Width(), rect.Height()));
	imshow("ԭʼͼ", locateImg);

	if (resultVec.size() == 0)
	{
		// ��ʾ��Ϣ�Ի���   
		INT_PTR nRes;
		nRes = MessageBox(_T("�޷���λ�����ƣ�\n������ѡ��ͼƬ��"), _T("��ʾ"), MB_ICONWARNING);
		std::cout << "û�з��ֳ��ƣ�" << std::endl;
		
	}
	else
	{
		
		std::cout << "���� " << resultVec.size() << " ������" << std::endl;
		
		int num = resultVec.size();
		for (int j = 0; j < num; j++)
		{
			Mat resultMat = resultVec[j].getPlateMat();

			//namedWindow("��ȡ����", WINDOW_NORMAL);
			//HWND hWnd = (HWND)cvGetWindowHandle("��ȡ����");
			//HWND hParent = ::GetParent(hWnd);
			//::SetParent(hWnd, GetDlgItem(IDC_SHOWIMG)->m_hWnd);
			//::ShowWindow(hParent, SW_HIDE);
			////���²�����ȡͼ�οؼ��ߴ粢�Դ˸ı�ͼƬ�ߴ�    
			//CRect rect;
			//GetDlgItem(IDC_SHOWIMG)->GetClientRect(&rect);
			//Rect dst(rect.left, rect.top, rect.right, rect.bottom);
			//resize(resultMat, resultMat, cv::Size(rect.Width(), rect.Height()));

			end = clock();
			//cout << "dd" << endl;

			//��ʾ��λ��ʱ
			ostringstream buffer;
			buffer << (float)(end - start) / CLOCKS_PER_SEC;
			string locateTime = buffer.str();
			locateTime += " s";
			CString clocateTime(locateTime.c_str());
			m_locateTime.SetWindowText(clocateTime);//�༭����ʾ��λʱ��
			cout << "��λ��ʱ�� " << (float)(end - start) / CLOCKS_PER_SEC << "s" << endl;

			//char locateTime_tmp[100];
			//sprintf_s(locateTime_tmp, "%f", float((end - start) / CLOCKS_PER_SEC));
			//string locateTime(locateTime_tmp);
			//locateTime = to_string(float((end - start) / CLOCKS_PER_SEC));
			
			

			//cv::imshow("��ȡ����", resultMat);  


			//CBitmap bitmap;  // CBitmap�������ڼ���λͼ   
			//HBITMAP hBmp;    // ����CBitmap���ص�λͼ�ľ�� 
			//bitmap.LoadBitmap(resultMat);  // ��λͼIDB_BITMAP1���ص�bitmap   
			//hBmp = (HBITMAP)bitmap.GetSafeHandle();  // ��ȡbitmap����λͼ�ľ��   
			//m_showImg.SetBitmap(resultMat);


			//waitKey(0);
		}
	}
	
	


	//************************�ɰ�*************************************************************
	//�������
	//resultVec.swap(vector<M>());   
	//plateLicense.swap(vector<string>());

	//cv::Mat src = cv::imread(imageOpenPath);
	////cv::namedWindow("ԭʼͼ");
 //   //cv:; imshow("ԭʼͼ", src);

	//CPlateDetect pDetect;
	//pDetect.plateDetectDeep(src, resultVec, 0, 10);  //��ȡͨ��svm���ĳ��ƣ�����ԭͼsrc�����ͼ�е���ȷ���ƣ�����resultVec


	//if (resultVec.size() == 0)
	//	std::cout << "û�з��ֳ��ƣ�" << std::endl;
	//else
	//{
	//	std::cout << "���� " << resultVec.size() << " ������" << std::endl;
	//	int num = resultVec.size();
	//	for (int j = 0; j < num; j++)
	//	{
	//		Mat resultMat = resultVec[j];
	//		cv::imshow("��ȡ����", resultMat);   //
	//		waitKey(0);
	//	}
	//}

	////20160606�޸ģ�������ɫ��λ
	///*
	//CPlateJudge pJudge;

	//pJudge.LoadModel("model/svm1.1.xml"); //���ز�ͬsvmģ��   ��Ҫ�޸�svm���룬��Ϊ1.0��ֱ������ͼ��Ϳɣ��������汾��Ҫ��ȡ������Ȼ��������

	//int result = pJudge.plateJudge(matVec, resultVec); 
	////if (0 != result)
	//	//std::cout << "û�з��ֳ��ƣ�" << std::endl;
	//if(resultVec.size()==0)
	//	std::cout << "û�з��ֳ��ƣ�" << std::endl;
	//else
	//{
	//	std::cout << "���� " << resultVec.size() << " ������" << std::endl;
	//	int num = resultVec.size();
	//	for (int j = 0; j < num;j++)
	//	{
	//		Mat resultMat = resultVec[j];
	//		cv::imshow("��ȡ����", resultMat);   //������ʾ������ƣ������ٽ��
	//		waitKey(0);
	//	}
	//}
	//*/
}




//�ַ�ʶ��1.0��Ч�������Ҫ�ο������汾
//�Ѿ�����Ϊ1.5��
void CLPR_MFCDlg::OnBnClickedRecognizeButton()  //�����ַ�ʶ��Ļص�����
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	//m_Edit.SetWindowText(_T(" "));//��ձ༭��
	//m_recognizeTime.SetWindowText(_T(" 11"));//��ձ༭��
	clock_t start,end;
	string recogniseTime="";

	start = clock();
	plateLicense.swap(vector<string>());
	
	CCharsRecognise cRecognise;
	//vector<string> plateLicense;//����ʶ���ƺ�����

	
	//1.5ʹ�ò���ΪCPlate��string�ĺ���charsRecognise
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
	m_recognizeTime.SetWindowText(crecognizeTime);//�༭����ʾʶ��ʱ��

	//recogniseTime = to_string(float((end - start) / CLOCKS_PER_SEC));
	//recogniseTime += " s";
	//CString crecogniseTime(recogniseTime.c_str());
	//m_recognizeTime.SetWindowText(crecogniseTime);//�༭����ʾ��λʱ��

	cout << "��λ��ʱ�� " << (float)(end - start) / CLOCKS_PER_SEC << "s" << endl;


	//1.3�汾
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
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	CDialogEx::OnOK();
}


void CLPR_MFCDlg::OnEnChangeEdit1()
{

	// TODO:  ����ÿؼ��� RICHEDIT �ؼ���������
	// ���ʹ�֪ͨ��������д CDialogEx::OnInitDialog()
	// ���������� CRichEditCtrl().SetEventMask()��
	// ͬʱ�� ENM_CHANGE ��־�������㵽�����С�

	// TODO:  �ڴ���ӿؼ�֪ͨ����������
}
