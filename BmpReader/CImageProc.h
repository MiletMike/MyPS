// CImageProc.h
#pragma once
#include <afxwin.h>
#include <complex>      // 复数支持
#include <vector>       // 向量容器
#include <cmath>        // 数学函数
#include <float.h>      // FLT_MAX
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
	void SobelEdgeDetection(int kernelSize = 3, int threshold = 80, bool bBinaryOutput = true);
	void PrewittEdgeDetection(int kernelSize = 3, int threshold = 80);

	// 噪声添加函数
	void AddSaltPepperNoise(double saltProb = 0.05, double pepperProb = 0.05);
	void AddImpulseNoise(double probability = 0.05);
	void AddGaussianNoise(double mean = 0, double stddev = 25);
	void AddWhiteGaussianNoise(double mean = 0, double stddev = 30);

	// 同态滤波函数
	void HomomorphicFilter(float gammaH = 2.0f, float gammaL = 0.5f, float c = 1.0f, float D0 = 30.0f);

	// 伪彩色
	void ApplyPseudoColor(int scheme = SCHEME_DEFAULT);

	// 保存原始图像数据（用于恢复）
	void SaveOriginalData();

	// FFT/IFFT相关函数
	bool ComputeFFT2D();                    // 计算二维FFT
	bool ComputeIFFT2D();                   // 计算二维IFFT
	void ShowSpectrum(CDC* pDC);            // 显示频谱图
	bool IsFFTValid() const { return m_bFFTValid; }  // 检查FFT是否已计算
	void SwitchToFrequencyDomain();         // 切换到频域显示
	void SwitchToSpatialDomain();           // 切换回空域显示

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

private:
	// FFT核心函数
	void FFT(std::complex<double>* data, int n, bool inverse);
	int NextPowerOfTwo(int n);
	void FFT2D(BYTE* spatialData, std::complex<double>* freqData, int width, int height, bool inverse);

	// 辅助函数
	void CenterSpectrum(std::complex<double>* data, int width, int height);
	void LogScaleSpectrum(double* magnitude, int size);

	// FFT相关数据
	std::complex<double>* m_pFFTData;       // 频域数据（复数）
	bool m_bFFTValid;                       // FFT是否已计算
	bool m_bInFrequencyDomain;              // 当前是否显示频域
	int m_nFFTWidth;                        // FFT宽度（2的幂）
	int m_nFFTHeight;                       // FFT高度（2的幂）

	void CleanUp();
	DWORD m_rMask, m_gMask, m_bMask;
	DWORD m_dwRedMask, m_dwGreenMask, m_dwBlueMask;
	int   m_nRedShift, m_nGreenShift, m_nBlueShift;
	int   m_nRedBits, m_nGreenBits, m_nBlueBits;
	BYTE* m_pOriginalRGB24;      // 原始图像数据备份
	int m_nOriginalWidth;        // 原始图像宽度
	int m_nOriginalHeight;       // 原始图像高度
};