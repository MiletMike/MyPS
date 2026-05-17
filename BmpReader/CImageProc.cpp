// CImageProc.cpp
#include "stdafx.h"
#include "CImageProc.h"
#include "MainFrm.h"
#include <atlimage.h>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <float.h>
#include "HistogramDlg.h"

CImageProc::CImageProc()
    : m_nWidth(0), m_nHeight(0), m_pRGB24(nullptr)
{
    m_hDib = NULL;
    pDib = new BYTE;
    pBFH = new BITMAPFILEHEADER;
    pBIH = new BITMAPINFOHEADER;
    pQUAD = new RGBQUAD;
    pBits = new BYTE;
    nWidth = nHeight = nNumColors = 0;
    m_rMask = m_gMask = m_bMask = 0;
    m_dwRedMask = m_dwGreenMask = m_dwBlueMask = 0;
    m_nRedShift = m_nGreenShift = m_nBlueShift = 0;
    m_nRedBits = m_nGreenBits = m_nBlueBits = 0;
    m_pOriginalRGB24 = nullptr;
    m_nOriginalWidth = 0;
    m_nOriginalHeight = 0;
    m_pFFTData = nullptr;
    m_bFFTValid = false;
    m_bInFrequencyDomain = false;
    m_nFFTWidth = 0;
    m_nFFTHeight = 0;
}

CImageProc::~CImageProc()
{
    CleanUp();
    delete pDib;
    delete pBFH;
    delete pBIH;
    delete pQUAD;
    delete pBits;
    if (m_hDib != NULL) GlobalUnlock(m_hDib);
    if (m_pOriginalRGB24) { delete[] m_pOriginalRGB24; m_pOriginalRGB24 = nullptr; }
    if (m_pFFTData) { delete[] m_pFFTData;    m_pFFTData = nullptr; }

}

void CImageProc::CleanUp()
{
    if (pBits) { delete[] pBits; pBits = nullptr; }
    if (pQUAD) { delete[] pQUAD; pQUAD = nullptr; }
    if (m_hDib) { ::GlobalFree(m_hDib); m_hDib = NULL; }
    if (m_pRGB24) { delete[] m_pRGB24; m_pRGB24 = nullptr; }
    if (m_pOriginalRGB24) { delete[] m_pOriginalRGB24; m_pOriginalRGB24 = nullptr; }
    if (m_pFFTData) { delete[] m_pFFTData; m_pFFTData = nullptr; }
    m_bFFTValid = false;
    m_bInFrequencyDomain = false;
    m_nFFTWidth = 0;
    m_nFFTHeight = 0;
}

void CImageProc::OpenFile()
{
    CFileDialog dlg(TRUE, _T("bmp"), NULL,
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        _T("BMP Files (*.bmp)|*.bmp|JPG Files (*.jpg)|*.jpg|All Files (*.*)|*.*||"));
    if (dlg.DoModal() == IDOK)
    {
        CString path = dlg.GetPathName();
        CString ext = path.Right(3);
        ext.MakeLower();
        if (ext == _T("bmp"))
            LoadBmp(path);
        else if (ext == _T("jpg"))
            LoadJpg(path);
        CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
        CView* pView = pFrame->GetActiveView();
        if (pView) pView->Invalidate(TRUE);
    }
}

void CImageProc::LoadBmp(CString strPathName)
{
    CleanUp();
    CFile file;
    if (!file.Open(strPathName, CFile::modeRead)) return;

    file.Read(pBFH, sizeof(BITMAPFILEHEADER));
    if (pBFH->bfType != 0x4D42) { AfxMessageBox(_T("不是有效的BMP文件")); return; }
    file.Read(pBIH, sizeof(BITMAPINFOHEADER));

    nWidth = pBIH->biWidth;
    nHeight = pBIH->biHeight;
    int bitCount = pBIH->biBitCount;

    if (pBIH->biClrUsed != 0)
        nNumColors = pBIH->biClrUsed;
    else if (bitCount <= 8)
        nNumColors = 1 << bitCount;
    else
        nNumColors = 0;

    if (bitCount == 16 && pBIH->biCompression == BI_BITFIELDS)
    {
        file.Read(&m_rMask, sizeof(DWORD));
        file.Read(&m_gMask, sizeof(DWORD));
        file.Read(&m_bMask, sizeof(DWORD));
    }
    else
    {
        m_rMask = m_gMask = m_bMask = 0;
    }

    if (nNumColors > 0)
    {
        pQUAD = new RGBQUAD[nNumColors];
        file.Read(pQUAD, sizeof(RGBQUAD) * nNumColors);
    }

    int bytesPerLine = ((nWidth * bitCount + 31) / 32) * 4;
    int dataSize = bytesPerLine * abs(nHeight);
    pBits = new BYTE[dataSize];
    file.Seek(pBFH->bfOffBits, CFile::begin);
    file.Read(pBits, dataSize);
    file.Close();

    if (pBIH->biHeight > 0)
    {
        BYTE* temp = new BYTE[bytesPerLine];
        for (int i = 0; i < nHeight / 2; i++)
        {
            memcpy(temp, pBits + i * bytesPerLine, bytesPerLine);
            memcpy(pBits + i * bytesPerLine, pBits + (nHeight - 1 - i) * bytesPerLine, bytesPerLine);
            memcpy(pBits + (nHeight - 1 - i) * bytesPerLine, temp, bytesPerLine);
        }
        delete[] temp;
        pBIH->biHeight = -pBIH->biHeight;
    }
    nHeight = abs(nHeight);

    m_nWidth = nWidth;
    m_nHeight = nHeight;
    int bytesPerLine24 = ((nWidth * 24 + 31) / 32) * 4;
    m_pRGB24 = new BYTE[bytesPerLine24 * nHeight];
    memset(m_pRGB24, 0, bytesPerLine24 * nHeight);

    for (int y = 0; y < nHeight; y++)
    {
        BYTE* pSrcRow = pBits + y * bytesPerLine;
        BYTE* pDstRow = m_pRGB24 + y * bytesPerLine24;

        for (int x = 0; x < nWidth; x++)
        {
            BYTE r = 0, g = 0, b = 0;
            if (bitCount == 1)
            {
                int byteIdx = x / 8;
                int bitIdx = 7 - (x % 8);
                int idx = (pSrcRow[byteIdx] >> bitIdx) & 1;
                r = pQUAD[idx].rgbRed;
                g = pQUAD[idx].rgbGreen;
                b = pQUAD[idx].rgbBlue;
            }
            else if (bitCount == 4)
            {
                int byteIdx = x / 2;
                int idx = (x % 2 == 0) ? (pSrcRow[byteIdx] >> 4) : (pSrcRow[byteIdx] & 0x0F);
                r = pQUAD[idx].rgbRed;
                g = pQUAD[idx].rgbGreen;
                b = pQUAD[idx].rgbBlue;
            }
            else if (bitCount == 8)
            {
                int idx = pSrcRow[x];
                r = pQUAD[idx].rgbRed;
                g = pQUAD[idx].rgbGreen;
                b = pQUAD[idx].rgbBlue;
            }
            else if (bitCount == 16)
            {
                WORD pixel = *(WORD*)(pSrcRow + x * 2);
                if (pBIH->biCompression == BI_BITFIELDS && (m_rMask | m_gMask | m_bMask) != 0)
                {
                    auto extract = [](WORD px, DWORD mask) -> BYTE {
                        if (mask == 0) return 0;
                        int shift = 0;
                        while ((mask & (1 << shift)) == 0) shift++;
                        int bits = 0;
                        DWORD m = mask >> shift;
                        while (m) { bits++; m >>= 1; }
                        int val = (px & mask) >> shift;
                        int maxVal = (1 << bits) - 1;
                        return (BYTE)((val * 255.0 / maxVal) + 0.5);
                        };
                    r = extract(pixel, m_rMask);
                    g = extract(pixel, m_gMask);
                    b = extract(pixel, m_bMask);
                }
                else
                {
                    r = (BYTE)((((pixel & 0x7C00) >> 10) * 255.0 / 31) + 0.5);
                    g = (BYTE)((((pixel & 0x03E0) >> 5) * 255.0 / 31) + 0.5);
                    b = (BYTE)(((pixel & 0x001F) * 255.0 / 31) + 0.5);
                }
            }
            else if (bitCount == 24)
            {
                b = pSrcRow[x * 3];
                g = pSrcRow[x * 3 + 1];
                r = pSrcRow[x * 3 + 2];
            }
            else if (bitCount == 32)
            {
                b = pSrcRow[x * 4];
                g = pSrcRow[x * 4 + 1];
                r = pSrcRow[x * 4 + 2];
            }
            pDstRow[x * 3] = b;
            pDstRow[x * 3 + 1] = g;
            pDstRow[x * 3 + 2] = r;
        }
    }

    int extraSize = 0;
    if (bitCount == 16 && pBIH->biCompression == BI_BITFIELDS)
        extraSize = 3 * sizeof(DWORD);

    BITMAPINFO* pBMI = (BITMAPINFO*)new BYTE[sizeof(BITMAPINFOHEADER) + nNumColors * sizeof(RGBQUAD) + extraSize];
    memcpy(pBMI, pBIH, sizeof(BITMAPINFOHEADER));
    if (nNumColors > 0)
        memcpy(pBMI->bmiColors, pQUAD, nNumColors * sizeof(RGBQUAD));

    if (extraSize > 0)
    {
        DWORD* pMasks = (DWORD*)((BYTE*)pBMI + sizeof(BITMAPINFOHEADER) + nNumColors * sizeof(RGBQUAD));
        pMasks[0] = m_rMask;
        pMasks[1] = m_gMask;
        pMasks[2] = m_bMask;
    }

    m_hDib = ::GlobalAlloc(GHND, sizeof(BITMAPINFOHEADER) + nNumColors * sizeof(RGBQUAD) + extraSize + dataSize);
    if (m_hDib)
    {
        BYTE* lpDib = (BYTE*)::GlobalLock(m_hDib);
        memcpy(lpDib, pBMI, sizeof(BITMAPINFOHEADER) + nNumColors * sizeof(RGBQUAD) + extraSize);
        memcpy(lpDib + sizeof(BITMAPINFOHEADER) + nNumColors * sizeof(RGBQUAD) + extraSize, pBits, dataSize);
        ::GlobalUnlock(m_hDib);
    }
    delete[](BYTE*)pBMI;
    SaveOriginalData();
}

