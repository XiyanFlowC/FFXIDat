// BlockAdapter.cpp: 实现文件
//

#include "pch.h"
#include "FFXIMenu.h"
#include "BlockAdapter.h"
#include "FFXIMenuDoc.h"
#include <algorithm>

// 辅助函数：解码 16 位颜色（RGB565）
inline uint32_t DecodeRGB565(uint16_t color) {
	uint32_t r = (color >> 11) & 0x1F;
	uint32_t g = (color >> 5) & 0x3F;
	uint32_t b = color & 0x1F;

	r = (r << 3) | (r >> 2);  // 扩展到 8 位
	g = (g << 2) | (g >> 4);  // 扩展到 8 位
	b = (b << 3) | (b >> 2);  // 扩展到 8 位

	return (r << 16) | (g << 8) | b;
}

// 解码 DXT1 纹理
void DecodeDXT1(char *dst, const char *src, int w, int h) {
	int numBlocksX = (w + 3) / 4; // 处理非4倍数的宽高
	int numBlocksY = (h + 3) / 4;

	for (int blockY = 0; blockY < numBlocksY; ++blockY) {
		for (int blockX = 0; blockX < numBlocksX; ++blockX) {
			int blockOffset = (blockY * numBlocksX + blockX) * 8;
			uint16_t color0 = *(uint16_t *)(src + blockOffset);
			uint16_t color1 = *(uint16_t *)(src + blockOffset + 2);

			// 解码RGB565到RGB888
			uint32_t r0 = ((color0 >> 11) & 0x1F) * 255 / 31;
			uint32_t g0 = ((color0 >> 5) & 0x3F) * 255 / 63;
			uint32_t b0 = (color0 & 0x1F) * 255 / 31;
			uint32_t color0Decoded = 0xFF000000 | (r0 << 16) | (g0 << 8) | b0;

			uint32_t r1 = ((color1 >> 11) & 0x1F) * 255 / 31;
			uint32_t g1 = ((color1 >> 5) & 0x3F) * 255 / 63;
			uint32_t b1 = (color1 & 0x1F) * 255 / 31;
			uint32_t color1Decoded = 0xFF000000 | (r1 << 16) | (g1 << 8) | b1;

			bool color0gt1 = color0 > color1;

			// 计算中间颜色
			uint32_t color2, color3;
			if (color0gt1) {
				// (2*color0 + color1)/3
				uint32_t r2 = (2 * r0 + r1 + 1) / 3;
				uint32_t g2 = (2 * g0 + g1 + 1) / 3;
				uint32_t b2 = (2 * b0 + b1 + 1) / 3;
				color2 = 0xFF000000 | (r2 << 16) | (g2 << 8) | b2;

				// (color0 + 2*color1)/3
				uint32_t r3 = (r0 + 2 * r1 + 1) / 3;
				uint32_t g3 = (g0 + 2 * g1 + 1) / 3;
				uint32_t b3 = (b0 + 2 * b1 + 1) / 3;
				color3 = 0xFF000000 | (r3 << 16) | (g3 << 8) | b3;
			}
			else {
				// (color0 + color1)/2
				uint32_t r2 = (r0 + r1) / 2;
				uint32_t g2 = (g0 + g1) / 2;
				uint32_t b2 = (b0 + b1) / 2;
				color2 = 0xFF000000 | (r2 << 16) | (g2 << 8) | b2;

				// 透明黑色（实际DXT1中可能为透明或黑色，根据格式）
				color3 = 0x00000000; // 假设使用透明
			}

			// 处理索引
			uint32_t indices = *(uint32_t *)(src + blockOffset + 4);

			// 计算目标块位置（每个块4x4像素，每个像素4字节）
			int dstX = blockX * 4;
			int dstY = blockY * 4;
			for (int y = 0; y < 4; ++y) {
				for (int x = 0; x < 4; ++x) {
					// 确保不超出图像边界
					if (dstX + x >= w || dstY + y >= h) continue;

					// 计算像素索引（按DXT索引顺序）
					int pixelIndex = y * 4 + x;
					uint8_t index = (indices >> (2 * pixelIndex)) & 0x03;

					// 获取颜色
					uint32_t color;
					switch (index) {
					case 0: color = color0Decoded; break;
					case 1: color = color1Decoded; break;
					case 2: color = color2; break;
					case 3: color = color3; break;
					}

					// 写入目标像素（假设dst为32bpp，行优先）
					uint32_t *pixel = (uint32_t *)(dst + ((dstY + y) * w + (dstX + x)) * 4);
					*pixel = color;
				}
			}
		}
	}
}

