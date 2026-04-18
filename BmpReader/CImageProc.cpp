// CImageProc.cpp
#include "stdafx.h"
#include "CImageProc.h"
#include "MainFrm.h"      // 用于刷新视图
#include <atlimage.h>     // 用于CImage加载JPG
#include <algorithm>
#include <cmath>
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
		// 刷新视图（显示图像）
		CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
		CView* pView = pFrame->GetActiveView();
		if (pView) pView->Invalidate(TRUE);
	}
}

// 自主读取BMP文件（支持1/4/8/16/24/32位）
void CImageProc::LoadBmp(CString strPathName)
{
	CleanUp();
	CFile file;
	if (!file.Open(strPathName, CFile::modeRead)) return;

	// 读取文件头和信息头
	file.Read(pBFH, sizeof(BITMAPFILEHEADER));
	if (pBFH->bfType != 0x4D42) { AfxMessageBox(_T("不是有效的BMP文件")); return; }
	file.Read(pBIH, sizeof(BITMAPINFOHEADER));

	nWidth = pBIH->biWidth;
	nHeight = pBIH->biHeight;
	int bitCount = pBIH->biBitCount;
	// 然后填充 m_pRGB24 数据...
	// 然后填充 m_pRGB24 数据...

	// 计算颜色表项数
	if (pBIH->biClrUsed != 0)
		nNumColors = pBIH->biClrUsed;
	else if (bitCount <= 8)
		nNumColors = 1 << bitCount;
	else
		nNumColors = 0;

	// 读取掩码（16位 BI_BITFIELDS 模式）
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

	// 读取颜色表（仅当 bitCount <= 8）
	if (nNumColors > 0)
	{
		pQUAD = new RGBQUAD[nNumColors];
		file.Read(pQUAD, sizeof(RGBQUAD) * nNumColors);
	}

	// 计算图像数据大小和行字节对齐
	int bytesPerLine = ((nWidth * bitCount + 31) / 32) * 4;
	int dataSize = bytesPerLine * abs(nHeight);
	pBits = new BYTE[dataSize];
	file.Seek(pBFH->bfOffBits, CFile::begin);
	file.Read(pBits, dataSize);
	file.Close();

	// 若高度为正（BMP存储为倒向），转换为正向存储以便坐标计算
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
		pBIH->biHeight = -pBIH->biHeight;   // 设为负值，表示正向存储
	}
	nHeight = abs(nHeight);

	//转换为24位RGB并保存到 m_pRGB24 
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	int bytesPerLine24 = ((nWidth * 24 + 31) / 32) * 4;   // 24位每行字节数（4字节对齐）
	m_pRGB24 = new BYTE[bytesPerLine24 * nHeight];
	memset(m_pRGB24, 0, bytesPerLine24 * nHeight);

	// 遍历每个像素，转换为RGB
	for (int y = 0; y < nHeight; y++)
	{
		// 原始数据行指针（pBits 已经是正向存储，因为前面做了翻转）
		BYTE* pSrcRow = pBits + y * bytesPerLine;
		// 目标行指针
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
					// 辅助函数：从掩码中提取颜色分量并缩放到0-255（浮点四舍五入）
					auto extract = [](WORD px, DWORD mask) -> BYTE {
						if (mask == 0) return 0;
						// 找到掩码中最低位1的位置（偏移量）
						int shift = 0;
						while ((mask & (1 << shift)) == 0) shift++;
						// 计算掩码中1的个数（位数）
						int bits = 0;
						DWORD m = mask >> shift;
						while (m) { bits++; m >>= 1; }
						// 提取像素值
						int val = (px & mask) >> shift;
						int maxVal = (1 << bits) - 1;
						// 浮点数四舍五入（更精确匹配 Windows）
						return (BYTE)((val * 255.0 / maxVal) + 0.5);
					};
					r = extract(pixel, m_rMask);
					g = extract(pixel, m_gMask);
					b = extract(pixel, m_bMask);
				}
				else
				{
					// 默认5-5-5格式（没有掩码时），浮点四舍五入
					r = (BYTE)((((pixel & 0x7C00) >> 10) * 255.0 / 31) + 0.5);
					g = (BYTE)((((pixel & 0x03E0) >> 5)  * 255.0 / 31) + 0.5);
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
				// 第四个字节Alpha忽略
			}
			pDstRow[x * 3] = b;
			pDstRow[x * 3 + 1] = g;
			pDstRow[x * 3 + 2] = r;
		}
	}

	// 准备显示用的DIB（包含掩码）
	int extraSize = 0;
	if (bitCount == 16 && pBIH->biCompression == BI_BITFIELDS)
		extraSize = 3 * sizeof(DWORD);

	BITMAPINFO* pBMI = (BITMAPINFO*)new BYTE[sizeof(BITMAPINFOHEADER) + nNumColors * sizeof(RGBQUAD) + extraSize];
	memcpy(pBMI, pBIH, sizeof(BITMAPINFOHEADER));
	if (nNumColors > 0)
		memcpy(pBMI->bmiColors, pQUAD, nNumColors * sizeof(RGBQUAD));

	// 拷贝掩码（如果存在）
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
	delete[] (BYTE*)pBMI;
}

