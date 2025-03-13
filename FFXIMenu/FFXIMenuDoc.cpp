
// FFXIMenuDoc.cpp: CFFXIMenuDoc 类的实现
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "FFXIMenu.h"
#endif
#include "MainFrm.h"
#include "BlockAdapter.h"

#include "OutputWnd.h"
#include "xystring.h"
#include "FFXIMenuDoc.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CFFXIMenuDoc

IMPLEMENT_DYNCREATE(CFFXIMenuDoc, CDocument)

BEGIN_MESSAGE_MAP(CFFXIMenuDoc, CDocument)
END_MESSAGE_MAP()


// CFFXIMenuDoc 构造/析构

CFFXIMenuDoc::CFFXIMenuDoc() noexcept
{
	blockFileInstance = nullptr;
	rootNode = nullptr;
	focusNode = nullptr;
}

CFFXIMenuDoc::~CFFXIMenuDoc()
{
}

// TODO: 以下绘图相关迁移到View
Gdiplus::Bitmap *CFFXIMenuDoc::GetActivatedBitmap()
{
	if (TileNode *node = dynamic_cast<TileNode *>(focusNode))
	{
		if (focusNode->GetParent() && focusNode->GetParent()->GetBitmap())
			return focusNode->GetParent()->GetBitmap();
	}

	if (!focusNode || !focusNode->GetBitmap()) return GetOrCreateNoImageBitmap();

	return focusNode->GetBitmap();
}

BOOL CFFXIMenuDoc::HasBoundary()
{
	if (focusNode)
	{
		if (dynamic_cast<ClipNode *>(focusNode))
			return TRUE;
		if (dynamic_cast<TileNode *>(focusNode))
			return TRUE;
	}
	return FALSE;
}

CPoint CFFXIMenuDoc::GetVertex0()
{
	if (focusNode)
	{
		if (ClipNode *cn = dynamic_cast<ClipNode *>(focusNode))
		{
			auto xy = cn->GetXY();
			auto wh = cn->GetWH();
			return CPoint(xy.x, xy.y);
		}
		if (TileNode *tn = dynamic_cast<TileNode *>(focusNode))
		{
			auto p = tn->GetVertex0();

			int w = GetActivatedBitmap()->GetWidth();
			int h = GetActivatedBitmap()->GetHeight();
			return CPoint(p.x + w/2, p.y + h/2);
		}
	}

	return CPoint();
}

CPoint CFFXIMenuDoc::GetVertex1()
{
	if (focusNode)
	{
		if (ClipNode *cn = dynamic_cast<ClipNode *>(focusNode))
		{
			auto xy = cn->GetXY();
			auto wh = cn->GetWH();
			return CPoint(xy.x + wh.x, xy.y);
		}
		if (TileNode *tn = dynamic_cast<TileNode *>(focusNode))
		{
			auto p = tn->GetVertex1();

			int w = GetActivatedBitmap()->GetWidth();
			int h = GetActivatedBitmap()->GetHeight();
			return CPoint(p.x + w / 2, p.y + h / 2);
		}
	}

	return CPoint();
}

CPoint CFFXIMenuDoc::GetVertex2()
{
	if (focusNode)
	{
		if (ClipNode *cn = dynamic_cast<ClipNode *>(focusNode))
		{
			auto xy = cn->GetXY();
			auto wh = cn->GetWH();
			return CPoint(xy.x + wh.x, xy.y + wh.y);
		}
		if (TileNode *tn = dynamic_cast<TileNode *>(focusNode))
		{
			auto p = tn->GetVertex3();

			int w = GetActivatedBitmap()->GetWidth();
			int h = GetActivatedBitmap()->GetHeight();
			return CPoint(p.x + w / 2, p.y + h / 2);
		}
	}

	return CPoint();
}

CPoint CFFXIMenuDoc::GetVertex3()
{
	if (focusNode)
	{
		if (ClipNode *cn = dynamic_cast<ClipNode *>(focusNode))
		{
			auto xy = cn->GetXY();
			auto wh = cn->GetWH();
			return CPoint(xy.x, xy.y + wh.y);
		}
		if (TileNode *tn = dynamic_cast<TileNode *>(focusNode))
		{
			auto p = tn->GetVertex2();

			int w = GetActivatedBitmap()->GetWidth();
			int h = GetActivatedBitmap()->GetHeight();
			return CPoint(p.x + w / 2, p.y + h / 2);
		}
	}

	return CPoint();
}

