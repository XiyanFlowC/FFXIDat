
#include "pch.h"
#include "framework.h"
#include "MainFrm.h"
#include "ContentView.h"
#include "Resource.h"
#include "FFXIMenu.h"
#include "FFXIMenuDoc.h"

#define ICOIDX_FILE (0)
#define ICOIDX_IMGRP (1)
#define ICOIDX_CATEGORY (2)
#define ICOIDX_TEX (3)
#define ICOIDX_TEXCLIP (4)

class CContentViewMenuButton : public CMFCToolBarMenuButton
{
	friend class CContentView;

	DECLARE_SERIAL(CContentViewMenuButton)

public:
	CContentViewMenuButton(HMENU hMenu = nullptr) noexcept : CMFCToolBarMenuButton((UINT)-1, hMenu, -1)
	{
	}

	virtual void OnDraw(CDC* pDC, const CRect& rect, CMFCToolBarImages* pImages, BOOL bHorz = TRUE,
		BOOL bCustomizeMode = FALSE, BOOL bHighlight = FALSE, BOOL bDrawBorder = TRUE, BOOL bGrayDisabledButtons = TRUE)
	{
		pImages = CMFCToolBar::GetImages();

		CAfxDrawState ds;
		pImages->PrepareDrawImage(ds);

		CMFCToolBarMenuButton::OnDraw(pDC, rect, pImages, bHorz, bCustomizeMode, bHighlight, bDrawBorder, bGrayDisabledButtons);

		pImages->EndDrawImage(ds);
	}
};

IMPLEMENT_SERIAL(CContentViewMenuButton, CMFCToolBarMenuButton, 1)

//////////////////////////////////////////////////////////////////////
// 构造/析构
//////////////////////////////////////////////////////////////////////

CContentView::CContentView() noexcept
{
	m_nCurrSort = ID_SORTING_GROUPBYTYPE;
}

CContentView::~CContentView()
{
}

BEGIN_MESSAGE_MAP(CContentView, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_NEW_ITEM, OnNewItem)
	ON_COMMAND(ID_DELETE_ITEM, OnDeleteItem)
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_COMMAND_RANGE(ID_SORTING_GROUPBYTYPE, ID_SORTING_SORTBYACCESS, OnSort)
	ON_COMMAND(ID_TEXTURE_EXPORT, OnTextureExport)
	ON_COMMAND(ID_TEXTURE_IMPORT, OnTextureImport)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SORTING_GROUPBYTYPE, ID_SORTING_SORTBYACCESS, OnUpdateSort)
	ON_NOTIFY(TVN_SELCHANGED, 2, OnSelChanged)
	ON_NOTIFY(NM_RCLICK, 2, OnRClicked)
	ON_MESSAGE(WM_TREE_UPDATE_MSG, OnTreeUpdate)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CContentView 消息处理程序

int CContentView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// 创建视图: 
	const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (!m_wndContentView.Create(dwViewStyle, rectDummy, this, 2))
	{
		TRACE0("未能创建内容视图\n");
		return -1;      // 未能创建
	}

	// 加载图像: 
	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_SORT);
	m_wndToolBar.LoadToolBar(IDR_SORT, 0, 0, TRUE /* 已锁定*/);

	OnChangeVisualStyle();

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));

	m_wndToolBar.SetOwner(this);

	// 所有命令将通过此控件路由，而不是通过主框架路由: 
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	CContentViewMenuButton* pButton =  DYNAMIC_DOWNCAST(CContentViewMenuButton, m_wndToolBar.GetButton(0));

	if (pButton != nullptr)
	{
		pButton->m_bText = FALSE;
		pButton->m_bImage = TRUE;
		pButton->SetImage(GetCmdMgr()->GetCmdImage(m_nCurrSort));
		pButton->SetMessageWnd(this);
	}

	// 填入一些静态树视图数据(此处只需填入虚拟代码，而不是复杂的数据)
	// FillContentView();

	return 0;
}

void CContentView::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CContentView::FillContentView()
{
	HTREEITEM hRoot = m_wndContentView.InsertItem(_T("Menu 文件"), ICOIDX_FILE, ICOIDX_FILE);
	m_wndContentView.SetItemState(hRoot, TVIS_BOLD, TVIS_BOLD);

	m_wndContentView.Expand(hRoot, TVE_EXPAND);
}

