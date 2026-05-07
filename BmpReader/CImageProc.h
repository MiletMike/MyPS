// CImageProc.h
#pragma once
#include <afxwin.h>

class CImageProc
{
public:
	CImageProc();
	~CImageProc();

	void OpenFile();                // 打开文件对话框
	void LoadBmp(CString strPathName);   // 自主解析BMP
	void LoadJpg(CString strPathName);   // 扩展JPG（使用CImage）
	void ShowBMP(CDC* pDC);         // 显示图像
	COLORREF GetColor(CDC* pDC, int x, int y);  // 返回指定点的颜色
	COLORREF GetPixelColorManual(int x,int y);//自编程获取颜色
	COLORREF GetOriginalPixel(int x, int y);   // 根据原始坐标获取颜色

	// 图像增强函数
	void ShowHistogram();                // 显示灰度直方图（弹出对话框）
	void LinearTransform(int low_in, int high_in, int low_out, int high_out); // 线性变换
	void HistogramEqualize();            // 直方图均衡化
	void AdaptiveHistogramEqualize(int blockSize = 8);   // 局部直方图均衡，默认分块大小为 8×8
	bool HistogramSpecify(const CString& strTargetPath);// 直方图规格化（匹配目标图像的直方图）
	void PaletteTransform();             // 调色板变换（伪彩色）
	void CalculateHistogram(int hist[256], int& maxCount) const;
	// 辅助函数：将当前24位图像转为灰度图（原地修改）
	void ConvertToGray();
	// 空域滤波（核大小可调，kSize 为奇数）
	void MeanFilter(int kSize);
	void MedianFilter(int kSize);
	void MaxFilter(int kSize);
	// 成员变量
	HANDLE      m_hDib;
	BYTE*       pDib;
	BITMAPFILEHEADER* pBFH;
	BITMAPINFOHEADER* pBIH;
	RGBQUAD*    pQUAD;
	BYTE*       pBits;
	int         nWidth;
	int         nHeight;
	int         nNumColors;

	int     m_nWidth;      // 图像宽度
	int     m_nHeight;     // 图像高度
	BYTE*   m_pRGB24;      // 24位RGB数据指针

	// 伪彩色方案枚举
	enum PseudoColorScheme {
		SCHEME_DEFAULT = 0,   // 原有的蓝绿红渐变
		SCHEME_HOT,           // 热金属（黑→红→黄→白）
		SCHEME_RAINBOW,       // 彩虹色
		SCHEME_COOL,          // 冷色调（蓝→青→绿）
		SCHEME_INVERT         // 灰度反转（简单反色）
	};

	// 应用伪彩色（新增）
	void ApplyPseudoColor(int scheme = SCHEME_DEFAULT);
private:
	void CleanUp();                 // 释放内存
	DWORD m_rMask, m_gMask, m_bMask;   // 16位掩码
	DWORD m_dwRedMask;
	DWORD m_dwGreenMask;
	DWORD m_dwBlueMask;
	int   m_nRedShift, m_nGreenShift, m_nBlueShift;
	int   m_nRedBits,  m_nGreenBits,  m_nBlueBits;
};