// 扩展：JPG读取（调用CImage）
void CImageProc::LoadJpg(CString strPathName)
{
	CleanUp();
	CImage img;
	if (FAILED(img.Load(strPathName))) return;
	nWidth = img.GetWidth();
	nHeight = img.GetHeight();
	// 转换为24位DIB存储
	BITMAPINFOHEADER bi = { 0 };
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = nWidth;
	bi.biHeight = -nHeight; // 正向
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
	// 构造DIB
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
	delete[] (BYTE*)pBMI;
}

// 显示图像
void CImageProc::ShowBMP(CDC* pDC)
{
	if (!m_hDib) return;
	BITMAPINFOHEADER* pHeader = (BITMAPINFOHEADER*)::GlobalLock(m_hDib);
	if (!pHeader) return;

	BYTE* pData = (BYTE*)pHeader + sizeof(BITMAPINFOHEADER);
	// 跳过颜色表或掩码
	if (pHeader->biBitCount <= 8)
	{
		int numColors = pHeader->biClrUsed;
		if (numColors == 0) numColors = 1 << pHeader->biBitCount;
		pData += numColors * sizeof(RGBQUAD);
	}
	else if (pHeader->biBitCount == 16 && pHeader->biCompression == BI_BITFIELDS)
	{
		pData += 3 * sizeof(DWORD);   // 跳过掩码
	}

	::SetStretchBltMode(pDC->m_hDC, COLORONCOLOR);
	::StretchDIBits(pDC->m_hDC, 0, 0, nWidth, nHeight,
		0, 0, nWidth, nHeight,
		pData, (BITMAPINFO*)pHeader, DIB_RGB_COLORS, SRCCOPY);
	::GlobalUnlock(m_hDib);
}

COLORREF CImageProc::GetColor(CDC* pDC, int x, int y)
{
	if (!m_pRGB24) return RGB(0,0,0);
	if (x < 0 || x >= m_nWidth || y < 0 || y >= m_nHeight) return RGB(0,0,0);
	return pDC->GetPixel(x, y);
}

COLORREF CImageProc::GetPixelColorManual(int x, int y)
{
	// 边界检查
	if (x < 0 || x >= m_nWidth || y < 0 || y >= m_nHeight || m_pRGB24 == NULL)
		return RGB(0,0,0);

	// 计算每行字节数（24位，4字节对齐）
	int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
	// 定位到 (x, y) 位置的像素
	BYTE* pPixel = m_pRGB24 + y * bytesPerLine + x * 3;
	// 注意 m_pRGB24 存储的是 BGR 顺序，要转成 RGB
	BYTE b = pPixel[0];
	BYTE g = pPixel[1];
	BYTE r = pPixel[2];
	return RGB(r, g, b);
}

COLORREF CImageProc::GetOriginalPixel(int x, int y)
{
	if (!m_pRGB24 || x < 0 || x >= m_nWidth || y < 0 || y >= m_nHeight)
		return RGB(0, 0, 0);
	// 计算每行字节数（24位图像，按4字节对齐）
	int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
	BYTE* pPixel = m_pRGB24 + y * bytesPerLine + x * 3;
	// m_pRGB24 存储的是 BGR 顺序
	return RGB(pPixel[2], pPixel[1], pPixel[0]);
}

void CImageProc::ConvertToGray()
{	if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0) return;
	if (!m_pRGB24) return;
	for (int y = 0; y < m_nHeight; y++)
	{
		int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
		BYTE* pRow = m_pRGB24 + y * bytesPerLine;
		for (int x = 0; x < m_nWidth; x++)
		{
			BYTE b = pRow[x*3];
			BYTE g = pRow[x*3+1];
			BYTE r = pRow[x*3+2];
			BYTE gray = (BYTE)(0.299 * r + 0.587 * g + 0.114 * b);
			pRow[x*3] = gray;
			pRow[x*3+1] = gray;
			pRow[x*3+2] = gray;
		}
	}
}

