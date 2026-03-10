#include "pch.h"
#include "framework.h"
#include "FFXIMenu.h"
#include "MergeDialog.h"

CMergeDialog::CMergeDialog(std::vector<Item> items, CWnd *pParent)
	: CDialogEx(IDD_MERGE_DIALOG, pParent), m_items(std::move(items))
{
}

const std::vector<CMergeDialog::Item> &CMergeDialog::GetItems() const
{
	return m_items;
}

void CMergeDialog::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MERGE_LIST, m_list);
	DDX_Control(pDX, IDC_MERGE_DETAIL, m_detail);
}

BEGIN_MESSAGE_MAP(CMergeDialog, CDialogEx)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_MERGE_LIST, &CMergeDialog::OnItemChanged)
END_MESSAGE_MAP()

BOOL CMergeDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);
	m_list.InsertColumn(0, _T("应用B"), LVCFMT_LEFT, 60);
	m_list.InsertColumn(1, _T("块索引"), LVCFMT_LEFT, 55);
	m_list.InsertColumn(2, _T("块名称"), LVCFMT_LEFT, 68);
	m_list.InsertColumn(3, _T("A类型"), LVCFMT_LEFT, 48);
	m_list.InsertColumn(4, _T("B类型"), LVCFMT_LEFT, 48);
	m_list.InsertColumn(5, _T("组状态"), LVCFMT_LEFT, 74);
	m_list.InsertColumn(6, _T("状态"), LVCFMT_LEFT, 80);

	for (int i = 0; i < (int)m_items.size(); ++i)
	{
		const auto &it = m_items[i];
		CString idx;
		if (!it.indexText.IsEmpty()) idx = it.indexText;
		else idx.Format(_T("%d"), it.index);
		CString ta;
		ta.Format(_T("%u"), it.typeA);
		CString tb;
		tb.Format(_T("%u"), it.typeB);

		m_list.InsertItem(i, _T(""));
		m_list.SetItemText(i, 1, idx);
		m_list.SetItemText(i, 2, it.blockName);
		m_list.SetItemText(i, 3, ta);
		m_list.SetItemText(i, 4, tb);
		m_list.SetItemText(i, 5, it.groupStatus);
		m_list.SetItemText(i, 6, it.status);
		m_list.SetCheck(i, it.selectable && it.selected);
	}

	if (!m_items.empty())
	{
		m_list.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
		m_detail.SetWindowText(m_items[0].detail);
	}

	return TRUE;
}

void CMergeDialog::OnItemChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int idx = pNMLV->iItem;
	if (idx >= 0 && idx < (int)m_items.size())
	{
		if ((pNMLV->uChanged & LVIF_STATE) != 0)
		{
			if (!m_items[idx].selectable)
			{
				m_list.SetCheck(idx, FALSE);
			}
		}
		if ((m_list.GetItemState(idx, LVIS_SELECTED) & LVIS_SELECTED) != 0)
		{
			m_detail.SetWindowText(m_items[idx].detail);
		}
	}
	*pResult = 0;
}

void CMergeDialog::OnOK()
{
	for (int i = 0; i < (int)m_items.size(); ++i)
	{
		m_items[i].selected = m_items[i].selectable ? m_list.GetCheck(i) : FALSE;
	}
	CDialogEx::OnOK();
}