void CImageProc::LoadJpg(CString strPathName)
{
    CleanUp();
    CImage img;
    if (FAILED(img.Load(strPathName))) return;
    nWidth = img.GetWidth();
    nHeight = img.GetHeight();
    BITMAPINFOHEADER bi = { 0 };
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = nWidth;
    bi.biHeight = -nHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    int bytesPerLine = ((nWidth * 24 + 31) / 32) * 4;
    bi.biSizeImage = bytesPerLine * nHeight;
    pBits = new BYTE[bi.biSizeImage];
    for (int y = 0; y < nHeight; y++)
    {
        for (int x = 0; x < nWidth; x++)
        {
            COLORREF cr = img.GetPixel(x, y);
            BYTE* dst = pBits + y * bytesPerLine + x * 3;
            dst[0] = GetBValue(cr);
            dst[1] = GetGValue(cr);
            dst[2] = GetRValue(cr);
        }
    }
    BITMAPINFO* pBMI = (BITMAPINFO*)new BYTE[sizeof(BITMAPINFOHEADER)];
    memcpy(pBMI, &bi, sizeof(BITMAPINFOHEADER));
    m_hDib = ::GlobalAlloc(GHND, sizeof(BITMAPINFOHEADER) + bi.biSizeImage);
    if (m_hDib)
    {
        BYTE* lpDib = (BYTE*)::GlobalLock(m_hDib);
        memcpy(lpDib, pBMI, sizeof(BITMAPINFOHEADER));
        memcpy(lpDib + sizeof(BITMAPINFOHEADER), pBits, bi.biSizeImage);
        ::GlobalUnlock(m_hDib);
    }
    delete[](BYTE*)pBMI;
    SaveOriginalData();
}

void CImageProc::ShowBMP(CDC* pDC)
{
    if (!m_hDib) return;
    BITMAPINFOHEADER* pHeader = (BITMAPINFOHEADER*)::GlobalLock(m_hDib);
    if (!pHeader) return;

    BYTE* pData = (BYTE*)pHeader + sizeof(BITMAPINFOHEADER);
    if (pHeader->biBitCount <= 8)
    {
        int numColors = pHeader->biClrUsed;
        if (numColors == 0) numColors = 1 << pHeader->biBitCount;
        pData += numColors * sizeof(RGBQUAD);
    }
    else if (pHeader->biBitCount == 16 && pHeader->biCompression == BI_BITFIELDS)
    {
        pData += 3 * sizeof(DWORD);
    }

    ::SetStretchBltMode(pDC->m_hDC, COLORONCOLOR);
    ::StretchDIBits(pDC->m_hDC, 0, 0, nWidth, nHeight,
        0, 0, nWidth, nHeight,
        pData, (BITMAPINFO*)pHeader, DIB_RGB_COLORS, SRCCOPY);
    ::GlobalUnlock(m_hDib);
}

COLORREF CImageProc::GetColor(CDC* pDC, int x, int y)
{
    if (!m_pRGB24) return RGB(0, 0, 0);
    if (x < 0 || x >= m_nWidth || y < 0 || y >= m_nHeight) return RGB(0, 0, 0);
    return pDC->GetPixel(x, y);
}

COLORREF CImageProc::GetPixelColorManual(int x, int y)
{
    if (x < 0 || x >= m_nWidth || y < 0 || y >= m_nHeight || m_pRGB24 == NULL)
        return RGB(0, 0, 0);

    int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
    BYTE* pPixel = m_pRGB24 + y * bytesPerLine + x * 3;
    BYTE b = pPixel[0];
    BYTE g = pPixel[1];
    BYTE r = pPixel[2];
    return RGB(r, g, b);
}

COLORREF CImageProc::GetOriginalPixel(int x, int y)
{
    if (!m_pRGB24 || x < 0 || x >= m_nWidth || y < 0 || y >= m_nHeight)
        return RGB(0, 0, 0);
    int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
    BYTE* pPixel = m_pRGB24 + y * bytesPerLine + x * 3;
    return RGB(pPixel[2], pPixel[1], pPixel[0]);
}

