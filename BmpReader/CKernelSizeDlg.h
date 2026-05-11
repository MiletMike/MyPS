// CKernelSizeDlg.h
#pragma once

#include "afxcmn.h"
#include "resource.h"

class CKernelSizeDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CKernelSizeDlg)

public:
    // 原有构造函数（用于核大小输入）
    CKernelSizeDlg(CWnd* pParent = nullptr);

    // ========== 新增：通用输入对话框构造函数 ==========
    // lpszPrompt: 提示文字
    // nDefault:   默认值
    // nMin:       最小值
    // nMax:       最大值
    CKernelSizeDlg(LPCTSTR lpszPrompt, int nDefault = 50, int nMin = 1, int nMax = 255);
    // =================================================

    virtual ~CKernelSizeDlg();

    // 输入值（共用）
    int m_nValue;

    // 核大小（为了兼容原有代码，保留原名）
    int m_nKernelSize;

    enum { IDD = IDD_KERNELSIZE };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

public:
    CEdit m_editKernel;
    CSpinButtonCtrl m_spinKernel;
    void UpdatePromptText(LPCTSTR lpszPrompt);

private:
    // ========== 新增：用于通用输入的成员变量 ==========
    CString m_strPrompt;    // 提示文字
    int m_nMinValue;        // 最小值
    int m_nMaxValue;        // 最大值
    BOOL m_bUseGenericMode; // 是否使用通用模式
    // =================================================
public:
    afx_msg void OnStnClickedStaticPrompt();
    afx_msg void OnEnChangeEditKernelsize();
};