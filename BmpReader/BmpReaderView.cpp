// BmpReaderView.cpp : implementation of the CBmpReaderView class
//

#include "stdafx.h"
#ifndef SHARED_HANDLERS
#include "BmpReader.h"
#endif

#include "BmpReaderDoc.h"
#include "BmpReaderView.h"
#include "BlockSizeDlg.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CBmpReaderView

IMPLEMENT_DYNCREATE(CBmpReaderView, CView)

BEGIN_MESSAGE_MAP(CBmpReaderView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CBmpReaderView::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_TOOL_ZOOMIN, OnToolZoomIn)
	ON_COMMAND(ID_TOOL_ZOOMOUT, OnToolZoomOut)
	ON_COMMAND(ID_TOOL_GRAY, OnToolGray)
	ON_COMMAND(ID_VIEW_SHOWPIXEL, OnViewShowcolorDlg)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWPIXEL, OnUpdateViewShowcolorDlg)
	ON_WM_MOUSEWHEEL()
	ON_COMMAND(ID_PROCESS_HISTOGRAM, OnProcessHistogram)
	ON_COMMAND(ID_PROCESS_LINEAR, OnProcessLinear)
	ON_COMMAND(ID_PROCESS_EQUALIZE, OnProcessEqualize)
	ON_COMMAND(ID_PROCESS_ADAPTIVE_HISTOGRAM, &CBmpReaderView::OnProcessAdaptiveHistogram)
	ON_COMMAND(ID_PROCESS_SPECIFY, OnProcessSpecify)
	ON_COMMAND(ID_PROCESS_PALETTE, OnProcessPalette)
	ON_COMMAND(ID_PALETTE_DEFAULT, &CBmpReaderView::OnPaletteDefault)
	ON_COMMAND(ID_PALETTE_HOT, &CBmpReaderView::OnPaletteHot)
	ON_COMMAND(ID_PALETTE_RAINBOW, &CBmpReaderView::OnPaletteRainbow)
	ON_COMMAND(ID_PALETTE_COOL, &CBmpReaderView::OnPaletteCool)
	ON_COMMAND(ID_PALETTE_INVERT, &CBmpReaderView::OnPaletteInvert)
	ON_MESSAGE(WM_USER_DESTROY_HIST, OnDestroyHistogramDlg)
	ON_MESSAGE(WM_USER_DESTROY_ADAPTIVE, &CBmpReaderView::OnDestroyAdaptiveDlg)
	ON_COMMAND(ID_PROCESS_MEANFILTER, &CBmpReaderView::OnProcessMeanFilter)
	ON_COMMAND(ID_PROCESS_MEDIANFILTER, &CBmpReaderView::OnProcessMedianFilter)
	ON_COMMAND(ID_PROCESS_MAXFILTER, &CBmpReaderView::OnProcessMaxFilter)

    	// ========== 新增：边缘检测菜单映射 ==========
	ON_COMMAND(ID_PROCESS_SOBEL, &CBmpReaderView::OnProcessSobel)
	ON_COMMAND(ID_PROCESS_PREWITT, &CBmpReaderView::OnProcessPrewitt)


	// ========== 新增：噪声处理命令映射 ==========
	ON_COMMAND(ID_PROCESS_SALTPEPPER, &CBmpReaderView::OnProcessSaltPepper)
	ON_COMMAND(ID_PROCESS_IMPULSE, &CBmpReaderView::OnProcessImpulse)
	ON_COMMAND(ID_PROCESS_GAUSSIAN, &CBmpReaderView::OnProcessGaussian)
	ON_COMMAND(ID_PROCESS_WHITEGAUSSIAN, &CBmpReaderView::OnProcessWhiteGaussian)
END_MESSAGE_MAP()

// CBmpReaderView construction/destruction

CBmpReaderView::CBmpReaderView()
{
	m_pColorDlg = NULL;
	m_bShowColorDlg = TRUE;
	m_zoomFactor = 1.0;
	m_pHistogramDlg = NULL;
	m_pAdaptiveHistoDlg = NULL;
}