void CContentView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	CTreeCtrl* pWndTree = (CTreeCtrl*)&m_wndContentView;
	ASSERT_VALID(pWndTree);

	if (pWnd != pWndTree)
	{
		CDockablePane::OnContextMenu(pWnd, point);
		return;
	}

	if (point != CPoint(-1, -1))
	{
		// 选择已单击的项: 
		CPoint ptTree = point;
		pWndTree->ScreenToClient(&ptTree);

		UINT flags = 0;
		HTREEITEM hTreeItem = pWndTree->HitTest(ptTree, &flags);
		if (hTreeItem != nullptr)
		{
			pWndTree->SelectItem(hTreeItem);
		}
	}

	pWndTree->SetFocus();
}

void CContentView::AdjustLayout()
{
	if (GetSafeHwnd() == nullptr)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	m_wndToolBar.SetWindowPos(nullptr, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndContentView.SetWindowPos(nullptr, rectClient.left + 1, rectClient.top + cyTlb + 1, rectClient.Width() - 2, rectClient.Height() - cyTlb - 2, SWP_NOACTIVATE | SWP_NOZORDER);
}

BOOL CContentView::PreTranslateMessage(MSG* pMsg)
{
	return CDockablePane::PreTranslateMessage(pMsg);
}

void CContentView::OnSort(UINT id)
{
	if (m_nCurrSort == id)
	{
		return;
	}

	m_nCurrSort = id;

	CContentViewMenuButton* pButton =  DYNAMIC_DOWNCAST(CContentViewMenuButton, m_wndToolBar.GetButton(0));

	if (pButton != nullptr)
	{
		pButton->SetImage(GetCmdMgr()->GetCmdImage(id));
		m_wndToolBar.Invalidate();
		m_wndToolBar.UpdateWindow();
	}
}

void CContentView::OnUpdateSort(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(pCmdUI->m_nID == m_nCurrSort);
}

void CContentView::OnTextureExport()
{
	HTREEITEM hItem = m_wndContentView.GetSelectedItem();
	ContentNode *pNode = reinterpret_cast<ContentNode *>(
		m_wndContentView.GetItemData(hItem)
		);
	ImageBlockNode *in = dynamic_cast<ImageBlockNode *>(pNode);
	if (!in) return;

	CString name(in->GetTextureId(), 16);
	CFileDialog fileDlg(
		FALSE,
		_T("dds"),
		name.GetString(),
		OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
		_T("DDS 图像 (*.dds)|*.dds|所有文件 (*.*)|*.*||"),
		this
	);

	// 设置对话框标题
	fileDlg.m_ofn.lpstrTitle = _T("导出图片");

	// 显示对话框
	if (fileDlg.DoModal() == IDOK)
	{
		CString path = fileDlg.GetPathName();
		in->DumpTexture(path.GetString());
	}
}

void CContentView::OnTextureImport()
{
	HTREEITEM hItem = m_wndContentView.GetSelectedItem();
	ContentNode *pNode = reinterpret_cast<ContentNode *>(
		m_wndContentView.GetItemData(hItem)
		);
	ImageBlockNode *in = dynamic_cast<ImageBlockNode *>(pNode);
	if (!in) return;

	CString name(in->GetTextureId(), 16);
	CFileDialog fileDlg(
		TRUE,
		_T("dds"),
		name.GetString(),
		OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
		_T("DDS 图像 (*.dds)|*.dds|所有文件 (*.*)|*.*||"),
		this
	);

	// 设置对话框标题
	fileDlg.m_ofn.lpstrTitle = _T("导入图片");

	// 显示对话框
	if (fileDlg.DoModal() == IDOK)
	{
		CString path = fileDlg.GetPathName();
		in->ImportTexture(path.GetString());

		AfxGetMainWnd()->SendMessage(WM_NODE_FOCUS_CHANGE_MSG);
		AfxGetMainWnd()->SendMessage(WM_TEXTURE_UPDATE_MSG);
	}
}

LRESULT CContentView::OnTreeUpdate(WPARAM, LPARAM)
{
	ReloadTreeData();
	return 0;
}

void CContentView::OnSelChanged(NMHDR * pHdr, LRESULT *pResult)
{
	HTREEITEM hItem = m_wndContentView.GetSelectedItem();

	if (hItem) {
		ContentNode *pNode = reinterpret_cast<ContentNode *>(
			m_wndContentView.GetItemData(hItem)
			);

		if (pNode) {
			CMainFrame *pMainFrame = (CMainFrame *)AfxGetMainWnd();
			CFFXIMenuDoc *pDoc = (CFFXIMenuDoc *)pMainFrame->GetActiveDocument();
			pDoc->FocusOn(pNode);
		}
	}
	*pResult = 0;
}

void CContentView::OnRClicked(NMHDR *, LRESULT *pResult)
{
	CPoint point;
	GetCursorPos(&point);
	CPoint pointInTree = point;
	m_wndContentView.ScreenToClient(&pointInTree);

	HTREEITEM item;
	UINT flag = TVHT_ONITEM;
	item = m_wndContentView.HitTest(pointInTree, &flag);

	if (item != NULL)
	{
		m_wndContentView.SelectItem(item);

		ContentNode *node = (ContentNode *)m_wndContentView.GetItemData(item);

		if (ImageBlockNode *in = dynamic_cast<ImageBlockNode *>(node))
		{
			//CMenu menu;
			//menu.LoadMenu(IDR_POPUP_TEXTURE);
			/*menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN |
				TPM_RIGHTBUTTON, point.x, point.y, this, NULL);*/
			theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_TEXTURE, point.x, point.y, this, TRUE);
		}
	}
}