void CImageProc::ShowHistogram()
{
	if (!m_pRGB24) return;

	int hist[256] = {0};
	int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;

	// 遍历每个像素，计算灰度值（使用亮度公式）
	for (int y = 0; y < m_nHeight; y++)
	{
		BYTE* pRow = m_pRGB24 + y * bytesPerLine;
		for (int x = 0; x < m_nWidth; x++)
		{
			BYTE b = pRow[x*3];
			BYTE g = pRow[x*3+1];
			BYTE r = pRow[x*3+2];
			BYTE gray = (BYTE)(0.299 * r + 0.587 * g + 0.114 * b);
			hist[gray]++;
		}
	}

	// 找到最大值用于归一化显示
	int maxCount = 0;
	for (int i = 0; i < 256; i++)
		if (hist[i] > maxCount) maxCount = hist[i];

	// 创建并显示对话框
	CHistogramDlg dlg;
	dlg.SetHistogramData(hist, maxCount);
	dlg.DoModal();
}

void CImageProc::LinearTransform(int low_in, int high_in, int low_out, int high_out)
{
	if (!m_pRGB24) return;
	// 先转为灰度（可选，你也可以对彩色三个通道分别做）
	ConvertToGray();  // 变为灰度图后，三个通道值相同，处理一个通道即可
	for (int y = 0; y < m_nHeight; y++)
	{
		int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
		BYTE* pRow = m_pRGB24 + y * bytesPerLine;
		for (int x = 0; x < m_nWidth; x++)
		{
			int old = pRow[x*3]; // 因为是灰度，R=G=B
			int newVal = (old - low_in) * (high_out - low_out) / (high_in - low_in) + low_out;
			if (newVal < 0) newVal = 0;
			if (newVal > 255) newVal = 255;
			BYTE val = (BYTE)newVal;
			pRow[x*3] = pRow[x*3+1] = pRow[x*3+2] = val;
		}
	}
	// 刷新显示
}

void CImageProc::HistogramEqualize()
{
	if (!m_pRGB24) return;
	ConvertToGray();  // 转为灰度
	int hist[256] = {0};
	int totalPixels = m_nWidth * m_nHeight;
	// 计算直方图
	for (int y = 0; y < m_nHeight; y++)
	{
		int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
		BYTE* pRow = m_pRGB24 + y * bytesPerLine;
		for (int x = 0; x < m_nWidth; x++)
		{
			hist[pRow[x*3]]++;
		}
	}
	// 计算累积分布
	int cdf[256] = {0};
	cdf[0] = hist[0];
	for (int i=1; i<256; i++) cdf[i] = cdf[i-1] + hist[i];
	// 映射：new = round(cdf[old] * 255 / totalPixels)
	BYTE map[256];
	for (int i=0; i<256; i++)
	{
		map[i] = (BYTE)( (double)cdf[i] * 255 / totalPixels );
	}
	// 应用映射
	for (int y = 0; y < m_nHeight; y++)
	{
		int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
		BYTE* pRow = m_pRGB24 + y * bytesPerLine;
		for (int x = 0; x < m_nWidth; x++)
		{
			BYTE newVal = map[pRow[x*3]];
			pRow[x*3] = pRow[x*3+1] = pRow[x*3+2] = newVal;
		}
	}
}

void CImageProc::PaletteTransform()
{
	if (!m_pRGB24) return;
	ConvertToGray(); // 先转为灰度
	// 定义一个简单的伪彩色映射：0->蓝色，127->绿色，255->红色
	for (int y = 0; y < m_nHeight; y++)
	{
		int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;
		BYTE* pRow = m_pRGB24 + y * bytesPerLine;
		for (int x = 0; x < m_nWidth; x++)
		{
			int gray = pRow[x*3];
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
			pRow[x*3] = b;
			pRow[x*3+1] = g;
			pRow[x*3+2] = r;
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
			BYTE b = pRow[x*3];
			BYTE g = pRow[x*3+1];
			BYTE r = pRow[x*3+2];
			BYTE gray = (BYTE)(0.299 * r + 0.587 * g + 0.114 * b);
			hist[gray]++;
		}
	}

	for (int i = 0; i < 256; i++)
		if (hist[i] > maxCount) maxCount = hist[i];
}