CBmpReaderView::~CBmpReaderView()
{
	if (m_pColorDlg != NULL)
	{
		m_pColorDlg->DestroyWindow();
		delete m_pColorDlg;
		m_pColorDlg = NULL;
	}
	CloseHistogramWindow();
	if (m_pAdaptiveHistoDlg && m_pAdaptiveHistoDlg->GetSafeHwnd())
		m_pAdaptiveHistoDlg->DestroyWindow();
	m_pAdaptiveHistoDlg = NULL;
}

BOOL CBmpReaderView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

// CBmpReaderView drawing

void CBmpReaderView::OnDraw(CDC* pDC)
{
	CBmpReaderDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc) return;

	if (pDoc->pImage && pDoc->pImage->m_pRGB24)
	{
		int nWidth = pDoc->pImage->m_nWidth;
		int nHeight = pDoc->pImage->m_nHeight;
		int drawWidth = (int)(nWidth * m_zoomFactor);
		int drawHeight = (int)(nHeight * m_zoomFactor);

		BITMAPINFO bmi = { 0 };
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = nWidth;
		bmi.bmiHeader.biHeight = -nHeight;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 24;
		bmi.bmiHeader.biCompression = BI_RGB;

		::SetStretchBltMode(pDC->m_hDC, COLORONCOLOR);
		::StretchDIBits(pDC->m_hDC, 0, 0, drawWidth, drawHeight,
			0, 0, nWidth, nHeight,
			pDoc->pImage->m_pRGB24, &bmi, DIB_RGB_COLORS, SRCCOPY);
	}
	else if (pDoc->pImage && pDoc->pImage->m_hDib)
	{
		pDoc->pImage->ShowBMP(pDC);
	}
}

// CBmpReaderView printing

void CBmpReaderView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL CBmpReaderView::OnPreparePrinting(CPrintInfo* pInfo)
{
	return DoPreparePrinting(pInfo);
}

void CBmpReaderView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

void CBmpReaderView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

void CBmpReaderView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CBmpReaderView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}

// CBmpReaderView diagnostics

#ifdef _DEBUG
void CBmpReaderView::AssertValid() const
{
	CView::AssertValid();
}

void CBmpReaderView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CBmpReaderDoc* CBmpReaderView::GetDocument() const
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CBmpReaderDoc)));
	return (CBmpReaderDoc*)m_pDocument;
}
#endif //_DEBUG

// CBmpReaderView message handlers

void CBmpReaderView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (pDoc && pDoc->pImage && pDoc->pImage->m_pRGB24)
	{
		int origX = (int)(point.x / m_zoomFactor);
		int origY = (int)(point.y / m_zoomFactor);

		if (origX >= 0 && origX < pDoc->pImage->m_nWidth &&
			origY >= 0 && origY < pDoc->pImage->m_nHeight)
		{
			CClientDC dc(this);
			COLORREF crManual = pDoc->pImage->GetPixelColorManual(origX, origY);
			COLORREF crGetPixel = crManual;

			if (m_bShowColorDlg)
			{
				if (m_pColorDlg == NULL)
				{
					m_pColorDlg = new CColorInfoDlg(this);
					m_pColorDlg->Create(IDD_COLOR_INFO, this);
				}
				m_pColorDlg->SetInfo(origX, origY, crManual, crGetPixel);

				CPoint screenPt = point;
				ClientToScreen(&screenPt);
				m_pColorDlg->SetWindowPos(NULL, screenPt.x + 10, screenPt.y + 10, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
				m_pColorDlg->ShowWindow(SW_SHOW);
			}
			else
			{
				if (m_pColorDlg != NULL && m_pColorDlg->GetSafeHwnd())
					m_pColorDlg->ShowWindow(SW_HIDE);
			}
		}
	}
	CView::OnLButtonDown(nFlags, point);
}

