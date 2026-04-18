// BlockSizeDlg.cpp
#include "stdafx.h"
#include "BmpReader.h"
#include "BlockSizeDlg.h"
#include "BmpReaderView.h"      // 需要访问视图的更新函数

IMPLEMENT_DYNAMIC(CBlockSizeDlg, CDialogEx)

CBlockSizeDlg::CBlockSizeDlg(CBmpReaderView* pView)
	: CDialogEx(CBlockSizeDlg::IDD, pView)   // 父窗口设为 pView
	, m_nBlockSize(8)
	, m_pView(pView)
{
}

CBlockSizeDlg::~CBlockSizeDlg()
{
}

void CBlockSizeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SLIDER_BLOCKSIZE, m_sliderBlock);
	DDX_Text(pDX, IDC_EDIT_BLOCKSIZE, m_nBlockSize);
	DDV_MinMaxInt(pDX, m_nBlockSize, 2, 64);
}

BEGIN_MESSAGE_MAP(CBlockSizeDlg, CDialogEx)
	ON_WM_HSCROLL()
	ON_EN_CHANGE(IDC_EDIT_BLOCKSIZE, &CBlockSizeDlg::OnEnChangeEditBlocksize)
	ON_BN_CLICKED(IDOK, &CBlockSizeDlg::OnBnClickedOk)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CBlockSizeDlg::Create(UINT nIDTemplate, CWnd* pParentWnd)
{
	return CDialogEx::Create(nIDTemplate, pParentWnd);
}

void CBlockSizeDlg::PostNcDestroy()
{
	CDialogEx::PostNcDestroy();
	delete this;    // 非模态窗口必须自行释放内存
}

BOOL CBlockSizeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 初始化滑块范围 (2~64)
	m_sliderBlock.SetRange(2, 64);
	m_sliderBlock.SetTicFreq(4);       // 每4个单位一个刻度
	m_sliderBlock.SetPos(m_nBlockSize);

	UpdateData(FALSE);   // 将变量显示到控件

	return TRUE;
}

void CBlockSizeDlg::SetBlockSize(int nSize)
{
	m_nBlockSize = nSize;
	if (GetSafeHwnd())
	{
		UpdateData(FALSE);
		m_sliderBlock.SetPos(m_nBlockSize);
	}
}

void CBlockSizeDlg::UpdateControlsFromValue()
{
	UpdateData(TRUE);   // 从编辑框读取数值
	m_sliderBlock.SetPos(m_nBlockSize);
}

// 滑块拖动消息
// 滑块拖动消息
void CBlockSizeDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar == (CScrollBar*)&m_sliderBlock)
	{
		m_nBlockSize = m_sliderBlock.GetPos();
		SetDlgItemInt(IDC_EDIT_BLOCKSIZE, m_nBlockSize);  // 只更新编辑框

		if (m_pView != NULL)
			m_pView->ApplyAdaptiveEqualize(m_nBlockSize);
	}
	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

// 编辑框内容变化（用户手动输入时）
void CBlockSizeDlg::OnEnChangeEditBlocksize()
{
	// 可以在此处不做处理，等用户点击“应用”或按回车
	// 如果想实时响应输入，可以调用 UpdateControlsFromValue 并触发预览，
	// 但频繁触发可能造成卡顿，建议仅在应用按钮处理。
}

// “应用”按钮点击（IDOK）
void CBlockSizeDlg::OnBnClickedOk()
{
	UpdateData(TRUE);
	m_sliderBlock.SetPos(m_nBlockSize);

	if (m_pView != NULL)
		m_pView->ApplyAdaptiveEqualize(m_nBlockSize);
}

void CBlockSizeDlg::OnDestroy()
{
	// 通知父窗口本对话框已关闭
	if (GetParent())
		::SendMessage(GetParent()->m_hWnd, WM_USER_DESTROY_ADAPTIVE, 0, 0);
	CDialogEx::OnDestroy();
}