void CFFXIMenuDoc::SetVertex0(CPoint pt)
{
	if (focusNode)
	{
		if (ClipNode *cn = dynamic_cast<ClipNode *>(focusNode))
		{
			cn->SetXY(pt);
		}
		if (TileNode *tn = dynamic_cast<TileNode *>(focusNode))
		{
			int w = GetActivatedBitmap()->GetWidth();
			int h = GetActivatedBitmap()->GetHeight();
			tn->SetVertex0({ (int16_t)(pt.x - w / 2), (int16_t)(pt.y - h / 2) });
		}
	}

	UpdateAllViews(NULL, HINT_PROPERTIES_CHANGED);
	if (CMainFrame *frm = dynamic_cast<CMainFrame *>(AfxGetMainWnd()))
	{
		frm->SendMessage(WM_PROPERTIES_CHANGE_MSG);
	}
	m_bModified = TRUE;
}

void CFFXIMenuDoc::SetVertex1(CPoint pt)
{
	if (focusNode)
	{
		if (ClipNode *cn = dynamic_cast<ClipNode *>(focusNode))
		{
			auto p = cn->GetXY();
			cn->SetWH({ (int16_t)pt.x - p.x, cn->GetWH().y});
		}
		if (TileNode *tn = dynamic_cast<TileNode *>(focusNode))
		{
			int w = GetActivatedBitmap()->GetWidth();
			int h = GetActivatedBitmap()->GetHeight();
			tn->SetVertex1({ (int16_t)(pt.x - w / 2), (int16_t)(pt.y - h / 2) });
		}
	}

	UpdateAllViews(NULL, HINT_PROPERTIES_CHANGED);
	if (CMainFrame *frm = dynamic_cast<CMainFrame *>(AfxGetMainWnd()))
	{
		frm->SendMessage(WM_PROPERTIES_CHANGE_MSG);
	}
	m_bModified = TRUE;
}

void CFFXIMenuDoc::SetVertex2(CPoint pt)
{
	if (focusNode)
	{
		if (ClipNode *cn = dynamic_cast<ClipNode *>(focusNode))
		{
			auto p = cn->GetXY();
			cn->SetWH({pt.x - p.x, pt.y - p.y});
		}
		if (TileNode *tn = dynamic_cast<TileNode *>(focusNode))
		{
			int w = GetActivatedBitmap()->GetWidth();
			int h = GetActivatedBitmap()->GetHeight();
			tn->SetVertex3({ (int16_t)(pt.x - w / 2), (int16_t)(pt.y - h / 2) });
		}
	}

	UpdateAllViews(NULL, HINT_PROPERTIES_CHANGED);
	if (CMainFrame *frm = dynamic_cast<CMainFrame *>(AfxGetMainWnd()))
	{
		frm->SendMessage(WM_PROPERTIES_CHANGE_MSG);
	}
	m_bModified = TRUE;
}

void CFFXIMenuDoc::SetVertex3(CPoint pt)
{
	if (focusNode)
	{
		if (ClipNode *cn = dynamic_cast<ClipNode *>(focusNode))
		{
			auto p = cn->GetXY();
			cn->SetWH({ cn->GetWH().x, pt.y - p.y });
		}
		if (TileNode *tn = dynamic_cast<TileNode *>(focusNode))
		{
			int w = GetActivatedBitmap()->GetWidth();
			int h = GetActivatedBitmap()->GetHeight();
			tn->SetVertex2({ (int16_t)(pt.x - w / 2), (int16_t)(pt.y - h / 2) });
		}
	}

	UpdateAllViews(NULL, HINT_PROPERTIES_CHANGED);
	if (CMainFrame *frm = dynamic_cast<CMainFrame *>(AfxGetMainWnd()))
	{
		frm->SendMessage(WM_PROPERTIES_CHANGE_MSG);
	}
	m_bModified = TRUE;
}

Gdiplus::Bitmap *CFFXIMenuDoc::QueryTexture(CString group, CString name)
{
	char buf[16];
	memset(buf, ' ', 16);
	auto g = xybase::string::to_string(group.GetString());
	auto n = xybase::string::to_string(name.GetString());
	memcpy(buf, g.c_str(), g.size());
	memcpy(buf + 8, n.c_str(), n.size());

	return QueryTexture(buf);
}

