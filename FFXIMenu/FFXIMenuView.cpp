
// FFXIMenuView.cpp: CFFXIMenuView 类的实现
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "FFXIMenu.h"
#endif

#include "FFXIMenuDoc.h"
#include "FFXIMenuView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFFXIMenuView

IMPLEMENT_DYNCREATE(CFFXIMenuView, CView)

BEGIN_MESSAGE_MAP(CFFXIMenuView, CView)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_VIEW_RESET, OnViewReset)
	ON_COMMAND(ID_EXPORT_IMAGE, OnExportImage)
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

// CFFXIMenuView 构造/析构

CFFXIMenuView::CFFXIMenuView() noexcept
	: m_zoomFactor(1.0)
	, m_offset(0, 0)
	, m_bDragging(FALSE)
	, m_bMemValid(FALSE)
	, m_bMoving(FALSE)
	, m_movingVertex(-1)
{
}

CFFXIMenuView::~CFFXIMenuView()
{
}

BOOL CFFXIMenuView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

// CFFXIMenuView 绘图

void CFFXIMenuView::OnDraw(CDC* pDC)
{
	CFFXIMenuDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	CRect clientRect;
	GetClientRect(&clientRect);

	// 创建/更新内存DC
	if (!m_bMemValid ||
		m_memDC.GetSafeHdc() == NULL ||
		m_memBitmap.GetSafeHandle() == NULL ||
		clientRect.Width() != m_memDC.GetDeviceCaps(HORZRES) ||
		clientRect.Height() != m_memDC.GetDeviceCaps(VERTRES))
	{
		// 释放旧资源
		if (m_memDC.GetSafeHdc()) m_memDC.DeleteDC();
		if (m_memBitmap.GetSafeHandle()) m_memBitmap.DeleteObject();

		// 创建新内存DC
		CDC *pScreenDC = GetDC();
		m_memDC.CreateCompatibleDC(pScreenDC);
		m_memBitmap.CreateCompatibleBitmap(pScreenDC,
			clientRect.Width(), clientRect.Height());
		ReleaseDC(pScreenDC);

		m_memDC.SelectObject(&m_memBitmap);
		m_bMemValid = TRUE;
	}

	// 清空内存DC背景
	m_memDC.FillSolidRect(clientRect, RGB(120, 206, 220));

	// 绘制到内存DC
	if (Gdiplus::Bitmap *pBitmap = pDoc->GetActivatedBitmap())
	{
		/* 没用，精度问题？
		if (m_bMoving)
		{
			CSize picSize(pBitmap->GetHeight() / 2,
				pBitmap->GetWidth() / 2);
			picSize.cx *= m_zoomFactor;
			picSize.cy *= m_zoomFactor;
			CPoint newCentre = m_offset + picSize;

			if (m_lastCentre != newCentre)
			{
				m_offset += newCentre - m_lastCentre;
				m_lastCentre = newCentre;
			}
		}*/

		Gdiplus::Graphics graphics(m_memDC.GetSafeHdc());
		graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

		const int scaledW = static_cast<int>(pBitmap->GetWidth() * m_zoomFactor);
		const int scaledH = static_cast<int>(pBitmap->GetHeight() * m_zoomFactor);

		graphics.DrawImage(pBitmap,
			Gdiplus::RectF(static_cast<Gdiplus::REAL>(m_offset.x),
				static_cast<Gdiplus::REAL>(m_offset.y),
				static_cast<Gdiplus::REAL>(scaledW),
				static_cast<Gdiplus::REAL>(scaledH)),
			0, 0, pBitmap->GetWidth(), pBitmap->GetHeight(),
			Gdiplus::UnitPixel);

		if (pDoc->HasBoundary())
		{
			// 获取四个顶点并转换坐标
			Gdiplus::PointF points[4];
			points[0] = ConvertDocPointToView(pDoc->GetVertex0(), -.5f);
			points[1] = ConvertDocPointToView(pDoc->GetVertex1(), -.5f);
			points[2] = ConvertDocPointToView(pDoc->GetVertex2(), -.5f);
			points[3] = ConvertDocPointToView(pDoc->GetVertex3(), -.5f);

			// 创建红色2像素宽虚线笔
			Gdiplus::Pen pen(Gdiplus::Color(255, 255, 0, 0), 2.0f);
			pen.SetDashStyle(Gdiplus::DashStyleDash);

			// 绘制闭合多边形
			graphics.DrawPolygon(&pen, points, 4);
		}
	}

	// 将内存DC内容拷贝到屏幕DC
	pDC->BitBlt(0, 0,
		clientRect.Width(), clientRect.Height(),
		&m_memDC,
		0, 0,
		SRCCOPY);
}

