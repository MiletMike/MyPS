#pragma once
#include "afxcmn.h"
#include "resource.h"

class CKernelSizeDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CKernelSizeDlg)

public:
    CKernelSizeDlg(CWnd* pParent = nullptr);
    virtual ~CKernelSizeDlg();

    int m_nKernelSize;

    enum { IDD = IDD_KERNELSIZE};

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    DECLARE_MESSAGE_MAP()

public:
    CEdit m_editKernel;
    CSpinButtonCtrl m_spinKernel;
};