void CImageProc::ConvertToGray()
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0) return;
    for (int y = 0; y < m_nHeight; y++)
    {
        int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < m_nWidth; x++)
        {
            BYTE b = pRow[x * 3];
            BYTE g = pRow[x * 3 + 1];
            BYTE r = pRow[x * 3 + 2];
            BYTE gray = (BYTE)(0.299 * r + 0.587 * g + 0.114 * b);
            pRow[x * 3] = gray;
            pRow[x * 3 + 1] = gray;
            pRow[x * 3 + 2] = gray;
        }
    }
}

void CImageProc::ShowHistogram()
{
    if (!m_pRGB24) return;
    int hist[256] = { 0 };
    int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
    for (int y = 0; y < m_nHeight; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < m_nWidth; x++)
        {
            BYTE b = pRow[x * 3];
            BYTE g = pRow[x * 3 + 1];
            BYTE r = pRow[x * 3 + 2];
            BYTE gray = (BYTE)(0.299 * r + 0.587 * g + 0.114 * b);
            hist[gray]++;
        }
    }
    int maxCount = 0;
    for (int i = 0; i < 256; i++)
        if (hist[i] > maxCount) maxCount = hist[i];
    CHistogramDlg dlg;
    dlg.SetHistogramData(hist, maxCount);
    dlg.DoModal();
}

void CImageProc::LinearTransform(int low_in, int high_in, int low_out, int high_out)
{
    if (!m_pRGB24) return;
    ConvertToGray();
    for (int y = 0; y < m_nHeight; y++)
    {
        int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < m_nWidth; x++)
        {
            int old = pRow[x * 3];
            int newVal = (old - low_in) * (high_out - low_out) / (high_in - low_in) + low_out;
            if (newVal < 0) newVal = 0;
            if (newVal > 255) newVal = 255;
            BYTE val = (BYTE)newVal;
            pRow[x * 3] = pRow[x * 3 + 1] = pRow[x * 3 + 2] = val;
        }
    }
}

void CImageProc::HistogramEqualize()
{
    if (!m_pRGB24) return;
    ConvertToGray();
    int hist[256] = { 0 };
    int totalPixels = m_nWidth * m_nHeight;
    for (int y = 0; y < m_nHeight; y++)
    {
        int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < m_nWidth; x++)
        {
            hist[pRow[x * 3]]++;
        }
    }
    int cdf[256] = { 0 };
    cdf[0] = hist[0];
    for (int i = 1; i < 256; i++) cdf[i] = cdf[i - 1] + hist[i];
    BYTE map[256];
    for (int i = 0; i < 256; i++)
    {
        map[i] = (BYTE)((double)cdf[i] * 255 / totalPixels);
    }
    for (int y = 0; y < m_nHeight; y++)
    {
        int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < m_nWidth; x++)
        {
            BYTE newVal = map[pRow[x * 3]];
            pRow[x * 3] = pRow[x * 3 + 1] = pRow[x * 3 + 2] = newVal;
        }
    }
}

void CImageProc::PaletteTransform()
{
    if (!m_pRGB24) return;
    ConvertToGray();
    for (int y = 0; y < m_nHeight; y++)
    {
        int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < m_nWidth; x++)
        {
            int gray = pRow[x * 3];
            BYTE r, g, b;
            if (gray < 128)
            {
                r = 0;
                g = gray * 2;
                b = 255 - gray * 2;
            }
            else
            {
                r = (gray - 128) * 2;
                g = 255 - (gray - 128) * 2;
                b = 0;
            }
            pRow[x * 3] = b;
            pRow[x * 3 + 1] = g;
            pRow[x * 3 + 2] = r;
        }
    }
}

void CImageProc::CalculateHistogram(int hist[256], int& maxCount) const
{
    memset(hist, 0, sizeof(int) * 256);
    if (!m_pRGB24) return;
    int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
    maxCount = 0;
    for (int y = 0; y < m_nHeight; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < m_nWidth; x++)
        {
            BYTE b = pRow[x * 3];
            BYTE g = pRow[x * 3 + 1];
            BYTE r = pRow[x * 3 + 2];
            BYTE gray = (BYTE)(0.299 * r + 0.587 * g + 0.114 * b);
            hist[gray]++;
        }
    }
    for (int i = 0; i < 256; i++)
        if (hist[i] > maxCount) maxCount = hist[i];
}

void CImageProc::AdaptiveHistogramEqualize(int blockSize)
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0) return;
    ConvertToGray();
    int width = m_nWidth;
    int height = m_nHeight;
    int bytesPerLine = ((width * 24 + 31) / 32) * 4;
    BYTE* grayData = new BYTE[width * height];
    for (int y = 0; y < height; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < width; x++)
        {
            grayData[y * width + x] = pRow[x * 3];
        }
    }
    int blocksX = (width + blockSize - 1) / blockSize;
    int blocksY = (height + blockSize - 1) / blockSize;
    BYTE** blockMaps = new BYTE * [blocksY * blocksX];
    for (int i = 0; i < blocksY * blocksX; i++)
        blockMaps[i] = new BYTE[256];
    for (int by = 0; by < blocksY; by++)
    {
        for (int bx = 0; bx < blocksX; bx++)
        {
            int xStart = bx * blockSize;
            int yStart = by * blockSize;
            int xEnd = min(xStart + blockSize, width);
            int yEnd = min(yStart + blockSize, height);
            int totalPixels = (xEnd - xStart) * (yEnd - yStart);
            int hist[256] = { 0 };
            for (int y = yStart; y < yEnd; y++)
            {
                for (int x = xStart; x < xEnd; x++)
                {
                    BYTE val = grayData[y * width + x];
                    hist[val]++;
                }
            }
            int cdf[256] = { 0 };
            cdf[0] = hist[0];
            for (int i = 1; i < 256; i++)
                cdf[i] = cdf[i - 1] + hist[i];
            BYTE* map = blockMaps[by * blocksX + bx];
            for (int i = 0; i < 256; i++)
            {
                map[i] = (BYTE)((double)cdf[i] * 255.0 / totalPixels);
            }
        }
    }
    BYTE* resultGray = new BYTE[width * height];
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float fx = (float)(x) / blockSize - 0.5f;
            float fy = (float)(y) / blockSize - 0.5f;
            int bx = (int)floor(fx);
            int by = (int)floor(fy);
            int bx0 = max(0, min(bx, blocksX - 1));
            int bx1 = max(0, min(bx + 1, blocksX - 1));
            int by0 = max(0, min(by, blocksY - 1));
            int by1 = max(0, min(by + 1, blocksY - 1));
            float u = fx - bx;
            float v = fy - by;
            if (u < 0) u = 0; if (u > 1) u = 1;
            if (v < 0) v = 0; if (v > 1) v = 1;
            BYTE gray = grayData[y * width + x];
            BYTE v00 = blockMaps[by0 * blocksX + bx0][gray];
            BYTE v10 = blockMaps[by0 * blocksX + bx1][gray];
            BYTE v01 = blockMaps[by1 * blocksX + bx0][gray];
            BYTE v11 = blockMaps[by1 * blocksX + bx1][gray];
            float val = (1 - u) * (1 - v) * v00 + u * (1 - v) * v10 + (1 - u) * v * v01 + u * v * v11;
            resultGray[y * width + x] = (BYTE)(val + 0.5f);
        }
    }
    for (int y = 0; y < height; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < width; x++)
        {
            BYTE val = resultGray[y * width + x];
            pRow[x * 3] = val;
            pRow[x * 3 + 1] = val;
            pRow[x * 3 + 2] = val;
        }
    }
    delete[] grayData;
    delete[] resultGray;
    for (int i = 0; i < blocksY * blocksX; i++)
        delete[] blockMaps[i];
    delete[] blockMaps;
}