// 局部直方图均衡（自适应直方图均衡 - AHE）
void CImageProc::AdaptiveHistogramEqualize(int blockSize)
{	
	if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0)
		return;

	// 先转换为灰度图（局部均衡通常对亮度通道进行）
	ConvertToGray();

	int width = m_nWidth;
	int height = m_nHeight;
	int bytesPerLine = ((width * 24 + 31) / 32) * 4;

	// 计算每个像素的灰度值并存入临时数组（避免重复计算）
	BYTE* grayData = new BYTE[width * height];
	for (int y = 0; y < height; y++)
	{
		BYTE* pRow = m_pRGB24 + y * bytesPerLine;
		for (int x = 0; x < width; x++)
		{
			grayData[y * width + x] = pRow[x * 3];  // 灰度值（RGB 三通道相同）
		}
	}

	// 计算分块数量
	int blocksX = (width + blockSize - 1) / blockSize;
	int blocksY = (height + blockSize - 1) / blockSize;

	// 为每个分块计算累积分布函数 (CDF) 映射表
	// 存储每个块从原灰度到新灰度的映射表 [0..255]
	BYTE** blockMaps = new BYTE*[blocksY * blocksX];
	for (int i = 0; i < blocksY * blocksX; i++)
		blockMaps[i] = new BYTE[256];

	for (int by = 0; by < blocksY; by++)
	{
		for (int bx = 0; bx < blocksX; bx++)
		{
			// 当前块的有效范围
			int xStart = bx * blockSize;
			int yStart = by * blockSize;
			int xEnd = min(xStart + blockSize, width);
			int yEnd = min(yStart + blockSize, height);
			int blockWidth = xEnd - xStart;
			int blockHeight = yEnd - yStart;
			int totalPixels = blockWidth * blockHeight;

			// 统计直方图
			int hist[256] = {0};
			for (int y = yStart; y < yEnd; y++)
			{
				for (int x = xStart; x < xEnd; x++)
				{
					BYTE val = grayData[y * width + x];
					hist[val]++;
				}
			}

			// 计算累积分布 CDF
			int cdf[256] = {0};
			cdf[0] = hist[0];
			for (int i = 1; i < 256; i++)
				cdf[i] = cdf[i-1] + hist[i];

			// 生成映射表（均衡化公式）
			BYTE* map = blockMaps[by * blocksX + bx];
			for (int i = 0; i < 256; i++)
			{
				map[i] = (BYTE)((double)cdf[i] * 255.0 / totalPixels);
			}
		}
	}

	// 双线性插值计算每个像素的新灰度值
	BYTE* resultGray = new BYTE[width * height];
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			// 像素所属的块索引（以块中心为基准计算浮点坐标）
			float fx = (float)(x) / blockSize - 0.5f;
			float fy = (float)(y) / blockSize - 0.5f;

			int bx = (int)floor(fx);
			int by = (int)floor(fy);

			// 四个相邻块的索引，并进行边界检查
			int bx0 = max(0, min(bx, blocksX - 1));
			int bx1 = max(0, min(bx + 1, blocksX - 1));
			int by0 = max(0, min(by, blocksY - 1));
			int by1 = max(0, min(by + 1, blocksY - 1));

			// 计算插值权重
			float u = fx - bx;
			float v = fy - by;
			if (u < 0) u = 0; if (u > 1) u = 1;
			if (v < 0) v = 0; if (v > 1) v = 1;

			BYTE gray = grayData[y * width + x];

			// 获取四个块的映射值
			BYTE v00 = blockMaps[by0 * blocksX + bx0][gray];
			BYTE v10 = blockMaps[by0 * blocksX + bx1][gray];
			BYTE v01 = blockMaps[by1 * blocksX + bx0][gray];
			BYTE v11 = blockMaps[by1 * blocksX + bx1][gray];

			// 双线性插值
			float val = (1 - u) * (1 - v) * v00 +
				u * (1 - v) * v10 +
				(1 - u) * v * v01 +
				u * v * v11;

			BYTE newVal = (BYTE)(val + 0.5f);
			resultGray[y * width + x] = newVal;
		}
	}

	// 将处理后的灰度值写回图像数据
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

	// 释放临时内存
	delete[] grayData;
	delete[] resultGray;
	for (int i = 0; i < blocksY * blocksX; i++)
		delete[] blockMaps[i];
	delete[] blockMaps;
}

