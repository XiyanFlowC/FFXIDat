#include "ContentView.h"
#include <windowsx.h>
#include <Image.h>

namespace {
	// Helper function to convert LF to CRLF for Windows Edit control
	std::wstring LFtoCRLF(const std::wstring& text) {
		std::wstring result;
		result.reserve(text.length() * 2);
		
		for (size_t i = 0; i < text.length(); ++i) {
			if (text[i] == L'\n') {
				// Check if it's not already CRLF
				if (i == 0 || text[i - 1] != L'\r') {
					result += L'\r';
				}
				result += L'\n';
			}
			else if (text[i] == L'\r') {
				result += L'\r';
				// Ensure LF follows
				if (i + 1 < text.length() && text[i + 1] == L'\n') {
					result += L'\n';
					i++;
				}
				else {
					result += L'\n';
				}
			}
			else {
				result += text[i];
			}
		}
		
		return result;
	}
	
	// Helper function to convert CRLF to LF for internal storage
	std::wstring CRLFtoLF(const std::wstring& text) {
		std::wstring result;
		result.reserve(text.length());
		
		for (size_t i = 0; i < text.length(); ++i) {
			if (text[i] == L'\r') {
				// Skip CR, only keep LF
				if (i + 1 < text.length() && text[i + 1] == L'\n') {
					result += L'\n';
					i++;
				}
				else {
					// Standalone CR, convert to LF
					result += L'\n';
				}
			}
			else {
				result += text[i];
			}
		}
		
		return result;
	}
}

const wchar_t* ContentView::CLASS_NAME = L"FFXIDatEGContentView";

ContentView::ContentView()
	: m_hwnd(nullptr)
	, m_hEdit(nullptr)
	, m_selectedIndex(-1)
	, m_hoveredIndex(-1)
	, m_scrollPos(0)
	, m_scrollPosH(0)
	, m_itemHeight(24)
	, m_headerHeight(24)
	, m_editingIndex(-1)
	, m_editingColumn(-1)
	, m_isResizingColumn(false)
	, m_resizingColumn(-1)
	, m_resizeStartX(0)
	, m_resizeStartWidth(0)
	, m_isResizingRow(false)
	, m_resizingRow(-1)
	, m_resizeStartY(0)
	, m_resizeStartHeight(0)
	, m_hFont(nullptr)
	, m_hHeaderFont(nullptr)
	, m_hBackBrush(nullptr)
	, m_hSelBrush(nullptr)
	, m_hHoverBrush(nullptr)
	, m_hCursorResize(nullptr)
	, m_fontName(L"Yu Gothic UI")
	, m_fontSize(16)
	, m_searchResultIndex(-1)
	, m_lastCaseSensitive(false)
	, m_modified(false)
{
}

// Column configuration
void ContentView::SetColumnCount(int count)
{
	m_columnTitles.resize(count);
	m_columnTypes.resize(count, ColumnDataType::Text);
	m_columnWidths.resize(count, 100);
}

void ContentView::SetColumnTitle(int index, const std::wstring& title)
{
	if (index < 0) return;
	if (index >= (int)m_columnTitles.size())
		m_columnTitles.resize(index + 1);
	m_columnTitles[index] = title;
}

void ContentView::SetColumnType(int index, ColumnDataType type)
{
	if (index < 0) return;
	if (index >= (int)m_columnTypes.size())
		m_columnTypes.resize(index + 1, ColumnDataType::Text);
	m_columnTypes[index] = type;
}

void ContentView::SetColumnWidth(int index, int width)
{
	if (index < 0) return;
	if (index >= (int)m_columnWidths.size())
		m_columnWidths.resize(index + 1, 100);
	m_columnWidths[index] = width;
}

const std::wstring& ContentView::GetColumnTitle(int index) const
{
	static const std::wstring kEmpty;
	if (index < 0 || index >= static_cast<int>(m_columnTitles.size()))
		return kEmpty;
	return m_columnTitles[index];
}

ColumnDataType ContentView::GetColumnType(int index) const
{
	if (index < 0 || index >= static_cast<int>(m_columnTypes.size()))
		return ColumnDataType::Text;
	return m_columnTypes[index];
}

ContentView::~ContentView()
{
	if (m_hFont) DeleteObject(m_hFont);
	if (m_hHeaderFont) DeleteObject(m_hHeaderFont);
	if (m_hBackBrush) DeleteObject(m_hBackBrush);
	if (m_hSelBrush) DeleteObject(m_hSelBrush);
	if (m_hHoverBrush) DeleteObject(m_hHoverBrush);
	if (m_hCursorResize) DestroyCursor(m_hCursorResize);
}

