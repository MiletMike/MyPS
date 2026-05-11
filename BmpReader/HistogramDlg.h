// HistogramDlg.h
#pragma once
#include "resource.h"

class CHistogramDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CHistogramDlg)
public:
    CHistogramDlg(CWnd* pParent = NULL);
    virtual ~CHistogramDlg();

    void SetHistogramData(int* pHist, int maxCount);
    virtual BOOL OnInitDialog();
    virtual BOOL Create(UINT nIDTemplate, CWnd* pParentWnd);

    enum { IDD = IDD_HISTOGRAM_DLG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual void OnPaint();
    virtual void PostNcDestroy();

    int m_hist[256];
    int m_maxCount;

    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()
};