// 解码 DXT3 纹理
void DecodeDXT3(char *dst, const char *src, int w, int h) {
	int numBlocksX = (w + 3) / 4;
	int numBlocksY = (h + 3) / 4;

	for (int blockY = 0; blockY < numBlocksY; ++blockY) {
		for (int blockX = 0; blockX < numBlocksX; ++blockX) {
			int blockOffset = (blockY * numBlocksX + blockX) * 16;

			// ----------------------------
			// Alpha部分处理（8字节）
			// ----------------------------
			uint64_t alphaBits = *(uint64_t *)(src + blockOffset);
			uint8_t alphaValues[16];
			for (int i = 0; i < 16; ++i) {
				// 每个alpha值占4位（0-15），需要扩展到8位（0-255）
				uint8_t a = (alphaBits >> (4 * i)) & 0x0F;
				alphaValues[i] = (a << 4) | a; // 0x0F -> 0xFF, 0x07 -> 0x77等
			}

			// ----------------------------
			// 颜色部分处理（同DXT1但强制四色模式）
			// ----------------------------
			uint16_t color0 = *(uint16_t *)(src + blockOffset + 8);
			uint16_t color1 = *(uint16_t *)(src + blockOffset + 10);
			uint32_t indices = *(uint32_t *)(src + blockOffset + 12);

			// 解码RGB颜色（与DXT1相同）
			uint32_t r0 = ((color0 >> 11) & 0x1F) * 255 / 31;
			uint32_t g0 = ((color0 >> 5) & 0x3F) * 255 / 63;
			uint32_t b0 = (color0 & 0x1F) * 255 / 31;
			uint32_t color0Decoded = (r0 << 16) | (g0 << 8) | b0;

			uint32_t r1 = ((color1 >> 11) & 0x1F) * 255 / 31;
			uint32_t g1 = ((color1 >> 5) & 0x3F) * 255 / 63;
			uint32_t b1 = (color1 & 0x1F) * 255 / 31;
			uint32_t color1Decoded = (r1 << 16) | (g1 << 8) | b1;

			// DXT3总是四色模式
			// 计算中间颜色（固定使用四色插值）
			uint32_t r2 = (2 * r0 + r1 + 1) / 3;
			uint32_t g2 = (2 * g0 + g1 + 1) / 3;
			uint32_t b2 = (2 * b0 + b1 + 1) / 3;
			uint32_t color2 = (r2 << 16) | (g2 << 8) | b2;

			uint32_t r3 = (r0 + 2 * r1 + 1) / 3;
			uint32_t g3 = (g0 + 2 * g1 + 1) / 3;
			uint32_t b3 = (b0 + 2 * b1 + 1) / 3;
			uint32_t color3 = (r3 << 16) | (g3 << 8) | b3;

			// ----------------------------
			// 合并颜色和alpha
			// ----------------------------
			int dstX = blockX * 4;
			int dstY = blockY * 4;

			for (int y = 0; y < 4; ++y) {
				for (int x = 0; x < 4; ++x) {
					if (dstX + x >= w || dstY + y >= h) continue;

					int pixelIndex = y * 4 + x;

					// 获取alpha值
					uint8_t alpha = alphaValues[pixelIndex];

					// 获取颜色索引
					uint8_t index = (indices >> (2 * pixelIndex)) & 0x03;

					// 获取颜色值
					uint32_t color;
					switch (index) {
					case 0: color = color0Decoded; break;
					case 1: color = color1Decoded; break;
					case 2: color = color2; break;
					case 3: color = color3; break;
					}

					// 组合ARGB（假设目标格式是ARGB8888）
					uint32_t finalColor = (alpha << 24) | color;

					// 写入目标内存
					uint32_t *pixel = (uint32_t *)(dst + ((dstY + y) * w + (dstX + x)) * 4);
					*pixel = finalColor;
				}
			}
		}
	}
}

// 解码 DXT5 纹理
void DecodeDXT5(char *dst, const char *src, int w, int h) {
	int numBlocksX = w / 4;
	int numBlocksY = h / 4;

	for (int blockY = 0; blockY < numBlocksY; ++blockY) {
		for (int blockX = 0; blockX < numBlocksX; ++blockX) {
			int blockOffset = (blockY * numBlocksX + blockX) * 16;
			uint16_t color0 = *(uint16_t *)(src + blockOffset);
			uint16_t color1 = *(uint16_t *)(src + blockOffset + 2);

			uint32_t color0Decoded = DecodeRGB565(color0);
			uint32_t color1Decoded = DecodeRGB565(color1);

			uint32_t *block = (uint32_t *)(dst + (blockY * numBlocksX + blockX) * 16);

			uint32_t alphaTable = *(uint32_t *)(src + blockOffset + 4);
			uint32_t *alphaTableDecoded = new uint32_t[8];
			for (int i = 0; i < 8; ++i) {
				uint32_t alpha = (alphaTable >> (i * 4)) & 0xF;
				alphaTableDecoded[i] = alpha * 255 / 15;
			}

			for (int i = 0; i < 16; ++i) {
				uint8_t alphaIndex = (alphaTable >> (2 * i)) & 0x03;
				uint32_t alphaValue = alphaTableDecoded[alphaIndex];

				block[i] = (alphaValue << 24) | (color0Decoded & 0x00FFFFFF);
			}

			delete[] alphaTableDecoded;
		}
	}
}

// 主解码函数
void DecodeTexture(char *dst, const char *src, int w, int h, int type) {
	switch (type) {
	case 1:
		DecodeDXT1(dst, src, w, h);
		break;
	case 3:
		DecodeDXT3(dst, src, w, h);
		break;
	case 5:
		DecodeDXT5(dst, src, w, h);
		break;
	default:
		// 不支持的类型
		break;
	}
}


void ClipNode::GetProperties(CMFCPropertyGridCtrl &grid) {
	CMFCPropertyGridProperty *pClip = new CMFCPropertyGridProperty(_T("裁切区域"), 15520);
	pClip->AddSubItem(CreateBoundedProperty(_T("X偏移"), imageRef->x, 0, 65535, _T("相对于纹理左上角的水平偏移。")));
	pClip->AddSubItem(CreateBoundedProperty(_T("Y偏移"), imageRef->y, 0, 65535, _T("相对于纹理左上角的垂直偏移。")));
	pClip->AddSubItem(CreateBoundedProperty(_T("宽度"), imageRef->w, 1, 4096, _T("切下来的纹理宽度。")));
	pClip->AddSubItem(CreateBoundedProperty(_T("高度"), imageRef->h, 1, 4096, _T("切下来的纹理高度")));
	grid.AddProperty(pClip);

	CMFCPropertyGridProperty *pTex = new CMFCPropertyGridProperty(_T("纹理来源"), 15521);
	pTex->AddSubItem(CreateTextureNameProperty(_T("组名"), imageRef->group));
	pTex->AddSubItem(CreateTextureNameProperty(_T("名称"), imageRef->name));
	grid.AddProperty(pTex);

	m_propGrid = &grid;
}

void ClipNode::UpdateData() {
	CMFCPropertyGridCtrl *pGrid = m_propGrid;
	if (!pGrid) return;

	// 更新裁切参数
	if (CMFCPropertyGridProperty *pClip = pGrid->FindItemByData(15520)) {
		imageRef->x = GetSubPropValue(pClip, 0, imageRef->x);
		imageRef->y = GetSubPropValue(pClip, 1, imageRef->y);
		imageRef->w = GetSubPropValue(pClip, 2, imageRef->w);
		imageRef->h = GetSubPropValue(pClip, 3, imageRef->h);
	}

	// 更新纹理名称
	if (CMFCPropertyGridProperty *pTex = pGrid->FindItemByData(15521)) {
		UpdateTextureName(pTex->GetSubItem(0), imageRef->group);
		UpdateTextureName(pTex->GetSubItem(1), imageRef->name);
	}

	// FIXME: 深耦合，此操作完全依赖ImageGroupNode的当前结构！！
	// 为更好处理、考虑，将ClipNode等改为ImageGroupNode之内部类？
	GetParent()->GetParent()->GetChildren()[1]->GetChildren()[m_index]->Flush();
}