void CBmpReaderView::OnMouseMove(UINT nFlags, CPoint point)
{
	CBmpReaderDoc* pDoc = GetDocument();
	CString strMsg;
	if (pDoc && pDoc->pImage && pDoc->pImage->m_pRGB24)
	{
		int origX = (int)(point.x / m_zoomFactor);
		int origY = (int)(point.y / m_zoomFactor);

		if (origX >= 0 && origX < pDoc->pImage->m_nWidth &&
			origY >= 0 && origY < pDoc->pImage->m_nHeight)
		{
			COLORREF cr = pDoc->pImage->GetPixelColorManual(origX, origY);
			strMsg.Format(_T("原始坐标: (%d, %d)  RGB: (%d, %d, %d)  [视图坐标: (%d, %d)]"),
				origX, origY, GetRValue(cr), GetGValue(cr), GetBValue(cr), point.x, point.y);
		}
		else
		{
			strMsg = _T("鼠标超出图像范围");
		}
	}
	else
	{
		strMsg = _T("未打开图像");
	}

	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (pFrame)
	{
		pFrame->SetStatusBarText(strMsg, 0);
	}
	CView::OnMouseMove(nFlags, point);
}

void CBmpReaderView::OnViewShowcolorDlg()
{
	m_bShowColorDlg = !m_bShowColorDlg;
	if (!m_bShowColorDlg && m_pColorDlg != NULL && m_pColorDlg->GetSafeHwnd())
	{
		m_pColorDlg->ShowWindow(SW_HIDE);
	}
	else if (m_bShowColorDlg && m_pColorDlg != NULL && m_pColorDlg->GetSafeHwnd())
	{
		m_pColorDlg->ShowWindow(SW_SHOW);
	}
}

void CBmpReaderView::OnUpdateViewShowcolorDlg(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bShowColorDlg);
}

BOOL CBmpReaderView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (zDelta > 0)
		m_zoomFactor *= 1.1;
	else
		m_zoomFactor *= 0.9;
	if (m_zoomFactor < 0.1) m_zoomFactor = 0.1;
	if (m_zoomFactor > 10.0) m_zoomFactor = 10.0;
	Invalidate();
	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void CBmpReaderView::OnToolZoomIn()
{
	m_zoomFactor *= 1.1;
	if (m_zoomFactor > 10.0) m_zoomFactor = 10.0;
	Invalidate();
}

void CBmpReaderView::OnToolZoomOut()
{
	m_zoomFactor *= 0.9;
	if (m_zoomFactor < 0.1) m_zoomFactor = 0.1;
	Invalidate();
}

void CBmpReaderView::OnToolGray()
{
	AfxMessageBox(_T("灰度化"));
}

void CBmpReaderView::OnProcessHistogram()
{
	if (m_pHistogramDlg == NULL)
	{
		m_pHistogramDlg = new CHistogramDlg(this);
		if (!m_pHistogramDlg->Create(IDD_HISTOGRAM_DLG, this))
		{
			delete m_pHistogramDlg;
			m_pHistogramDlg = NULL;
			return;
		}

		CMainFrame* pFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
		if (pFrame)
		{
			CRect rcFrame;
			pFrame->GetClientRect(&rcFrame);
			CRect rcDlg;
			m_pHistogramDlg->GetWindowRect(&rcDlg);
			int dlgWidth = rcDlg.Width();
			int dlgHeight = rcDlg.Height();
			int x = rcFrame.right - dlgWidth - 20;
			int y = 100;
			pFrame->ClientToScreen(&rcFrame);
			x = rcFrame.left + x;
			y = rcFrame.top + y;
			m_pHistogramDlg->SetWindowPos(NULL, x, y, dlgWidth, dlgHeight, SWP_NOZORDER);
		}
	}
	m_pHistogramDlg->ShowWindow(SW_SHOW);
	UpdateHistogramWindow();
}

void CBmpReaderView::OnProcessLinear()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (pDoc && pDoc->pImage) pDoc->pImage->LinearTransform(50, 200, 0, 255);
	Invalidate();
	UpdateHistogramWindow();
}

void CBmpReaderView::OnProcessEqualize()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (pDoc && pDoc->pImage) pDoc->pImage->HistogramEqualize();
	Invalidate();
	UpdateHistogramWindow();
}