void CImageProc::ApplyPseudoColor(int scheme)
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0) return;
    ConvertToGray();
    int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
    for (int y = 0; y < m_nHeight; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < m_nWidth; x++)
        {
            BYTE gray = pRow[x * 3];
            BYTE r, g, b;
            switch (scheme)
            {
            case SCHEME_DEFAULT:
                if (gray < 128)
                {
                    r = 0; g = gray * 2; b = 255 - gray * 2;
                }
                else
                {
                    r = (gray - 128) * 2; g = 255 - (gray - 128) * 2; b = 0;
                }
                break;
            case SCHEME_HOT:
            {
                float t = gray / 255.0f;
                if (t < 0.333f) { r = (BYTE)(255 * (t / 0.333f)); g = 0; b = 0; }
                else if (t < 0.666f) { r = 255; g = (BYTE)(255 * ((t - 0.333f) / 0.333f)); b = 0; }
                else { r = 255; g = 255; b = (BYTE)(255 * ((t - 0.666f) / 0.334f)); }
            }
            break;
            case SCHEME_RAINBOW:
            {
                float t = gray / 255.0f;
                r = (BYTE)((sin(t * 2 * 3.14159f + 0) * 0.5f + 0.5f) * 255);
                g = (BYTE)((sin(t * 2 * 3.14159f + 2.094f) * 0.5f + 0.5f) * 255);
                b = (BYTE)((sin(t * 2 * 3.14159f + 4.188f) * 0.5f + 0.5f) * 255);
            }
            break;
            case SCHEME_COOL:
            {
                float t = gray / 255.0f;
                r = 0; g = (BYTE)(255 * t); b = (BYTE)(255 * (1.0f - t));
            }
            break;
            case SCHEME_INVERT:
            {
                BYTE inv = 255 - gray;
                r = g = b = inv;
            }
            break;
            default:
                r = g = b = gray;
                break;
            }
            pRow[x * 3] = b;
            pRow[x * 3 + 1] = g;
            pRow[x * 3 + 2] = r;
        }
    }
}

bool CImageProc::HistogramSpecify(const CString& strTargetPath)
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0) return false;
    CImageProc targetImg;
    CString ext = strTargetPath.Right(3);
    ext.MakeLower();
    if (ext == _T("bmp"))
        targetImg.LoadBmp(strTargetPath);
    else if (ext == _T("jpg"))
        targetImg.LoadJpg(strTargetPath);
    else
        return false;
    if (!targetImg.m_pRGB24 || targetImg.m_nWidth <= 0 || targetImg.m_nHeight <= 0) return false;
    int bytesPerLineSrc = ((m_nWidth * 24 + 31) / 32) * 4;
    int bytesPerLineTgt = ((targetImg.m_nWidth * 24 + 31) / 32) * 4;
    int totalSrc = m_nWidth * m_nHeight;
    int totalTgt = targetImg.m_nWidth * targetImg.m_nHeight;
    auto ComputeChannelCDF = [](BYTE* pData, int width, int height, int bytesPerLine, int channelOffset, int totalPixels, double cdf[256]) {
        int hist[256] = { 0 };
        for (int y = 0; y < height; y++)
        {
            BYTE* pRow = pData + y * bytesPerLine;
            for (int x = 0; x < width; x++)
            {
                BYTE val = pRow[x * 3 + channelOffset];
                hist[val]++;
            }
        }
        int sum = 0;
        for (int i = 0; i < 256; i++)
        {
            sum += hist[i];
            cdf[i] = (double)sum / totalPixels;
        }
        };
    auto BuildMap = [](double srcCDF[256], double tgtCDF[256], BYTE map[256]) {
        for (int s = 0; s < 256; s++)
        {
            double srcVal = srcCDF[s];
            int bestT = 0;
            double minDiff = 1.0;
            for (int t = 0; t < 256; t++)
            {
                double diff = fabs(tgtCDF[t] - srcVal);
                if (diff < minDiff)
                {
                    minDiff = diff;
                    bestT = t;
                }
            }
            map[s] = (BYTE)bestT;
        }
        };
    double srcCDF_B[256], srcCDF_G[256], srcCDF_R[256];
    double tgtCDF_B[256], tgtCDF_G[256], tgtCDF_R[256];
    ComputeChannelCDF(m_pRGB24, m_nWidth, m_nHeight, bytesPerLineSrc, 0, totalSrc, srcCDF_B);
    ComputeChannelCDF(m_pRGB24, m_nWidth, m_nHeight, bytesPerLineSrc, 1, totalSrc, srcCDF_G);
    ComputeChannelCDF(m_pRGB24, m_nWidth, m_nHeight, bytesPerLineSrc, 2, totalSrc, srcCDF_R);
    ComputeChannelCDF(targetImg.m_pRGB24, targetImg.m_nWidth, targetImg.m_nHeight, bytesPerLineTgt, 0, totalTgt, tgtCDF_B);
    ComputeChannelCDF(targetImg.m_pRGB24, targetImg.m_nWidth, targetImg.m_nHeight, bytesPerLineTgt, 1, totalTgt, tgtCDF_G);
    ComputeChannelCDF(targetImg.m_pRGB24, targetImg.m_nWidth, targetImg.m_nHeight, bytesPerLineTgt, 2, totalTgt, tgtCDF_R);
    BYTE mapB[256], mapG[256], mapR[256];
    BuildMap(srcCDF_B, tgtCDF_B, mapB);
    BuildMap(srcCDF_G, tgtCDF_G, mapG);
    BuildMap(srcCDF_R, tgtCDF_R, mapR);
    for (int y = 0; y < m_nHeight; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLineSrc;
        for (int x = 0; x < m_nWidth; x++)
        {
            pRow[x * 3 + 0] = mapB[pRow[x * 3 + 0]];
            pRow[x * 3 + 1] = mapG[pRow[x * 3 + 1]];
            pRow[x * 3 + 2] = mapR[pRow[x * 3 + 2]];
        }
    }
    return true;
}

void CImageProc::MeanFilter(int kSize)
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0 || kSize % 2 == 0) return;
    ConvertToGray();
    int width = m_nWidth;
    int height = m_nHeight;
    int bytesPerLine = ((width * 24 + 31) / 32) * 4;
    int half = kSize / 2;
    BYTE* src = new BYTE[width * height];
    for (int y = 0; y < height; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < width; x++)
            src[y * width + x] = pRow[x * 3];
    }
    for (int y = 0; y < height; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < width; x++)
        {
            int sum = 0, cnt = 0;
            for (int dy = -half; dy <= half; dy++)
            {
                int ny = y + dy;
                if (ny < 0 || ny >= height) continue;
                for (int dx = -half; dx <= half; dx++)
                {
                    int nx = x + dx;
                    if (nx < 0 || nx >= width) continue;
                    sum += src[ny * width + nx];
                    cnt++;
                }
            }
            BYTE mean = (cnt > 0) ? (BYTE)(sum / cnt) : src[y * width + x];
            pRow[x * 3] = pRow[x * 3 + 1] = pRow[x * 3 + 2] = mean;
        }
    }
    delete[] src;
}