Gdiplus::Bitmap *ClipNode::GetBitmap()
{
	return GetRootNode()->GetDocument()->QueryTexture(GetTexRef());
}

void ClipNode::SetXY(CPoint pt) 
{
	imageRef->x = pt.x;
	imageRef->y = pt.y;

	// FIXME: 
	GetParent()->GetParent()->GetChildren()[1]->GetChildren()[m_index]->Flush();
}

void ClipNode::SetWH(CPoint pt)
{
	if (pt.x < 0 || pt.y < 0) return;

	imageRef->w = pt.x;
	imageRef->h = pt.y;

	GetParent()->GetParent()->GetChildren()[1]->GetChildren()[m_index]->Flush();
}

void ClipNode::DeleteNode()
{
	ContentNode *parent = GetParent();
	if (!parent) return;
	ContentNode *grandParent = parent->GetParent();
	if (ImageGroupNode *ig = dynamic_cast<ImageGroupNode *>(grandParent))
	{
		ig->DeleteClipTileOf(m_index);
	}
}

// 创建带校验的纹理名称属性

CMFCPropertyGridProperty *ClipNode::CreateTextureNameProperty(LPCTSTR name, const char(&field)[8]) {
	CString strVal(BlockNameGetCleanName(field).c_str());
	auto prop = new CMFCPropertyGridProperty(name, strVal);
	return prop;
}

// 更新纹理名称字段

void ClipNode::UpdateTextureName(CMFCPropertyGridProperty *prop, char(&field)[8]) {
	if (!prop) return;
	CString newVal = prop->GetValue().bstrVal;
	BlockNamePutPaddedName(field, xybase::string::to_string(newVal.GetString()));
}

// 带范围限制的属性创建

CMFCPropertyGridProperty *ClipNode::CreateBoundedProperty(LPCTSTR name, uint16_t value, int minVal, int maxVal, LPCTSTR desc)
{
	auto prop = new CMFCPropertyGridProperty(name, (_variant_t)(long)value, desc);
	prop->EnableSpinControl(TRUE, minVal, maxVal);
	return prop;
}

void TileNode::GetProperties(CMFCPropertyGridCtrl &grid) {
	CMFCPropertyGridProperty *pMappingGroup = new CMFCPropertyGridProperty(_T("纹理映射"), 15770);
	CMFCPropertyGridProperty *pMapping = new CMFCPropertyGridProperty(
		_T("翻转映射"),
		(_variant_t)(long)static_cast<int>(imageRef->type),
		_T("纹理的左右/上下翻转属性。\r\n0 - 正常\r\n1 - 水平\r\n2 - 垂直\r\n3 - 双向")
	);
	pMapping->SetValue((long)static_cast<int>(imageRef->type));
	pMappingGroup->AddSubItem(pMapping);

	for (int i = 0; i < 4; ++i) {
		CString strName;
		strName.Format(_T("参数[%d]"), i + 1);
		/*CString strVal;
		strVal.Format(_T("0x%08X"), imageRef->ukn[i]);*/
		pMappingGroup->AddSubItem(new CMFCPropertyGridProperty(
			strName,
			(_variant_t)(int)imageRef->ukn[i],
			_T("未解析映射参数。")));
	}

	grid.AddProperty(pMappingGroup);

	// 顶点属性组
	AddVertexProperties(grid, imageRef->tlPoint, imageRef->tlColour, _T("左上"), 15771);
	AddVertexProperties(grid, imageRef->trPoint, imageRef->trColour, _T("右上"), 15772);
	AddVertexProperties(grid, imageRef->blPoint, imageRef->blColour, _T("左下"), 15773);
	AddVertexProperties(grid, imageRef->brPoint, imageRef->brColour, _T("右下"), 15774);

	m_propGrid = &grid;
}

void TileNode::UpdateData() {
	CMFCPropertyGridCtrl *pGrid = m_propGrid;
	if (!pGrid) return;

	// 更新映射类型
	if (CMFCPropertyGridProperty *pMapping = pGrid->FindItemByData(15770)) {
		imageRef->type = static_cast<decltype(imageRef->type)>(
			pMapping->GetSubItem(0)->GetValue().intVal);
		imageRef->ukn[0] = static_cast<uint8_t>(
			pMapping->GetSubItem(1)->GetValue().intVal);
		imageRef->ukn[1] = static_cast<uint8_t>(
			pMapping->GetSubItem(2)->GetValue().intVal);
		imageRef->ukn[2] = static_cast<uint8_t>(
			pMapping->GetSubItem(3)->GetValue().intVal);
		imageRef->ukn[3] = static_cast<uint8_t>(
			pMapping->GetSubItem(4)->GetValue().intVal);
	}

	// 更新顶点数据
	UpdateVertexData(imageRef->tlPoint, imageRef->tlColour, _T("左上"), 15771);
	UpdateVertexData(imageRef->trPoint, imageRef->trColour, _T("右上"), 15772);
	UpdateVertexData(imageRef->blPoint, imageRef->blColour, _T("左下"), 15773);
	UpdateVertexData(imageRef->brPoint, imageRef->brColour, _T("右下"), 15774);

	m_bDirtyBitmap = TRUE;
	if (TileCategoryNode *p = dynamic_cast<TileCategoryNode *>(parent))
	{
		p->UpdateData();
	}
}

Gdiplus::Bitmap *TileNode::GetBitmap()
{
	if (!m_bDirtyBitmap) return m_bitmap;

	Gdiplus::Bitmap *bitmap = GetRootNode()->GetDocument()->QueryTexture(imageRef->group);
	PrepareBitmap(bitmap);
	return m_bitmap;
}

xybase::Vec2<int16_t> TileNode::GetVertex0() const
{
	return imageRef->tlPoint;
}

xybase::Vec2<int16_t> TileNode::GetVertex1() const
{
	return imageRef->trPoint;
}

