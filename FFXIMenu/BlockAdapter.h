#pragma once

#include "pch.h"
#include <xystring.h>
#include <BlockFile.h>
#include <gdiplus.h>

class FileNode;

// ContentNode 基类
class ContentNode {
public:
	virtual CString GetName() const = 0;
	virtual int GetIconId() const { return 0; }
	virtual ContentNode *GetParent() const { return parent; }
	virtual bool HasProperties() const { return false; }
	virtual void GetProperties(CMFCPropertyGridCtrl &grid) {}
	virtual void UpdateData() {} // 从界面更新数据
	virtual void OnBlur() {} // 
	virtual bool HasBitmap() { return false; }
	virtual Gdiplus::Bitmap *GetBitmap() { return NULL; }
	virtual void Flush()
	{
		for (ContentNode *child : children)
		{
			child->Flush();
		}
	}

	virtual void DeleteChild(size_t index);
	virtual void InsertChildAfter(size_t index, ContentNode *child);

	std::vector<ContentNode *> GetChildren() { return children; }

	virtual ~ContentNode()
	{
		for (ContentNode *child : children)
		{
			delete child;
		}
	}

protected:
	ContentNode *parent = nullptr;
	FileNode *GetRootNode() { return parent ? parent->GetRootNode() : (FileNode *)this; }
	std::vector<ContentNode *> children;
};

void DecodeTexture(char *dst, const char *src, int w, int h, int type);

// 单个裁剪属性节点
class ClipNode : public ContentNode {
public:
	virtual int GetIconId() const { return 4; }
	ClipNode(ContentNode *parent,
		BlockFile::ImageSetBlock::ImageGroup::ImageRef *ref,
		int index)
		: imageRef(ref), m_index(index), m_propGrid(NULL) {
		this->parent = parent;
	}

	CString GetName() const override {
		CString ret;
		ret.Format(_T("裁切 %d"), m_index);
		return ret;
	}

	bool HasProperties() const override { return true; }

	void GetProperties(CMFCPropertyGridCtrl &grid) override;

	void UpdateData() override;

	virtual void OnBlur() override { m_propGrid = NULL; }

	virtual bool HasBitmap() override { return true; }
	virtual Gdiplus::Bitmap *GetBitmap() override;

	const char *GetTexRef() { return imageRef->group; }

	const xybase::Vec2<int16_t> GetXY() const { return xybase::Vec2<int16_t>(imageRef->x, imageRef->y); }
	void SetXY(CPoint pt);
	const xybase::Vec2<int16_t> GetWH() const { return xybase::Vec2<int16_t>(imageRef->w, imageRef->h); }
	void SetWH(CPoint pt);

	void DeleteNode();

	int GetIndex() const { return m_index; }
	// 使用上特别注意
	void SetIndex(int index) { m_index = index; }

	CString GetIni() const;
	void SetIni(const CString &str) const;

private:
	BlockFile::ImageSetBlock::ImageGroup::ImageRef *imageRef;
	CMFCPropertyGridCtrl *m_propGrid;
	int m_index;

	// 创建带校验的纹理名称属性
	CMFCPropertyGridProperty *CreateTextureNameProperty(LPCTSTR name, const char(&field)[8]);

	// 更新纹理名称字段
	void UpdateTextureName(CMFCPropertyGridProperty *prop, char(&field)[8]);

	// 带范围限制的属性创建
	CMFCPropertyGridProperty *CreateBoundedProperty(LPCTSTR name,
		uint16_t value,
		int minVal,
		int maxVal,
		LPCTSTR desc);

	// 安全获取属性值
	template<typename T>
	T GetSubPropValue(CMFCPropertyGridProperty *parent,
		int idx,
		T defaultValue)
	{
		if (CMFCPropertyGridProperty *prop = parent->GetSubItem(idx)) {
			return static_cast<T>(prop->GetValue().intVal);
		}
		return defaultValue;
	}
};


// 裁剪分类节点
class ClipCategoryNode : public ContentNode {
public:
	virtual int GetIconId() const { return 2; }
	ClipCategoryNode(ContentNode *parent,
		BlockFile::ImageSetBlock::ImageGroup *group)
		: group(group) {
		this->parent = parent;
		int i = 0;
		for (auto &ref : group->imageRefs) {
			children.push_back(new ClipNode(this, &ref, i++));
		}
	}