void CContentView::ReloadTreeData()
{
	CMainFrame *pMainFrame = (CMainFrame *)AfxGetMainWnd();
	CFFXIMenuDoc *pDoc = (CFFXIMenuDoc *)pMainFrame->GetActiveDocument();
	if (pDoc && pDoc->rootNode)
	{
		m_wndContentView.DeleteAllItems();
		BuildTree(pDoc->rootNode, TVI_ROOT);
	}
}

void CContentView::BuildTree(ContentNode *pNode, HTREEITEM hNode)
{
	if (!pNode) return;

	// 插入当前节点
	HTREEITEM hItem = m_wndContentView.InsertItem(
		pNode->GetName(),
		pNode->GetIconId(),
		pNode->GetIconId(),
		hNode
	);

	// 递归添加子节点
	for (auto &child : pNode->GetChildren())
	{
		BuildTree(child, hItem);
	}
	m_wndContentView.SetItemData(hItem, (DWORD_PTR)pNode);

	if (hNode == TVI_ROOT)
		m_wndContentView.Expand(hItem, TVE_EXPAND);
}

void CContentView::DeleteChildren(HTREEITEM hNode)
{
	HTREEITEM hNext;
	HTREEITEM hChild = m_wndContentView.GetChildItem(hNode);

	while (hChild != NULL)
	{
		hNext = m_wndContentView.GetNextItem(hChild, TVGN_NEXT);
		m_wndContentView.DeleteItem(hChild);
		hChild = hNext;
	}
}

void CContentView::OnDeleteItem()
{
	HTREEITEM hItem = m_wndContentView.GetSelectedItem();

	if (hItem) {
		ContentNode *pNode = reinterpret_cast<ContentNode *>(
			m_wndContentView.GetItemData(hItem));
		HTREEITEM hParent = m_wndContentView.GetParentItem(hItem);

		int index = -1;
		if (ClipNode *clipNode = dynamic_cast<ClipNode *>(pNode)) {
			index = clipNode->GetIndex();
		}
		else if (TileNode *tileNode = dynamic_cast<TileNode *>(pNode)) {
			index = tileNode->GetIndex();
		}
		else
		{
			AfxMessageBox(_T("此节点不可删除！"));
			return;
		}

		HTREEITEM hGroup = m_wndContentView.GetParentItem(hParent);
		if (ImageGroupNode *groupNode =
			dynamic_cast<ImageGroupNode *>((ContentNode *)m_wndContentView.GetItemData(hGroup)))
		{
			// 数据层面删除
			groupNode->DeleteClipTileOf(index);

			if (CMainFrame *frame = dynamic_cast<CMainFrame *>(AfxGetMainWnd()))
			{
				frame->GetActiveDocument()->SetModifiedFlag();
			}

			if (CMainFrame *frame = dynamic_cast<CMainFrame *>(AfxGetMainWnd()))
			{
				frame->GetActiveDocument()->SetModifiedFlag();
			}

			// 删除子节点
			HTREEITEM hClipC = m_wndContentView.GetChildItem(hGroup);
			HTREEITEM hTileC = m_wndContentView.GetNextItem(hClipC, TVGN_NEXT);

			DeleteChildren(hClipC);
			DeleteChildren(hTileC);

			for (auto child : groupNode->GetChildren()[0]->GetChildren())
			{
				BuildTree(child, hClipC);
			}
			for (auto child : groupNode->GetChildren()[1]->GetChildren())
			{
				BuildTree(child, hTileC);
			}

			// 更新选择
			m_wndContentView.SelectItem(hParent);
			m_wndContentView.Expand(hParent, TVE_EXPAND);
		}
	}
}