xybase::Vec2<int16_t> TileNode::GetVertex2() const
{
	return imageRef->blPoint;
}

xybase::Vec2<int16_t> TileNode::GetVertex3() const
{
	return imageRef->brPoint;
}

void TileNode::SetVertex0(xybase::Vec2<int16_t> pt)
{
	auto diff = pt - imageRef->tlPoint;
	imageRef->tlPoint = pt;

	imageRef->brPoint = imageRef->brPoint + diff;
	imageRef->blPoint = imageRef->blPoint + diff;
	imageRef->trPoint = imageRef->trPoint + diff;
}

void TileNode::SetVertex1(xybase::Vec2<int16_t> pt)
{
	imageRef->trPoint = pt;

	imageRef->brPoint = imageRef->tlPoint +
		(imageRef->blPoint - imageRef->tlPoint) +
		(imageRef->trPoint - imageRef->tlPoint);
}

void TileNode::SetVertex2(xybase::Vec2<int16_t> pt)
{
	imageRef->blPoint = pt;

	imageRef->brPoint = imageRef->tlPoint +
		(imageRef->blPoint - imageRef->tlPoint) +
		(imageRef->trPoint - imageRef->tlPoint);
}

void TileNode::SetVertex3(xybase::Vec2<int16_t> p_pt)
{
	using namespace xybase;
	// 确保使用浮点运算
	Vec2<float> tl = Vec2<float>::From(imageRef->tlPoint); // 原点
	Vec2<float> bl = Vec2<float>::From(imageRef->blPoint); // 原左下点
	Vec2<float> tr = Vec2<float>::From(imageRef->trPoint); // 原右上点

	Vec2<float> pt = Vec2<float>::From(p_pt);

	// 计算基底向量
	Vec2<float> u = bl - tl; // 左基底向量
	Vec2<float> v = tr - tl; // 右基底向量

	// 将输入点转换为相对坐标
	Vec2<float> target = pt - tl;

	// 解线性方程组: target = a*u + b*v
	// 矩阵形式: [u.x  v.x] [a]   = [target.x]
	//          [u.y  v.y] [b]     [target.y]
	float det = u.x * v.y - u.y * v.x; // 行列式

	if (std::fabs(det) < 1e-6f) {
		// 基底共线或接近共线，无法构成平行四边形
		// 可抛出异常或保持原值
		return;
	}

	// 计算系数 a, b
	float a = (target.x * v.y - target.y * v.x) / det;
	float b = (u.x * target.y - u.y * target.x) / det;

	// 更新顶点（保持平行四边形性质）
	imageRef->blPoint = Vec2<int16_t>::From(tl + u * a); // 新左下点
	imageRef->trPoint = Vec2<int16_t>::From(tl + v * b); // 新右上点

	imageRef->brPoint = imageRef->tlPoint +
		(imageRef->blPoint - imageRef->tlPoint) +
		(imageRef->trPoint - imageRef->tlPoint);
}

void TileNode::DeleteNode()
{
	ContentNode *parent = GetParent();
	if (!parent) return;
	ContentNode *grandParent = parent->GetParent();
	if (ImageGroupNode *ig = dynamic_cast<ImageGroupNode *>(grandParent))
	{
		ig->DeleteClipTileOf(m_index);
	}
}

BYTE Lerp(BYTE a, BYTE b, float t) {
	return static_cast<BYTE>(a + (b - a) * t);
}

Gdiplus::Color LerpColor(Gdiplus::Color a, Gdiplus::Color b, float t) {
	return Gdiplus::Color(
		Lerp(a.GetA(), b.GetA(), t),
		Lerp(a.GetR(), b.GetR(), t),
		Lerp(a.GetG(), b.GetG(), t),
		Lerp(a.GetB(), b.GetB(), t)
	);
}