	CString GetName() const override { return _T("裁剪"); }
	void UpdateChildren();

	virtual void DeleteChild(size_t index) override;
	virtual void InsertChildAfter(size_t index, ContentNode *child) override;

	CString GetIni() const;
	void SetIni(const CString &str);

private:
	BlockFile::ImageSetBlock::ImageGroup *group;
	void ProcessIniSection(const CString &sectionName, const CString &sectionData);
};

class TileNode : public ContentNode {
public:
	virtual int GetIconId() const { return 5; }
	virtual ~TileNode() { if (m_bitmap) delete m_bitmap; }
	TileNode(ContentNode *parent,
		BlockFile::ImageSetBlock::ImageGroup::ImageRef *ref,
		int index)
		: imageRef(ref), m_index(index) {
		this->parent = parent;
	}

	CString GetName() const override {
		CString ret;
		ret.Format(_T("拼贴 %d"), m_index);
		return ret;
	}

	bool HasProperties() const override { return true; }

	void GetProperties(CMFCPropertyGridCtrl &grid) override;

	void UpdateData() override;
	virtual void Flush() override {
		m_bDirtyBitmap = TRUE;
	}

	virtual void OnBlur() override { m_propGrid = NULL; }

	// virtual bool HasBitmap() override { return true; }
	// 并非用于显示
	virtual Gdiplus::Bitmap *GetBitmap() override;

	const BlockFile::ImageSetBlock::ImageGroup::ImageRef *GetRef() const { return imageRef; }

	xybase::Vec2<int16_t> GetVertex0() const;
	xybase::Vec2<int16_t> GetVertex1() const;
	xybase::Vec2<int16_t> GetVertex2() const;
	xybase::Vec2<int16_t> GetVertex3() const;
	void SetVertex0(xybase::Vec2<int16_t> pt);
	void SetVertex1(xybase::Vec2<int16_t> pt);
	void SetVertex2(xybase::Vec2<int16_t> pt);
	void SetVertex3(xybase::Vec2<int16_t> pt);

	void DeleteNode();

	int GetIndex() const { return m_index; }
	// 使用上特别注意
	void SetIndex(int index) { m_index = index; }

	CString GetIni() const;
	void SetIni(const CString &str);

private:
	BlockFile::ImageSetBlock::ImageGroup::ImageRef *imageRef;
	CMFCPropertyGridCtrl *m_propGrid = NULL;
	Gdiplus::Bitmap *m_bitmap = NULL;
	BOOL m_bDirtyBitmap = TRUE;
	int m_index;

	void SerializeVertex(CString &strIni, LPCTSTR prefix, const xybase::Vec2<int16_t> &point, const RGBA &color) const;
	void GetVertexByPrefix(const CString &prefix, xybase::Vec2<int16_t> *&pPoint, RGBA *&pColor) const;

	void PrepareBitmap(Gdiplus::Bitmap *source);

	// 顶点属性组
	void AddVertexProperties(CMFCPropertyGridCtrl &grid,
		const xybase::Vec2<int16_t> &point,
		const RGBA &color,
		const CString &prefix,
		int data);

	// 坐标属性（带符号）
	CMFCPropertyGridProperty *CreateCoordProperty(LPCTSTR name, int16_t value);

	// 颜色分量属性
	CMFCPropertyGridProperty *CreateColorProperty(LPCTSTR name, uint8_t value);

	// 透明度属性
	CMFCPropertyGridProperty *CreateAlphaProperty(uint8_t value);

	// 更新顶点数据
	void UpdateVertexData(xybase::Vec2<int16_t> &point,
		RGBA &color,
		const CString &prefix,
		int data);
};

// 拼贴分类节点
class TileCategoryNode : public ContentNode
{
public:
	virtual int GetIconId() const { return 2; }
	virtual ~TileCategoryNode() { if (m_bitmap) delete m_bitmap; }
	TileCategoryNode(ContentNode *parent,
		BlockFile::ImageSetBlock::ImageGroup *group);
	virtual bool HasBitmap() override { return true; }
	virtual void UpdateData() override { m_bDirtyBitmap = TRUE; }
	// 在这里缓存拼好的图像
	virtual Gdiplus::Bitmap *GetBitmap() override;
	CString GetName() const override { return _T("拼贴"); }

