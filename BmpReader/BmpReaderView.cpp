// BmpReaderView.cpp
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

IMPLEMENT_DYNCREATE(CBmpReaderView, CView)

BEGIN_MESSAGE_MAP(CBmpReaderView, CView)
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
    ON_COMMAND(ID_PROCESS_SOBEL, &CBmpReaderView::OnProcessSobel)
    ON_COMMAND(ID_PROCESS_PREWITT, &CBmpReaderView::OnProcessPrewitt)
    ON_COMMAND(ID_PROCESS_SALTPEPPER, &CBmpReaderView::OnProcessSaltPepper)
    ON_COMMAND(ID_PROCESS_IMPULSE, &CBmpReaderView::OnProcessImpulse)
    ON_COMMAND(ID_PROCESS_GAUSSIAN, &CBmpReaderView::OnProcessGaussian)
    ON_COMMAND(ID_PROCESS_WHITEGAUSSIAN, &CBmpReaderView::OnProcessWhiteGaussian)
    ON_COMMAND(ID_PROCESS_ADD_IMAGES, &CBmpReaderView::OnProcessAddImages)
    ON_COMMAND(ID_PROCESS_MULTIPLY_IMAGES, &CBmpReaderView::OnProcessMultiplyImages)
    ON_COMMAND(ID_PROCESS_RESTORE_ORIGINAL, &CBmpReaderView::OnProcessRestoreOriginal)
    ON_COMMAND(ID_PROCESS_LAPLACIAN, &CBmpReaderView::OnProcessLaplacian)
    ON_COMMAND(ID_PROCESS_POWER_LAW, &CBmpReaderView::OnProcessPowerLaw)
    ON_COMMAND(ID_PROCESS_FFT, &CBmpReaderView::OnProcessFFT)
    ON_COMMAND(ID_PROCESS_IFFT, &CBmpReaderView::OnProcessIFFT)
    ON_COMMAND(ID_PROCESS_SHOW_SPECTRUM, &CBmpReaderView::OnProcessShowSpectrum)
END_MESSAGE_MAP()

CBmpReaderView::CBmpReaderView()
{
    m_pColorDlg = NULL;
    m_bShowColorDlg = TRUE;
    m_zoomFactor = 1.0;
    m_pHistogramDlg = NULL;
    m_pAdaptiveHistoDlg = NULL;
    m_bShowSpectrum = FALSE;
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

void CBmpReaderView::OnDraw(CDC* pDC)
{
    CBmpReaderDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    if (!pDoc) return;

    // ==== 修改：添加频谱显示支持 ====
    if (m_bShowSpectrum && pDoc->pImage && pDoc->pImage->IsFFTValid())
    {
        // 显示频谱图
        pDoc->pImage->ShowSpectrum(pDC);
    }
    else if (pDoc->pImage && pDoc->pImage->m_pRGB24)
    {
        // 显示原始图像（支持缩放）
        int nWidth = pDoc->pImage->m_nWidth;
        int nHeight = pDoc->pImage->m_nHeight;
        int drawWidth = (int)(nWidth * m_zoomFactor);
        int drawHeight = (int)(nHeight * m_zoomFactor);

        BITMAPINFO bmi = { 0 };
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = nWidth;
        bmi.bmiHeader.biHeight = -nHeight;  // 负值表示从上到下
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
        // 如果只有DIB数据，使用原有方式显示
        pDoc->pImage->ShowBMP(pDC);
    }
    else
    {
        // 如果没有图像数据，绘制一个简单的背景
        CRect rect;
        GetClientRect(&rect);
        pDC->FillSolidRect(&rect, RGB(240, 240, 240));

        CString msg = _T("请打开图像文件");
        pDC->SetTextColor(RGB(100, 100, 100));
        pDC->SetBkMode(TRANSPARENT);
        pDC->DrawText(msg, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

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
#endif

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
            strMsg.Format(_T("Original: (%d, %d) RGB: (%d, %d, %d) View: (%d, %d)"),
                origX, origY, GetRValue(cr), GetGValue(cr), GetBValue(cr), point.x, point.y);
        }
        else
        {
            strMsg = _T("Mouse out of image range");
        }
    }
    else
    {
        strMsg = _T("No image opened");
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
    AfxMessageBox(_T("Grayscale"));
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
        AfxMessageBox(_T("Please open an image first"));
        return;
    }

    CFileDialog dlg(TRUE, _T("bmp"), NULL,
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        _T("Image Files (*.bmp;*.jpg)|*.bmp;*.jpg|All Files (*.*)|*.*||"));
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
        AfxMessageBox(_T("Histogram specification failed"));
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
        int kSize = dlg.m_nKernelSize;
        if (kSize % 2 == 0) kSize++;
        pDoc->pImage->MeanFilter(kSize);
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

void CBmpReaderView::OnProcessSaltPepper()
{
    CBmpReaderDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage)
    {
        AfxMessageBox(_T("Please open an image first"));
        return;
    }
    pDoc->pImage->AddSaltPepperNoise(0.05, 0.05);
    Invalidate();
    UpdateHistogramWindow();
    AfxMessageBox(_T("Salt & Pepper noise added"));
}

void CBmpReaderView::OnProcessImpulse()
{
    CBmpReaderDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage)
    {
        AfxMessageBox(_T("Please open an image first"));
        return;
    }
    pDoc->pImage->AddImpulseNoise(0.05);
    Invalidate();
    UpdateHistogramWindow();
    AfxMessageBox(_T("Impulse noise added"));
}