bool ContentView::Create(HWND hParent, int x, int y, int width, int height)
{
	// Register window class
	WNDCLASSEXW wc = { 0 };
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = CLASS_NAME;
	
	static bool registered = false;
	if (!registered)
	{
		if (!RegisterClassExW(&wc))
			return false;
		registered = true;
	}
	
	// Create window
	m_hwnd = CreateWindowExW(
		WS_EX_CLIENTEDGE,
		CLASS_NAME,
		nullptr,
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
		x, y, width, height,
		hParent,
		nullptr,
		GetModuleHandle(nullptr),
		this
	);
	
	if (!m_hwnd)
		return false;
	
	// Create fonts - use Shift-JIS compatible charset for Japanese text
	m_hFont = CreateFontW(m_fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
						 SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
						 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, m_fontName.c_str());
	
	m_hHeaderFont = CreateFontW(m_fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
							   SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
							   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, m_fontName.c_str());
	
	// Create brushes
	m_hBackBrush = CreateSolidBrush(RGB(255, 255, 255));
	m_hSelBrush = CreateSolidBrush(RGB(51, 153, 255));
	m_hHoverBrush = CreateSolidBrush(RGB(229, 243, 255));
	
	// Load resize cursor
	m_hCursorResize = LoadCursor(nullptr, IDC_SIZEWE);
	
	// Ensure at least one column exists by default
	if (m_columnTitles.empty()) {
		m_columnTitles.push_back(L"(Empty)");
		m_columnTypes.push_back(ColumnDataType::Text);
		m_columnWidths.push_back(200);
	}
	
	return true;
}

LRESULT CALLBACK ContentView::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ContentView* pThis = nullptr;
	
	if (uMsg == WM_NCCREATE)
	{
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		pThis = reinterpret_cast<ContentView*>(pCreate->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
		pThis->m_hwnd = hwnd;
	}
	else
	{
		pThis = reinterpret_cast<ContentView*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}
	
	if (pThis)
	{
		switch (uMsg)
		{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			pThis->OnPaint(hdc);
			EndPaint(hwnd, &ps);
			return 0;
		}
		
		case WM_SIZE:
			pThis->OnSize(LOWORD(lParam), HIWORD(lParam));
			return 0;
		
		case WM_MOUSEMOVE:
			pThis->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		
		case WM_LBUTTONDOWN:
			pThis->OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		
		case WM_LBUTTONUP:
			if (pThis->m_isResizingColumn)
			{
				pThis->EndColumnResize();
				ReleaseCapture();
			}
			else if (pThis->m_isResizingRow)
			{
				pThis->EndRowResize();
				ReleaseCapture();
			}
			return 0;
		
		case WM_LBUTTONDBLCLK:
			pThis->OnLButtonDblClk(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		
		case WM_SETCURSOR:
		{
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(hwnd, &pt);
			
			if (pThis->HitTestColumnResize(pt.x, pt.y) >= 0)
			{
				SetCursor(pThis->m_hCursorResize);
				return TRUE;
			}
			else if (pThis->HitTestRowResize(pt.x, pt.y) >= 0)
			{
				SetCursor(LoadCursor(nullptr, IDC_SIZENS));
				return TRUE;
			}
			break;
		}
		
		case WM_VSCROLL:
			pThis->OnVScroll(wParam);
			return 0;
		
		case WM_HSCROLL:
			pThis->OnHScroll(wParam);
			return 0;
		
		case WM_MOUSEWHEEL:
			pThis->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
			return 0;
		
		case WM_ERASEBKGND:
			return 1;  // We handle background in OnPaint
		}
	}
	
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void ContentView::Clear()
{
	EndEdit(false);
	m_items.clear();
	m_selectedIndex = -1;
	m_hoveredIndex = -1;
	m_scrollPos = 0;
	m_scrollPosH = 0;
	m_modified = false;
	UpdateScrollBar();
	UpdateHScrollBar();
	InvalidateRect(m_hwnd, nullptr, FALSE);
}

void ContentView::AddItem(std::unique_ptr<ContentItem> item)
{
	item->index = static_cast<int>(m_items.size());
	m_items.push_back(std::move(item));
	UpdateScrollBar();
	UpdateHScrollBar();
	InvalidateRect(m_hwnd, nullptr, FALSE);
}

const ContentItem* ContentView::GetSelectedItem() const
{
	if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size()))
		return m_items[m_selectedIndex].get();
	return nullptr;
}

