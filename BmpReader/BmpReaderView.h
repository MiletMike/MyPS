
// BmpReaderView.h : interface of the CBmpReaderView class
//

#pragma once
#include "ColorInfoDlg.h"
#include "HistogramDlg.h" 
#include "BlockSizeDlg.h"
#define WM_USER_DESTROY_HIST  (WM_USER + 100)
#define WM_USER_DESTROY_ADAPTIVE  (WM_USER + 101)
class CBmpReaderDoc;


class CBmpReaderView : public CView
{
protected: // create from serialization only
	CBmpReaderView();
	DECLARE_DYNCREATE(CBmpReaderView)

// Attributes
public:
	CBmpReaderDoc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CBmpReaderView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CColorInfoDlg* m_pColorDlg;  
// Generated message map functions
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
	afx_msg LRESULT OnDestroyHistogramDlg(WPARAM, LPARAM);
	afx_msg LRESULT OnDestroyAdaptiveDlg(WPARAM, LPARAM);
	void UpdateHistogramWindow();     // +++ 更新直方图窗口
	void CloseHistogramWindow();      // +++ 关闭直方图窗口
	void ApplyAdaptiveEqualize(int blockSize);   // 执行处理并刷新


private:
	BOOL m_bShowColorDlg;   // 是否显示颜色信息窗口
	CHistogramDlg* m_pHistogramDlg;   // +++ 非模态直方图窗口指针
	CBlockSizeDlg* m_pAdaptiveHistoDlg;          // 使用改造后的类
protected:
	double m_zoomFactor;   // 缩放倍数，1.0为原始大小
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnPaletteDefault();
	afx_msg void OnPaletteHot();
	afx_msg void OnPaletteRainbow();
	afx_msg void OnPaletteCool();
	afx_msg void OnPaletteInvert();
};

#ifndef _DEBUG  // debug version in BmpReaderView.cpp
inline CBmpReaderDoc* CBmpReaderView::GetDocument() const
   { return reinterpret_cast<CBmpReaderDoc*>(m_pDocument); }
#endif

