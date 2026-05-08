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
}

void CImageProc::CleanUp()
{
	if (pBits) { delete[] pBits; pBits = nullptr; }
	if (pQUAD) { delete[] pQUAD; pQUAD = nullptr; }
	if (m_hDib) { ::GlobalFree(m_hDib); m_hDib = NULL; }
	if (m_pRGB24) { delete[] m_pRGB24; m_pRGB24 = nullptr; }
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

// ========== 噪声添加函数实现 ==========

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

// ============================================
// Sobel 边缘检测（固定使用 3x3 算子）
// ============================================
void CImageProc::SobelEdgeDetection(int kernelSize, int threshold)
{
	if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0)
		return;

	// 仅支持 3x3 核，若传入其他尺寸则提示并返回（或可自动改为3）
	if (kernelSize != 3) {
		AfxMessageBox(_T("Sobel 边缘检测目前仅支持 3x3 核大小"));
		return;
	}

	// 先转为灰度图像
	ConvertToGray();

	int width = m_nWidth;
	int height = m_nHeight;
	int bytesPerLine = ((width * 24 + 31) / 32) * 4;

	// 标准 3x3 Sobel 算子
	float gx[9] = { -1, 0, 1,
					-2, 0, 2,
					-1, 0, 1 };

	float gy[9] = { -1, -2, -1,
					 0,  0,  0,
					 1,  2,  1 };

	// 备份原始灰度数据（单通道）
	BYTE* src = new BYTE[width * height];
	for (int y = 0; y < height; y++) {
		BYTE* pRow = m_pRGB24 + y * bytesPerLine;
		for (int x = 0; x < width; x++) {
			src[y * width + x] = pRow[x * 3];   // 灰度图 R=G=B，取任一通道
		}
	}

	int half = 1;  // 3x3 核半径
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
			// 计算梯度幅值
			float magnitude = sqrtf(sumX * sumX + sumY * sumY);
			// 二值化
			BYTE val = (magnitude > threshold) ? 255 : 0;
			// 写回图像（彩色格式，三通道相同值）
			pRow[x * 3] = pRow[x * 3 + 1] = pRow[x * 3 + 2] = val;
		}
	}

	delete[] src;
}

// ============================================
// Prewitt 边缘检测（固定 3x3 算子）
// ============================================
void CImageProc::PrewittEdgeDetection(int kernelSize, int threshold)
{
	if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0)
		return;

	// 仅支持 3x3 核，若传入其他尺寸则提示并返回
	if (kernelSize != 3) {
		AfxMessageBox(_T("Prewitt 边缘检测目前仅支持 3x3 核大小"));
		return;
	}

	ConvertToGray();

	int width = m_nWidth;
	int height = m_nHeight;
	int bytesPerLine = ((width * 24 + 31) / 32) * 4;

	// 标准 3x3 Prewitt 算子
	float gx[9] = { -1, 0, 1,
					-1, 0, 1,
					-1, 0, 1 };
	float gy[9] = { -1, -1, -1,
					 0,  0,  0,
					 1,  1,  1 };

	// 备份原始灰度数据
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
			// 不除以归一化因子，直接计算幅值，范围约 0~255*3 = 765
			float magnitude = sqrtf(sumX * sumX + sumY * sumY);
			// 可选：限制到 0~255 以便二值化（但并不必须，因为比较阈值即可）
			BYTE val = (magnitude > threshold) ? 255 : 0;
			pRow[x * 3] = pRow[x * 3 + 1] = pRow[x * 3 + 2] = val;
		}
	}

	delete[] src;
}
