// ColorInfoDlg.h
#pragma once
#include "afxcmn.h"

// CColorInfoDlg 对话框
class CColorInfoDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CColorInfoDlg)

public:
	CColorInfoDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CColorInfoDlg();

	void SetInfo(int x, int y, COLORREF crManual, COLORREF crGetPixel);

	// 对话框数据
	enum { IDD = IDD_COLOR_INFO };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
protected:
	virtual BOOL OnInitDialog();   

private:
	BOOL m_bSizeAdjusted;   // 是否已经调整过窗口大小
public:
	afx_msg void OnEnChangeCoordText();
};