void CImageProc::MedianFilter(int kSize)
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0 || kSize % 2 == 0) return;
    ConvertToGray();
    int width = m_nWidth;
    int height = m_nHeight;
    int bytesPerLine = ((width * 24 + 31) / 32) * 4;
    int half = kSize / 2;
    BYTE* src = new BYTE[width * height];
    for (int y = 0; y < height; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < width; x++)
            src[y * width + x] = pRow[x * 3];
    }
    std::vector<BYTE> neighbors;
    for (int y = 0; y < height; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < width; x++)
        {
            neighbors.clear();
            for (int dy = -half; dy <= half; dy++)
            {
                int ny = y + dy;
                if (ny < 0 || ny >= height) continue;
                for (int dx = -half; dx <= half; dx++)
                {
                    int nx = x + dx;
                    if (nx < 0 || nx >= width) continue;
                    neighbors.push_back(src[ny * width + nx]);
                }
            }
            if (neighbors.empty())
            {
                pRow[x * 3] = pRow[x * 3 + 1] = pRow[x * 3 + 2] = src[y * width + x];
                continue;
            }
            std::nth_element(neighbors.begin(), neighbors.begin() + neighbors.size() / 2, neighbors.end());
            BYTE med = neighbors[neighbors.size() / 2];
            pRow[x * 3] = pRow[x * 3 + 1] = pRow[x * 3 + 2] = med;
        }
    }
    delete[] src;
}

void CImageProc::MaxFilter(int kSize)
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0 || kSize % 2 == 0) return;
    ConvertToGray();
    int width = m_nWidth;
    int height = m_nHeight;
    int bytesPerLine = ((width * 24 + 31) / 32) * 4;
    int half = kSize / 2;
    BYTE* src = new BYTE[width * height];
    for (int y = 0; y < height; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < width; x++)
            src[y * width + x] = pRow[x * 3];
    }
    for (int y = 0; y < height; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < width; x++)
        {
            BYTE maxVal = 0;
            bool found = false;
            for (int dy = -half; dy <= half; dy++)
            {
                int ny = y + dy;
                if (ny < 0 || ny >= height) continue;
                for (int dx = -half; dx <= half; dx++)
                {
                    int nx = x + dx;
                    if (nx < 0 || nx >= width) continue;
                    BYTE val = src[ny * width + nx];
                    if (!found || val > maxVal) { maxVal = val; found = true; }
                }
            }
            if (!found) maxVal = src[y * width + x];
            pRow[x * 3] = pRow[x * 3 + 1] = pRow[x * 3 + 2] = maxVal;
        }
    }
    delete[] src;
}

void CImageProc::AddSaltPepperNoise(double saltProb, double pepperProb)
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0) return;
    int width = m_nWidth;
    int height = m_nHeight;
    int bytesPerLine = ((width * 24 + 31) / 32) * 4;
    srand((unsigned int)time(NULL));
    for (int y = 0; y < height; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < width; x++)
        {
            double r = (double)rand() / RAND_MAX;
            if (r < saltProb)
            {
                pRow[x * 3] = 255;
                pRow[x * 3 + 1] = 255;
                pRow[x * 3 + 2] = 255;
            }
            else if (r < saltProb + pepperProb)
            {
                pRow[x * 3] = 0;
                pRow[x * 3 + 1] = 0;
                pRow[x * 3 + 2] = 0;
            }
        }
    }
}

void CImageProc::AddImpulseNoise(double probability)
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0) return;
    int width = m_nWidth;
    int height = m_nHeight;
    int bytesPerLine = ((width * 24 + 31) / 32) * 4;
    srand((unsigned int)time(NULL));
    for (int y = 0; y < height; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < width; x++)
        {
            double r = (double)rand() / RAND_MAX;
            if (r < probability)
            {
                int pulse = rand() % 2;
                BYTE value = (pulse == 0) ? 255 : 0;
                pRow[x * 3] = value;
                pRow[x * 3 + 1] = value;
                pRow[x * 3 + 2] = value;
            }
        }
    }
}

void CImageProc::AddGaussianNoise(double mean, double stddev)
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0) return;
    int width = m_nWidth;
    int height = m_nHeight;
    int bytesPerLine = ((width * 24 + 31) / 32) * 4;
    srand((unsigned int)time(NULL));
    for (int y = 0; y < height; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < width; x++)
        {
            double u1 = (double)rand() / RAND_MAX;
            double u2 = (double)rand() / RAND_MAX;
            double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * 3.1415926535 * u2);
            int noise = (int)(mean + stddev * z0);
            for (int c = 0; c < 3; c++)
            {
                int newVal = pRow[x * 3 + c] + noise;
                if (newVal < 0) newVal = 0;
                if (newVal > 255) newVal = 255;
                pRow[x * 3 + c] = (BYTE)newVal;
            }
        }
    }
}

void CImageProc::AddWhiteGaussianNoise(double mean, double stddev)
{
    AddGaussianNoise(mean, stddev);
}

void CImageProc::SobelEdgeDetection(int kernelSize, int threshold, bool bBinaryOutput)
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0) return;

    if (kernelSize < 3) kernelSize = 3;
    if (kernelSize % 2 == 0) kernelSize++;

    const int MAX_KERNEL = 7;
    if (kernelSize > MAX_KERNEL) kernelSize = MAX_KERNEL;

    ConvertToGray();

    int width = m_nWidth;
    int height = m_nHeight;
    int bytesPerLine = ((width * 24 + 31) / 32) * 4;

    BYTE* src = new BYTE[width * height];
    for (int y = 0; y < height; y++) {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < width; x++) {
            src[y * width + x] = pRow[x * 3];
        }
    }

    int half = kernelSize / 2;
    int kernelArea = kernelSize * kernelSize;

    int* sobelX = new int[kernelArea];
    int* sobelY = new int[kernelArea];
    memset(sobelX, 0, sizeof(int) * kernelArea);
    memset(sobelY, 0, sizeof(int) * kernelArea);

    if (kernelSize == 3) {
        sobelX[0] = -1; sobelX[1] = 0; sobelX[2] = 1;
        sobelX[3] = -2; sobelX[4] = 0; sobelX[5] = 2;
        sobelX[6] = -1; sobelX[7] = 0; sobelX[8] = 1;
        sobelY[0] = -1; sobelY[1] = -2; sobelY[2] = -1;
        sobelY[3] = 0;  sobelY[4] = 0;  sobelY[5] = 0;
        sobelY[6] = 1;  sobelY[7] = 2;  sobelY[8] = 1;
    }
    else {
        for (int i = 0; i < kernelSize; i++) {
            for (int j = 0; j < kernelSize; j++) {
                int dx = j - half;
                int dy = i - half;
                double gaussian = exp(-(dx * dx + dy * dy) / (2.0 * (half / 1.5) * (half / 1.5)));
                if (abs(dx) <= half && abs(dy) <= half) {
                    double weightX = (dx == -half) ? -1 : ((dx == half) ? 1 : 0) * gaussian;
                    double weightY = (dy == -half) ? -1 : ((dy == half) ? 1 : 0) * gaussian;
                    sobelX[i * kernelSize + j] = (int)(weightX * 10);
                    sobelY[i * kernelSize + j] = (int)(weightY * 10);
                }
            }
        }
    }

    double* magnitude = new double[width * height];
    memset(magnitude, 0, sizeof(double) * width * height);

    double maxMag = 0;
    double sumMag = 0;
    int magCount = 0;

    for (int y = half; y < height - half; y++) {
        for (int x = half; x < width - half; x++) {
            double gx = 0, gy = 0;
            for (int ky = -half; ky <= half; ky++) {
                for (int kx = -half; kx <= half; kx++) {
                    int pixel = src[(y + ky) * width + (x + kx)];
                    int kidx = (ky + half) * kernelSize + (kx + half);
                    gx += pixel * sobelX[kidx];
                    gy += pixel * sobelY[kidx];
                }
            }
            double mag = sqrt(gx * gx + gy * gy);
            magnitude[y * width + x] = mag;
            if (mag > maxMag) maxMag = mag;
            sumMag += mag;
            if (mag > 0) magCount++;
        }
    }

    int finalThreshold = threshold;
    if (finalThreshold <= 0 && magCount > 0) {
        finalThreshold = (int)((sumMag / magCount) * 1.5);
        if (finalThreshold < 10) finalThreshold = 10;
        if (finalThreshold > 100) finalThreshold = 100;
    }

    for (int y = 0; y < height; y++) {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < width; x++) {
            BYTE val;
            if (x >= half && x < width - half && y >= half && y < height - half) {
                double mag = magnitude[y * width + x];
                if (bBinaryOutput) {
                    val = (mag > finalThreshold) ? 255 : 0;
                }
                else {
                    if (maxMag > 0) {
                        val = (BYTE)((mag / maxMag) * 255);
                    }
                    else {
                        val = 0;
                    }
                }
            }
            else {
                val = 0;
            }
            pRow[x * 3] = pRow[x * 3 + 1] = pRow[x * 3 + 2] = val;
        }
    }

    delete[] src;
    delete[] sobelX;
    delete[] sobelY;
    delete[] magnitude;
}

