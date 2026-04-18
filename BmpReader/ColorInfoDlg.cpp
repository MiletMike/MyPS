// ColorInfoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "BmpReader.h"
#include "resource.h"
#include "ColorInfoDlg.h"
#include "afxdialogex.h"


// CColorInfoDlg dialog

IMPLEMENT_DYNAMIC(CColorInfoDlg, CDialogEx)

CColorInfoDlg::CColorInfoDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CColorInfoDlg::IDD, pParent)
{
	m_bSizeAdjusted = FALSE;
}

CColorInfoDlg::~CColorInfoDlg()
{
}

void CColorInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CColorInfoDlg, CDialogEx)
	ON_EN_CHANGE(IDC_COORD_TEXT, &CColorInfoDlg::OnEnChangeCoordText)
END_MESSAGE_MAP()


// CColorInfoDlg message handlers
// 在文件的最后，添加这个函数
void CColorInfoDlg::SetInfo(int x, int y, COLORREF crManual, COLORREF crGetPixel)
{
	// 1. 坐标（单独一个控件）
	CString strCoord;
	strCoord.Format(_T("坐标：( %d , %d )"), x, y);
	SetDlgItemText(IDC_COORD_TEXT, strCoord);

	// 2. 自编程 RGB（单独一个控件）
	CString strManual;
	strManual.Format(_T("自编程 RGB：( %d , %d , %d )"),
		GetRValue(crManual), GetGValue(crManual), GetBValue(crManual));
	SetDlgItemText(IDC_COLOR_TEXT, strManual);

	// 3. GetPixel RGB + 对比结果（第三个控件）
	// 对比结果（允许每个分量误差 ≤1）
	int r1 = GetRValue(crManual), g1 = GetGValue(crManual), b1 = GetBValue(crManual);
	int r2 = GetRValue(crGetPixel), g2 = GetGValue(crGetPixel), b2 = GetBValue(crGetPixel);
	BOOL bMatch = (abs(r1 - r2) <= 1 && abs(g1 - g2) <= 1 && abs(b1 - b2) <= 1);
	CString strSystem;
	strSystem.Format(_T("GetPixel RGB：( %d , %d , %d )\n对比结果：%s"),
		GetRValue(crGetPixel), GetGValue(crGetPixel), GetBValue(crGetPixel),
		bMatch ? _T("一致") : _T("不一致"));
	SetDlgItemText(IDC_SYSTEM_TEXT, strSystem);  
}
BOOL CColorInfoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 使窗口具有分层属性（支持半透明）
	SetWindowLong(this->m_hWnd, GWL_EXSTYLE, GetWindowLong(this->m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	// 设置不透明度：第二个参数 0 表示使用第三个参数作为不透明度值，取值范围 0~255，255 完全不透明，这里设为 200（约 78% 不透明）
	SetLayeredWindowAttributes(0, 200, LWA_ALPHA);

	return TRUE;
}

void CColorInfoDlg::OnEnChangeCoordText()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}