void CFFXIMenuView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	m_bMemValid = FALSE; // 标记需要重建内存DC
	ClampOffset();
	Invalidate();
}

void CFFXIMenuView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CFFXIMenuView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}

void CFFXIMenuView::OnViewReset()
{
	ResetView();
}

void CFFXIMenuView::OnExportImage()
{
	CFileDialog fileDlg(
		FALSE,                  // FALSE 表示保存对话框 (TRUE 为打开对话框)
		_T("png"),              // 默认扩展名
		_T("untitled.png"),     // 默认文件名
		OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST, // 标志：覆盖提示 + 路径必须存在
		_T("PNG 图像 (*.png)|*.png|所有文件 (*.*)|*.*||"), // 文件过滤器
		this                 // 父窗口
	);

	// 设置对话框标题
	fileDlg.m_ofn.lpstrTitle = _T("导出图片");

	// 显示对话框
	if (fileDlg.DoModal() == IDOK)
	{
		CString path = fileDlg.GetPathName();
		GetDocument()->GetActivatedBitmap()->Save(path.GetString(), &CLSID_WICPngEncoder);
	}
}

void CFFXIMenuView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	// 初始化缩放比例为200%
	m_zoomFactor = 2.0;
	m_offset = CPoint(0, 0);
	m_bDragging = FALSE;

	// 设置映射模式为带缩放功能的MM_ANISOTROPIC
	CClientDC dc(this);
	dc.SetMapMode(MM_ANISOTROPIC);

	ClampOffset();
}

BOOL CFFXIMenuView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// 计算新缩放因子
	double oldZoom = m_zoomFactor;
	m_zoomFactor *= (zDelta > 0) ? 1.1 : 0.9;
	m_zoomFactor = max(0.1, min(20.0, m_zoomFactor));

	// 将鼠标点转换为图像坐标
	ScreenToClient(&pt);
	CPoint imagePt = CPoint(
		static_cast<int>((pt.x - m_offset.x) / oldZoom),
		static_cast<int>((pt.y - m_offset.y) / oldZoom));

	// 调整偏移以保持鼠标位置不变
	m_offset.x = pt.x - imagePt.x * m_zoomFactor;
	m_offset.y = pt.y - imagePt.y * m_zoomFactor;

	ClampOffset();
	Invalidate();
	return TRUE;
}
void CFFXIMenuView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CFFXIMenuDoc *pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	m_ptDragStart = point;

	// 确认拖动对象
	// 检查是否正在拖动节点
	if (pDoc->HasBoundary())
	{
		Gdiplus::PointF points[4];
		points[0] = ConvertDocPointToView(pDoc->GetVertex0(), -.5f);
		points[1] = ConvertDocPointToView(pDoc->GetVertex1(), -.5f);
		points[2] = ConvertDocPointToView(pDoc->GetVertex2(), -.5f);
		points[3] = ConvertDocPointToView(pDoc->GetVertex3(), -.5f);

		BOOL nearPoints = FALSE;
		for (int i = 0; i < 4; ++i)
		{
			Gdiplus::PointF diff = Gdiplus::PointF(point.x, point.y) - points[i];
			if (abs(diff.X) <= 5.0f && abs(diff.Y) <= 5.0f)
			{
				nearPoints = TRUE;
				m_movingVertex = i;
				break;
			}
		}

		if (nearPoints)
		{
			if (m_movingVertex == 0)
				SetCursor(LoadCursor(NULL, IDC_SIZEALL));
			else if (m_movingVertex == 1)
				SetCursor(LoadCursor(NULL, IDC_SIZENESW));
			else if (m_movingVertex == 2)
				SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
			else if (m_movingVertex == 3)
				SetCursor(LoadCursor(NULL, IDC_SIZENESW));
			m_bMoving = TRUE;
			AfxGetMainWnd()->SendMessage(WM_PROP_FREQ_CHANGE_BEGIN_MSG);

			// 计算中心点位置
			CSize picSize(pDoc->GetActivatedBitmap()->GetHeight() / 2,
				pDoc->GetActivatedBitmap()->GetWidth() / 2);
			picSize.cx *= m_zoomFactor;
			picSize.cy *= m_zoomFactor;
			m_lastCentre = m_offset + picSize;
		}
		else
			m_bDragging = TRUE;
	}
	else
	{
		m_bDragging = TRUE;
	}


	SetCapture();
	CView::OnLButtonDown(nFlags, point);
}