bool ContentView::SetCellValue(size_t row, size_t column, const std::wstring& value)
{
	if (row >= m_items.size())
		return false;
	auto& item = m_items[row];
	if (!item->editable)
		return false;
	if (column >= item->columns.size())
		return false;
	auto& cell = item->columns[column];
	if (!cell.editable || cell.type == ColumnDataType::Image)
		return false;

	bool changed = false;
	try
	{
		switch (cell.type)
		{
		case ColumnDataType::Text:
		case ColumnDataType::MultilineText:
			if (cell.textValue != value)
			{
				cell.textValue = value;
				changed = true;
			}
			break;
		case ColumnDataType::Integer:
		{
			int64_t newValue = value.empty() ? 0 : std::stoll(value);
			if (cell.intValue != newValue)
			{
				cell.intValue = newValue;
				changed = true;
			}
			break;
		}
		case ColumnDataType::Number:
		{
			double newValue = value.empty() ? 0.0 : std::stod(value);
			if (cell.numberValue != newValue)
			{
				cell.numberValue = newValue;
				changed = true;
			}
			break;
		}
		case ColumnDataType::Image:
			return false;
		}
	}
	catch (...)
	{
		return false;
	}

	if (changed)
	{
		m_modified = true;
		InvalidateRect(m_hwnd, nullptr, FALSE);
	}

	return changed;
}

void ContentView::BeginEdit(int index, int column)
{
	if (index < 0 || index >= static_cast<int>(m_items.size()))
		return;
	
	EndEdit(false);  // End any existing edit

	if (m_items[index]->editable == false)
	{
		// Item is not editable
		return;
	}

	if (m_items[index]->columns[column].editable == false)
	{
		// Column is not editable
		return;
	}

	if (m_items[index]->columns[column].type != ColumnDataType::Text &&
		m_items[index]->columns[column].type != ColumnDataType::MultilineText)
	{
		// Only text and multiline text items are editable
		return;
	}
	
	m_editingIndex = index;
	m_editingColumn = column;
	
	const ContentItem* item = m_items[index].get();
	bool isMultiline = (item->type == ContentItemType::Multiline);
	
	// Calculate edit rect
	RECT rcClient;
	GetClientRect(m_hwnd, &rcClient);
	
	// Calculate Y position using pixel-based positioning
	int itemYPos = GetItemYPosition(index);
	int yPos = m_headerHeight + itemYPos - m_scrollPos;
	
	int xPos = -m_scrollPosH;
	for (int i = 0; i < column && i < static_cast<int>(m_columnWidths.size()); ++i)
	{
		xPos += m_columnWidths[i];
	}
	
	int width = column < static_cast<int>(m_columnWidths.size()) ? 
				m_columnWidths[column] : 150;
	
	int height = CalculateItemHeight(item);
	
	// Create edit control with appropriate style
	DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER;
	if (isMultiline)
	{
		style |= ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN;
	}
	else
	{
		style |= ES_AUTOHSCROLL;
	}
	
	m_hEdit = CreateWindowExW(
		0, L"EDIT", L"",
		style,
		xPos, yPos, width, height,
		m_hwnd, nullptr, GetModuleHandle(nullptr), nullptr
	);
	
	if (m_hEdit)
	{
		SendMessage(m_hEdit, WM_SETFONT, (WPARAM)m_hFont, TRUE);
		
		// Set initial text with CRLF for Windows Edit control
		if (column < static_cast<int>(m_items[index]->columns.size()))
		{
			std::wstring editText = LFtoCRLF(m_items[index]->columns[column].textValue);
			SetWindowTextW(m_hEdit, editText.c_str());
		}
		
		SetFocus(m_hEdit);
		SendMessage(m_hEdit, EM_SETSEL, 0, -1);
	}
}

void ContentView::EndEdit(bool save)
{
	if (!m_hEdit)
		return;
	
	if (save && m_editingIndex >= 0 && m_editingIndex < static_cast<int>(m_items.size()))
	{
		// Get text length and allocate buffer
		int textLen = GetWindowTextLengthW(m_hEdit);
		std::unique_ptr<wchar_t[]> text(new wchar_t[textLen + 1]);
		GetWindowTextW(m_hEdit, text.get(), textLen + 1);
		
		if (m_editingColumn < static_cast<int>(m_items[m_editingIndex]->columns.size()))
		{
			// Convert CRLF to LF for internal storage
			std::wstring internalText = CRLFtoLF(text.get());
			m_modified = m_items[m_editingIndex]->columns[m_editingColumn].textValue != internalText;
			m_items[m_editingIndex]->columns[m_editingColumn].textValue = internalText;
			
			// Update item height if it's multiline text
			ContentItem* item = m_items[m_editingIndex].get();
			if (false && item->type == ContentItemType::Multiline)
			{
				// Recalculate line count
				int lineCount = 1;
				for (wchar_t ch : internalText)
				{
					if (ch == L'\n')
						lineCount++;
				}
				item->customHeight = lineCount * 24;
				UpdateScrollBar();
			}
		}
	}
	
	DestroyWindow(m_hEdit);
	m_hEdit = nullptr;
	m_editingIndex = -1;
	m_editingColumn = -1;
	
	InvalidateRect(m_hwnd, nullptr, FALSE);
}