void CBmpReaderView::OnProcessSpecify()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (!pDoc || !pDoc->pImage)
	{
		AfxMessageBox(_T("请先打开一张图像"));
		return;
	}

	CFileDialog dlg(TRUE, _T("bmp"), NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("图像文件 (*.bmp;*.jpg)|*.bmp;*.jpg|所有文件 (*.*)|*.*||"));
	if (dlg.DoModal() != IDOK)
		return;

	CString targetPath = dlg.GetPathName();

	if (pDoc->pImage->HistogramSpecify(targetPath))
	{
		Invalidate();
		UpdateHistogramWindow();
	}
	else
	{
		AfxMessageBox(_T("规格化失败，请确保目标图像格式正确"));
	}
}

void CBmpReaderView::OnProcessPalette()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (pDoc && pDoc->pImage) pDoc->pImage->PaletteTransform();
	Invalidate();
	UpdateHistogramWindow();
}

void CBmpReaderView::OnProcessAdaptiveHistogram()
{
	if (m_pAdaptiveHistoDlg == NULL)
	{
		m_pAdaptiveHistoDlg = new CBlockSizeDlg(this);
		if (!m_pAdaptiveHistoDlg->Create(CBlockSizeDlg::IDD, this))
		{
			delete m_pAdaptiveHistoDlg;
			m_pAdaptiveHistoDlg = NULL;
			return;
		}
		m_pAdaptiveHistoDlg->SetBlockSize(8);

		CRect rc;
		m_pAdaptiveHistoDlg->GetWindowRect(&rc);
		int w = rc.Width();
		int h = rc.Height();
		CMainFrame* pFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
		if (pFrame)
		{
			CRect rcFrame;
			pFrame->GetClientRect(&rcFrame);
			pFrame->ClientToScreen(&rcFrame);
			int x = rcFrame.right - w - 20;
			int y = rcFrame.top + 300;
			m_pAdaptiveHistoDlg->SetWindowPos(NULL, x, y, w, h, SWP_NOZORDER);
		}
	}
	m_pAdaptiveHistoDlg->ShowWindow(SW_SHOW);
}

void CBmpReaderView::UpdateHistogramWindow()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (!pDoc || !pDoc->pImage) return;

	int hist[256];
	int maxCount;
	pDoc->pImage->CalculateHistogram(hist, maxCount);

	if (m_pHistogramDlg && m_pHistogramDlg->GetSafeHwnd())
	{
		m_pHistogramDlg->SetHistogramData(hist, maxCount);
	}
}

void CBmpReaderView::CloseHistogramWindow()
{
	if (m_pHistogramDlg)
	{
		if (m_pHistogramDlg->GetSafeHwnd())
			m_pHistogramDlg->DestroyWindow();
		m_pHistogramDlg = NULL;
	}
}

LRESULT CBmpReaderView::OnDestroyHistogramDlg(WPARAM, LPARAM)
{
	m_pHistogramDlg = NULL;
	return 0;
}

void CBmpReaderView::ApplyAdaptiveEqualize(int blockSize)
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (pDoc && pDoc->pImage)
	{
		pDoc->pImage->AdaptiveHistogramEqualize(blockSize);
		Invalidate();
		UpdateWindow();
		UpdateHistogramWindow();
	}
}

LRESULT CBmpReaderView::OnDestroyAdaptiveDlg(WPARAM, LPARAM)
{
	m_pAdaptiveHistoDlg = NULL;
	return 0;
}

void CBmpReaderView::OnPaletteDefault()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (pDoc && pDoc->pImage)
	{
		pDoc->pImage->ApplyPseudoColor(CImageProc::SCHEME_DEFAULT);
		Invalidate();
		UpdateHistogramWindow();
	}
}

void CBmpReaderView::OnPaletteHot()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (pDoc && pDoc->pImage)
	{
		pDoc->pImage->ApplyPseudoColor(CImageProc::SCHEME_HOT);
		Invalidate();
		UpdateHistogramWindow();
	}
}

void CBmpReaderView::OnPaletteRainbow()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (pDoc && pDoc->pImage)
	{
		pDoc->pImage->ApplyPseudoColor(CImageProc::SCHEME_RAINBOW);
		Invalidate();
		UpdateHistogramWindow();
	}
}

