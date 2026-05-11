// CKernelSizeDlg.cpp
#include "stdafx.h"
#include "CKernelSizeDlg.h"
#include "resource.h"

IMPLEMENT_DYNAMIC(CKernelSizeDlg, CDialogEx)

CKernelSizeDlg::CKernelSizeDlg(CWnd* pParent)
    : CDialogEx(IDD_KERNELSIZE, pParent)
    , m_nKernelSize(3)
    , m_nValue(3)
    , m_bUseGenericMode(FALSE)
    , m_nMinValue(3)
    , m_nMaxValue(15)
{
}

CKernelSizeDlg::CKernelSizeDlg(LPCTSTR lpszPrompt, int nDefault, int nMin, int nMax)
    : CDialogEx(IDD_KERNELSIZE, NULL)
    , m_strPrompt(lpszPrompt)
    , m_nValue(nDefault)
    , m_nKernelSize(nDefault)
    , m_nMinValue(nMin)
    , m_nMaxValue(nMax)
    , m_bUseGenericMode(TRUE)
{
}

CKernelSizeDlg::~CKernelSizeDlg()
{
}

void CKernelSizeDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_KERNELSIZE, m_editKernel);
    DDX_Control(pDX, ID_SPIN_KERNELSIZE, m_spinKernel);
    DDX_Text(pDX, IDC_EDIT_KERNELSIZE, m_nValue);
    DDV_MinMaxInt(pDX, m_nValue, m_nMinValue, m_nMaxValue);
}

BOOL CKernelSizeDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_spinKernel.SetRange(m_nMinValue, m_nMaxValue);
    m_spinKernel.SetPos(m_nValue);
    if (m_bUseGenericMode)
    {
        SetWindowText(_T("参数输入"));
    }
    return TRUE;
}

void CKernelSizeDlg::UpdatePromptText(LPCTSTR lpszPrompt)
{
    m_strPrompt = lpszPrompt;
    m_bUseGenericMode = TRUE;

    // 如果对话框已经创建，更新静态文本控件
    if (GetSafeHwnd())
    {
        SetWindowText(_T("参数输入"));  // 更新对话框标题

        // 更新提示文字（假设控件ID为 IDC_STATIC_PROMPT）
        CWnd* pStatic = GetDlgItem(IDC_STATIC_PROMPT);
        if (pStatic)
        {
            pStatic->SetWindowText(lpszPrompt);
        }
    }
}

BEGIN_MESSAGE_MAP(CKernelSizeDlg, CDialogEx)
    ON_STN_CLICKED(IDC_STATIC_PROMPT, &CKernelSizeDlg::OnStnClickedStaticPrompt)
    ON_EN_CHANGE(IDC_EDIT_KERNELSIZE, &CKernelSizeDlg::OnEnChangeEditKernelsize)
END_MESSAGE_MAP()
void CKernelSizeDlg::OnStnClickedStaticPrompt()
{
    // TODO: 在此添加控件通知处理程序代码
}

void CKernelSizeDlg::OnEnChangeEditKernelsize()
{
    // TODO:  如果该控件是 RICHEDIT 控件，它将不
    // 发送此通知，除非重写 CDialogEx::OnInitDialog()
    // 函数并调用 CRichEditCtrl().SetEventMask()，
    // 同时将 ENM_CHANGE 标志“或”运算到掩码中。

    // TODO:  在此添加控件通知处理程序代码
}