void ContentView::OnPaint(HDC hdc)
{
	RECT rcClient;
	GetClientRect(m_hwnd, &rcClient);
	
	// Create back buffer
	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rcClient.right, rcClient.bottom);
	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
	
	// Fill background
	FillRect(hdcMem, &rcClient, m_hBackBrush);
	
	// Draw header
	RECT rcHeader = { 0, 0, rcClient.right, m_headerHeight };
	FillRect(hdcMem, &rcHeader, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
	
	SelectObject(hdcMem, m_hHeaderFont);
	SetBkMode(hdcMem, TRANSPARENT);
	
	int xPos = -m_scrollPosH;  // Apply horizontal offset
	HPEN hPenDivider = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
	HPEN hOldPen = (HPEN)SelectObject(hdcMem, hPenDivider);
	
	for (size_t i = 0; i < m_columnTitles.size(); ++i)
	{
		RECT rcCol = { xPos + 4, 0, xPos + m_columnWidths[i] - 4, m_headerHeight };
		::DrawTextW(hdcMem, m_columnTitles[i].c_str(), -1, &rcCol,
				 DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		
		xPos += m_columnWidths[i];
		
		// Draw column divider line (including after last column)
		MoveToEx(hdcMem, xPos, 0, nullptr);
		LineTo(hdcMem, xPos, rcClient.bottom);
	}
	
	SelectObject(hdcMem, hOldPen);
	DeleteObject(hPenDivider);
	
	// Draw items
	SelectObject(hdcMem, m_hFont);
	
	int yPos = m_headerHeight;
	int currentPixel = 0;
	
	for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
	{
		int itemHeight = CalculateItemHeight(m_items[i].get());
		
		// Check if this item is in the visible range
		if (currentPixel + itemHeight > m_scrollPos && currentPixel < m_scrollPos + (rcClient.bottom - m_headerHeight))
		{
			int drawY = yPos + (currentPixel - m_scrollPos);
			
			if (drawY < rcClient.bottom)
			{
				bool selected = (i == m_selectedIndex);
				bool hovered = (i == m_hoveredIndex);
				
				DrawItem(hdcMem, i, drawY, selected || hovered);
			}
		}
		
		currentPixel += itemHeight;
		
		if (currentPixel - m_scrollPos >= rcClient.bottom)
			break;
	}
	
	// Copy back buffer to screen
	BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, hdcMem, 0, 0, SRCCOPY);
	
	SelectObject(hdcMem, hbmOld);
	DeleteObject(hbmMem);
	DeleteDC(hdcMem);
}

void ContentView::DrawItem(HDC hdc, int index, int yPos, bool selected)
{
	if (index < 0 || index >= static_cast<int>(m_items.size()))
		return;
	
	const ContentItem* item = m_items[index].get();
	int itemHeight = CalculateItemHeight(item);
	
	RECT rcClient;
	GetClientRect(m_hwnd, &rcClient);
	
	RECT rcItem = { 0, yPos, rcClient.right, yPos + itemHeight };
	
	// Draw selection/hover background
	if (selected)
	{
		FillRect(hdc, &rcItem, index == m_selectedIndex ? m_hSelBrush : m_hHoverBrush);
		SetTextColor(hdc, index == m_selectedIndex ? RGB(255, 255, 255) : RGB(0, 0, 0));
	}
	else
	{
		SetTextColor(hdc, RGB(0, 0, 0));
	}
	
	int xPos = -m_scrollPosH;  // Apply horizontal offset
	bool isMultiline = (item->type == ContentItemType::Multiline);
	
	for (size_t i = 0; i < item->columns.size() && i < m_columnWidths.size(); ++i)
	{
		RECT rcCol = { xPos + 2, yPos + 2, xPos + m_columnWidths[i] - 2, yPos + itemHeight - 2 };
			
		const ColumnData& colData = item->columns[i];
		switch (colData.type)
		{
		case ColumnDataType::Text:
			DrawTextItem(hdc, colData.textValue, rcCol, isMultiline);
			break;

		case ColumnDataType::MultilineText:
			DrawTextItem(hdc, colData.textValue, rcCol, true);
			break;
				
		case ColumnDataType::Image:
			if (colData.imageValue)
			{
				DrawImageFromImage(hdc, colData.imageValue.get(), rcCol);
			}
			break;
				
		case ColumnDataType::Number:
		{
			wchar_t numStr[64];
			swprintf_s(numStr, L"%.2f", colData.numberValue);
			DrawTextItem(hdc, numStr, rcCol, false);
			break;
		}

		case ColumnDataType::Integer:
		{
			wchar_t intStr[64];
			swprintf_s(intStr, L"%lld", colData.intValue);
			DrawTextItem(hdc, intStr, rcCol, false);
			break;
		}

		}
			
		xPos += m_columnWidths[i];
	}
}

