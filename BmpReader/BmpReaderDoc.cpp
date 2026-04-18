
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
	// Ë¢ÐÂÊÓÍ¼
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	CView* pView = pFrame->GetActiveView();
	if (pView) {
		pView->Invalidate(TRUE);
		CBmpReaderView* pMyView = dynamic_cast<CBmpReaderView*>(pView);
		if (pMyView)
			pMyView->UpdateHistogramWindow();
	}
}