	virtual void DeleteChild(size_t index) override;
	virtual void InsertChildAfter(size_t index, ContentNode *child) override;

	void UpdateChildren();
	virtual void Flush() override {
		ContentNode::Flush();
		m_bDirtyBitmap = TRUE;
	}

	CString GetIni() const;
	void SetIni(const CString &str);

private:
	BlockFile::ImageSetBlock::ImageGroup *group;
	Gdiplus::Bitmap *m_bitmap = NULL;
	BOOL m_bDirtyBitmap = TRUE;
	void ProcessIniSection(const CString &sectionName, const CString &sectionData);
};


// ImageGroup 节点
class ImageGroupNode : public ContentNode {
public:
	virtual int GetIconId() const { return 1; }
	ImageGroupNode(ContentNode *parent,
		BlockFile::ImageSetBlock::ImageGroup *group,
		size_t index)
		: group(group), index(index) {
		this->parent = parent;
		m_clipCate = new ClipCategoryNode(this, group);
		m_tileCate = new TileCategoryNode(this, group);
		children.push_back(m_clipCate);
		children.push_back(m_tileCate);
	}

	CString GetName() const override {
		return CString(std::to_string(index).c_str());
	}

	virtual bool HasBitmap() override { return true; }
	// 图像组显示拼贴节点（结果）的Bitmap即可
	virtual Gdiplus::Bitmap *GetBitmap() override { return children[1]->GetBitmap(); };

	void DeleteClipTileOf(int index);
	void InsertClipTileAfter(int index, const BlockFile::ImageSetBlock::ImageGroup::ImageRef &ref);

private:
	BlockFile::ImageSetBlock::ImageGroup *group;
	ClipCategoryNode *m_clipCate;
	TileCategoryNode *m_tileCate;
	size_t index;
};

// 通用块节点
class BlockNode : public ContentNode {
public:

	virtual int GetIconId() const { return 7; }

	BlockNode(ContentNode *parent, BlockFile::Block *block)
		: block(block) {
		this->parent = parent;
	}

	CString GetName() const override {
		return CString(block->blockHeader.name, 4);
	}

	bool HasProperties() const override { return true; }

	void GetProperties(CMFCPropertyGridCtrl &grid) override;

protected:
	BlockFile::Block *block;
};

// ImageSetBlock 专用节点
class ImageSetBlockNode : public BlockNode {
public:
	virtual int GetIconId() const { return 5; }
	ImageSetBlockNode(ContentNode *parent, BlockFile::ImageSetBlock *block);

	void GetProperties(CMFCPropertyGridCtrl &grid) override;

private:
	BlockFile::ImageSetBlock *imageSetBlock;

	CString FormatStringList(const std::vector<std::string> &list);
};

// 单个纹理，节点
class ImageBlockNode : public BlockNode {
public:
	virtual int GetIconId() const { return 3; }
	virtual ~ImageBlockNode() { if (bitmap) delete bitmap; }
	ImageBlockNode(ContentNode *parent, BlockFile::ImageBlock *block)
		: BlockNode(parent, block), 
		imageBlock(block), 
		bitmap(nullptr)
	{}

	void GetProperties(CMFCPropertyGridCtrl &grid) override;
	const char *GetTextureId() { return imageBlock->image.header.group; };

	virtual bool HasBitmap() override { return true; }
	virtual Gdiplus::Bitmap *GetBitmap() override;

	void DumpTexture(LPCTSTR path);
	void ImportTexture(LPCTSTR path);

private:
	BlockFile::ImageBlock *imageBlock;
	Gdiplus::Bitmap *bitmap;
	void UpdateBitmap();
};

class CFFXIMenuDoc;

// 文件根节点
class FileNode : public ContentNode {
public:
	FileNode(BlockFile *file, CFFXIMenuDoc *doc);

	CString GetName() const override {
		return CString(file->path.filename().c_str());
	}

	virtual bool HasProperties() const override { return true; }

	void GetProperties(CMFCPropertyGridCtrl &grid) override;

	CFFXIMenuDoc *GetDocument() { return doc; }
protected:
	CFFXIMenuDoc *doc;
private:
	BlockFile *file;
};
