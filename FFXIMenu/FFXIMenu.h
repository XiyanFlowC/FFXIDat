// FFXIMenu.h: FFXIMenu 应用程序的主头文件
//
#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含 'pch.h' 以生成 PCH"
#endif

#include "resource.h"       // 主符号


// CFFXIMenuApp:
// 有关此类的实现，请参阅 FFXIMenu.cpp
//

class CFFXIMenuApp : public CWinAppEx
{
public:
	CFFXIMenuApp() noexcept;


// 重写
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// 实现
	ULONG_PTR m_gdiplusToken;

	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();

	afx_msg void OnAppAbout();
	afx_msg void OnFileMerge();
	DECLARE_MESSAGE_MAP()
};

extern CFFXIMenuApp theApp;
