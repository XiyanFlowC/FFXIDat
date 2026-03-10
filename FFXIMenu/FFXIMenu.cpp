// FFXIMenu.cpp: 定义应用程序的类行为。
//

#include "pch.h"
#include "framework.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "FFXIMenu.h"
#include "MainFrm.h"


#include <gdiplus.h>
#include "FFXIMenuDoc.h"
#include "FFXIMenuView.h"
#include "MergeDialog.h"

#include "../FFXIDat/BlockFile.h"

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFFXIMenuApp

BEGIN_MESSAGE_MAP(CFFXIMenuApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CFFXIMenuApp::OnAppAbout)
	ON_COMMAND(ID_FILE_MERGE, &CFFXIMenuApp::OnFileMerge)
	// 基于文件的标准文档命令
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
END_MESSAGE_MAP()


// CFFXIMenuApp 构造

CFFXIMenuApp::CFFXIMenuApp() noexcept
{
	m_bHiColorIcons = TRUE;


	m_nAppLook = 0;
	// 支持重新启动管理器
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
#ifdef _MANAGED
	// 如果应用程序是利用公共语言运行时支持(/clr)构建的，则: 
	//     1) 必须有此附加设置，“重新启动管理器”支持才能正常工作。
	//     2) 在您的项目中，您必须按照生成顺序向 System.Windows.Forms 添加引用。
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	SetAppID(_T("SingingHill.FFXIMenu.FFXIMenuEditor.NoVersion"));

	// TODO:  在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}

// 唯一的 CFFXIMenuApp 对象

CFFXIMenuApp theApp;


// CFFXIMenuApp 初始化

BOOL CFFXIMenuApp::InitInstance()
{

	// GDI+ 初始化
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);
	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControlsEx()。  否则，将无法创建窗口。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();


	EnableTaskbarInteraction(FALSE);

	// 使用 RichEdit 控件需要 AfxInitRichEdit2()
	// AfxInitRichEdit2();

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	SetRegistryKey(_T("SingingHill"));
	LoadStdProfileSettings(5);  // 加载标准 INI 文件选项(包括 MRU)


	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// 注册应用程序的文档模板。  文档模板
	// 将用作文档、框架窗口和视图之间的连接
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CFFXIMenuDoc),
		RUNTIME_CLASS(CMainFrame),       // 主 SDI 框架窗口
		RUNTIME_CLASS(CFFXIMenuView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);


	// 分析标准 shell 命令、DDE、打开文件操作的命令行
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// 启用“DDE 执行”
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);


	// 调度在命令行中指定的命令。  如果
	// 用 /RegServer、/Register、/Unregserver 或 /Unregister 启动应用程序，则返回 FALSE。
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// 唯一的一个窗口已初始化，因此显示它并对其进行更新
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	// 仅当具有后缀时才调用 DragAcceptFiles
	//  在 SDI 应用程序中，这应在 ProcessShellCommand 之后发生
	// 启用拖/放
	m_pMainWnd->DragAcceptFiles();

	return TRUE;
}

int CFFXIMenuApp::ExitInstance()
{
	// GDI+ 关闭
	Gdiplus::GdiplusShutdown(m_gdiplusToken);

	return CWinAppEx::ExitInstance();
}

// CFFXIMenuApp 消息处理程序


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg() noexcept;

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() noexcept : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// 用于运行对话框的应用程序命令
void CFFXIMenuApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

namespace
{
	using Block = BlockFile::Block;
	using ImageSetBlock = BlockFile::ImageSetBlock;

	struct MergeEntry
	{
		int indexA = -1;
		int indexB = -1;
		Block *a = nullptr;
		Block *b = nullptr;
		bool same = false;
		bool selectedUseB = false;
		CString status;
		CString detail;
	};

	size_t GetImageTextureSize(const Image &img)
	{
		if (!img.texture) return 0;
		if (img.type != Image::ImageType::IT_BITMAP)
		{
			return img.dxtHeader.textureSize;
		}
		switch (img.header.bitCount)
		{
		case 8:
			return 256 * 4 + img.header.width * img.header.height;
		case 16:
			return img.header.width * img.header.height * 2;
		case 24:
			return img.header.width * img.header.height * 3;
		case 32:
			return img.header.width * img.header.height * 4;
		default:
			return 0;
		}
	}

