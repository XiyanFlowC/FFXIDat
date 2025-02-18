
#pragma once

#include "ViewTree.h"

class CContentToolBar : public CMFCToolBar
{
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}

	virtual BOOL AllowShowOnList() const { return FALSE; }
};

class CContentView : public CDockablePane
{
public:
	CContentView() noexcept;
	virtual ~CContentView();

	void AdjustLayout();
	void OnChangeVisualStyle();

protected:
	CContentToolBar m_wndToolBar;
	CViewTree m_wndContentView;
	CImageList m_ContentViewImages;
	UINT m_nCurrSort;

	void FillContentView();

// 重写
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void ReloadTreeData();

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnDeleteItem();
	afx_msg void OnNewItem();
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg LRESULT OnChangeActiveTab(WPARAM, LPARAM);
	afx_msg void OnSort(UINT id);
	afx_msg void OnUpdateSort(CCmdUI* pCmdUI);
	afx_msg void OnTextureExport();
	afx_msg void OnTextureImport();
	afx_msg LRESULT OnTreeUpdate(WPARAM, LPARAM);
	afx_msg void OnSelChanged(NMHDR *, LRESULT *pResult);
	afx_msg void OnRClicked(NMHDR *, LRESULT *pResult);

	void BuildTree(class ContentNode *pNode, HTREEITEM hParent);
	void DeleteChildren(HTREEITEM pNode);

	DECLARE_MESSAGE_MAP()
};