void ContentView::DrawTextItem(HDC hdc, const std::wstring& text, RECT& rect, bool multiline)
{
	if (multiline)
	{
		::DrawTextW(hdc, text.c_str(), -1, &rect, 
				 DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS);
	}
	else
	{
		::DrawTextW(hdc, text.c_str(), -1, &rect, 
				 DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
	}
}

void ContentView::DrawImage(HDC hdc, HBITMAP hBitmap, int width, int height, RECT& rect)
{
	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hBitmap);
	
	int dstWidth = rect.right - rect.left;
	int dstHeight = rect.bottom - rect.top;
	
	// Scale to fit
	float scale = min((float)dstWidth / width, (float)dstHeight / height);
	int scaledWidth = (int)(width * scale);
	int scaledHeight = (int)(height * scale);
	
	int xOffset = rect.left + (dstWidth - scaledWidth) / 2;
	int yOffset = rect.top + (dstHeight - scaledHeight) / 2;
	
	SetStretchBltMode(hdc, HALFTONE);
	StretchBlt(hdc, xOffset, yOffset, scaledWidth, scaledHeight,
			  hdcMem, 0, 0, width, height, SRCCOPY);
	
	SelectObject(hdcMem, hbmOld);
	DeleteDC(hdcMem);
}

void ContentView::DrawImageFromImage(HDC hdc, const Image* image, RECT& rect)
{
	if (!image || !image->texture)
		return;
	
	// Get image dimensions
	int width = image->GetWidth();
	int height = image->GetHeight();
	
	if (width <= 0 || height <= 0)
		return;
	
	// Convert Image to HBITMAP for drawing
	// Get bitmap data from Image
	std::unique_ptr<char[]> bitmapData = image->GetBitmap();
	if (!bitmapData)
		return;
	
	// Create DIB from bitmap data
	BITMAPFILEHEADER* fileHeader = reinterpret_cast<BITMAPFILEHEADER*>(bitmapData.get());
	BITMAPINFOHEADER* infoHeader = reinterpret_cast<BITMAPINFOHEADER*>(bitmapData.get() + sizeof(BITMAPFILEHEADER));
	
	// Calculate data offset
	char* pixelData = bitmapData.get() + fileHeader->bfOffBits;
	
	// Create compatible DC and bitmap
	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hBitmap = CreateDIBitmap(hdc, infoHeader, CBM_INIT, pixelData, 
									 reinterpret_cast<BITMAPINFO*>(infoHeader), DIB_RGB_COLORS);
	
	if (hBitmap)
	{
		HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
		
		int dstWidth = rect.right - rect.left;
		int dstHeight = rect.bottom - rect.top;
		
		// Scale to fit while maintaining aspect ratio
		float scale = min((float)dstWidth / width, (float)dstHeight / height);
		int scaledWidth = (int)(width * scale);
		int scaledHeight = (int)(height * scale);
		
		int xOffset = rect.left + (dstWidth - scaledWidth) / 2;
		int yOffset = rect.top + (dstHeight - scaledHeight) / 2;
		
		SetStretchBltMode(hdc, HALFTONE);
		StretchBlt(hdc, xOffset, yOffset, scaledWidth, scaledHeight,
				  hdcMem, 0, 0, width, height, SRCCOPY);
		
		SelectObject(hdcMem, hOldBitmap);
		DeleteObject(hBitmap);
	}
	
	DeleteDC(hdcMem);
}

void ContentView::OnSize(int width, int height)
{
	UpdateScrollBar();
	UpdateHScrollBar();
	InvalidateRect(m_hwnd, nullptr, FALSE);
}

void ContentView::OnMouseMove(int x, int y)
{
	if (m_isResizingColumn)
	{
		UpdateColumnResize(x);
		return;
	}
	
	if (m_isResizingRow)
	{
		UpdateRowResize(y);
		return;
	}
	
	int oldHovered = m_hoveredIndex;
	m_hoveredIndex = HitTest(x, y);
	
	if (oldHovered != m_hoveredIndex)
	{
		InvalidateRect(m_hwnd, nullptr, FALSE);
	}
}

void ContentView::OnLButtonDown(int x, int y)
{
	// Check if clicking on column divider
	int column = HitTestColumnResize(x, y);
	if (column >= 0)
	{
		StartColumnResize(column, x);
		SetCapture(m_hwnd);
		return;
	}
	
	// Check if clicking on row divider
	int row = HitTestRowResize(x, y);
	if (row >= 0)
	{
		StartRowResize(row, y);
		SetCapture(m_hwnd);
		return;
	}
	
	int oldSelected = m_selectedIndex;
	m_selectedIndex = HitTest(x, y);
	
	if (oldSelected != m_selectedIndex)
	{
		EndEdit(true);
		InvalidateRect(m_hwnd, nullptr, FALSE);
	}
}

