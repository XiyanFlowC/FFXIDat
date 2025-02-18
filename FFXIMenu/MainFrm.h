
// MainFrm.h: CMainFrame 类的接口
//

#pragma once
#include "ContentView.h"
#include "OutputWnd.h"
#include "PropertiesWnd.h"

class CMainFrame : public CFrameWndEx
{
	
protected: // 仅从序列化创建
	CMainFrame() noexcept;
	DECLARE_DYNCREATE(CMainFrame)

// 特性
public:

// 操作
public:
	void OutputLog(LPCTSTR lpszMessage, int isError = 0);
	void UpdateContentView();
	void SuppressPropertyUpdate(BOOL bSuppress = TRUE);

// 重写
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = nullptr, CCreateContext* pContext = nullptr);

// 实现
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // 控件条嵌入成员
	CMFCMenuBar       m_wndMenuBar;
	CMFCToolBar       m_wndToolBar;
	CMFCStatusBar     m_wndStatusBar;
	CMFCToolBarImages m_UserImages;
	CContentView      m_wndContentView;
	COutputWnd        m_wndOutput;
	CPropertiesWnd    m_wndProperties;

// 生成的消息映射函数
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewCustomize();
	afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnTreeUpdateRequest(WPARAM, LPARAM);
	afx_msg LRESULT OnFocusChanged(WPARAM, LPARAM);
	afx_msg LRESULT OnPropertiesChanged(WPARAM, LPARAM);
	afx_msg LRESULT OnPropFrqUpdBgn(WPARAM, LPARAM);
	afx_msg LRESULT OnPropFrqUpdEnd(WPARAM, LPARAM);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnLoadExtraTexture();
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg LRESULT OnTextureUpdated(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

	BOOL CreateDockingWindows();
	void SetDockingWindowIcons(BOOL bHiColorIcons);
private:
	BOOL m_bSuppressPropUpd;
};