void CBmpReaderView::OnPaletteCool()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (pDoc && pDoc->pImage)
	{
		pDoc->pImage->ApplyPseudoColor(CImageProc::SCHEME_COOL);
		Invalidate();
		UpdateHistogramWindow();
	}
}

void CBmpReaderView::OnPaletteInvert()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (pDoc && pDoc->pImage)
	{
		pDoc->pImage->ApplyPseudoColor(CImageProc::SCHEME_INVERT);
		Invalidate();
		UpdateHistogramWindow();
	}
}

void CBmpReaderView::OnProcessMeanFilter()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (!pDoc || !pDoc->pImage) return;

	CKernelSizeDlg dlg;
	if (dlg.DoModal() == IDOK)
	{
		pDoc->pImage->MeanFilter(dlg.m_nKernelSize);
		Invalidate();
		UpdateHistogramWindow();
	}
}

void CBmpReaderView::OnProcessMedianFilter()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (!pDoc || !pDoc->pImage) return;

	CKernelSizeDlg dlg;
	if (dlg.DoModal() == IDOK)
	{
		pDoc->pImage->MedianFilter(dlg.m_nKernelSize);
		Invalidate();
		UpdateHistogramWindow();
	}
}

void CBmpReaderView::OnProcessMaxFilter()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (!pDoc || !pDoc->pImage) return;

	CKernelSizeDlg dlg;
	if (dlg.DoModal() == IDOK)
	{
		pDoc->pImage->MaxFilter(dlg.m_nKernelSize);
		Invalidate();
		UpdateHistogramWindow();
	}
}

// ========== 噪声添加命令处理 ==========

void CBmpReaderView::OnProcessSaltPepper()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (!pDoc || !pDoc->pImage)
	{
		AfxMessageBox(_T("请先打开一张图片！"));
		return;
	}

	pDoc->pImage->AddSaltPepperNoise(0.05, 0.05);
	Invalidate();
	UpdateHistogramWindow();
	AfxMessageBox(_T("已添加椒盐噪声"));
}

void CBmpReaderView::OnProcessImpulse()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (!pDoc || !pDoc->pImage)
	{
		AfxMessageBox(_T("请先打开一张图片！"));
		return;
	}

	pDoc->pImage->AddImpulseNoise(0.05);
	Invalidate();
	UpdateHistogramWindow();
	AfxMessageBox(_T("已添加脉冲噪声"));
}

void CBmpReaderView::OnProcessGaussian()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (!pDoc || !pDoc->pImage)
	{
		AfxMessageBox(_T("请先打开一张图片！"));
		return;
	}

	pDoc->pImage->AddGaussianNoise(0, 25);
	Invalidate();
	UpdateHistogramWindow();
	AfxMessageBox(_T("已添加高斯噪声"));
}

void CBmpReaderView::OnProcessWhiteGaussian()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (!pDoc || !pDoc->pImage)
	{
		AfxMessageBox(_T("请先打开一张图片！"));
		return;
	}

	pDoc->pImage->AddWhiteGaussianNoise(0, 30);
	Invalidate();
	UpdateHistogramWindow();
	AfxMessageBox(_T("已添加高斯白噪声"));
}

// ========== 边缘检测菜单处理函数 ==========

void CBmpReaderView::OnProcessSobel()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (!pDoc || !pDoc->pImage)
	{
		AfxMessageBox(_T("请先打开一张图片"));
		return;
	}

	CKernelSizeDlg dlg;
	if (dlg.DoModal() == IDOK)
	{
		pDoc->pImage->SobelEdgeDetection(dlg.m_nKernelSize, 80);
		Invalidate();
		UpdateHistogramWindow();
	}
}

void CBmpReaderView::OnProcessPrewitt()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (!pDoc || !pDoc->pImage)
	{
		AfxMessageBox(_T("请先打开一张图片"));
		return;
	}

	CKernelSizeDlg dlg;
	if (dlg.DoModal() == IDOK)
	{
		pDoc->pImage->PrewittEdgeDetection(dlg.m_nKernelSize, 80);
		Invalidate();
		UpdateHistogramWindow();
	}
}