void ContentView::OnLButtonDblClk(int x, int y)
{
	int column = -1;
	int index = HitTest(x, y, &column);
	
	if (index >= 0 && column >= 0)
	{
		BeginEdit(index, column);
	}
}

void ContentView::OnVScroll(WPARAM wParam)
{
	int oldPos = m_scrollPos;
	
	RECT rcClient;
	GetClientRect(m_hwnd, &rcClient);
	int visibleHeight = rcClient.bottom - m_headerHeight;
	int totalHeight = GetTotalContentHeight();
	int maxScroll = max(0, totalHeight - visibleHeight);
	
	switch (LOWORD(wParam))
	{
	case SB_LINEUP:
		m_scrollPos -= m_itemHeight;
		break;
	case SB_LINEDOWN:
		m_scrollPos += m_itemHeight;
		break;
	case SB_PAGEUP:
		m_scrollPos -= visibleHeight;
		break;
	case SB_PAGEDOWN:
		m_scrollPos += visibleHeight;
		break;
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
	{
		// Use GetScrollInfo to get full 32-bit position instead of HIWORD which is limited to 16-bit
		SCROLLINFO si = { 0 };
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_TRACKPOS;
		GetScrollInfo(m_hwnd, SB_VERT, &si);
		m_scrollPos = si.nTrackPos;
		break;
	}
	}
	
	m_scrollPos = max(0, min(m_scrollPos, maxScroll));
	
	if (oldPos != m_scrollPos)
	{
		SetScrollPos(m_hwnd, SB_VERT, m_scrollPos, TRUE);
		InvalidateRect(m_hwnd, nullptr, FALSE);
	}
}

void ContentView::OnMouseWheel(int delta)
{
	int lines = delta / WHEEL_DELTA * 3;
	int scrollAmount = lines * m_itemHeight;
	
	m_scrollPos -= scrollAmount;
	
	RECT rcClient;
	GetClientRect(m_hwnd, &rcClient);
	int visibleHeight = rcClient.bottom - m_headerHeight;
	int totalHeight = GetTotalContentHeight();
	int maxScroll = max(0, totalHeight - visibleHeight);
	
	m_scrollPos = max(0, min(m_scrollPos, maxScroll));
	
	SetScrollPos(m_hwnd, SB_VERT, m_scrollPos, TRUE);
	InvalidateRect(m_hwnd, nullptr, FALSE);
}

void ContentView::UpdateScrollBar()
{
	RECT rcClient;
	GetClientRect(m_hwnd, &rcClient);
	
	int visibleHeight = rcClient.bottom - m_headerHeight;
	int totalHeight = GetTotalContentHeight();
	
	SCROLLINFO si = { 0 };
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	si.nMin = 0;
	si.nMax = max(0, totalHeight - 1);
	si.nPage = visibleHeight;
	si.nPos = m_scrollPos;
	
	SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
}

int ContentView::HitTest(int x, int y, int* column)
{
	if (y < m_headerHeight)
		return -1;
	
	int relativeY = y - m_headerHeight + m_scrollPos;
	int index = GetItemIndexAtY(relativeY);
	
	if (index >= 0 && column)
	{
		int xPos = -m_scrollPosH;
		*column = -1;
		
		for (size_t j = 0; j < m_columnWidths.size(); ++j)
		{
			if (x >= xPos && x < xPos + m_columnWidths[j])
			{
				*column = static_cast<int>(j);
				break;
			}
			xPos += m_columnWidths[j];
		}
	}
	
	return index;
}

int ContentView::HitTestColumnResize(int x, int y)
{
	if (y >= m_headerHeight)
		return -1;
	
	int xPos = -m_scrollPosH;  // Apply horizontal offset
	const int resizeMargin = 3;  // Pixels on each side of divider
	
	for (size_t i = 0; i < m_columnWidths.size(); ++i)
	{
		xPos += m_columnWidths[i];
		
		if (x >= xPos - resizeMargin && x <= xPos + resizeMargin)
		{
			return static_cast<int>(i);
		}
	}
	
	return -1;
}

int ContentView::HitTestRowResize(int x, int y)
{
	if (y < m_headerHeight)
		return -1;
	
	const int resizeMargin = 3;
	int relativeY = y - m_headerHeight + m_scrollPos;
	int cumHeight = 0;
	
	for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
	{
		int itemHeight = CalculateItemHeight(m_items[i].get());
		cumHeight += itemHeight;
		
		// Check if mouse is near the bottom edge of this item
		if (relativeY >= cumHeight - resizeMargin && relativeY <= cumHeight + resizeMargin)
		{
			return i;
		}
	}
	
	return -1;
}