void CBmpReaderView::OnProcessGaussian()
{
    CBmpReaderDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage)
    {
        AfxMessageBox(_T("Please open an image first"));
        return;
    }
    pDoc->pImage->AddGaussianNoise(0, 25);
    Invalidate();
    UpdateHistogramWindow();
    AfxMessageBox(_T("Gaussian noise added"));
}

void CBmpReaderView::OnProcessWhiteGaussian()
{
    CBmpReaderDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage)
    {
        AfxMessageBox(_T("Please open an image first"));
        return;
    }
    pDoc->pImage->AddWhiteGaussianNoise(0, 30);
    Invalidate();
    UpdateHistogramWindow();
    AfxMessageBox(_T("White Gaussian noise added"));
}

void CBmpReaderView::OnProcessSobel()
{
    CBmpReaderDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage)
    {
        AfxMessageBox(_T("Please open an image first"));
        return;
    }

    // 输入核大小
    CKernelSizeDlg dlgKernel(
        _T("Please enter Sobel kernel size (odd number 3-7):\n")
        _T("(3x3: standard, 5x5/7x7: larger edges)"),
        3, 3, 7
    );

    if (dlgKernel.DoModal() != IDOK) return;

    // 输入阈值
    CKernelSizeDlg dlgThreshold(
        _T("Please enter edge detection threshold (0-200):\n")
        _T("(0 = auto threshold, lower = more edges)"),
        80, 0, 200
    );

    if (dlgThreshold.DoModal() != IDOK) return;

    int kernelSize = dlgKernel.m_nValue;
    if (kernelSize % 2 == 0) kernelSize++;

    pDoc->pImage->SobelEdgeDetection(kernelSize, dlgThreshold.m_nValue, true);
    Invalidate();
    UpdateHistogramWindow();

    CString msg;
    if (dlgThreshold.m_nValue == 0)
        msg.Format(_T("Sobel edge detection completed!\nKernel size: %d x %d\nAuto threshold used"),
            kernelSize, kernelSize);
    else
        msg.Format(_T("Sobel edge detection completed!\nKernel size: %d x %d\nThreshold = %d"),
            kernelSize, kernelSize, dlgThreshold.m_nValue);
    AfxMessageBox(msg);
}

void CBmpReaderView::OnProcessPrewitt()
{
    CBmpReaderDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage)
    {
        AfxMessageBox(_T("Please open an image first"));
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

void CBmpReaderView::OnProcessAddImages()
{
    CBmpReaderDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->m_pRGB24)
    {
        AfxMessageBox(_T("Please open an image first"));
        return;
    }

    CFileDialog dlg(TRUE, _T("bmp"), NULL,
        OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
        _T("Image Files (*.bmp;*.jpg;*.png)|*.bmp;*.jpg;*.png|All Files (*.*)|*.*||"));

    if (dlg.DoModal() != IDOK) return;

    CString strPath = dlg.GetPathName();

    if (pDoc->pImage->AddImages(strPath))
    {
        Invalidate();
        UpdateHistogramWindow();
        AfxMessageBox(_T("Image addition completed"));
    }
    else
    {
        AfxMessageBox(_T("Image addition failed, make sure images have same size"));
    }
}

void CBmpReaderView::OnProcessMultiplyImages()
{
    CBmpReaderDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->m_pRGB24)
    {
        AfxMessageBox(_T("Please open an image first"));
        return;
    }

    CFileDialog dlg(TRUE, _T("bmp"), NULL,
        OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
        _T("Image Files (*.bmp;*.jpg;*.png)|*.bmp;*.jpg;*.png|All Files (*.*)|*.*||"));

    if (dlg.DoModal() != IDOK) return;

    CString strPath = dlg.GetPathName();

    if (pDoc->pImage->MultiplyImages(strPath))
    {
        Invalidate();
        UpdateHistogramWindow();
        AfxMessageBox(_T("Image multiplication completed"));
    }
    else
    {
        AfxMessageBox(_T("Image multiplication failed, make sure images have same size"));
    }
}

