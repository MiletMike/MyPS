﻿// BmpReaderView.cpp
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
    ON_COMMAND(ID_PROCESS_INVERSE_FILTER, &CBmpReaderView::OnProcessInverseFilter)
    ON_COMMAND(ID_PROCESS_WIENER_FILTER, &CBmpReaderView::OnProcessWienerFilter)
  ON_COMMAND(ID_PROCESS_MOTION_BLUR, &CBmpReaderView::OnProcessMotionBlur)
  ON_COMMAND(ID_PROCESS_TURBULENCE_BLUR, &CBmpReaderView::OnProcessTurbulenceBlur)
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

    if (m_bShowSpectrum && pDoc->pImage && pDoc->pImage->IsFFTValid())
    {
        pDoc->pImage->ShowSpectrum(pDC);
    }
    else if (pDoc->pImage && pDoc->pImage->m_pRGB24)
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
    else
    {
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

    CKernelSizeDlg dlgKernel(
        _T("Please enter Sobel kernel size (odd number 3-7):\n")
        _T("(3x3: standard, 5x5/7x7: larger edges)"),
        3, 3, 7
    );

    if (dlgKernel.DoModal() != IDOK) return;

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

    int kernelResult = AfxMessageBox(_T("选择拉普拉斯算子类型：\n是 - 8邻域(3x3，边缘更强)\n否 - 4邻域(1x1，边缘较细)"),
        MB_YESNOCANCEL);
    if (kernelResult == IDCANCEL) return;
    int kernelSize = (kernelResult == IDYES) ? 3 : 1;

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

void CBmpReaderView::OnProcessInverseFilter()
{
    CBmpReaderDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->m_pRGB24)
    {
        AfxMessageBox(_T("Please open an image first"));
        return;
    }

    // 第一步：选择退化类型
    int blurResult = AfxMessageBox(_T("选择退化类型：\n是 - 运动模糊 (Motion Blur)\n否 - 大气湍流 (Turbulence)"),
        MB_YESNOCANCEL);
    if (blurResult == IDCANCEL) return;
    int blurType = (blurResult == IDYES) ? CImageProc::BLUR_MOTION : CImageProc::BLUR_TURBULENCE;

    double p1 = 0.1, p2 = 0.1, p3 = 1.0;
    double threshold = 1.0;

    if (blurType == CImageProc::BLUR_MOTION)
    {
        // 运动模糊参数输入：a, b, T (放大100倍为整数)
        CKernelSizeDlg dlgA(_T("【运动模糊参数 a (×100)】\na 为水平方向运动分量\n推荐值: 10 (即 0.10)"), 10, 1, 50);
        if (dlgA.DoModal() != IDOK) return;
        p1 = dlgA.m_nValue / 100.0;

        CKernelSizeDlg dlgB(_T("【运动模糊参数 b (×100)】\nb 为垂直方向运动分量\n推荐值: 10 (即 0.10)"), 10, 0, 50);
        if (dlgB.DoModal() != IDOK) return;
        p2 = dlgB.m_nValue / 100.0;

        CKernelSizeDlg dlgT(_T("【运动模糊参数 T (×100)】\nT 为曝光时间\n推荐值: 100 (即 1.00)"), 100, 1, 200);
        if (dlgT.DoModal() != IDOK) return;
        p3 = dlgT.m_nValue / 100.0;
    }
    else
    {
        // 湍流参数 k (放大10000倍为整数)
        CKernelSizeDlg dlgK(_T("【大气湍流参数 k (×10000)】\nk 值越小模糊越轻\n推荐值: 25 (即 0.0025) 对应 turb_0pt0025.bmp"), 25, 1, 500);
        if (dlgK.DoModal() != IDOK) return;
        p1 = dlgK.m_nValue / 10000.0;
    }

    // 阈值参数（Tikhonov 正则化强度）
    CKernelSizeDlg dlgThresh(_T("【逆滤波正则化强度 (百分比)】\n"
        "Tikhonov: F=conj(H)·G/(|H|²+λ)\n"
        "推荐值:\n"
        "  1% = 轻度正则化（无噪声图）\n"
        "  3~5% = 中等（含噪声图）\n"
        "  10% = 强正则化（高噪声）"), 1, 1, 20);
    if (dlgThresh.DoModal() != IDOK) return;
    threshold = (double)dlgThresh.m_nValue;

    // 执行逆滤波
    pDoc->pImage->InverseFilter(blurType, p1, p2, p3, threshold);
    Invalidate();
    UpdateHistogramWindow();

    CString msg;
    if (blurType == CImageProc::BLUR_MOTION)
        msg.Format(_T("逆滤波完成！(Tikhonov)\n类型: 运动模糊\na=%.2f, b=%.2f, T=%.2f\n正则化强度: %d%%"), p1, p2, p3, (int)threshold);
    else
        msg.Format(_T("逆滤波完成！(Tikhonov)\n类型: 大气湍流\nk=%.4f\n正则化强度: %d%%"), p1, (int)threshold);
    AfxMessageBox(msg);
}

