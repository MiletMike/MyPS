#include "stdafx.h"
#include "CKernelSizeDlg.h"
#include "resource.h"

IMPLEMENT_DYNAMIC(CKernelSizeDlg, CDialogEx)

CKernelSizeDlg::CKernelSizeDlg(CWnd* pParent)
    : CDialogEx(IDD_KERNELSIZE, pParent)
    , m_nKernelSize(3)
{
}

CKernelSizeDlg::~CKernelSizeDlg() {}

BEGIN_MESSAGE_MAP(CKernelSizeDlg, CDialogEx)
END_MESSAGE_MAP()

void CKernelSizeDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_KERNELSIZE, m_editKernel);
    DDX_Control(pDX, ID_SPIN_KERNELSIZE, m_spinKernel);
    DDX_Text(pDX, IDC_EDIT_KERNELSIZE, m_nKernelSize);
    DDV_MinMaxInt(pDX, m_nKernelSize, 3, 15);
}

BOOL CKernelSizeDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_spinKernel.SetRange(3, 15);
    m_spinKernel.SetPos(m_nKernelSize);
    return TRUE;
}