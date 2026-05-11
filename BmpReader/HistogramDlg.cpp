// HistogramDlg.cpp
#include "stdafx.h"
#include "BmpReader.h"
#include "HistogramDlg.h"
#include "BmpReaderView.h" 
#include "afxdialogex.h"
IMPLEMENT_DYNAMIC(CHistogramDlg, CDialogEx)

CHistogramDlg::CHistogramDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_HISTOGRAM_DLG, pParent), m_maxCount(0)
{
	memset(m_hist, 0, sizeof(m_hist));
}

CHistogramDlg::~CHistogramDlg()
{
}

void CHistogramDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CHistogramDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

// +++ 非模态创建函数
BOOL CHistogramDlg::Create(UINT nIDTemplate, CWnd* pParentWnd)
{
	return CDialogEx::Create(nIDTemplate, pParentWnd);
}

// +++ 非模态窗口销毁时释放自身
void CHistogramDlg::PostNcDestroy()
{
	CDialogEx::PostNcDestroy();
	delete this;   // 必须！因为非模态对话框通常用 new 创建
}

void CHistogramDlg::SetHistogramData(int* pHist, int maxCount)
{
	memcpy(m_hist, pHist, sizeof(int) * 256);
	m_maxCount = maxCount;
	if (GetSafeHwnd())          // 窗口已创建则刷新
		Invalidate();
}

void CHistogramDlg::OnPaint()
{
	CPaintDC dc(this);
	CRect rect;
	GetClientRect(rect);

	// 填充白色背景
	dc.FillSolidRect(rect, RGB(255, 255, 255));

	// 定义绘图区域，留出边距用于坐标轴和标签
	const int marginLeft = 50;   // 左侧留空给纵轴标签
	const int marginRight = 20;  // 右侧留空
	const int marginTop = 20;    // 顶部留空
	const int marginBottom = 40;  // 底部留空给横轴标签

	int plotWidth = rect.Width() - marginLeft - marginRight;
	int plotHeight = rect.Height() - marginTop - marginBottom;
	int plotLeft = marginLeft;
	int plotRight = marginLeft + plotWidth;
	int plotTop = marginTop;
	int plotBottom = rect.Height() - marginBottom;

	// 画坐标轴（黑色实线）
	CPen penAxis(PS_SOLID, 1, RGB(0, 0, 0));
	dc.SelectObject(&penAxis);
	// 横轴
	dc.MoveTo(plotLeft, plotBottom);
	dc.LineTo(plotRight, plotBottom);
	// 纵轴
	dc.MoveTo(plotLeft, plotTop);
	dc.LineTo(plotLeft, plotBottom);

	// 画箭头（简单小三角形）
	CPen penArrow(PS_SOLID, 1, RGB(0, 0, 0));
	dc.SelectObject(&penArrow);
	// 横轴箭头
	dc.MoveTo(plotRight - 5, plotBottom - 3);
	dc.LineTo(plotRight, plotBottom);
	dc.LineTo(plotRight - 5, plotBottom + 3);
	// 纵轴箭头
	dc.MoveTo(plotLeft - 3, plotTop + 5);
	dc.LineTo(plotLeft, plotTop);
	dc.LineTo(plotLeft + 3, plotTop + 5);

	// 计算每个灰度级对应的柱子宽度
	int barWidth = plotWidth / 256;
	if (barWidth < 1) barWidth = 1;

	// 画直方图（使用蓝色）
	CPen penHist(PS_SOLID, barWidth, RGB(0, 0, 255));
	dc.SelectObject(&penHist);
	for (int i = 0; i < 256; i++)
	{
		int barHeight = 0;
		if (m_maxCount > 0)
			barHeight = (int)((double)m_hist[i] / m_maxCount * plotHeight);
		if (barHeight > plotHeight) barHeight = plotHeight;
		int x = plotLeft + i * barWidth;
		dc.MoveTo(x, plotBottom);
		dc.LineTo(x, plotBottom - barHeight);
	}

	// 添加横轴刻度标签（灰度值）
	CFont font;
	font.CreatePointFont(80, _T("Arial"));  // 小字号
	CFont* pOldFont = dc.SelectObject(&font);
	dc.SetBkMode(TRANSPARENT);
	for (int val = 0; val <= 255; val += 64)  // 每隔64标一次
	{
		int x = plotLeft + (int)((double)val / 256 * plotWidth);
		CString label;
		label.Format(_T("%d"), val);
		// 标签居中于刻度线
		dc.TextOut(x - 10, plotBottom + 5, label);
		// 画短刻度线
		dc.MoveTo(x, plotBottom);
		dc.LineTo(x, plotBottom + 4);
	}

	// 添加纵轴刻度标签（频数）
	// 纵轴最大值 m_maxCount，分 5 等分
	int numTicks = 5;
	for (int i = 0; i <= numTicks; i++)
	{
		int freq = (int)((double)m_maxCount * i / numTicks);
		int y = plotBottom - (int)((double)i / numTicks * plotHeight);
		if (y < plotTop) y = plotTop;
		CString label;
		label.Format(_T("%d"), freq);
		// 右对齐标签
		dc.TextOut(plotLeft - 30, y - 6, label);
		// 短刻度线
		dc.MoveTo(plotLeft - 4, y);
		dc.LineTo(plotLeft, y);
	}

	// 添加轴标题
	dc.TextOut(plotLeft + plotWidth / 2 - 20, plotBottom + 20, _T("灰度值"));
	// 旋转纵轴标题（简单处理：竖排文本需要旋转，这里用普通文本代替）
	dc.TextOut(10, plotTop + plotHeight / 2 - 10, _T("频数"));

	dc.SelectObject(pOldFont);
}

BOOL CHistogramDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	Invalidate();   // 触发重绘
	return TRUE;
}

void CHistogramDlg::OnDestroy()
{
	// 通知父窗口（View）把直方图窗口指针置 NULL
	if (GetParent())
		::SendMessage(GetParent()->m_hWnd, WM_USER_DESTROY_HIST, 0, 0);
	CDialogEx::OnDestroy();
}