void CFFXIMenuView::OnMouseMove(UINT nFlags, CPoint point)
{
	CFFXIMenuDoc *pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// 圖像拖動中
	if (m_bDragging)
	{
		CSize delta = point - m_ptDragStart;
		m_ptDragStart = point;

		// 更新偏移并限制边界
		m_offset += delta;
		ClampOffset();

		Invalidate();
	}
	// 邊界點移動中
	else if (m_bMoving)
	{
		// 转换屏幕坐标到图像坐标
		CPoint dst = point - m_offset;
		dst.x /= m_zoomFactor;
		dst.y /= m_zoomFactor;

		switch (m_movingVertex)
		{
		case 0:
			pDoc->SetVertex0(dst);
			break;
		case 1:
			pDoc->SetVertex1(dst);
			break;
		case 2:
			pDoc->SetVertex2(dst);
			break;
		case 3:
			pDoc->SetVertex3(dst);
			break;
		default:
			break;
		}
	}
	else
	{
		// 普通状态：设置指示
		if (pDoc->HasBoundary())
		{
			Gdiplus::PointF points[4];
			points[0] = ConvertDocPointToView(pDoc->GetVertex0(), -.5f);
			points[1] = ConvertDocPointToView(pDoc->GetVertex1(), -.5f);
			points[2] = ConvertDocPointToView(pDoc->GetVertex2(), -.5f);
			points[3] = ConvertDocPointToView(pDoc->GetVertex3(), -.5f);

			BOOL nearPoints = FALSE;
			for (int i = 0; i < 4; ++i)
			{
				Gdiplus::PointF diff = Gdiplus::PointF(point.x, point.y) - points[i];
				if (abs(diff.X) <= 5.0f && abs(diff.Y) <= 5.0f)
				{
					nearPoints = TRUE;
					m_movingVertex = i;
					break;
				}
			}

			if (nearPoints)
			{
				if (m_movingVertex == 0)
					SetCursor(LoadCursor(NULL, IDC_SIZEALL));
				else if (m_movingVertex == 1)
					SetCursor(LoadCursor(NULL, IDC_SIZENESW));
				else if (m_movingVertex == 2)
					SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
				else if (m_movingVertex == 3)
					SetCursor(LoadCursor(NULL, IDC_SIZENESW));
			}
			else
				SetCursor(LoadCursor(NULL, IDC_ARROW));
		}
	}

	CView::OnMouseMove(nFlags, point);
}

void CFFXIMenuView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bDragging)
	{
		ReleaseCapture();
		m_bDragging = FALSE;
	}
	if (m_bMoving)
	{
		ReleaseCapture();
		m_bMoving = FALSE;
		AfxGetMainWnd()->SendMessage(WM_PROP_FREQ_CHANGE_END_MSG);
	}
	CView::OnLButtonUp(nFlags, point);
}

// 辅助函数：限制偏移范围
void CFFXIMenuView::ClampOffset()
{
	if (m_bMoving) return;

	CRect clientRect;
	GetClientRect(&clientRect);

	// 计算缩放后图像尺寸
	CFFXIMenuDoc *pDoc = GetDocument();
	Gdiplus::Bitmap *pBitmap = pDoc->GetActivatedBitmap();
	if (!pBitmap) return;

	int scaledW = static_cast<int>(pBitmap->GetWidth() * m_zoomFactor);
	int scaledH = static_cast<int>(pBitmap->GetHeight() * m_zoomFactor);

	// 水平限制
	if (scaledW <= clientRect.Width())
		m_offset.x = (clientRect.Width() - scaledW) / 2;
	else
		m_offset.x = max(clientRect.Width() - scaledW, min(0, m_offset.x));

	// 垂直限制
	if (scaledH <= clientRect.Height())
		m_offset.y = (clientRect.Height() - scaledH) / 2;
	else
		m_offset.y = max(clientRect.Height() - scaledH, min(0, m_offset.y));
}

Gdiplus::PointF CFFXIMenuView::ConvertDocPointToView(CPoint docPoint, float tilt) const
{
	float x = (float)docPoint.x;
	float y = (float)docPoint.y;
	if (x > 0.5f) x += tilt;
	if (x < -.5f) x -= tilt;
	if (y > 0.5f) y += tilt;
	if (y < -.5f) y -= tilt;
	return Gdiplus::PointF(
		static_cast<Gdiplus::REAL>(x * m_zoomFactor + m_offset.x),
		static_cast<Gdiplus::REAL>(y * m_zoomFactor + m_offset.y)
	);
}