void CContentView::OnNewItem()
{
	HTREEITEM hItem = m_wndContentView.GetSelectedItem();

	if (hItem) {
		ContentNode *pNode = reinterpret_cast<ContentNode *>(
			m_wndContentView.GetItemData(hItem));
		HTREEITEM hParent = m_wndContentView.GetParentItem(hItem);

		int index = -1;
		if (ClipNode *clipNode = dynamic_cast<ClipNode *>(pNode)) {
			index = clipNode->GetIndex();
		}
		else if (TileNode *tileNode = dynamic_cast<TileNode *>(pNode)) {
			index = tileNode->GetIndex();
		}
		else
		{
			AfxMessageBox(_T("不能在这里插入。"));
			return;
		}

		HTREEITEM hGroup = m_wndContentView.GetParentItem(hParent);
		if (ImageGroupNode *groupNode =
			dynamic_cast<ImageGroupNode *>((ContentNode *)m_wndContentView.GetItemData(hGroup)))
		{
			// 数据层面插入
			BlockFile::ImageSetBlock::ImageGroup::ImageRef ref;
			ref.ukn[0] = 1;
			ref.ukn[1] = 0;
			ref.ukn[2] = 2;
			ref.ukn[3] = 1;
			ref.tlColour.Set(127, 127, 127, 127);
			ref.trColour.Set(127, 127, 127, 127);
			ref.blColour.Set(127, 127, 127, 127);
			ref.brColour.Set(127, 127, 127, 127);
			groupNode->InsertClipTileAfter(index, ref);

			if (CMainFrame *frame = dynamic_cast<CMainFrame *>(AfxGetMainWnd()))
			{
				frame->GetActiveDocument()->SetModifiedFlag();
			}
			// 数据上已经插入
			// 删除子节点
			HTREEITEM hClipC = m_wndContentView.GetChildItem(hGroup);
			HTREEITEM hTileC = m_wndContentView.GetNextItem(hClipC, TVGN_NEXT);

			DeleteChildren(hClipC);
			DeleteChildren(hTileC);

			for (auto child : groupNode->GetChildren()[0]->GetChildren())
			{
				BuildTree(child, hClipC);
			}
			for (auto child : groupNode->GetChildren()[1]->GetChildren())
			{
				BuildTree(child, hTileC);
			}

			// 更新选择
			m_wndContentView.SelectItem(hParent);
			m_wndContentView.Expand(hParent, TVE_EXPAND);
		}
	}
}

void CContentView::OnPaint()
{
	CPaintDC dc(this); // 用于绘制的设备上下文

	CRect rectTree;
	m_wndContentView.GetWindowRect(rectTree);
	ScreenToClient(rectTree);

	rectTree.InflateRect(1, 1);
	dc.Draw3dRect(rectTree, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DSHADOW));
}

void CContentView::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);

	m_wndContentView.SetFocus();
}

void CContentView::OnChangeVisualStyle()
{
	m_ContentViewImages.DeleteImageList();

	UINT uiBmpId = theApp.m_bHiColorIcons ? IDB_CLASS_VIEW_24 : IDB_CLASS_VIEW;

	CBitmap bmp;
	if (!bmp.LoadBitmap(uiBmpId))
	{
		TRACE(_T("无法加载位图: %x\n"), uiBmpId);
		ASSERT(FALSE);
		return;
	}

	BITMAP bmpObj;
	bmp.GetBitmap(&bmpObj);

	UINT nFlags = ILC_MASK;

	nFlags |= (theApp.m_bHiColorIcons) ? ILC_COLOR24 : ILC_COLOR4;

	m_ContentViewImages.Create(16, bmpObj.bmHeight, nFlags, 0, 0);
	m_ContentViewImages.Add(&bmp, RGB(255, 0, 0));

	m_wndContentView.SetImageList(&m_ContentViewImages, TVSIL_NORMAL);

	m_wndToolBar.CleanUpLockedImages();
	m_wndToolBar.LoadBitmap(theApp.m_bHiColorIcons ? IDB_SORT_24 : IDR_SORT, 0, 0, TRUE /* 锁定*/);
}
