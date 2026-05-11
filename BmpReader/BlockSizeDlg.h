// BlockSizeDlg.h
#pragma once
#include "afxcmn.h"      // Required for slider control header
class CBmpReaderView;   // Forward declaration
class CBlockSizeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CBlockSizeDlg)

public:
	CBlockSizeDlg(CBmpReaderView* pView);
	CBlockSizeDlg(CWnd* pParent = NULL);
	virtual ~CBlockSizeDlg();

	// Non-modal create function (replacement for DoModal)
	virtual BOOL Create(UINT nIDTemplate, CWnd* pParentWnd);

	// Set/Get current block size
	void SetBlockSize(int nSize);
	int GetBlockSize() const { return m_nBlockSize; }

	// Dialog data
	enum { IDD = IDD_INPUT_BLOCKSIZE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();       // Required for non-modal dialogs

	// Control variables
	CSliderCtrl m_sliderBlock;
	int         m_nBlockSize;

	// Helper function: synchronize slider and edit box
	void UpdateControlsFromValue();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnEnChangeEditBlocksize();
	afx_msg void OnBnClickedOk();       // Apply button (IDOK)
	afx_msg void OnDestroy();

private:
	CBmpReaderView* m_pView;   // Store view pointer
};