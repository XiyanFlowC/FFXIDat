#pragma once

#include <vector>

class CMergeDialog : public CDialogEx
{
public:
	struct Item
	{
		int index = -1;
		CString indexText;
		CString blockName;
		uint32_t typeA = 0;
		uint32_t typeB = 0;
		CString groupStatus;
		CString status;
		CString detail;
		BOOL selectable = TRUE;
		BOOL selected = FALSE;
	};

	CMergeDialog(std::vector<Item> items, CWnd *pParent = nullptr);
	const std::vector<Item> &GetItems() const;

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MERGE_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange *pDX) override;
	virtual BOOL OnInitDialog() override;
	virtual void OnOK() override;
	afx_msg void OnItemChanged(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()

private:
	CListCtrl m_list;
	CEdit m_detail;
	std::vector<Item> m_items;
};