void CFFXIMenuView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	ResetView();

	CView::OnLButtonDblClk(nFlags, point);
}

void CFFXIMenuView::OnUpdate(CView *pSender, LPARAM lHint, CObject *pHint)
{
	if (lHint == HINT_FOCUS_ITEM_CHANGED)
	{
		ClampOffset();
		Invalidate();
	}
	else if (lHint == HINT_PROPERTIES_CHANGED)
	{
		Invalidate();
	}
}

void CFFXIMenuView::OnDestroy()
{
	CView::OnDestroy();

	if (m_memDC.GetSafeHdc()) m_memDC.DeleteDC();
	if (m_memBitmap.GetSafeHandle()) m_memBitmap.DeleteObject();
}

BOOL CFFXIMenuView::OnEraseBkgnd(CDC *pDC)
{
	return TRUE;
}

void CFFXIMenuView::ResetView()
{
	CRect clientRect;
	GetClientRect(&clientRect);

	CFFXIMenuDoc *pDoc = GetDocument();
	Gdiplus::Bitmap *pBitmap = pDoc->GetActivatedBitmap();

	if (pBitmap)
	{
		if (!m_bMoving)
		{
			m_zoomFactor = 2.0;

			int scaledW = static_cast<int>(pBitmap->GetWidth() * m_zoomFactor);
			int scaledH = static_cast<int>(pBitmap->GetHeight() * m_zoomFactor);

			m_offset.x = (clientRect.Width() - scaledW) / 2;
			m_offset.y = (clientRect.Height() - scaledH) / 2;
		}
	}

	Invalidate();
}


// CFFXIMenuView 诊断

#ifdef _DEBUG
void CFFXIMenuView::AssertValid() const
{
	CView::AssertValid();
}

void CFFXIMenuView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CFFXIMenuDoc* CFFXIMenuView::GetDocument() const // 非调试版本是内联的
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFFXIMenuDoc)));
	return (CFFXIMenuDoc*)m_pDocument;
}
#endif //_DEBUG


// CFFXIMenuView 消息处理程序


void CFFXIMenuView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CFFXIMenuDoc *pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	if (nChar == VK_SPACE)
	{
		if (TileNode *tn = dynamic_cast<TileNode *>(pDoc->focusNode))
		{
			int w = tn->GetRef()->w, h = tn->GetRef()->h;
			auto tl = tn->GetVertex0();
			tn->SetVertex1(xybase::Vec2<int16_t>(tl.x + w, tl.y));
			tn->SetVertex2(xybase::Vec2<int16_t>(tl.x, tl.y + h));
			tn->SetVertex3(xybase::Vec2<int16_t>(tl.x + w, tl.y + h));

			AfxGetMainWnd()->SendMessage(WM_PROPERTIES_CHANGE_MSG);
			Invalidate();
		}
	}
	if (nChar == VK_LEFT || nChar == VK_RIGHT || nChar == VK_UP || nChar == VK_DOWN)
	{
		if (pDoc->HasBoundary())
		{
			auto v0 = pDoc->GetVertex0();
			auto v3 = pDoc->GetVertex2();

			BOOL sizeCtrl = GetAsyncKeyState(VK_CONTROL);
			int movePace = GetAsyncKeyState(VK_SHIFT) ? 5 : 1;
			switch (nChar)
			{
			case VK_LEFT:
				if (sizeCtrl)
					v3.x -= movePace;
				else
					v0.x -= movePace;
				break;
			case VK_RIGHT:
				if (sizeCtrl)
					v3.x += movePace;
				else
					v0.x += movePace;
				break;
			case VK_UP:
				if (sizeCtrl)
					v3.y -= movePace;
				else
					v0.y -= movePace;
				break;
			case VK_DOWN:
				if (sizeCtrl)
					v3.y += movePace;
				else
					v0.y += movePace;
				break;
			default:
				break;
			}

			if (sizeCtrl)
				pDoc->SetVertex2(v3);
			else
				pDoc->SetVertex0(v0);
		}
	}
	if (nChar == VK_HOME)
	{
		ResetView();
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}


BOOL CFFXIMenuView::PreTranslateMessage(MSG *pMsg)
{
	// SendMessage(pMsg->message, pMsg->wParam, pMsg->lParam);

	return CView::PreTranslateMessage(pMsg);
}
