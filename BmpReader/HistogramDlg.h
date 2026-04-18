// HistogramDlg.h
#pragma once
#include "resource.h"

class CHistogramDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CHistogramDlg)

public:
	CHistogramDlg(CWnd* pParent = NULL);
	virtual ~CHistogramDlg();

	// 设置直方图数据，并刷新显示
	void SetHistogramData(int* pHist, int maxCount);
	virtual BOOL OnInitDialog();

	// +++ 非模态创建函数
	virtual BOOL Create(UINT nIDTemplate, CWnd* pParentWnd);

	enum { IDD = IDD_HISTOGRAM_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual void OnPaint();
	virtual void PostNcDestroy();   // +++ 非模态窗口必须重载

	int m_hist[256];
	int m_maxCount;
	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()
};