void CImageProc::ApplyPseudoColor(int scheme)
{
	if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0) return;

	// 先转为灰度图像（确保三个通道值相同，便于伪彩色映射）
	ConvertToGray();

	int bytesPerLine = ((m_nWidth * 24 + 31) / 32) * 4;

	// 遍历每个像素，根据灰度值生成新颜色
	for (int y = 0; y < m_nHeight; y++)
	{
		BYTE* pRow = m_pRGB24 + y * bytesPerLine;
		for (int x = 0; x < m_nWidth; x++)
		{
			BYTE gray = pRow[x * 3];  // 灰度值（0-255）

			BYTE r, g, b;
			switch (scheme)
			{
			case SCHEME_DEFAULT:
				{
					// 原 PaletteTransform 中的逻辑：蓝→绿→红
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
				}
				break;

			case SCHEME_HOT:
				{
					// 热金属：黑→红→黄→白
					float t = gray / 255.0f;
					if (t < 0.333f)
					{
						r = (BYTE)(255 * (t / 0.333f));
						g = 0;
						b = 0;
					}
					else if (t < 0.666f)
					{
						r = 255;
						g = (BYTE)(255 * ((t - 0.333f) / 0.333f));
						b = 0;
					}
					else
					{
						r = 255;
						g = 255;
						b = (BYTE)(255 * ((t - 0.666f) / 0.334f));
					}
				}
				break;

			case SCHEME_RAINBOW:
				{
					// 彩虹色：利用正弦函数生成平滑过渡
					float t = gray / 255.0f;
					r = (BYTE)((sin(t * 2 * 3.14159f + 0) * 0.5f + 0.5f) * 255);
					g = (BYTE)((sin(t * 2 * 3.14159f + 2.094f) * 0.5f + 0.5f) * 255);
					b = (BYTE)((sin(t * 2 * 3.14159f + 4.188f) * 0.5f + 0.5f) * 255);
				}
				break;

			case SCHEME_COOL:
				{
					// 冷色调：蓝→青→绿
					float t = gray / 255.0f;
					r = 0;
					g = (BYTE)(255 * t);
					b = (BYTE)(255 * (1.0f - t));
				}
				break;

			case SCHEME_INVERT:
				{
					// 灰度反转：新灰度 = 255 - 原灰度
					BYTE inv = 255 - gray;
					r = g = b = inv;
				}
				break;

			default:
				r = g = b = gray; // 保持灰度
				break;
			}

			// 写回RGB数据（注意顺序为BGR）
			pRow[x * 3] = b;
			pRow[x * 3 + 1] = g;
			pRow[x * 3 + 2] = r;
		}
	}
}

// 辅助函数：计算给定图像数据的灰度直方图和累积分布
static void ComputeHistAndCDF(BYTE* pData, int width, int height, int bytesPerLine, 
	int hist[256], double cdf[256])
{
	memset(hist, 0, sizeof(int) * 256);
	for (int y = 0; y < height; y++)
	{
		BYTE* pRow = pData + y * bytesPerLine;
		for (int x = 0; x < width; x++)
		{
			BYTE gray = pRow[x * 3];  // 假定已经是灰度图（R=G=B）
			hist[gray]++;
		}
	}

	// 计算 CDF（归一化到 0.0~1.0）
	int total = width * height;
	int sum = 0;
	for (int i = 0; i < 256; i++)
	{
		sum += hist[i];
		cdf[i] = (double)sum / total;
	}
}

bool CImageProc::HistogramSpecify(const CString& strTargetPath)
{
	// 1. 检查当前图像是否有效
	if (!m_pRGB24 || m_nWidth <= 0 || m_nHeight <= 0)
		return false;

	// 2. 加载目标图像
	CImageProc targetImg;
	CString ext = strTargetPath.Right(3);
	ext.MakeLower();
	if (ext == _T("bmp"))
		targetImg.LoadBmp(strTargetPath);
	else if (ext == _T("jpg"))
		targetImg.LoadJpg(strTargetPath);
	else
		return false;

	if (!targetImg.m_pRGB24 || targetImg.m_nWidth <= 0 || targetImg.m_nHeight <= 0)
		return false;

	// 3. 分别计算源图像和目标图像 R、G、B 三个通道的直方图和 CDF
	int bytesPerLineSrc = ((m_nWidth * 24 + 31) / 32) * 4;
	int bytesPerLineTgt = ((targetImg.m_nWidth * 24 + 31) / 32) * 4;
	int totalSrc = m_nWidth * m_nHeight;
	int totalTgt = targetImg.m_nWidth * targetImg.m_nHeight;

	// 辅助 lambda：计算单通道 CDF
	auto ComputeChannelCDF = [](BYTE* pData, int width, int height, int bytesPerLine, 
		int channelOffset, int totalPixels, double cdf[256]) 
	{
		int hist[256] = {0};
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

	// 辅助 lambda：建立映射表
	auto BuildMap = [](double srcCDF[256], double tgtCDF[256], BYTE map[256]) 
	{
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

	// 为 B、G、R 三个通道分别处理（注意存储顺序为 BGR）
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

	// 4. 应用映射到当前图像
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