void TileNode::PrepareBitmap(Gdiplus::Bitmap *source)
{
	Gdiplus::Bitmap *target;
	if (source == NULL)
	{
		target = new Gdiplus::Bitmap(imageRef->w, imageRef->h, PixelFormat32bppARGB);

		Gdiplus::Graphics graphics(target);
		graphics.Clear(Gdiplus::Color(255, 255, 255));  // 清除背景为白色

		// 设置画笔（红色）
		Gdiplus::Pen redPen(Gdiplus::Color(255, 255, 0, 0), 4);  // 红色，线宽 4

		// 绘制红色大叉（两条交叉的线）
		graphics.DrawLine(&redPen, 0, 0, imageRef->w, imageRef->h);  // 从左上到右下
		graphics.DrawLine(&redPen, 0, imageRef->h, imageRef->w, 0);  // 从左下到右上
	}
	else
	{
		target = source->Clone(Gdiplus::Rect(imageRef->x, imageRef->y, imageRef->w, imageRef->h), PixelFormat32bppARGB);
	}

	Gdiplus::Bitmap flipped(imageRef->w, imageRef->h, PixelFormat32bppARGB);
	Gdiplus::Graphics flipGraphics(&flipped);
	if ((int)imageRef->type & 1) {
		flipGraphics.ScaleTransform(-1, 1);
		flipGraphics.TranslateTransform(imageRef->w, 0, Gdiplus::MatrixOrderAppend);
	}
	if ((int)imageRef->type & 2) {
		flipGraphics.ScaleTransform(1, -1);
		flipGraphics.TranslateTransform(0, imageRef->h, Gdiplus::MatrixOrderAppend);
	}
	flipGraphics.DrawImage(target, 0, 0, imageRef->w, imageRef->h);

	Gdiplus::Bitmap colorGradient(imageRef->w, imageRef->h, PixelFormat32bppARGB);
	Gdiplus::Color tlColor(imageRef->tlColour.a * 2, imageRef->tlColour.r * 2, imageRef->tlColour.g * 2, imageRef->tlColour.b * 2);
	Gdiplus::Color trColor(imageRef->trColour.a * 2, imageRef->trColour.r * 2, imageRef->trColour.g * 2, imageRef->trColour.b * 2);
	Gdiplus::Color blColor(imageRef->blColour.a * 2, imageRef->blColour.r * 2, imageRef->blColour.g * 2, imageRef->blColour.b * 2);
	Gdiplus::Color brColor(imageRef->brColour.a * 2, imageRef->brColour.r * 2, imageRef->brColour.g * 2, imageRef->brColour.b * 2);
	Gdiplus::BitmapData colorData;
	Gdiplus::Rect imageRect(0, 0, imageRef->w, imageRef->h);
	colorGradient.LockBits(&imageRect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &colorData);
	for (int y = 0; y < imageRef->h; ++y) {
		BYTE *row = (BYTE *)colorData.Scan0 + y * colorData.Stride;
		for (int x = 0; x < imageRef->w; ++x) {
			float u = x / (float)(imageRef->w - 1);
			float v = y / (float)(imageRef->h - 1);
			Gdiplus::Color top = LerpColor(tlColor, trColor, u);
			Gdiplus::Color bottom = LerpColor(blColor, brColor, u);
			Gdiplus::Color interpolated = LerpColor(top, bottom, v);
			((Gdiplus::Color *)row)[x] = interpolated;
		}
	}
	colorGradient.UnlockBits(&colorData);

	if (m_bitmap) delete m_bitmap;
	m_bitmap = new Gdiplus::Bitmap(imageRef->w, imageRef->h, PixelFormat32bppARGB);
	Gdiplus::BitmapData flippedData, finalData;
	flipped.LockBits(&imageRect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &flippedData);
	colorGradient.LockBits(&imageRect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &colorData);
	m_bitmap->LockBits(&imageRect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &finalData);
	for (int y = 0; y < imageRef->h; ++y) {
		BYTE *flippedRow = (BYTE *)flippedData.Scan0 + y * flippedData.Stride;
		BYTE *colorRow = (BYTE *)colorData.Scan0 + y * colorData.Stride;
		BYTE *finalRow = (BYTE *)finalData.Scan0 + y * finalData.Stride;
		for (int x = 0; x < imageRef->w; ++x) {
			int flippedIdx = x * 4;
			int colorIdx = x * 4;
			int finalIdx = x * 4;
			// 调制颜色
			// 剪影开关（推定
			if (imageRef->ukn[1] == 0x2)
			{
				finalRow[finalIdx + 0] = 0;
				finalRow[finalIdx + 1] = 0;
				finalRow[finalIdx + 2] = 0;
			}
			else
			{
				finalRow[finalIdx + 0] = (flippedRow[flippedIdx + 0] * colorRow[colorIdx + 0]) / 255; // B
				finalRow[finalIdx + 1] = (flippedRow[flippedIdx + 1] * colorRow[colorIdx + 1]) / 255; // G
				finalRow[finalIdx + 2] = (flippedRow[flippedIdx + 2] * colorRow[colorIdx + 2]) / 255; // R
			}
			// 总是钳制
			int clampedAlpha = min(127, flippedRow[flippedIdx + 3]) * 255 / 127;
			finalRow[finalIdx + 3] = (clampedAlpha * colorRow[colorIdx + 3]) / 255; // A
		}
	}
	m_bitmap->UnlockBits(&finalData);
	colorGradient.UnlockBits(&colorData);
	flipped.UnlockBits(&flippedData);

	m_bDirtyBitmap = false;
	delete target;
}

// 顶点属性组
void TileNode::AddVertexProperties(CMFCPropertyGridCtrl &grid, const xybase::Vec2<int16_t> &point, const RGBA &color, const CString &prefix, int data)
{
	CString strGroup;
	strGroup.Format(_T("%s%s"), prefix.GetString(), _T("顶点"));

	CMFCPropertyGridProperty *pGroup = new CMFCPropertyGridProperty(strGroup, data);

	// 坐标
	CMFCPropertyGridProperty *pCoord = new CMFCPropertyGridProperty(_T("坐标"), 0, TRUE);
	pCoord->AddSubItem(CreateCoordProperty(_T("X"), point.x));
	pCoord->AddSubItem(CreateCoordProperty(_T("Y"), point.y));
	pGroup->AddSubItem(pCoord);

	// 颜色
	CMFCPropertyGridProperty *pColor = new CMFCPropertyGridProperty(_T("颜色"), 0, TRUE);
	pColor->AddSubItem(CreateColorProperty(_T("红"), color.r));
	pColor->AddSubItem(CreateColorProperty(_T("绿"), color.g));
	pColor->AddSubItem(CreateColorProperty(_T("蓝"), color.b));
	pColor->AddSubItem(CreateAlphaProperty(color.a));
	pGroup->AddSubItem(pColor);

	grid.AddProperty(pGroup);
}

// 坐标属性（带符号）

CMFCPropertyGridProperty *TileNode::CreateCoordProperty(LPCTSTR name, int16_t value) {
	auto prop = new CMFCPropertyGridProperty(name, (_variant_t)(long)value, _T("坐标值（像素）"));
	prop->EnableSpinControl(TRUE, -32768, 32767);
	return prop;
}

// 颜色分量属性

CMFCPropertyGridProperty *TileNode::CreateColorProperty(LPCTSTR name, uint8_t value) {
	auto prop = new CMFCPropertyGridProperty(name, (_variant_t)(long)value, _T("颜色值滤过（127为100%）"));
	prop->EnableSpinControl(TRUE, 0, 255);
	return prop;
}

// 透明度属性

CMFCPropertyGridProperty *TileNode::CreateAlphaProperty(uint8_t value) {
	auto prop = new CMFCPropertyGridProperty(_T("不透明度"), (_variant_t)(long)value, _T("不透明度滤过（127为100%，全不透明）"));
	prop->EnableSpinControl(TRUE, 0, 255);
	return prop;
}

// 更新顶点数据

void TileNode::UpdateVertexData(xybase::Vec2<int16_t> &point, RGBA &color, const CString &prefix, int data)
{
	if (CMFCPropertyGridProperty *pGroup = m_propGrid->FindItemByData(data)) {
		// 坐标
		if (CMFCPropertyGridProperty *pCoord = pGroup->GetSubItem(0)) {
			point.x = static_cast<int16_t>(pCoord->GetSubItem(0)->GetValue().intVal);
			point.y = static_cast<int16_t>(pCoord->GetSubItem(1)->GetValue().intVal);
		}

		// 颜色
		if (CMFCPropertyGridProperty *pColor = pGroup->GetSubItem(1)) {
			color.r = static_cast<uint8_t>(pColor->GetSubItem(0)->GetValue().intVal);
			color.g = static_cast<uint8_t>(pColor->GetSubItem(1)->GetValue().intVal);
			color.b = static_cast<uint8_t>(pColor->GetSubItem(2)->GetValue().intVal);
			color.a = static_cast<uint8_t>(pColor->GetSubItem(3)->GetValue().intVal);
		}
	}
}