	bool IsImageBlockEqual(const BlockFile::ImageBlock *a, const BlockFile::ImageBlock *b)
	{
		if (memcmp(&a->image.header, &b->image.header, sizeof(ImageHeader)) != 0) return false;
		if (memcmp(&a->image.dxtHeader, &b->image.dxtHeader, sizeof(DxtImageHeader)) != 0) return false;
		size_t sa = GetImageTextureSize(a->image);
		size_t sb = GetImageTextureSize(b->image);
		if (sa != sb) return false;
		if (sa == 0) return true;
		if (!a->image.texture || !b->image.texture) return false;
		return memcmp(a->image.texture.get(), b->image.texture.get(), sa) == 0;
	}

	bool IsImageSetBlockEqual(const ImageSetBlock *a, const ImageSetBlock *b)
	{
		if (a->group != b->group || a->name != b->name) return false;
		if (a->refTextures != b->refTextures) return false;
		if (a->groups.size() != b->groups.size()) return false;
		for (size_t i = 0; i < a->groups.size(); ++i)
		{
			const auto &ga = a->groups[i].imageRefs;
			const auto &gb = b->groups[i].imageRefs;
			if (ga.size() != gb.size()) return false;
			for (size_t j = 0; j < ga.size(); ++j)
			{
				if (memcmp(&ga[j], &gb[j], sizeof(ImageSetBlock::ImageGroup::ImageRef)) != 0) return false;
			}
		}
		return true;
	}

	bool IsMenuBlockEqual(const BlockFile::MenuBlock *a, const BlockFile::MenuBlock *b)
	{
		if (a->data.size() != b->data.size()) return false;
		for (size_t i = 0; i < a->data.size(); ++i)
		{
			if (!a->data[i] || !b->data[i]) return false;
			uint16_t sa = *(uint16_t *)a->data[i].get();
			uint16_t sb = *(uint16_t *)b->data[i].get();
			if (sa != sb) return false;
			if (memcmp(a->data[i].get(), b->data[i].get(), sa) != 0) return false;
		}
		return true;
	}

	bool IsUnknownBlockEqual(const BlockFile::UnknownBlock *a, const BlockFile::UnknownBlock *b)
	{
		size_t sa = a->blockHeader.size * 16 - 16;
		size_t sb = b->blockHeader.size * 16 - 16;
		if (sa != sb) return false;
		if (sa == 0) return true;
		if (!a->GetRawData() || !b->GetRawData()) return false;
		return memcmp(a->GetRawData(), b->GetRawData(), sa) == 0;
	}

	bool IsBlockEqual(const Block *a, const Block *b)
	{
		if (!a || !b) return false;
		if (a->blockHeader.type != b->blockHeader.type) return false;
		if (memcmp(a->blockHeader.name, b->blockHeader.name, sizeof(a->blockHeader.name)) != 0) return false;

		if (dynamic_cast<const BlockFile::EmptyBlock *>(a) && dynamic_cast<const BlockFile::EmptyBlock *>(b)) return true;
		if (auto ia = dynamic_cast<const BlockFile::ImageBlock *>(a))
		{
			auto ib = dynamic_cast<const BlockFile::ImageBlock *>(b);
			return ib != nullptr && IsImageBlockEqual(ia, ib);
		}
		if (auto sa = dynamic_cast<const ImageSetBlock *>(a))
		{
			auto sb = dynamic_cast<const ImageSetBlock *>(b);
			return sb != nullptr && IsImageSetBlockEqual(sa, sb);
		}
		if (auto ma = dynamic_cast<const BlockFile::MenuBlock *>(a))
		{
			auto mb = dynamic_cast<const BlockFile::MenuBlock *>(b);
			return mb != nullptr && IsMenuBlockEqual(ma, mb);
		}
		if (auto ua = dynamic_cast<const BlockFile::UnknownBlock *>(a))
		{
			auto ub = dynamic_cast<const BlockFile::UnknownBlock *>(b);
			return ub != nullptr && IsUnknownBlockEqual(ua, ub);
		}

		return false;
	}

	Block *CreateMergedImageSetBlock(const ImageSetBlock *a, const ImageSetBlock *b)
	{
		if (!a || !b) return nullptr;
		ImageSetBlock *ret = dynamic_cast<ImageSetBlock *>(a->Clone());
		if (!ret) return nullptr;

		size_t overlap = (std::min)(ret->groups.size(), b->groups.size());
		for (size_t i = 0; i < overlap; ++i)
		{
			ret->groups[i] = b->groups[i];
		}

		return ret;
	}

	CString BuildTypeString(Block *blk)
	{
		if (!blk) return _T("-");
		CString s;
		s.Format(_T("%u"), blk->blockHeader.type);
		return s;
	}

