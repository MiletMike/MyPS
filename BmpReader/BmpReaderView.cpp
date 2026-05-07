
// BmpReaderView.cpp : implementation of the CBmpReaderView class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
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
		m_pColorDlg->DestroyWindow();  // 销毁窗口
		delete m_pColorDlg;            // 删除对象
		m_pColorDlg = NULL;
	}
	//关闭直方图窗口
	CloseHistogramWindow();
	if (m_pAdaptiveHistoDlg && m_pAdaptiveHistoDlg->GetSafeHwnd())
		m_pAdaptiveHistoDlg->DestroyWindow();
	m_pAdaptiveHistoDlg = NULL;
}

BOOL CBmpReaderView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CBmpReaderView drawing

void CBmpReaderView::OnDraw(CDC* pDC)
{
	CBmpReaderDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc) return;

	// 使用新的24位数据 m_pRGB24（如果存在）
	if (pDoc->pImage && pDoc->pImage->m_pRGB24)
	{
		int nWidth = pDoc->pImage->m_nWidth;
		int nHeight = pDoc->pImage->m_nHeight;
		int drawWidth = (int)(nWidth * m_zoomFactor);
		int drawHeight = (int)(nHeight * m_zoomFactor);

		// 构造 BITMAPINFO 结构（24位RGB）
		BITMAPINFO bmi = {0};
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = nWidth;
		bmi.bmiHeader.biHeight = -nHeight;   // 负值表示正向（顶部第一行）
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 24;
		bmi.bmiHeader.biCompression = BI_RGB;

		::SetStretchBltMode(pDC->m_hDC, COLORONCOLOR);
		::StretchDIBits(pDC->m_hDC, 0, 0, drawWidth, drawHeight,
			0, 0, nWidth, nHeight,
			pDoc->pImage->m_pRGB24, &bmi, DIB_RGB_COLORS, SRCCOPY);
	}
	// 后备：如果 m_pRGB24 不存在但 m_hDib 存在，则用原来的方式显示（无缩放）
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
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CBmpReaderView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CBmpReaderView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
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

CBmpReaderDoc* CBmpReaderView::GetDocument() const // non-debug version is inline
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

			// 自编程读取原始图像颜色（使用映射后的原始坐标）
			COLORREF crManual = pDoc->pImage->GetPixelColorManual(origX, origY);
			// 系统 GetPixel 获取的是缩放后屏幕上的颜色，不再用于对比（因为缩放后一个屏幕像素可能对应多个原始像素）
			// 这里为了不破坏原有接口，简单地将系统颜色设为与自编程相同，避免弹出不一致的误导信息
			COLORREF crGetPixel = crManual;

			// 根据菜单开关决定是否显示非模态窗口
			if (m_bShowColorDlg)
			{
				if (m_pColorDlg == NULL)
				{
					m_pColorDlg = new CColorInfoDlg(this);
					m_pColorDlg->Create(IDD_COLOR_INFO, this);
				}
				// 注意：这里传入的是原始坐标 origX, origY 和正确的颜色
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
	CBmpReaderDoc* pDoc = GetDocument();    // 获取文档

	CString strMsg;
	if (pDoc && pDoc->pImage && pDoc->pImage->m_pRGB24)   // 缩放显示依赖 m_pRGB24
	{
		// 映射回原始图像坐标
		int origX = (int)(point.x / m_zoomFactor);
		int origY = (int)(point.y / m_zoomFactor);

		// 检查原始坐标是否在图像有效范围内
		if (origX >= 0 && origX < pDoc->pImage->m_nWidth &&
			origY >= 0 && origY < pDoc->pImage->m_nHeight)
		{
			// 获取原始图像颜色（使用自编程函数）
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

	// 将信息显示在状态栏（第一个窗格）
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (pFrame)
	{
		pFrame->SetStatusBarText(strMsg, 0);
	}
	CView::OnMouseMove(nFlags, point);
}

void CBmpReaderView::OnViewShowcolorDlg()
{
	// 切换状态
	m_bShowColorDlg = !m_bShowColorDlg;

	// 如果关闭显示，并且对话框存在，则隐藏它
	if (!m_bShowColorDlg && m_pColorDlg != NULL && m_pColorDlg->GetSafeHwnd())
	{
		m_pColorDlg->ShowWindow(SW_HIDE);
	}
	// 如果开启显示，并且对话框存在，则显示它（但不需要立即更新内容）
	else if (m_bShowColorDlg && m_pColorDlg != NULL && m_pColorDlg->GetSafeHwnd())
	{
		m_pColorDlg->ShowWindow(SW_SHOW);
	}
}

void CBmpReaderView::OnUpdateViewShowcolorDlg(CCmdUI* pCmdUI)
{
	// 设置菜单项的复选状态
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
	Invalidate();   // 刷新视图
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
	// 如果窗口还没创建，就创建并显示
	if (m_pHistogramDlg == NULL)
	{
		m_pHistogramDlg = new CHistogramDlg(this);
		if (!m_pHistogramDlg->Create(IDD_HISTOGRAM_DLG, this))
		{
			delete m_pHistogramDlg;
			m_pHistogramDlg = NULL;
			return;
		}

		// 获取主框架窗口
		CMainFrame* pFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
		if (pFrame)
		{
			CRect rcFrame;
			pFrame->GetClientRect(&rcFrame);   // 主框架客户区大小

			// 获取直方图对话框的当前大小
			CRect rcDlg;
			m_pHistogramDlg->GetWindowRect(&rcDlg);
			int dlgWidth = rcDlg.Width();
			int dlgHeight = rcDlg.Height();

			// 我们希望直方图窗口出现在主框架的右侧，距离右边缘 20 像素，上边缘 100 像素
			int x = rcFrame.right - dlgWidth - 20;
			int y = 100;

			// 把坐标转换为屏幕坐标（SetWindowPos 使用屏幕坐标）
			pFrame->ClientToScreen(&rcFrame);
			x = rcFrame.left + x;
			y = rcFrame.top + y;

			// 设置窗口位置和大小（也可以顺便固定大小，防止用户拉伸）
			m_pHistogramDlg->SetWindowPos(NULL, x, y, dlgWidth, dlgHeight, SWP_NOZORDER);
		}
	}

	// 显示窗口（如果处于隐藏状态）
	m_pHistogramDlg->ShowWindow(SW_SHOW);
	// 更新数据
	UpdateHistogramWindow();
}
void CBmpReaderView::OnProcessLinear()
{
	CBmpReaderDoc* pDoc = GetDocument();
	if (pDoc && pDoc->pImage) pDoc->pImage->LinearTransform(50,200,0,255);
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

	// 弹出文件对话框选择目标图像
	CFileDialog dlg(TRUE, _T("bmp"), NULL, 
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("图像文件 (*.bmp;*.jpg)|*.bmp;*.jpg|所有文件 (*.*)|*.*||"));
	if (dlg.DoModal() != IDOK)
		return;

	CString targetPath = dlg.GetPathName();

	// 执行直方图规格化
	if (pDoc->pImage->HistogramSpecify(targetPath))
	{
		Invalidate();               // 刷新视图
		UpdateHistogramWindow();    // 更新直方图窗口
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

		// 定位窗口（放在合适位置，比如直方图窗口附近）
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
			int y = rcFrame.top + 300;   // 可根据直方图窗口位置调整
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
		// 注意：非模态对话框在 PostNcDestroy 中 delete this，所以不需要 delete
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
		UpdateWindow();             // 立即重绘，不等待消息队列
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