
#pragma once

/////////////////////////////////////////////////////////////////////////////
// COutputList 窗口

class COutputList : public CListBox
{
// 构造
public:
	COutputList() noexcept;

// 实现
public:
	virtual ~COutputList();

protected:
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnEditCopy();
	afx_msg void OnEditClear();
	afx_msg void OnViewOutput();

	DECLARE_MESSAGE_MAP()
};

class COutputWnd : public CDockablePane
{
// 构造
public:
	COutputWnd() noexcept;

	void UpdateFonts();

// 特性
protected:
	CMFCTabCtrl	m_wndTabs;

	COutputList m_wndOutputInfo;
	COutputList m_wndOutputError;

protected:
	void FillBuildWindow();
	void FillDebugWindow();

	void AdjustHorzScroll(CListBox& wndListBox);

// 实现
public:
	virtual ~COutputWnd();

	void OutputInfo(LPCTSTR lpszMsg);
	void OutputError(LPCTSTR lpszMsg);

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()
};