void CImageProc::PrewittEdgeDetection(int kernelSize, int threshold)
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0) return;
    if (kernelSize != 3) {
        AfxMessageBox(_T("Prewitt only supports 3x3 kernel"));
        return;
    }
    ConvertToGray();
    int width = m_nWidth;
    int height = m_nHeight;
    int bytesPerLine = ((width * 24 + 31) / 32) * 4;
    float gx[9] = { -1, 0, 1, -1, 0, 1, -1, 0, 1 };
    float gy[9] = { -1, -1, -1, 0, 0, 0, 1, 1, 1 };
    BYTE* src = new BYTE[width * height];
    for (int y = 0; y < height; y++) {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < width; x++) {
            src[y * width + x] = pRow[x * 3];
        }
    }
    int half = 1;
    for (int y = half; y < height - half; y++) {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = half; x < width - half; x++) {
            float sumX = 0, sumY = 0;
            for (int ky = -half; ky <= half; ky++) {
                for (int kx = -half; kx <= half; kx++) {
                    int pixel = src[(y + ky) * width + (x + kx)];
                    int kidx = (ky + half) * 3 + (kx + half);
                    sumX += pixel * gx[kidx];
                    sumY += pixel * gy[kidx];
                }
            }
            float magnitude = sqrtf(sumX * sumX + sumY * sumY);
            BYTE val = (magnitude > threshold) ? 255 : 0;
            pRow[x * 3] = pRow[x * 3 + 1] = pRow[x * 3 + 2] = val;
        }
    }
    delete[] src;
}

void CImageProc::SaveOriginalData()
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0) return;
    if (m_pOriginalRGB24)
    {
        delete[] m_pOriginalRGB24;
        m_pOriginalRGB24 = nullptr;
    }
    int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
    int dataSize = bytesPerLine * m_nHeight;
    m_pOriginalRGB24 = new BYTE[dataSize];
    memcpy(m_pOriginalRGB24, m_pRGB24, dataSize);
    m_nOriginalWidth = m_nWidth;
    m_nOriginalHeight = m_nHeight;
}

bool CImageProc::RestoreOriginal()
{
    if (!m_pOriginalRGB24 || m_nOriginalWidth <= 0 || m_nOriginalHeight <= 0)
    {
        return false;
    }
    int bytesPerLine = ((m_nOriginalWidth * 24 + 31) / 32) * 4;
    int newDataSize = bytesPerLine * m_nOriginalHeight;
    if (m_pRGB24)
    {
        delete[] m_pRGB24;
    }
    m_pRGB24 = new BYTE[newDataSize];
    memcpy(m_pRGB24, m_pOriginalRGB24, newDataSize);
    m_nWidth = m_nOriginalWidth;
    m_nHeight = m_nOriginalHeight;
    return true;
}

bool CImageProc::AddImages(const CString& strSecondImagePath)
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0) return false;
    CImageProc secondImg;
    CString ext = strSecondImagePath.Right(3);
    ext.MakeLower();
    if (ext == _T("bmp"))
        secondImg.LoadBmp(strSecondImagePath);
    else if (ext == _T("jpg"))
        secondImg.LoadJpg(strSecondImagePath);
    else
        return false;
    if (secondImg.m_nWidth != m_nWidth || secondImg.m_nHeight != m_nHeight)
    {
        AfxMessageBox(_T("Image size mismatch"));
        return false;
    }
    int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
    for (int y = 0; y < m_nHeight; y++)
    {
        BYTE* pRow1 = m_pRGB24 + y * bytesPerLine;
        BYTE* pRow2 = secondImg.m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < m_nWidth; x++)
        {
            for (int c = 0; c < 3; c++)
            {
                int sum = pRow1[x * 3 + c] + pRow2[x * 3 + c];
                if (sum > 255) sum = 255;
                pRow1[x * 3 + c] = (BYTE)sum;
            }
        }
    }
    return true;
}

bool CImageProc::MultiplyImages(const CString& strSecondImagePath)
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0) return false;
    CImageProc secondImg;
    CString ext = strSecondImagePath.Right(3);
    ext.MakeLower();
    if (ext == _T("bmp"))
        secondImg.LoadBmp(strSecondImagePath);
    else if (ext == _T("jpg"))
        secondImg.LoadJpg(strSecondImagePath);
    else
        return false;
    if (secondImg.m_nWidth != m_nWidth || secondImg.m_nHeight != m_nHeight)
    {
        AfxMessageBox(_T("Image size mismatch"));
        return false;
    }
    int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
    for (int y = 0; y < m_nHeight; y++)
    {
        BYTE* pRow1 = m_pRGB24 + y * bytesPerLine;
        BYTE* pRow2 = secondImg.m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < m_nWidth; x++)
        {
            for (int c = 0; c < 3; c++)
            {
                int product = (pRow1[x * 3 + c] * pRow2[x * 3 + c]) / 255;
                if (product > 255) product = 255;
                pRow1[x * 3 + c] = (BYTE)product;
            }
        }
    }
    return true;
}