void BlockNode::GetProperties(CMFCPropertyGridCtrl &grid) {
	auto group = new CMFCPropertyGridProperty(_T("块属性"));
	group->AddSubItem(new CMFCPropertyGridProperty(
		_T("块类型"),
		(_variant_t)(int)block->blockHeader.type,
		_T("指示块的类型")));
	group->AddSubItem(new CMFCPropertyGridProperty(
		_T("块名"),
		(_variant_t)CString(block->blockHeader.name, 4),
		_T("此块的名字")));
	group->AddSubItem(new CMFCPropertyGridProperty(
		_T("块大小"),
		(_variant_t) (int)(block->blockHeader.size * 16),
		_T("此块的大小")));

	group->AllowEdit(FALSE);
	grid.AddProperty(group);
}

FileNode::FileNode(BlockFile *file, CFFXIMenuDoc *doc) : file(file), doc(doc) {
	for (auto &block : file->blocks) {
		switch ((BlockType)block->blockHeader.type)
		{
		case BlockType::BT_IMAGE_SET:
			children.push_back(new ImageSetBlockNode(this, dynamic_cast<BlockFile::ImageSetBlock *>(block)));
			break;
		case BlockType::BT_IMAGE:
			children.push_back(new ImageBlockNode(this, dynamic_cast<BlockFile::ImageBlock *>(block)));
			break;
		default:
			children.push_back(new BlockNode(this, block));
		}
	}
}

void FileNode::GetProperties(CMFCPropertyGridCtrl &grid) { // 添加文件路径
	auto prop = new CMFCPropertyGridProperty(
		_T("文件路径"),
		(_variant_t)CString(file->path.string().c_str()),
		_T("文件的存储路径"));
	prop->AllowEdit(FALSE);
	grid.AddProperty(prop);

	// 添加文件头信息
	CMFCPropertyGridProperty *headerGroup = new CMFCPropertyGridProperty(_T("文件头信息"));
	BlockFileHeader &header = file->header;

	// 类型字段，4字节字符数组
	CString typeStr;
	typeStr.Format(_T("%c%c%c%c"), header.type[0], header.type[1], header.type[2], header.type[3]);
	headerGroup->AddSubItem(new CMFCPropertyGridProperty(_T("魔法头"), typeStr, _T("用于标记文件类型的字符串。")));

	// 其他字段
	headerGroup->AddSubItem(new CMFCPropertyGridProperty(_T("旗标1"), (_variant_t)(long)header.flg1, _T("未知用途")));
	headerGroup->AddSubItem(new CMFCPropertyGridProperty(_T("旗标2"), (_variant_t)(long)header.flg2, _T("位置用途")));

	headerGroup->AllowEdit(FALSE);
	grid.AddProperty(headerGroup);
}

ImageSetBlockNode::ImageSetBlockNode(ContentNode *parent, BlockFile::ImageSetBlock *block)
	: BlockNode(parent, block), imageSetBlock(block) {
	for (size_t i = 0; i < imageSetBlock->groups.size(); ++i) {
		children.push_back(new ImageGroupNode(this, &imageSetBlock->groups[i], i));
	}
}

void ImageSetBlockNode::GetProperties(CMFCPropertyGridCtrl &grid)
{
	BlockNode::GetProperties(grid);

	// 图像集属性组
	CMFCPropertyGridProperty *pBaseGroup = new CMFCPropertyGridProperty(_T("图像集属性"));


	// 组名（可编辑）
	pBaseGroup->AddSubItem(new CMFCPropertyGridProperty(
		_T("组标识"),
		(_variant_t)imageSetBlock->group.c_str(),
		_T("图像集所属组标识"),
		(DWORD_PTR)&imageSetBlock->group));

	// 名称（可编辑）
	pBaseGroup->AddSubItem(new CMFCPropertyGridProperty(
		_T("资源名称"),
		(_variant_t)imageSetBlock->name.c_str(),
		_T("图像集资源名称"),
		(DWORD_PTR)&imageSetBlock->name));

	grid.AddProperty(pBaseGroup);
}

inline CString ImageSetBlockNode::FormatStringList(const std::vector<std::string> &list)
{
	CString strResult;
	for (const auto &s : list) {
		strResult += CString(s.c_str()) + _T("\r\n");
	}
	return strResult.TrimRight(_T("\r\n")); // 去除最后多余的空行
}

