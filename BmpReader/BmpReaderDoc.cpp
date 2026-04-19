
// BmpReaderDoc.cpp : implementation of the CBmpReaderDoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "BmpReader.h"
#endif

#include "BmpReaderDoc.h"
#include "MainFrm.h" 
#include "CImageProc.h"
#include "BmpReaderView.h" 
#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CBmpReaderDoc

IMPLEMENT_DYNCREATE(CBmpReaderDoc, CDocument)

BEGIN_MESSAGE_MAP(CBmpReaderDoc, CDocument)
	ON_COMMAND(ID_FILE_OPEN, &CBmpReaderDoc::OnFileOpen)
	ON_COMMAND(ID_FILE_SAVE, &CBmpReaderDoc::OnFileSave)
END_MESSAGE_MAP()


// CBmpReaderDoc construction/destruction

CBmpReaderDoc::CBmpReaderDoc()
{
	pImage = new CImageProc;
}

CBmpReaderDoc::~CBmpReaderDoc()
{
	delete pImage;
}

BOOL CBmpReaderDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CBmpReaderDoc serialization

void CBmpReaderDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CBmpReaderDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CBmpReaderDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data. 
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CBmpReaderDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CBmpReaderDoc diagnostics

#ifdef _DEBUG
void CBmpReaderDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CBmpReaderDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CBmpReaderDoc commands


void CBmpReaderDoc::OnFileOpen()
{
	if (pImage)
		pImage->OpenFile();
	// 刷新视图
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	CView* pView = pFrame->GetActiveView();
	if (pView) {
		pView->Invalidate(TRUE);
		CBmpReaderView* pMyView = dynamic_cast<CBmpReaderView*>(pView);
		if (pMyView)
			pMyView->UpdateHistogramWindow();
	}
}

// 保存图像处理结果
void CBmpReaderDoc::OnFileSave()
{
	// 1. 检查是否有图像数据
	if (!pImage || !pImage->m_pRGB24 || pImage->m_nWidth <= 0 || pImage->m_nHeight <= 0)
	{
		AfxMessageBox(_T("没有可保存的图像！"));
		return;
	}

	// 2. 弹出“另存为”对话框
	CFileDialog dlg(FALSE, _T("bmp"), _T("result.bmp"),
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("BMP 文件 (*.bmp)|*.bmp|JPEG 文件 (*.jpg)|*.jpg|PNG 文件 (*.png)|*.png|所有文件 (*.*)|*.*||"));

	if (dlg.DoModal() != IDOK)
		return;

	CString strPath = dlg.GetPathName();
	CString strExt = dlg.GetFileExt();
	strExt.MakeLower();

	// 3. 使用 CImage 创建图像并保存
	CImage img;
	int nWidth = pImage->m_nWidth;
	int nHeight = pImage->m_nHeight;

	// 创建 24 位 RGB 图像
	if (!img.Create(nWidth, nHeight, 24))
	{
		AfxMessageBox(_T("创建图像失败！"));
		return;
	}

	// 4. 将 m_pRGB24 的数据拷贝到 CImage 中
	// 注意：m_pRGB24 存储顺序为 BGR，且每行按 4 字节对齐
	int srcBytesPerLine = ((nWidth * 24 + 31) / 32) * 4;
	int dstBytesPerLine = img.GetPitch();  // CImage 每行字节数（可能为负，取绝对值）

	for (int y = 0; y < nHeight; y++)
	{
		BYTE* pSrc = pImage->m_pRGB24 + y * srcBytesPerLine;
		BYTE* pDst = (BYTE*)img.GetPixelAddress(0, y);
		if (pDst == nullptr) continue;
		// 逐像素复制（BGR 顺序保持一致）
		memcpy(pDst, pSrc, nWidth * 3);
	}

	// 5. 根据扩展名选择保存格式
	HRESULT hr = E_FAIL;
	if (strExt == _T("jpg") || strExt == _T("jpeg"))
	{
		// JPEG 质量参数（可选）
		// 这里直接用默认质量，也可以设置高质量
		// 参考：https://learn.microsoft.com/en-us/windows/win32/gdiplus/-gdiplus-constant-image-encoder-constants
		hr = img.Save(strPath, Gdiplus::ImageFormatJPEG);
	}
	else if (strExt == _T("png"))
	{
		hr = img.Save(strPath, Gdiplus::ImageFormatPNG);
	}
	else  // 默认保存为 BMP
	{
		hr = img.Save(strPath, Gdiplus::ImageFormatBMP);
	}

	if (SUCCEEDED(hr))
		AfxMessageBox(_T("图像保存成功！"));
	else
		AfxMessageBox(_T("保存失败，请检查路径或格式！"));
}