void CImageProc::LaplacianEdgeDetection(int kernelSize, int threshold)
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0) return;

    int width = m_nWidth;
    int height = m_nHeight;
    int bytesPerLine = ((width * 24 + 31) / 32) * 4;

    BYTE* original = new BYTE[bytesPerLine * height];
    memcpy(original, m_pRGB24, bytesPerLine * height);

    BYTE* gray = new BYTE[width * height];
    for (int y = 0; y < height; y++) {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < width; x++) {
            BYTE b = pRow[x * 3];
            BYTE g = pRow[x * 3 + 1];
            BYTE r = pRow[x * 3 + 2];
            gray[y * width + x] = (BYTE)(0.299 * r + 0.587 * g + 0.114 * b);
        }
    }

    int laplacian[9];
    if (kernelSize == 1) {
        laplacian[0] = 0;  laplacian[1] = -1; laplacian[2] = 0;
        laplacian[3] = -1; laplacian[4] = 4;  laplacian[5] = -1;
        laplacian[6] = 0;  laplacian[7] = -1; laplacian[8] = 0;
    }
    else {
        laplacian[0] = -1; laplacian[1] = -1; laplacian[2] = -1;
        laplacian[3] = -1; laplacian[4] = 8;  laplacian[5] = -1;
        laplacian[6] = -1; laplacian[7] = -1; laplacian[8] = -1;
    }

    int* edge = new int[width * height];
    memset(edge, 0, sizeof(int) * width * height);

    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sum = 0;
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int pixel = gray[(y + ky) * width + (x + kx)];
                    int kidx = (ky + 1) * 3 + (kx + 1);
                    sum += pixel * laplacian[kidx];
                }
            }
            edge[y * width + x] = sum;
        }
    }

    int finalThreshold = threshold;
    if (finalThreshold <= 0) {
        int sumResponse = 0;
        int responseCount = 0;
        for (int y = 1; y < height - 1; y++) {
            for (int x = 1; x < width - 1; x++) {
                int response = (edge[y * width + x] < 0) ? -edge[y * width + x] : edge[y * width + x];
                sumResponse += response;
                if (response > 0) responseCount++;
            }
        }
        if (responseCount > 0) {
            finalThreshold = sumResponse / responseCount;
            if (finalThreshold < 10) finalThreshold = 10;
            if (finalThreshold > 100) finalThreshold = 100;
        }
        else {
            finalThreshold = 30;
        }
    }

    double c = 0.8;

    for (int y = 0; y < height; y++) {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        BYTE* pOriginal = original + y * bytesPerLine;
        for (int x = 0; x < width; x++) {
            int edgeValue = (y >= 1 && y < height - 1 && x >= 1 && x < width - 1)
                ? edge[y * width + x] : 0;

            for (int cIdx = 0; cIdx < 3; cIdx++) {
                int newVal = pOriginal[x * 3 + cIdx] + (int)(c * edgeValue);
                if (newVal < 0) newVal = 0;
                if (newVal > 255) newVal = 255;
                pRow[x * 3 + cIdx] = (BYTE)newVal;
            }
        }
    }

    delete[] original;
    delete[] gray;
    delete[] edge;
}

void CImageProc::PowerLawTransform(double gamma)
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0) return;
    ConvertToGray();
    int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
    BYTE map[256];
    for (int i = 0; i < 256; i++)
    {
        double normalized = i / 255.0;
        double corrected = pow(normalized, gamma);
        map[i] = (BYTE)(corrected * 255.0 + 0.5);
    }
    for (int y = 0; y < m_nHeight; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < m_nWidth; x++)
        {
            BYTE newVal = map[pRow[x * 3]];
            pRow[x * 3] = pRow[x * 3 + 1] = pRow[x * 3 + 2] = newVal;
        }
    }
}

// ============================================================================
// FFT/IFFT 实现
// ============================================================================

int CImageProc::NextPowerOfTwo(int n)
{
    int power = 1;
    while (power < n)
        power <<= 1;
    return power;
}

void CImageProc::FFT(std::complex<double>* data, int n, bool inverse)
{
    if (n <= 1) return;

    std::vector<std::complex<double>> even(n / 2), odd(n / 2);
    for (int i = 0; i < n / 2; i++)
    {
        even[i] = data[i * 2];
        odd[i] = data[i * 2 + 1];
    }

    FFT(even.data(), n / 2, inverse);
    FFT(odd.data(), n / 2, inverse);

    double angle = 2 * 3.14159265358979323846 * (inverse ? 1 : -1) / n;
    std::complex<double> w(1), wn(cos(angle), sin(angle));

    for (int i = 0; i < n / 2; i++)
    {
        data[i] = even[i] + w * odd[i];
        data[i + n / 2] = even[i] - w * odd[i];
        if (inverse)
        {
            data[i] /= 2.0;
            data[i + n / 2] /= 2.0;
        }
        w *= wn;
    }
}

void CImageProc::FFT2D(BYTE* spatialData, std::complex<double>* freqData, int width, int height, bool inverse)
{
    int fftWidth = NextPowerOfTwo(width);
    int fftHeight = NextPowerOfTwo(height);

    std::vector<std::complex<double>> temp(fftWidth * fftHeight);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int srcIdx = y * width + x;
            int dstIdx = y * fftWidth + x;
            BYTE gray = spatialData[srcIdx * 3];
            temp[dstIdx] = std::complex<double>(gray, 0);
        }
        for (int x = width; x < fftWidth; x++)
        {
            temp[y * fftWidth + x] = std::complex<double>(0, 0);
        }
        FFT(&temp[y * fftWidth], fftWidth, inverse);
    }

    std::vector<std::complex<double>> column(fftHeight);
    for (int x = 0; x < fftWidth; x++)
    {
        for (int y = 0; y < fftHeight; y++)
        {
            column[y] = temp[y * fftWidth + x];
        }
        FFT(column.data(), fftHeight, inverse);
        for (int y = 0; y < fftHeight; y++)
        {
            temp[y * fftWidth + x] = column[y];
        }
    }

    if (freqData)
    {
        for (int i = 0; i < fftWidth * fftHeight; i++)
        {
            freqData[i] = temp[i];
        }
    }
}

void CImageProc::CenterSpectrum(std::complex<double>* data, int width, int height)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (((x + y) & 1) == 1)
            {
                data[y * width + x] *= -1;
            }
        }
    }
}

void CImageProc::LogScaleSpectrum(double* magnitude, int size)
{
    for (int i = 0; i < size; i++)
    {
        if (magnitude[i] > 0)
        {
            magnitude[i] = log(1 + magnitude[i]);
        }
    }
}

bool CImageProc::ComputeFFT2D()
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0)
        return false;

    ConvertToGray();

    m_nFFTWidth = NextPowerOfTwo(m_nWidth);
    m_nFFTHeight = NextPowerOfTwo(m_nHeight);

    if (m_pFFTData)
        delete[] m_pFFTData;

    m_pFFTData = new std::complex<double>[m_nFFTWidth * m_nFFTHeight];
    if (!m_pFFTData)
        return false;

    FFT2D(m_pRGB24, m_pFFTData, m_nWidth, m_nHeight, false);

    CenterSpectrum(m_pFFTData, m_nFFTWidth, m_nFFTHeight);

    m_bFFTValid = true;
    return true;
}

bool CImageProc::ComputeIFFT2D()
{
    if (!m_bFFTValid || !m_pFFTData)
        return false;

    CenterSpectrum(m_pFFTData, m_nFFTWidth, m_nFFTHeight);

    FFT2D(m_pRGB24, m_pFFTData, m_nWidth, m_nHeight, true);

    CenterSpectrum(m_pFFTData, m_nFFTWidth, m_nFFTHeight);

    return true;
}

