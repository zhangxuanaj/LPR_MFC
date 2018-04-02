
// LPR_MFCDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// CLPR_MFCDlg �Ի���
class CLPR_MFCDlg : public CDialogEx
{
// ����
public:
	CLPR_MFCDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_LPR_MFC_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedOpenButton();
	afx_msg void OnBnClickedSelectButton();
	afx_msg void OnBnClickedRecognizeButton();
	afx_msg void OnBnClickedOk();
	afx_msg void OnEnChangeEdit1();
	CEdit m_Edit;
	CStatic m_showImg;
	CEdit m_locateTime;
	CEdit m_recognizeTime;
};