void ImageBlockNode::GetProperties(CMFCPropertyGridCtrl &grid)
{
	BlockNode::GetProperties(grid);

	// 图像头属性组
	CMFCPropertyGridProperty *pImgHeaderGroup = new CMFCPropertyGridProperty(_T("图像头属性"));

	auto &imageHeader = imageBlock->image.header;
	// 类型（0x91=Bitmap, 0xA1=DXT）
	CString strType;
	switch (imageHeader.type) {
	case 0x91: strType = _T("Bitmap (0x91)"); break;
	case 0xA1: strType = _T("DXT (0xA1)"); break;
	default:  strType.Format(_T("未知类型 (0x%02X)"), imageHeader.type);
	}
	pImgHeaderGroup->AddSubItem(new CMFCPropertyGridProperty(
		_T("编码类型"),
		(_variant_t)strType,
		_T("图像编码格式标识")));

	// 组名（固定8字节）
	CString strGroup = CString(imageHeader.group, 8);
	pImgHeaderGroup->AddSubItem(new CMFCPropertyGridProperty(
		_T("组标识"),
		(_variant_t)strGroup.Trim(),
		_T("资源分组标识（8字符）")));

	// 名称（固定8字节）
	CString strName = CString(imageHeader.name, 8);
	pImgHeaderGroup->AddSubItem(new CMFCPropertyGridProperty(
		_T("资源名"),
		(_variant_t)strName.Trim(),
		_T("纹理资源名称（8字符）")));

	// 版本号（显示为十六进制）
	CString strVer;
	strVer.Format(_T("0x%08X"), imageHeader.version);
	pImgHeaderGroup->AddSubItem(new CMFCPropertyGridProperty(
		_T("版本标识"),
		(_variant_t)strVer,
		_T("文件版本标识（通常为0x28）")));

	// 尺寸属性
	CMFCPropertyGridProperty *pSizeGroup = new CMFCPropertyGridProperty(_T("图像尺寸"), 0, TRUE);
	pSizeGroup->AddSubItem(new CMFCPropertyGridProperty(
		_T("宽度"),
		(_variant_t)(long)imageHeader.width,
		_T("图像的宽度（单位：像素）")));
	pSizeGroup->AddSubItem(new CMFCPropertyGridProperty(
		_T("高度"),
		(_variant_t)(long)imageHeader.height,
		_T("图像的高度（单位：像素）")));
	pImgHeaderGroup->AddSubItem(pSizeGroup);

	// Mipmap和位深
	pImgHeaderGroup->AddSubItem(new CMFCPropertyGridProperty(
		_T("Mipmap层级"),
		(_variant_t)(long)imageHeader.mipmapCount,
		_T("纹理mipmap层级数")));
	pImgHeaderGroup->AddSubItem(new CMFCPropertyGridProperty(
		_T("位深度"),
		(_variant_t)(long)imageHeader.bitCount,
		_T("像素存储位深")));

	// 未知参数显示
	CMFCPropertyGridProperty *pUknGroup = new CMFCPropertyGridProperty(_T("未解析参数"));
	for (int i = 0; i < 6; ++i) {
		CString strName;
		strName.Format(_T("参数[%d]"), i + 1);
		CString strVal;
		strVal.Format(_T("0x%08X"), imageHeader.ukn[i]);
		pUknGroup->AddSubItem(new CMFCPropertyGridProperty(
			strName,
			(_variant_t)strVal,
			_T("保留字段")));
	}
	pImgHeaderGroup->AddSubItem(pUknGroup);

	// DXT压缩头属性组
	if (imageHeader.type == 0xA1) {
		CMFCPropertyGridProperty *pDxtGroup = new CMFCPropertyGridProperty(_T("DXT压缩属性"));

		auto &dxtHeader = imageBlock->image.dxtHeader;
		// FourCC代码（显示为ASCII字符串）
		CString strFourCC = CString(dxtHeader.fourCC, 4);
		pDxtGroup->AddSubItem(new CMFCPropertyGridProperty(
			_T("压缩格式"),
			(_variant_t)strFourCC,
			_T("DXT压缩格式标识（如DXT1/DXT5）")));

		// 纹理尺寸（自动换算单位）
		CString strSize;
		if (dxtHeader.textureSize > 1024 * 1024) {
			strSize.Format(_T("%.2f MiB"), dxtHeader.textureSize / (1024.0f * 1024));
		}
		else if (dxtHeader.textureSize > 1024) {
			strSize.Format(_T("%d KiB"), dxtHeader.textureSize / 1024);
		}
		else
		{
			strSize.Format(_T("%d B"), dxtHeader.textureSize);
		}
		pDxtGroup->AddSubItem(new CMFCPropertyGridProperty(
			_T("纹理大小"),
			(_variant_t)strSize,
			_T("压缩纹理数据体积")));

		// Pitch值（显示原始数值）
		pDxtGroup->AddSubItem(new CMFCPropertyGridProperty(
			_T("行对齐字节数"),
			(_variant_t)(long)dxtHeader.pitch,
			_T("DDS文件行对齐参数")));

		pImgHeaderGroup->AddSubItem(pDxtGroup);
	}

	grid.AddProperty(pImgHeaderGroup);
}

Gdiplus::Bitmap *ImageBlockNode::GetBitmap()
{
	if (bitmap) return bitmap;

	UpdateBitmap();

	// 返回生成的 CBitmap 对象
	return bitmap;
}

void ImageBlockNode::DumpTexture(LPCTSTR path)
{
	auto dds = imageBlock->image.GetDds();

	CFile file(path, CFile::modeWrite | CFile::modeCreate);
	unsigned size = 4 + 124 + imageBlock->image.dxtHeader.textureSize;
	file.Write(dds.get(), size);
}

void ImageBlockNode::ImportTexture(LPCTSTR path)
{
	CFile file(path, CFile::modeRead);
	int size = file.GetLength();
	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);
	file.Read(buffer.get(), size);
	file.Close();

	imageBlock->image.ImportDds(buffer.get());
	UpdateBitmap();
}

void ImageBlockNode::UpdateBitmap()
{
	int h = imageBlock->image.header.height;
	int w = imageBlock->image.header.width;

	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(h * w * 4);
	switch (*(int *)imageBlock->image.dxtHeader.fourCC)
	{
	case 'DXT1':
		DecodeDXT1(buffer.get(), imageBlock->image.texture.get(), w, h);
		break;
	case 'DXT3':
		DecodeDXT3(buffer.get(), imageBlock->image.texture.get(), w, h);
		break;
	case 'DXT5':
		DecodeDXT5(buffer.get(), imageBlock->image.texture.get(), w, h);
		break;
	}

	if (bitmap) delete bitmap;
	bitmap = new Gdiplus::Bitmap(w, h, PixelFormat32bppARGB);
	Gdiplus::BitmapData bitmapData;
	Gdiplus::Rect rect(0, 0, w, h);
	bitmap->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bitmapData);

	memcpy(bitmapData.Scan0, buffer.get(), w * h * 4);
#ifdef CLAMP_ALPHA
	BYTE *pixels = static_cast<BYTE *>(bitmapData.Scan0);
	int stride = bitmapData.Stride;
	for (int y = 0; y < static_cast<int>(bitmapData.Height); ++y)
	{
		BYTE *row = pixels + y * stride;
		for (UINT x = 0; x < bitmapData.Width; ++x)
		{
			BYTE &alpha = row[x * 4 + 3];
			alpha = static_cast<BYTE>((min(alpha, 127) * 255) / 127);
		}
	}
#endif
	bitmap->UnlockBits(&bitmapData);
}