	CString BuildBlockNameString(Block *blk)
	{
		if (!blk) return _T("-");
		char nameBuf[5] = { 0 };
		memcpy(nameBuf, blk->blockHeader.name, 4);
		for (int i = 3; i >= 0; --i)
		{
			if (nameBuf[i] == ' ' || nameBuf[i] == '\0') nameBuf[i] = '\0';
			else break;
		}
		if (nameBuf[0] == '\0')
		{
			return _T("(空)");
		}
		return CString(nameBuf);
	}

	CString BuildGroupStatusString(Block *a, Block *b)
	{
		auto *sa = dynamic_cast<ImageSetBlock *>(a);
		auto *sb = dynamic_cast<ImageSetBlock *>(b);
		if (!sa && !sb) return _T("-");

		CString gs;
		gs.Format(_T("A:%u / B:%u"),
			sa ? (unsigned)sa->groups.size() : 0,
			sb ? (unsigned)sb->groups.size() : 0);
		return gs;
	}
}

void CFFXIMenuApp::OnFileMerge()
{
	CFrameWnd *pFrame = dynamic_cast<CFrameWnd *>(AfxGetMainWnd());
	if (!pFrame)
	{
		AfxMessageBox(_T("无法获取主窗口。"));
		return;
	}
	CFFXIMenuDoc *pDoc = dynamic_cast<CFFXIMenuDoc *>(pFrame->GetActiveDocument());
	if (!pDoc || !pDoc->blockFileInstance)
	{
		AfxMessageBox(_T("当前没有打开的源文件。"));
		return;
	}

	CString pathA(pDoc->blockFileInstance->path.wstring().c_str());

	// 选择文件B
	CFileDialog dlgOpen(TRUE, _T("DAT"), nullptr,
		OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
		_T("FFXI Dat 文件 (*.DAT)|*.DAT|所有文件 (*.*)|*.*||"),
		AfxGetMainWnd());
	dlgOpen.m_ofn.lpstrTitle = _T("选择要合并的另一个文件(B)");
	if (dlgOpen.DoModal() != IDOK)
		return;

	CString pathB = dlgOpen.GetPathName();

	// 读取B
	BlockFile fileB(std::filesystem::path(pathB.GetString()));
	try
	{
		fileB.Read();
	}
	catch (...)
	{
		AfxMessageBox(_T("读取文件B失败。"));
		return;
	}

	BlockFile *fileA = pDoc->blockFileInstance;

	std::vector<MergeEntry> entries;
	std::vector<CMergeDialog::Item> dialogItems;

	std::vector<int> matchedBForA(fileA->blocks.size(), -1);
	std::vector<BOOL> matchedB(fileB.blocks.size(), FALSE);

	auto getNameKey = [](Block *blk) -> std::string {
		if (!blk) return std::string();
		return std::string(blk->blockHeader.name, 4);
	};

	for (size_t ia = 0; ia < fileA->blocks.size(); ++ia)
	{
		Block *blkA = fileA->blocks[ia];
		std::string key = getNameKey(blkA);

		int best = -1;
		for (size_t ib = 0; ib < fileB.blocks.size(); ++ib)
		{
			if (matchedB[ib]) continue;
			Block *blkB = fileB.blocks[ib];
			if (getNameKey(blkB) != key) continue;
			if ((uint32_t)blkA->blockHeader.type == (uint32_t)blkB->blockHeader.type)
			{
				best = (int)ib;
				break;
			}
			if (best < 0) best = (int)ib;
		}

		if (best >= 0)
		{
			matchedBForA[ia] = best;
			matchedB[best] = TRUE;
		}
	}

	for (size_t ia = 0; ia < fileA->blocks.size(); ++ia)
	{
		Block *blkA = fileA->blocks[ia];
		int ib = matchedBForA[ia];
		Block *blkB = ib >= 0 ? fileB.blocks[ib] : nullptr;

		bool same = (blkA && blkB) ? IsBlockEqual(blkA, blkB) : false;
		if (same) continue;

		MergeEntry entry;
		entry.indexA = (int)ia;
		entry.indexB = ib;
		entry.a = blkA;
		entry.b = blkB;
		entry.same = false;
		entry.selectedUseB = false;

		if (blkA && blkB)
		{
			entry.status = (blkA->blockHeader.type == blkB->blockHeader.type) ? _T("不同") : _T("类型不同");
		}
		else
		{
			entry.status = _T("仅A存在");
		}

		CString blkName = BuildBlockNameString(blkA ? blkA : blkB);
		CString grpStatus = BuildGroupStatusString(blkA, blkB);
		entry.detail.Format(_T("A索引=%d | B索引=%d\r\n块名=%s\r\nA类型=%s | B类型=%s\r\n组状态=%s\r\n状态=%s\r\n\r\n勾选含义：应用文件B。\r\n类型49(IMAGE_SET)勾选后：0~min(A,B)-1组使用B，A剩余组保留。"),
			entry.indexA, entry.indexB, blkName.GetString(), BuildTypeString(blkA).GetString(), BuildTypeString(blkB).GetString(), grpStatus.GetString(), entry.status.GetString());

		entries.push_back(entry);

		CMergeDialog::Item it;
		it.index = (int)ia;
		it.indexText.Format(_T("A:%d / B:%d"), entry.indexA, entry.indexB);
		it.blockName = blkName;
		it.typeA = blkA ? blkA->blockHeader.type : 0;
		it.typeB = blkB ? blkB->blockHeader.type : 0;
		it.groupStatus = grpStatus;
		it.status = entry.status;
		it.detail = entry.detail;
		it.selectable = blkB != nullptr;
		it.selected = FALSE;
		dialogItems.push_back(it);
	}

	for (size_t ib = 0; ib < fileB.blocks.size(); ++ib)
	{
		if (matchedB[ib]) continue;
		Block *blkB = fileB.blocks[ib];

		MergeEntry entry;
		entry.indexA = -1;
		entry.indexB = (int)ib;
		entry.a = nullptr;
		entry.b = blkB;
		entry.same = false;
		entry.selectedUseB = false;
		entry.status = _T("仅B存在");

		CString blkName = BuildBlockNameString(blkB);
		CString grpStatus = BuildGroupStatusString(nullptr, blkB);
		entry.detail.Format(_T("A索引=-1 | B索引=%d\r\n块名=%s\r\nA类型=%s | B类型=%s\r\n组状态=%s\r\n状态=%s\r\n\r\n勾选含义：将该B块追加到输出C。"),
			entry.indexB, blkName.GetString(), BuildTypeString(nullptr).GetString(), BuildTypeString(blkB).GetString(), grpStatus.GetString(), entry.status.GetString());

		entries.push_back(entry);

		CMergeDialog::Item it;
		it.index = (int)ib;
		it.indexText.Format(_T("A:- / B:%d"), entry.indexB);
		it.blockName = blkName;
		it.typeA = 0;
		it.typeB = blkB->blockHeader.type;
		it.groupStatus = grpStatus;
		it.status = entry.status;
		it.detail = entry.detail;
		it.selectable = TRUE;
		it.selected = FALSE;
		dialogItems.push_back(it);
	}

	if (dialogItems.empty())
	{
		AfxMessageBox(_T("A与B没有可合并差异。"));
		return;
	}

	CMergeDialog dlg(std::move(dialogItems), AfxGetMainWnd());
	if (dlg.DoModal() != IDOK)
		return;

	const auto &chosen = dlg.GetItems();
	for (size_t i = 0; i < entries.size() && i < chosen.size(); ++i)
	{
		entries[i].selectedUseB = chosen[i].selected != FALSE;
	}

	std::vector<BOOL> diffOnA(fileA->blocks.size(), FALSE);
	std::vector<BOOL> useBOnA(fileA->blocks.size(), FALSE);
	std::vector<BOOL> appendB(fileB.blocks.size(), FALSE);
	for (const auto &e : entries)
	{
		if (e.indexA >= 0)
		{
			diffOnA[e.indexA] = TRUE;
			useBOnA[e.indexA] = e.selectedUseB ? TRUE : FALSE;
		}
		else if (e.indexB >= 0)
		{
			appendB[e.indexB] = e.selectedUseB ? TRUE : FALSE;
		}
	}

	BlockFile fileC(pathA.GetString());
	fileC.header = fileA->header;
	fileC.type = fileA->type;

	for (size_t i = 0; i < fileA->blocks.size(); ++i)
	{
		Block *blkA = fileA->blocks[i];
		int ib = matchedBForA[i];
		Block *blkB = ib >= 0 ? fileB.blocks[ib] : nullptr;

		bool hasDiff = diffOnA[i] != FALSE;
		bool useB = useBOnA[i] != FALSE;

		if (!hasDiff)
		{
			if (blkA) fileC.blocks.push_back(blkA->Clone());
			continue;
		}

		if (blkA && blkB)
		{
			if (!useB)
			{
				fileC.blocks.push_back(blkA->Clone());
			}
			else if (blkA->blockHeader.type == (uint16_t)BlockType::BT_IMAGE_SET &&
				blkB->blockHeader.type == (uint16_t)BlockType::BT_IMAGE_SET)
			{
				auto *sa = dynamic_cast<ImageSetBlock *>(blkA);
				auto *sb = dynamic_cast<ImageSetBlock *>(blkB);
				Block *merged = CreateMergedImageSetBlock(sa, sb);
				fileC.blocks.push_back(merged ? merged : blkA->Clone());
			}
			else
			{
				fileC.blocks.push_back(blkB->Clone());
			}
		}
		else if (blkA)
		{
			fileC.blocks.push_back(blkA->Clone());
		}
	}

	for (size_t ib = 0; ib < fileB.blocks.size(); ++ib)
	{
		if (matchedB[ib]) continue;
		if (appendB[ib])
		{
			fileC.blocks.push_back(fileB.blocks[ib]->Clone());
		}
	}

	// 日志缓冲
	CString logText;
	logText.AppendFormat(_T("源文件A: %s\r\n"), pathA.GetString());
	logText.AppendFormat(_T("源文件B: %s\r\n"), pathB.GetString());
	for (const auto &e : entries)
	{
		CString line;
		if (e.a && e.b)
		{
			CString blkName = BuildBlockNameString(e.a ? e.a : e.b);
			CString grpStatus = BuildGroupStatusString(e.a, e.b);
			if (e.a->blockHeader.type == (uint16_t)BlockType::BT_IMAGE_SET &&
				e.b->blockHeader.type == (uint16_t)BlockType::BT_IMAGE_SET)
			{
				auto *sa = dynamic_cast<ImageSetBlock *>(e.a);
				auto *sb = dynamic_cast<ImageSetBlock *>(e.b);
				line.Format(_T("块 A:%d / B:%d (%s): 类型49(IMAGE_SET) 不同。A组数=%u, B组数=%u, 组状态=%s, 应用B=%s"),
					e.indexA,
					e.indexB,
					blkName.GetString(),
					sa ? (unsigned)sa->groups.size() : 0,
					sb ? (unsigned)sb->groups.size() : 0,
					grpStatus.GetString(),
					e.selectedUseB ? _T("是") : _T("否"));
			}
			else
			{
				line.Format(_T("块 A:%d / B:%d (%s): A类型=%u, B类型=%u, 组状态=%s, 应用B=%s"),
					e.indexA,
					e.indexB,
					blkName.GetString(),
					e.a->blockHeader.type,
					e.b->blockHeader.type,
					grpStatus.GetString(),
					e.selectedUseB ? _T("是") : _T("否"));
			}
		}
		else if (e.a)
		{
			line.Format(_T("块 A:%d / B:- (%s): 仅存在于A，类型%u，保持A。"),
				e.indexA,
				BuildBlockNameString(e.a).GetString(),
				e.a->blockHeader.type);
		}
		else
		{
			line.Format(_T("块 A:- / B:%d (%s): 仅存在于B，类型%u, 应用B=%s"),
				e.indexB,
				BuildBlockNameString(e.b).GetString(),
				e.b->blockHeader.type,
				e.selectedUseB ? _T("是") : _T("否"));
		}
		logText.Append(line + _T("\r\n"));
	}

	// 选择输出文件C
	CFileDialog dlgSave(FALSE, _T("DAT"), nullptr,
		OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
		_T("FFXI Dat 文件 (*.DAT)|*.DAT|所有文件 (*.*)|*.*||"),
		AfxGetMainWnd());
	dlgSave.m_ofn.lpstrTitle = _T("选择合并后输出文件(C)");
	if (dlgSave.DoModal() != IDOK)
		return;

	CString pathC = dlgSave.GetPathName();
	fileC.path = std::filesystem::path(pathC.GetString());

	try
	{
		fileC.Write();
	}
	catch (...)
	{
		AfxMessageBox(_T("写入文件C失败。"));
		return;
	}

	// 写日志
	CString pathLog = pathC + _T(".log");
	try
	{
		CStdioFile logFile(pathLog, CFile::modeCreate | CFile::modeWrite | CFile::typeText);
		logFile.WriteString(logText);
		logFile.Close();
	}
	catch (...)
	{
		// 忽略日志写失败
	}

	AfxMessageBox(_T("合并完成。"));
}

// CFFXIMenuApp 自定义加载/保存方法

void CFFXIMenuApp::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
	bNameValid = strName.LoadString(IDS_EXPLORER);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EXPLORER);
	GetContextMenuManager()->AddMenu(_T("纹理菜单"), IDR_POPUP_TEXTURE);
	GetContextMenuManager()->AddMenu(_T("剪贴画菜单"), IDR_POPUP_CLIP);
}

void CFFXIMenuApp::LoadCustomState()
{
}

void CFFXIMenuApp::SaveCustomState()
{
}

// CFFXIMenuApp 消息处理程序