void CImageProc::ShowSpectrum(CDC* pDC)
{
    if (!m_bFFTValid || !m_pFFTData || !pDC)
        return;

    std::vector<double> magnitude(m_nFFTWidth * m_nFFTHeight);
    double maxMag = 0;

    for (int i = 0; i < m_nFFTWidth * m_nFFTHeight; i++)
    {
        magnitude[i] = std::abs(m_pFFTData[i]);
        if (magnitude[i] > maxMag)
            maxMag = magnitude[i];
    }

    LogScaleSpectrum(magnitude.data(), m_nFFTWidth * m_nFFTHeight);

    maxMag = 0;
    for (int i = 0; i < m_nFFTWidth * m_nFFTHeight; i++)
    {
        if (magnitude[i] > maxMag)
            maxMag = magnitude[i];
    }

    if (maxMag == 0) return;

    int bytesPerLine = ((m_nFFTWidth * 24 + 31) / 32) * 4;
    BYTE* pSpectrumData = new BYTE[bytesPerLine * m_nFFTHeight];
    memset(pSpectrumData, 0, bytesPerLine * m_nFFTHeight);

    for (int y = 0; y < m_nFFTHeight; y++)
    {
        BYTE* pRow = pSpectrumData + y * bytesPerLine;
        for (int x = 0; x < m_nFFTWidth; x++)
        {
            double normalized = magnitude[y * m_nFFTWidth + x] / maxMag;
            BYTE gray = (BYTE)(normalized * 255);
            pRow[x * 3] = gray;
            pRow[x * 3 + 1] = gray;
            pRow[x * 3 + 2] = gray;
        }
    }

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = m_nFFTWidth;
    bmi.bmiHeader.biHeight = -m_nFFTHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    ::SetStretchBltMode(pDC->m_hDC, COLORONCOLOR);
    ::StretchDIBits(pDC->m_hDC, 0, 0, m_nFFTWidth, m_nFFTHeight,
        0, 0, m_nFFTWidth, m_nFFTHeight,
        pSpectrumData, &bmi, DIB_RGB_COLORS, SRCCOPY);

    delete[] pSpectrumData;
}

// ============================================================================
// 同态滤波实现
// ============================================================================

void CImageProc::HomomorphicFilter(float gammaH, float gammaL, float c, float D0)
{
    if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0)
    {
        AfxMessageBox(_T("请先打开图像"));
        return;
    }

    // 1. 转换为灰度图
    ConvertToGray();

    int width = m_nWidth;
    int height = m_nHeight;
    int bytesPerLine = ((width * 24 + 31) / 32) * 4;

    // 2. 创建浮点型数据并取对数
    float** fImage = new float* [height];
    for (int i = 0; i < height; i++)
        fImage[i] = new float[width];

    for (int y = 0; y < height; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < width; x++)
        {
            BYTE gray = pRow[x * 3];
            fImage[y][x] = log((float)(gray + 1));
        }
    }

    // 3. 计算FFT尺寸
    int fftWidth = NextPowerOfTwo(width);
    int fftHeight = NextPowerOfTwo(height);

    // 4. 创建复数数组并补零
    std::complex<double>* complexData = new std::complex<double>[fftWidth * fftHeight];
    for (int i = 0; i < fftWidth * fftHeight; i++)
        complexData[i] = std::complex<double>(0, 0);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            complexData[y * fftWidth + x] = std::complex<double>(fImage[y][x], 0);
        }
    }

    // 5. 对每一行做FFT
    for (int y = 0; y < fftHeight; y++)
    {
        FFT(&complexData[y * fftWidth], fftWidth, false);
    }

    // 6. 对每一列做FFT
    std::vector<std::complex<double>> column(fftHeight);
    for (int x = 0; x < fftWidth; x++)
    {
        for (int y = 0; y < fftHeight; y++)
        {
            column[y] = complexData[y * fftWidth + x];
        }
        FFT(column.data(), fftHeight, false);
        for (int y = 0; y < fftHeight; y++)
        {
            complexData[y * fftWidth + x] = column[y];
        }
    }

    // 7. 频谱中心化
    for (int y = 0; y < fftHeight; y++)
    {
        for (int x = 0; x < fftWidth; x++)
        {
            if (((x + y) & 1) == 1)
            {
                complexData[y * fftWidth + x] *= -1;
            }
        }
    }

    // 8. 创建同态滤波器并应用
    int cx = fftWidth / 2;
    int cy = fftHeight / 2;

    for (int y = 0; y < fftHeight; y++)
    {
        for (int x = 0; x < fftWidth; x++)
        {
            float dx = (float)(x - cx);
            float dy = (float)(y - cy);
            float D = sqrt(dx * dx + dy * dy);

            float D_sq = D * D;
            float D0_sq = D0 * D0;
            float ratio = -c * (D_sq / D0_sq);
            float expTerm = exp(ratio);
            float H = (gammaH - gammaL) * (1 - expTerm) + gammaL;

            complexData[y * fftWidth + x] *= H;
        }
    }

    // 9. 反中心化
    for (int y = 0; y < fftHeight; y++)
    {
        for (int x = 0; x < fftWidth; x++)
        {
            if (((x + y) & 1) == 1)
            {
                complexData[y * fftWidth + x] *= -1;
            }
        }
    }

    // 10. 对每一列做IFFT
    for (int x = 0; x < fftWidth; x++)
    {
        for (int y = 0; y < fftHeight; y++)
        {
            column[y] = complexData[y * fftWidth + x];
        }
        FFT(column.data(), fftHeight, true);
        for (int y = 0; y < fftHeight; y++)
        {
            complexData[y * fftWidth + x] = column[y];
        }
    }

    // 11. 对每一行做IFFT
    for (int y = 0; y < fftHeight; y++)
    {
        FFT(&complexData[y * fftWidth], fftWidth, true);
    }

    // 12. 取实部并指数变换
    float** result = new float* [height];
    for (int i = 0; i < height; i++)
        result[i] = new float[width];

    float minVal = FLT_MAX, maxVal = -FLT_MAX;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            double realVal = complexData[y * fftWidth + x].real();
            float expVal = (float)exp(realVal) - 1;
            result[y][x] = expVal;
            if (expVal < minVal) minVal = expVal;
            if (expVal > maxVal) maxVal = expVal;
        }
    }

    // 13. 归一化并写回图像
    float range = maxVal - minVal;
    if (range < 0.0001f) range = 1.0f;

    for (int y = 0; y < height; y++)
    {
        BYTE* pRow = m_pRGB24 + y * bytesPerLine;
        for (int x = 0; x < width; x++)
        {
            float normalized = (result[y][x] - minVal) / range;
            BYTE gray = (BYTE)(normalized * 255);
            pRow[x * 3] = gray;
            pRow[x * 3 + 1] = gray;
            pRow[x * 3 + 2] = gray;
        }
    }

    // 14. 释放内存
    delete[] complexData;
    for (int i = 0; i < height; i++)
    {
        delete[] fImage[i];
        delete[] result[i];
    }
    delete[] fImage;
    delete[] result;

    CString msg;
    msg.Format(_T("同态滤波完成！\n参数：γH=%.1f, γL=%.1f, c=%.1f, D0=%.1f"),
        gammaH, gammaL, c, D0);
    AfxMessageBox(msg);
}