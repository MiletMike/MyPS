// CImageProc.h
#pragma once
#include <afxwin.h>
#include "HistogramDlg.h"

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
	COLORREF GetPixelColorManual(int x, int y);//自编程获取颜色
	COLORREF GetOriginalPixel(int x, int y);   // 根据原始坐标获取颜色

	// 图像增强函数
	void ShowHistogram();                // 显示灰度直方图（弹出对话框）
	void LinearTransform(int low_in, int high_in, int low_out, int high_out); // 线性变换
	void HistogramEqualize();            // 直方图均衡化
	void AdaptiveHistogramEqualize(int blockSize = 8);   // 局部直方图均衡
	bool HistogramSpecify(const CString& strTargetPath);// 直方图规格化
	void PaletteTransform();             // 调色板变换
	void CalculateHistogram(int hist[256], int& maxCount) const;
	void ConvertToGray();                // 转为灰度图
	void MeanFilter(int kSize);          // 均值滤波
	void MedianFilter(int kSize);        // 中值滤波
	void MaxFilter(int kSize);           // 最大值滤波
	bool AddImages(const CString& strSecondImagePath);      // 图像相加
	bool MultiplyImages(const CString& strSecondImagePath); // 图像相乘
	bool RestoreOriginal();                                  // 返回原图
	void LaplacianEdgeDetection(int kernelSize = 3, int threshold = 30);        // 拉普拉斯边缘检测
	void PowerLawTransform(double gamma = 0.5);              // 幂律变换

	// 保存原始图像数据（用于恢复）
	void SaveOriginalData();                                 // 保存原始数据
	// ====================================
	void SobelEdgeDetection(int kernelSize = 3, int threshold = 80, bool bBinaryOutput = true);
	void PrewittEdgeDetection(int kernelSize = 3, int threshold = 80);


	// ========== 新增：噪声添加函数 ==========
	void AddSaltPepperNoise(double saltProb = 0.05, double pepperProb = 0.05);
	void AddImpulseNoise(double probability = 0.05);
	void AddGaussianNoise(double mean = 0, double stddev = 25);
	void AddWhiteGaussianNoise(double mean = 0, double stddev = 30);

	// 成员变量
	HANDLE      m_hDib;
	BYTE* pDib;
	BITMAPFILEHEADER* pBFH;
	BITMAPINFOHEADER* pBIH;
	RGBQUAD* pQUAD;
	BYTE* pBits;
	int         nWidth;
	int         nHeight;
	int         nNumColors;

	int     m_nWidth;
	int     m_nHeight;
	BYTE* m_pRGB24;

	enum PseudoColorScheme {
		SCHEME_DEFAULT = 0,
		SCHEME_HOT,
		SCHEME_RAINBOW,
		SCHEME_COOL,
		SCHEME_INVERT
	};

	void ApplyPseudoColor(int scheme = SCHEME_DEFAULT);
private:
	void CleanUp();
	DWORD m_rMask, m_gMask, m_bMask;
	DWORD m_dwRedMask, m_dwGreenMask, m_dwBlueMask;
	int   m_nRedShift, m_nGreenShift, m_nBlueShift;
	int   m_nRedBits, m_nGreenBits, m_nBlueBits;
	BYTE* m_pOriginalRGB24;      // 原始图像数据备份
	int m_nOriginalWidth;        // 原始图像宽度
	int m_nOriginalHeight;       // 原始图像高度
};