void CBmpReaderView::OnProcessRestoreOriginal()
{
    CBmpReaderDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage)
    {
        AfxMessageBox(_T("No image loaded"));
        return;
    }

    if (pDoc->pImage->RestoreOriginal())
    {
        Invalidate();
        UpdateHistogramWindow();
        AfxMessageBox(_T("Restored to original image"));
    }
    else
    {
        AfxMessageBox(_T("Restore failed"));
    }
}

void CBmpReaderView::OnProcessLaplacian()
{
    CBmpReaderDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->m_pRGB24)
    {
        AfxMessageBox(_T("Please open an image first"));
        return;
    }

    // 第一个对话框：选择算子类型（使用消息框，简单直接）
    int kernelResult = AfxMessageBox(_T("选择拉普拉斯算子类型：\n是 - 8邻域(3x3，边缘更强)\n否 - 4邻域(1x1，边缘较细)"),
        MB_YESNOCANCEL);
    if (kernelResult == IDCANCEL) return;
    int kernelSize = (kernelResult == IDYES) ? 3 : 1;

    // 第二个对话框：输入阈值 - 直接创建新的对话框实例，传入当前步骤的提示文字
    CKernelSizeDlg dlgThreshold(
        _T("【拉普拉斯边缘检测 - 阈值设置】\n\n")
        _T("阈值范围: 0-100\n")
        _T("• 0 = 自动计算最佳阈值\n")
        _T("• 1-30 = 较多边缘（推荐）\n")
        _T("• 31-60 = 适中边缘\n")
        _T("• 61-100 = 较少边缘（只保留强边缘）\n\n")
        _T("请输入阈值:"),
        30, 0, 100
    );

    if (dlgThreshold.DoModal() == IDOK)
    {
        pDoc->pImage->LaplacianEdgeDetection(kernelSize, dlgThreshold.m_nValue);
        Invalidate();
        UpdateHistogramWindow();

        CString msg;
        if (dlgThreshold.m_nValue == 0)
            msg.Format(_T("拉普拉斯边缘检测完成！\n算子：%s\n阈值：自动计算"),
                (kernelSize == 3) ? _T("8邻域(3x3)") : _T("4邻域(1x1)"));
        else
            msg.Format(_T("拉普拉斯边缘检测完成！\n算子：%s\n阈值：%d"),
                (kernelSize == 3) ? _T("8邻域(3x3)") : _T("4邻域(1x1)"),
                dlgThreshold.m_nValue);
        AfxMessageBox(msg);
    }
}

void CBmpReaderView::OnProcessPowerLaw()
{
    CBmpReaderDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->m_pRGB24)
    {
        AfxMessageBox(_T("Please open an image first"));
        return;
    }

    CKernelSizeDlg dlg(
        _T("Please enter gamma value (value/100): gamma<1 brighten, gamma=1 no change, gamma>1 darken. e.g., 50=0.5, 100=1.0, 200=2.0"),
        50, 10, 300
    );

    if (dlg.DoModal() == IDOK)
    {
        double gamma = dlg.m_nValue / 100.0;
        pDoc->pImage->PowerLawTransform(gamma);
        Invalidate();
        UpdateHistogramWindow();

        CString msg;
        msg.Format(_T("Power law transform completed!\nGamma = %.2f"), gamma);
        AfxMessageBox(msg);
    }
}

// ============================================================================
// FFT/IFFT 消息处理函数
// ============================================================================

void CBmpReaderView::OnProcessFFT()
{
    CBmpReaderDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage)
    {
        AfxMessageBox(_T("请先打开图像"));
        return;
    }

    if (pDoc->pImage->ComputeFFT2D())
    {
        m_bShowSpectrum = TRUE;
        Invalidate();
        AfxMessageBox(_T("傅里叶变换完成！"));
    }
    else
    {
        AfxMessageBox(_T("傅里叶变换失败"));
    }
}

void CBmpReaderView::OnProcessIFFT()
{
    CBmpReaderDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage)
    {
        AfxMessageBox(_T("请先打开图像"));
        return;
    }

    if (pDoc->pImage->ComputeIFFT2D())
    {
        m_bShowSpectrum = FALSE;
        Invalidate();
        AfxMessageBox(_T("反傅里叶变换完成！"));
    }
    else
    {
        AfxMessageBox(_T("反傅里叶变换失败，请先进行傅里叶变换"));
    }
}

void CBmpReaderView::OnProcessShowSpectrum()
{
    m_bShowSpectrum = !m_bShowSpectrum;
    Invalidate();

    CString msg;
    if (m_bShowSpectrum)
        msg = _T("显示频谱图");
    else
        msg = _T("显示原始图像");
    AfxMessageBox(msg);
}