void ContentView::StartColumnResize(int column, int x)
{
	m_isResizingColumn = true;
	m_resizingColumn = column;
	m_resizeStartX = x;
	m_resizeStartWidth = m_columnWidths[column];
}

void ContentView::UpdateColumnResize(int x)
{
	if (!m_isResizingColumn || m_resizingColumn < 0)
		return;
	
	int delta = x - m_resizeStartX;
	int newWidth = m_resizeStartWidth + delta;
	
	// Minimum column width
	if (newWidth < 30)
		newWidth = 30;
	
	m_columnWidths[m_resizingColumn] = newWidth;
	UpdateHScrollBar();  // Update horizontal scrollbar when column width changes
	InvalidateRect(m_hwnd, nullptr, FALSE);
}

void ContentView::EndColumnResize()
{
	m_isResizingColumn = false;
	m_resizingColumn = -1;
}

void ContentView::StartRowResize(int row, int y)
{
	m_isResizingRow = true;
	m_resizingRow = row;
	m_resizeStartY = y;
	m_resizeStartHeight = m_items[row]->customHeight > 0 ? 
						  m_items[row]->customHeight : m_itemHeight;
}

void ContentView::UpdateRowResize(int y)
{
	if (!m_isResizingRow || m_resizingRow < 0)
		return;
	
	int delta = y - m_resizeStartY;
	int newHeight = m_resizeStartHeight + delta;
	
	// Minimum row height
	if (newHeight < 20)
		newHeight = 20;
	
	m_items[m_resizingRow]->customHeight = newHeight;
	UpdateScrollBar();
	InvalidateRect(m_hwnd, nullptr, FALSE);
}

void ContentView::EndRowResize()
{
	m_isResizingRow = false;
	m_resizingRow = -1;
}

int ContentView::CalculateItemHeight(const ContentItem* item) const
{
	if (!item)
		return m_itemHeight;
	
	if (item->customHeight > 0)
		return item->customHeight;
	
	return m_itemHeight;
}

void ContentView::OnHScroll(WPARAM wParam)
{
	int oldPos = m_scrollPosH;
	
	switch (LOWORD(wParam))
	{
	case SB_LINELEFT:
		m_scrollPosH -= 20;
		break;
	case SB_LINERIGHT:
		m_scrollPosH += 20;
		break;
	case SB_PAGELEFT:
		m_scrollPosH -= 100;
		break;
	case SB_PAGERIGHT:
		m_scrollPosH += 100;
		break;
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
		m_scrollPosH = HIWORD(wParam);
		break;
	}
	
	// Calculate total width and max scroll
	int totalWidth = 0;
	for (int w : m_columnWidths)
		totalWidth += w;
	
	RECT rcClient;
	GetClientRect(m_hwnd, &rcClient);
	int maxScroll = max(0, totalWidth - rcClient.right);
	
	m_scrollPosH = max(0, min(m_scrollPosH, maxScroll));
	
	if (oldPos != m_scrollPosH)
	{
		SetScrollPos(m_hwnd, SB_HORZ, m_scrollPosH, TRUE);
		InvalidateRect(m_hwnd, nullptr, FALSE);
	}
}

void ContentView::UpdateHScrollBar()
{
	// Calculate total width
	int totalWidth = 0;
	for (int w : m_columnWidths)
		totalWidth += w;
	
	RECT rcClient;
	GetClientRect(m_hwnd, &rcClient);
	
	SCROLLINFO si = { 0 };
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	si.nMin = 0;
	si.nMax = max(0, totalWidth - 1);
	si.nPage = rcClient.right;
	si.nPos = m_scrollPosH;
	
	SetScrollInfo(m_hwnd, SB_HORZ, &si, TRUE);
}

void ContentView::SetFontName(const std::wstring& fontName)
{
	if (m_fontName == fontName)
		return;
	
	m_fontName = fontName;
	RecreateFont();
}

void ContentView::SetFontSize(int fontSize)
{
	if (m_fontSize == fontSize || fontSize < 8 || fontSize > 72)
		return;
	
	m_fontSize = fontSize;
	m_itemHeight = fontSize + 8;  // Adjust item height based on font size
	m_headerHeight = fontSize + 8;
	RecreateFont();
}

void ContentView::SetFont(const std::wstring& fontName, int fontSize)
{
	bool changed = false;
	
	if (m_fontName != fontName)
	{
		m_fontName = fontName;
		changed = true;
	}
	
	if (m_fontSize != fontSize && fontSize >= 8 && fontSize <= 72)
	{
		m_fontSize = fontSize;
		m_itemHeight = fontSize + 8;
		m_headerHeight = fontSize + 8;
		changed = true;
	}
	
	if (changed)
		RecreateFont();
}

