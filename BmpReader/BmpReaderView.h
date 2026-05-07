// BmpReaderView.h
#pragma once
#include "ColorInfoDlg.h"
#include "HistogramDlg.h" 
#include "BlockSizeDlg.h"
#include "CKernelSizeDlg.h"
#define WM_USER_DESTROY_HIST  (WM_USER + 100)
#define WM_USER_DESTROY_ADAPTIVE  (WM_USER + 101)
class CBmpReaderDoc;

class CBmpReaderView : public CView
{
protected:
	CBmpReaderView();
	DECLARE_DYNCREATE(CBmpReaderView)

public:
	CBmpReaderDoc* GetDocument() const;

public:
	virtual void OnDraw(CDC* pDC);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

public:
	virtual ~CBmpReaderView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CColorInfoDlg* m_pColorDlg;

protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnProcessAdaptiveHistogram();
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnViewShowcolorDlg();
	afx_msg void OnUpdateViewShowcolorDlg(CCmdUI* pCmdUI);
	afx_msg void OnToolZoomIn();
	afx_msg void OnToolZoomOut();
	afx_msg void OnToolGray();
	afx_msg void OnProcessHistogram();
	afx_msg void OnProcessLinear();
	afx_msg void OnProcessEqualize();
	afx_msg void OnProcessSpecify();
	afx_msg void OnProcessPalette();
	afx_msg void OnProcessMeanFilter();
	afx_msg void OnProcessMedianFilter();
	afx_msg void OnProcessMaxFilter();

	// ========== 新增：噪声处理命令 ==========
	afx_msg void OnProcessSaltPepper();
	afx_msg void OnProcessImpulse();
	afx_msg void OnProcessGaussian();
	afx_msg void OnProcessWhiteGaussian();

	afx_msg LRESULT OnDestroyHistogramDlg(WPARAM, LPARAM);
	afx_msg LRESULT OnDestroyAdaptiveDlg(WPARAM, LPARAM);
	void UpdateHistogramWindow();
	void CloseHistogramWindow();
	void ApplyAdaptiveEqualize(int blockSize);

private:
	BOOL m_bShowColorDlg;
	CHistogramDlg* m_pHistogramDlg;
	CBlockSizeDlg* m_pAdaptiveHistoDlg;

protected:
	double m_zoomFactor;
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnPaletteDefault();
	afx_msg void OnPaletteHot();
	afx_msg void OnPaletteRainbow();
	afx_msg void OnPaletteCool();
	afx_msg void OnPaletteInvert();
};

#ifndef _DEBUG
inline CBmpReaderDoc* CBmpReaderView::GetDocument() const
{
	return reinterpret_cast<CBmpReaderDoc*>(m_pDocument);
}
#endif