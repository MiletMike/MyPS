// BlockSizeDlg.h
#pragma once
#include "afxcmn.h"      // 需要包含滑块控件头文件
class CBmpReaderView;   // 前置声明
class CBlockSizeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CBlockSizeDlg)

public:
	CBlockSizeDlg(CBmpReaderView* pView);
	CBlockSizeDlg(CWnd* pParent = NULL);
	virtual ~CBlockSizeDlg();

	// 非模态创建函数（替代 DoModal）
	virtual BOOL Create(UINT nIDTemplate, CWnd* pParentWnd);

	// 设置/获取当前块大小
	void SetBlockSize(int nSize);
	int GetBlockSize() const { return m_nBlockSize; }

	// 对话框数据
	enum { IDD = IDD_INPUT_BLOCKSIZE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();       // 非模态必须重载

	// 控件变量
	CSliderCtrl m_sliderBlock;
	int         m_nBlockSize;

	// 辅助函数：同步滑块与编辑框
	void UpdateControlsFromValue();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnEnChangeEditBlocksize();
	afx_msg void OnBnClickedOk();       // 应用按钮（IDOK）
	afx_msg void OnDestroy();

private:
	CBmpReaderView* m_pView;   // 保存视图指针
};