void ContentView::RecreateFont()
{
	// Recreate fonts with new settings
	if (m_hFont)
		DeleteObject(m_hFont);
	if (m_hHeaderFont)
		DeleteObject(m_hHeaderFont);
	
	m_hFont = CreateFontW(m_fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
						 SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
						 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, m_fontName.c_str());
	
	m_hHeaderFont = CreateFontW(m_fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
							   SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
							   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, m_fontName.c_str());
	
	// Redraw
	InvalidateRect(m_hwnd, nullptr, FALSE);
}

bool ContentView::SearchText(const std::wstring& searchText, bool caseSensitive, bool searchDown, bool findNext)
{
	if (searchText.empty() || m_items.empty())
		return false;
	
	// Determine starting position
	int startIndex = 0;
	if (findNext && !m_lastSearchText.empty() && m_lastSearchText == searchText && m_lastCaseSensitive == caseSensitive)
	{
		// Continue from last result
		startIndex = m_searchResultIndex >= 0 ? m_searchResultIndex : 0;
		if (searchDown)
			startIndex++;
		else
			startIndex--;
	}
	else
	{
		// New search, start from selection or beginning
		startIndex = m_selectedIndex >= 0 ? m_selectedIndex : 0;
	}
	
	// Store search parameters
	m_lastSearchText = searchText;
	m_lastCaseSensitive = caseSensitive;
	
	// Prepare search text
	std::wstring searchLower = searchText;
	if (!caseSensitive)
	{
		for (wchar_t& ch : searchLower)
			ch = towlower(ch);
	}
	
	// Search through items
	int itemCount = static_cast<int>(m_items.size());
	for (int i = 0; i < itemCount; ++i)
	{
		int index = searchDown ? 
					(startIndex + i) % itemCount : 
					(startIndex - i + itemCount) % itemCount;
		
		const ContentItem* item = m_items[index].get();
		
		// Search in all columns
		for (const auto& col : item->columns)
		{
			if (col.type != ColumnDataType::Text)
				continue;

			std::wstring searchIn = col.textValue;
			if (!caseSensitive)
			{
				for (wchar_t& ch : searchIn)
					ch = towlower(ch);
			}
			
			if (searchIn.find(searchLower) != std::wstring::npos)
			{
				// Found!
				m_searchResultIndex = index;
				m_selectedIndex = index;
				ScrollToItem(index);
				InvalidateRect(m_hwnd, nullptr, FALSE);
				return true;
			}
		}
	}
	
	// Not found
	m_searchResultIndex = -1;
	return false;
}

void ContentView::ClearSearch()
{
	m_searchResultIndex = -1;
	m_lastSearchText.clear();
	InvalidateRect(m_hwnd, nullptr, FALSE);
}

void ContentView::ScrollToItem(int index)
{
	if (index < 0 || index >= static_cast<int>(m_items.size()))
		return;
	
	RECT rcClient;
	GetClientRect(m_hwnd, &rcClient);
	int visibleHeight = rcClient.bottom - m_headerHeight;
	
	int itemYPos = GetItemYPosition(index);
	int itemHeight = CalculateItemHeight(m_items[index].get());
	
	// Check if item is already visible
	if (itemYPos >= m_scrollPos && itemYPos + itemHeight <= m_scrollPos + visibleHeight)
		return;  // Already visible
	
	// Try to center the item
	int targetScroll = itemYPos - (visibleHeight - itemHeight) / 2;
	
	int totalHeight = GetTotalContentHeight();
	int maxScroll = max(0, totalHeight - visibleHeight);
	
	m_scrollPos = max(0, min(targetScroll, maxScroll));
	
	SetScrollPos(m_hwnd, SB_VERT, m_scrollPos, TRUE);
	InvalidateRect(m_hwnd, nullptr, FALSE);
}

int ContentView::GetTotalContentHeight() const
{
	int totalHeight = 0;
	for (const auto& item : m_items)
	{
		totalHeight += CalculateItemHeight(item.get());
	}
	return totalHeight;
}

int ContentView::GetItemYPosition(int index) const
{
	if (index < 0 || index >= static_cast<int>(m_items.size()))
		return 0;
	
	int yPos = 0;
	for (int i = 0; i < index; ++i)
	{
		yPos += CalculateItemHeight(m_items[i].get());
	}
	return yPos;
}

int ContentView::GetItemIndexAtY(int yPixel) const
{
	int cumHeight = 0;
	for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
	{
		int itemHeight = CalculateItemHeight(m_items[i].get());
		if (yPixel >= cumHeight && yPixel < cumHeight + itemHeight)
			return i;
		cumHeight += itemHeight;
	}
	return -1;
}