Gdiplus::Bitmap *CFFXIMenuDoc::QueryTexture(const char name[16])
{
	char buffer[16];
	for (int i = 0; i < 16; ++i)
	{
		buffer[i] = (name[i] >= 'A' && name[i] <= 'Z') ? name[i] - 'A' + 'a' : name[i];
	}
	for (auto n : rootNode->GetChildren())
	{
		if (ImageBlockNode *ib = dynamic_cast<ImageBlockNode *>(n))
		{
			if (memcmp(ib->GetTextureId(), buffer, 16) == 0)
			{
				return ib->GetBitmap();
			}
		}
	}
	if (auto pair = m_extraTextures.PLookup(CString(buffer, 16)))
	{
		if (CMainFrame *frm = dynamic_cast<CMainFrame *>(AfxGetMainWnd()))
		{
			CString msg;
			CString texName(name, 16);
			msg.Format(_T("获取了外部纹理 %s。"), texName.GetString());
			frm->OutputLog(msg.GetString());
		}
		return pair->value;
	}

	if (CMainFrame *frm = dynamic_cast<CMainFrame *>(AfxGetMainWnd()))
	{
		CString msg;
		CString texName(name, 16);
		msg.Format(_T("未能加载指定的纹理 %s，请尝试文件(F)->加载外部纹理(T)。"), texName.GetString());
		frm->OutputLog(msg.GetString(), 1);
	}

	return nullptr;
}

Gdiplus::Bitmap *CFFXIMenuDoc::QueryTexture(CString id)
{
	return QueryTexture(xybase::string::to_string(id.GetString()).c_str());
}

void CFFXIMenuDoc::LoadExtraTexture(LPCTSTR path)
{
	BlockFile tmp(path);
	tmp.Read();
	for (auto blk : tmp.blocks)
	{
		if (BlockFile::ImageBlock *ib = dynamic_cast<BlockFile::ImageBlock *>(blk))
		{
			int h = ib->image.header.height;
			int w = ib->image.header.width;

			std::unique_ptr<char[]> buffer = std::make_unique<char[]>(h * w * 4);
			switch (*(int *)ib->image.dxtHeader.fourCC)
			{
			case 'DXT1':
				DecodeTexture(buffer.get(), ib->image.texture.get(), w, h, 1);
				break;
			case 'DXT3':
				DecodeTexture(buffer.get(), ib->image.texture.get(), w, h, 3);
				break;
			case 'DXT5':
				DecodeTexture(buffer.get(), ib->image.texture.get(), w, h, 5);
				break;
			}

			Gdiplus::Bitmap *bitmap = ::new Gdiplus::Bitmap(w, h, PixelFormat32bppARGB);
			Gdiplus::BitmapData bitmapData;
			Gdiplus::Rect rect(0, 0, w, h);
			bitmap->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bitmapData);

			memcpy(bitmapData.Scan0, buffer.get(), w * h * 4);
			bitmap->UnlockBits(&bitmapData);

			m_extraTextures.SetAt(CString(ib->image.header.group, 16), bitmap);
		}
	}
}

void CFFXIMenuDoc::PropertyUpdate()
{
	if (!focusNode) return;

	m_bModified = TRUE;
	focusNode->UpdateData();
	// AfxGetMainWnd()->SendMessage(WM_PROPERTIES_CHANGE_MSG);
	UpdateAllViews(NULL, HINT_PROPERTIES_CHANGED);
}

void CFFXIMenuDoc::FocusOn(ContentNode *node)
{
	// sanity check
	if (!HasNode(node, rootNode)) return;

	Blur();

	focusNode = node;

	AfxGetMainWnd()->SendMessage(WM_NODE_FOCUS_CHANGE_MSG);
}

void CFFXIMenuDoc::Blur()
{
	if (!focusNode) return;

	focusNode->OnBlur();

	focusNode = nullptr;

	AfxGetMainWnd()->SendMessage(WM_NODE_FOCUS_CHANGE_MSG);
}

void CFFXIMenuDoc::FlushNodes()
{
	rootNode->Flush();
}

BOOL CFFXIMenuDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	if (blockFileInstance) delete blockFileInstance;
	blockFileInstance = nullptr;
	if (rootNode) delete rootNode;
	rootNode = nullptr;

	return TRUE;
}




// CFFXIMenuDoc 序列化

void CFFXIMenuDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		if (blockFileInstance)
		{
			blockFileInstance->path = ar.GetFile()->GetFilePath().GetString();
			blockFileInstance->Write();
		}
	}
	else
	{
		
	}
}