TileCategoryNode::TileCategoryNode(ContentNode *parent, BlockFile::ImageSetBlock::ImageGroup *group)
	: group(group) {
	this->parent = parent;
	int i = 0;
	for (auto &ref : group->imageRefs) {
		children.push_back(new TileNode(this, &ref, i++));
	}
}

Gdiplus::Bitmap *TileCategoryNode::GetBitmap()
{
	if (!m_bDirtyBitmap) return m_bitmap;

	int16_t minX = INT16_MAX, maxX = INT16_MIN;
	int16_t minY = INT16_MAX, maxY = INT16_MIN;
	for (const auto &ref : group->imageRefs)
	{
		minX = min(min(min(min(minX, ref.tlPoint.x), ref.trPoint.x), ref.blPoint.x), ref.brPoint.x);
		maxX = max(max(max(max(maxX, ref.tlPoint.x), ref.trPoint.x), ref.blPoint.x), ref.brPoint.x);
		minY = min(min(min(min(minY, ref.tlPoint.y), ref.trPoint.y), ref.blPoint.y), ref.brPoint.y);
		maxY = max(max(max(max(maxY, ref.tlPoint.y), ref.trPoint.y), ref.blPoint.y), ref.brPoint.y);
	}
	int xExtent = max(std::abs(minX), std::abs(maxX));
	int yExtent = max(std::abs(minY), std::abs(maxY));
	int canvasWidth = 2 * xExtent + 1;
	int canvasHeight = 2 * yExtent + 1;

	if (m_bitmap) delete m_bitmap;
	m_bitmap = new Gdiplus::Bitmap(canvasWidth, canvasHeight, PixelFormat32bppARGB);
	Gdiplus::Graphics graphics(m_bitmap);
	graphics.Clear(Gdiplus::Color::Transparent);

	int centerX = canvasWidth / 2;
	int centerY = canvasHeight / 2;

	for (auto child : children)
	{
		if (auto c = dynamic_cast<TileNode *>(child))
		{
			Gdiplus::Bitmap *clip = c->GetBitmap();
			auto r = c->GetRef();
			
			Gdiplus::PointF tl(centerX + r->tlPoint.x, centerY + r->tlPoint.y);
			Gdiplus::PointF tr(centerX + r->trPoint.x, centerY + r->trPoint.y);
			Gdiplus::PointF bl(centerX + r->blPoint.x, centerY + r->blPoint.y);
			Gdiplus::PointF br(centerX + r->brPoint.x, centerY + r->brPoint.y);

			// FIXME: 
			// 绘制两个三角形
			Gdiplus::PointF triangle1[] = { tl, tr, bl };
			graphics.DrawImage(clip, triangle1, 3, 0, 0, r->w, r->h, Gdiplus::UnitPixel);
			//Gdiplus::PointF triangle2[] = { tr, bl, br };
			//graphics.DrawImage(clip, triangle2, 3, 0, 0, r->w, r->h, Gdiplus::UnitPixel);
		}
	}
	return m_bitmap;
}

void TileCategoryNode::DeleteChild(size_t index)
{
	ContentNode::DeleteChild(index);

	for (int i = index; i < children.size(); ++i)
	{
		TileNode *tn = (TileNode *)children[i];
		tn->SetIndex(i);
	}
}

void TileCategoryNode::InsertChildAfter(size_t index, ContentNode *child)
{
	ContentNode::InsertChildAfter(index, child);

	for (int i = index + 2; i < children.size(); ++i)
	{
		TileNode *tn = (TileNode *)children[i];
		tn->SetIndex(i);
	}
}

void TileCategoryNode::UpdateChildren()
{
	children.clear();
	int i = 0;
	for (auto &ref : group->imageRefs) {
		children.push_back(new TileNode(this, &ref, i++));
	}
}

void ImageGroupNode::DeleteClipTileOf(int index)
{
	for (size_t i = index; i < group->imageRefs.size() - 1; ++i)
	{
		group->imageRefs[i] = group->imageRefs[i + 1];
	}
	group->imageRefs.pop_back();

	m_clipCate->UpdateChildren();
	m_tileCate->UpdateChildren();
}

void ImageGroupNode::InsertClipTileAfter(int index, const BlockFile::ImageSetBlock::ImageGroup::ImageRef &ref)
{
	auto &refs = group->imageRefs;
	if (index >= refs.size()) return;

	if (index == refs.size() - 1)
	{
		refs.push_back(ref);

		m_clipCate->UpdateChildren();
		m_tileCate->UpdateChildren();
		return;
	}

	refs.push_back(refs[refs.size() - 1]);
	for (size_t i = refs.size() - 2; i > index; --i)
	{
		refs[i] = refs[i - 1];
	}
	refs[index] = ref;

	m_clipCate->UpdateChildren();
	m_tileCate->UpdateChildren();
}

void ContentNode::DeleteChild(size_t index)
{
	if (index >= children.size()) return;

	for (size_t i = index; i < children.size() - 1; ++i)
	{
		children[i] = children[i + 1];
	}
	children.pop_back();
}

void ContentNode::InsertChildAfter(size_t index, ContentNode *child)
{
	if (index == children.size() - 1)
	{
		children.push_back(child);
		return;
	}

	if (index >= children.size()) return;

	children.push_back(children[children.size() - 1]);
	for (size_t i = children.size() - 2; i > index; --i)
	{
		children[i] = children[i - 1];
	}
	children[index + 1] = child;
}

void ClipCategoryNode::UpdateChildren()
{
	children.clear();
	int i = 0;
	for (auto &ref : group->imageRefs) {
		children.push_back(new ClipNode(this, &ref, i++));
	}
}

void ClipCategoryNode::DeleteChild(size_t index)
{
	ContentNode::DeleteChild(index);

	for (int i = index; i < children.size(); ++i)
	{
		ClipNode *cn = (ClipNode *)children[i];
		cn->SetIndex(i);
	}
}

void ClipCategoryNode::InsertChildAfter(size_t index, ContentNode *child)
{
	ContentNode::InsertChildAfter(index, child);

	for (int i = index + 2; i < children.size(); ++i)
	{
		ClipNode *cn = (ClipNode *)children[i];
		cn->SetIndex(i);
	}
}
