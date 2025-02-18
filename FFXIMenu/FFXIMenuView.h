
// FFXIMenuView.h: CFFXIMenuView 类的接口
//

#pragma once


class CFFXIMenuView : public CView
{
protected: // 仅从序列化创建
	CFFXIMenuView() noexcept;
	DECLARE_DYNCREATE(CFFXIMenuView)

// 特性
public:
	CFFXIMenuDoc* GetDocument() const;

// 操作
public:

// 重写
public:
	virtual void OnDraw(CDC* pDC);  // 重写以绘制该视图
	virtual void OnSize(UINT nType, int cx, int cy);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:

// 实现
public:
	virtual ~CFFXIMenuView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成的消息映射函数
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnViewReset();
	afx_msg void OnExportImage();

	virtual void OnInitialUpdate();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);

	afx_msg void OnUpdate(CView *pSender, LPARAM lHint, CObject *pHint);
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);

	DECLARE_MESSAGE_MAP()
private:
	double m_zoomFactor;    // 当前缩放因子
	CPoint m_offset;        // 图像偏移量
	CPoint m_ptDragStart;   // 拖动起始点
	BOOL m_bDragging;       // 是否正在拖动
	BOOL m_bMoving;         // 是否正在移动边界节点
	int m_movingVertex;     // 正在移动的顶点
	CSize m_totalSize;      // 缩放后的图像尺寸
	CDC m_memDC;         // 内存DC
	CBitmap m_memBitmap; // 双缓冲位图
	BOOL m_bMemValid;    // 内存位图是否有效
	CPoint m_lastCentre;  // 中心点位置

	void ResetView();
	void ClampOffset();
	Gdiplus::PointF ConvertDocPointToView(CPoint docPoint, float tilt = 0.0f) const;
public:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual BOOL PreTranslateMessage(MSG *pMsg);
};

#ifndef _DEBUG  // FFXIMenuView.cpp 中的调试版本
inline CFFXIMenuDoc* CFFXIMenuView::GetDocument() const
   { return reinterpret_cast<CFFXIMenuDoc*>(m_pDocument); }
#endif