BOOL CFFXIMenuDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName)) // 必须调用基类
		return FALSE;

	// 先清空旧数据
	DeleteContents();

	// 使用智能指针保证异常安全
	auto tempBlock = std::make_unique<BlockFile>(lpszPathName);

	try {
		tempBlock->Read(); // 直接读取文件
	}
	catch (...) {
		AfxMessageBox(_T("文件读取失败"));
		return FALSE;
	}
	auto tempRoot = std::make_unique<FileNode>(tempBlock.get(), this);

	// 原子化替换资源
	blockFileInstance = tempBlock.release();
	rootNode = tempRoot.release();
	focusNode = nullptr;

	// 通知视图更新
	UpdateAllViews(NULL, HINT_FILE_CHANGED);
	AfxGetMainWnd()->SendMessage(WM_TREE_UPDATE_MSG);
	return TRUE;
}

BOOL CFFXIMenuDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	if (!blockFileInstance) return FALSE;

	try {
		// 先保存到临时文件
		CString tempPath = lpszPathName + CString(".tmp");

		// 执行实际写入
		blockFileInstance->path = tempPath.GetString();
		blockFileInstance->Write();

		// 替换原始文件
		if (!ReplaceFile(lpszPathName, tempPath, nullptr,
			REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr))
		{
			if (!MoveFile(tempPath, lpszPathName))
				throw std::runtime_error("文件替换失败");
		}
	}
	catch (...) {
		AfxMessageBox(_T("文件保存失败"));
		return FALSE;
	}

	SetModifiedFlag(FALSE);
}

void CFFXIMenuDoc::DeleteContents()
{
	// 安全释放资源
	delete blockFileInstance;
	delete rootNode;
	blockFileInstance = nullptr;
	rootNode = nullptr;
	focusNode = nullptr;
}


BOOL CFFXIMenuDoc::HasNode(ContentNode *node, ContentNode *travNode)
{
	if (node == travNode) return TRUE;

	for (ContentNode *child : travNode->GetChildren())
	{
		if (HasNode(node, child)) return TRUE;
	}

	return FALSE;
}

#ifdef SHARED_HANDLERS

// 缩略图的支持
void CFFXIMenuDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// 修改此代码以绘制文档数据
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

// 搜索处理程序的支持
void CFFXIMenuDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// 从文档数据设置搜索内容。
	// 内容部分应由“;”分隔

	// 例如:     strSearchContent = _T("point;rectangle;circle;ole object;")；
	SetSearchContent(strSearchContent);
}

void CFFXIMenuDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = nullptr;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != nullptr)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

Gdiplus::Bitmap *CFFXIMenuDoc::GetOrCreateNoImageBitmap()
{
	if (m_noImageCache != nullptr) {
		return m_noImageCache;
	}

	// 创建一个新的 64x64 图像（RGBA格式）
	Gdiplus::Bitmap *pBitmap = ::new Gdiplus::Bitmap(256, 256, PixelFormat32bppARGB);

	// 创建图形对象来绘制图像
	Gdiplus::Graphics graphics(pBitmap);
	graphics.Clear(Gdiplus::Color::Transparent);

	Gdiplus::FontFamily fontFamily(L"Arial");
	Gdiplus::REAL fontSize = 36.0f;
	Gdiplus::Font font(&fontFamily, fontSize, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);

	// 2. 设置颜色
	Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 0, 0, 255)); // 蓝色文本

	// 3. 设置对齐方式（居中）
	Gdiplus::StringFormat stringFormat;
	stringFormat.SetAlignment(Gdiplus::StringAlignmentCenter);
	stringFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);

	// 4. 开启抗锯齿
	graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	const WCHAR *text = L"No Image";
	Gdiplus::RectF layoutRect(
		-(pBitmap->GetWidth() / 2.0f),  // 左上角X
		-(pBitmap->GetHeight() / 2.0f), // 左上角Y
		pBitmap->GetWidth(),          // 宽度
		pBitmap->GetHeight()          // 高度
	);

	// 6. 平移坐标系到画布中心
	graphics.TranslateTransform(
		pBitmap->GetWidth() / 2.0f,
		pBitmap->GetHeight() / 2.0f
	);

	// 7. 绘制文本
	graphics.DrawString(
		text,
		-1,
		&font,
		layoutRect,
		&stringFormat,
		&textBrush
	);

	// 将创建的图像缓存到 m_noImageCache 中
	m_noImageCache = pBitmap;

	return pBitmap;
}

// CFFXIMenuDoc 诊断

#ifdef _DEBUG
void CFFXIMenuDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CFFXIMenuDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CFFXIMenuDoc 命令
