
// FFXIMenuDoc.h: CFFXIMenuDoc 类的接口
//


#pragma once
#include "BlockAdapter.h"

#include <map>

class CFFXIMenuDoc : public CDocument
{
protected: // 仅从序列化创建
	CFFXIMenuDoc() noexcept;
	DECLARE_DYNCREATE(CFFXIMenuDoc)

// 特性
public:
	BlockFile *blockFileInstance;
	ContentNode *rootNode;

	ContentNode *focusNode;

// 操作
public:
	void FocusOn(ContentNode *node);
	void Blur();
	void FlushNodes();

	Gdiplus::Bitmap *GetActivatedBitmap();

	BOOL HasBoundary();
	CPoint GetVertex0();
	CPoint GetVertex1();
	CPoint GetVertex2();
	CPoint GetVertex3();
	void SetVertex0(CPoint pt);
	void SetVertex1(CPoint pt);
	void SetVertex2(CPoint pt);
	void SetVertex3(CPoint pt);

	Gdiplus::Bitmap *QueryTexture(CString group, CString name);
	Gdiplus::Bitmap *QueryTexture(const char[16]);
	Gdiplus::Bitmap *QueryTexture(CString id);

	void LoadExtraTexture(LPCTSTR path);

	void PropertyUpdate();

// 重写
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName) override;
	virtual void DeleteContents() override;
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// 实现
public:
	virtual ~CFFXIMenuDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	BOOL HasNode(ContentNode *node, ContentNode *travNode);

// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// 用于为搜索处理程序设置搜索内容的 Helper 函数
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
private:
	Gdiplus::Bitmap *m_noImageCache = nullptr;
	Gdiplus::Bitmap *GetOrCreateNoImageBitmap();
	CMap<CString, LPCTSTR, Gdiplus::Bitmap *, Gdiplus::Bitmap *> m_extraTextures;
};