void CBmpReaderView::OnProcessWienerFilter()
{
    CBmpReaderDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->m_pRGB24)
    {
        AfxMessageBox(_T("Please open an image first"));
        return;
    }

    // 第一步：选择退化类型
    int blurResult = AfxMessageBox(_T("选择退化类型：\n是 - 运动模糊 (Motion Blur)\n否 - 大气湍流 (Turbulence)"),
        MB_YESNOCANCEL);
    if (blurResult == IDCANCEL) return;
    int blurType = (blurResult == IDYES) ? CImageProc::BLUR_MOTION : CImageProc::BLUR_TURBULENCE;

    double p1 = 0.1, p2 = 0.1, p3 = 1.0;
    double K = 0.01;

    if (blurType == CImageProc::BLUR_MOTION)
    {
        CKernelSizeDlg dlgA(_T("【运动模糊参数 a (×100)】\na 为水平方向运动分量\n推荐值: 10 (即 0.10)"), 10, 1, 50);
        if (dlgA.DoModal() != IDOK) return;
        p1 = dlgA.m_nValue / 100.0;

        CKernelSizeDlg dlgB(_T("【运动模糊参数 b (×100)】\nb 为垂直方向运动分量\n推荐值: 10 (即 0.10)"), 10, 0, 50);
        if (dlgB.DoModal() != IDOK) return;
        p2 = dlgB.m_nValue / 100.0;

        CKernelSizeDlg dlgT(_T("【运动模糊参数 T (×100)】\nT 为曝光时间\n推荐值: 100 (即 1.00)"), 100, 1, 200);
        if (dlgT.DoModal() != IDOK) return;
        p3 = dlgT.m_nValue / 100.0;
    }
    else
    {
        CKernelSizeDlg dlgK(_T("【大气湍流参数 k (×10000)】\n推荐值: 25 (即 0.0025) 对应 turb_0pt0025.bmp"), 25, 1, 500);
        if (dlgK.DoModal() != IDOK) return;
        p1 = dlgK.m_nValue / 10000.0;
    }

    // 维纳滤波 K 值 (噪声/信号功率比)
    CKernelSizeDlg dlgKVal(_T("【维纳滤波参数 K (×1000)】\n"
        "K = 噪声功率 / 信号功率比\n"
        "输入 0 则自动估计\n"
        "K 越小复原越强，越大去噪越强\n"
        "含噪声推荐 10~50 (0.010~0.050)"),
        0, 0, 200);
    if (dlgKVal.DoModal() != IDOK) return;
    K = dlgKVal.m_nValue / 1000.0;

    // 执行维纳滤波
    pDoc->pImage->WienerFilter(blurType, p1, p2, p3, K);
    Invalidate();
    UpdateHistogramWindow();

    CString msg;
    if (blurType == CImageProc::BLUR_MOTION) {
        if (K <= 0)
            msg.Format(_T("维纳滤波完成！(自动 K)\n类型: 运动模糊\na=%.2f, b=%.2f, T=%.2f"), p1, p2, p3);
        else
            msg.Format(_T("维纳滤波完成！\n类型: 运动模糊\na=%.2f, b=%.2f, T=%.2f\nK=%.3f"), p1, p2, p3, K);
    } else {
        if (K <= 0)
            msg.Format(_T("维纳滤波完成！(自动 K)\n类型: 大气湍流\nk=%.4f"), p1);
        else
            msg.Format(_T("维纳滤波完成！\n类型: 大气湍流\nk=%.4f\nK=%.3f"), p1, K);
    }
    AfxMessageBox(msg);
}

void CBmpReaderView::OnProcessMotionBlur()
{
    CBmpReaderDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->m_pRGB24)
    {
        AfxMessageBox(_T("请先打开图像"));
        return;
    }

    int blurResult = AfxMessageBox(_T("选择运动模糊参数：\n是 - 使用默认参数 (a=0.10, b=0.10, T=1.00)\n否 - 自定义参数"),
        MB_YESNOCANCEL);
    if (blurResult == IDCANCEL) return;

    double a = 0.1, b = 0.1, T = 1.0;

    if (blurResult == IDNO)
    {
        CKernelSizeDlg dlgA(_T("【运动模糊参数 a (×100)】\na 为水平方向运动分量\n推荐值: 10 (即 0.10)"), 10, 1, 50);
        if (dlgA.DoModal() != IDOK) return;
        a = dlgA.m_nValue / 100.0;

        CKernelSizeDlg dlgB(_T("【运动模糊参数 b (×100)】\nb 为垂直方向运动分量\n推荐值: 10 (即 0.10)"), 10, 0, 50);
        if (dlgB.DoModal() != IDOK) return;
        b = dlgB.m_nValue / 100.0;

        CKernelSizeDlg dlgT(_T("【运动模糊参数 T (×100)】\nT 为曝光时间\n推荐值: 100 (即 1.00)"), 100, 1, 200);
        if (dlgT.DoModal() != IDOK) return;
        T = dlgT.m_nValue / 100.0;
    }

    pDoc->pImage->AddMotionBlur(a, b, T);
    Invalidate();
    UpdateHistogramWindow();

    CString msg;
    msg.Format(_T("运动模糊处理完成！\na=%.2f, b=%.2f, T=%.2f"), a, b, T);
    AfxMessageBox(msg);
}

void CBmpReaderView::OnProcessTurbulenceBlur()
{
    CBmpReaderDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->m_pRGB24)
    {
        AfxMessageBox(_T("请先打开图像"));
        return;
    }

    int blurResult = AfxMessageBox(_T("选择大气湍流模糊参数：\n是 - 使用默认参数 (k=0.0025)\n否 - 自定义参数"),
        MB_YESNOCANCEL);
    if (blurResult == IDCANCEL) return;

    double k = 0.0025;

    if (blurResult == IDNO)
    {
        CKernelSizeDlg dlgK(_T("【大气湍流参数 k (×10000)】\nk 值越大模糊越严重\n推荐值: 25 (即 0.0025)"), 25, 1, 500);
        if (dlgK.DoModal() != IDOK) return;
        k = dlgK.m_nValue / 10000.0;
    }

    pDoc->pImage->AddTurbulenceBlur(k);
    Invalidate();
    UpdateHistogramWindow();

    CString msg;
    msg.Format(_T("大气湍流模糊处理完成！\nk=%.4f"), k);
    AfxMessageBox(msg);
}
