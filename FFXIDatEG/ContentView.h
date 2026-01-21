#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <memory>
#include <variant>

// Forward declaration
class Image;

// Content item types
enum class ContentItemType
{
	SingleLine,
	Multiline
};

// Column data type - supports text, image, or number
enum class ColumnDataType
{
	Text,
	Image,
	Number,
	MultilineText,
	Integer
};

// Column data container
struct ColumnData
{
	ColumnDataType type;
	std::wstring textValue;
	std::shared_ptr<Image> imageValue;  // Shared pointer to Image from FFXIDat
	double numberValue;
	int64_t intValue;
	bool editable;
	
	ColumnData() : type(ColumnDataType::Text), numberValue(0.0), intValue(0), editable(true) {}
	
	// Helper constructors
	static ColumnData MakeText(const std::wstring& text)
	{
		ColumnData data;
		data.type = ColumnDataType::Text;
		data.textValue = text;
		return data;
	}

	static ColumnData MakeMultilineText(const std::wstring& text)
	{
		ColumnData data;
		data.type = ColumnDataType::MultilineText;
		data.textValue = text;
		return data;
	}
	
	static ColumnData MakeImage(std::shared_ptr<Image> image)
	{
		ColumnData data;
		data.type = ColumnDataType::Image;
		data.imageValue = image;
		return data;
	}
	
	static ColumnData MakeNumber(double number)
	{
		ColumnData data;
		data.type = ColumnDataType::Number;
		data.numberValue = number;
		return data;
	}

	static ColumnData MakeInteger(int64_t number)
	{
		ColumnData data;
		data.type = ColumnDataType::Integer;
		data.intValue = number;
		return data;
	}
};

// A single content item (row in the view)
struct ContentItem
{
	int index;
	std::vector<ColumnData> columns;
	int customHeight;  // Custom height for multi-line items (0 = use default)
	std::shared_ptr<Image> image;  // For storing Image object (StatusData icon, etc.)
	ContentItemType type;
	bool editable;
	
	ContentItem() : index(0), type(ContentItemType::SingleLine), customHeight(0), editable(true) {}
	
	~ContentItem()
	{
	}
};

// Custom content view window for displaying text and images
class ContentView
{
public:
	ContentView();
	~ContentView();
	
	bool Create(HWND hParent, int x, int y, int width, int height);
	HWND GetHandle() const { return m_hwnd; }
	
	// Data management
	void Clear();
	void SetColumnCount(int count);
	// Set column title
	void SetColumnTitle(int index, const std::wstring& title);
	// Set column data type (Text, MultilineText, Image, Number, Integer)
	void SetColumnType(int index, ColumnDataType type);
	void SetColumnWidth(int index, int width);
	void AddItem(std::unique_ptr<ContentItem> item);
	
	// Selection
	int GetSelectedIndex() const { return m_selectedIndex; }
	const ContentItem* GetSelectedItem() const;
	
	// Editing
	void BeginEdit(int index, int column);
	void EndEdit(bool save);
	
	// Font management
	void SetFontName(const std::wstring& fontName);
	const std::wstring& GetFontName() const { return m_fontName; }
	void SetFontSize(int fontSize);
	int GetFontSize() const { return m_fontSize; }
	void SetFont(const std::wstring& fontName, int fontSize);
	
	// Search functionality
	bool SearchText(const std::wstring& searchText, bool caseSensitive, bool searchDown, bool findNext);
	void ClearSearch();
	void ScrollToItem(int index);

	bool IsModified() { return m_modified; }
	void SetModified(bool modified) { m_modified = modified; }

	size_t GetItemCount() const { return m_items.size(); }
	const ContentItem* GetItem(size_t index) const
	{
		if (index < m_items.size())
			return m_items[index].get();
		return nullptr;
	}
	
private:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static const wchar_t* CLASS_NAME;
	
	void OnPaint(HDC hdc);
	void OnSize(int width, int height);
	void OnMouseMove(int x, int y);
	void OnLButtonDown(int x, int y);
	void OnLButtonDblClk(int x, int y);
	void OnVScroll(WPARAM wParam);
	void OnHScroll(WPARAM wParam);
	void OnMouseWheel(int delta);
	
	void UpdateScrollBar();
	void UpdateHScrollBar();
	void DrawItem(HDC hdc, int index, int yPos, bool selected);
	void DrawTextItem(HDC hdc, const std::wstring& text, RECT& rect, bool multiline = false);
	void DrawImage(HDC hdc, HBITMAP hBitmap, int width, int height, RECT& rect);
	void DrawImageFromImage(HDC hdc, const Image* image, RECT& rect);  // New: draw from Image object
	
	int HitTest(int x, int y, int* column = nullptr);
	int HitTestColumnResize(int x, int y);  // New: test if mouse is on column divider
	void StartColumnResize(int column, int x);
	void UpdateColumnResize(int x);
	void EndColumnResize();
	int CalculateItemHeight(const ContentItem* item) const;  // New: calculate height for item
	
	HWND m_hwnd;
	HWND m_hEdit;  // Edit control for in-place editing
	
	std::vector<std::unique_ptr<ContentItem>> m_items;
	// Column titles and types. Columns no longer store only a single wstring;
	// each column has a title and an explicit data type so cells can store
	// mixed data (text, image, floating point number, integer, multiline).
	std::vector<std::wstring> m_columnTitles;
	std::vector<ColumnDataType> m_columnTypes;
	std::vector<int> m_columnWidths;
	
	int m_selectedIndex;
	int m_hoveredIndex;
	int m_scrollPos;
	int m_scrollPosH;  // Horizontal scroll position
	int m_itemHeight;
	int m_headerHeight;
	int m_editingIndex;
	int m_editingColumn;
	
	// Column resizing
	bool m_isResizingColumn;
	int m_resizingColumn;
	int m_resizeStartX;
	int m_resizeStartWidth;
	
	// Row height resizing
	bool m_isResizingRow;
	int m_resizingRow;
	int m_resizeStartY;
	int m_resizeStartHeight;
	
	int HitTestRowResize(int x, int y);  // New: test if mouse is on row divider
	void StartRowResize(int row, int y);
	void UpdateRowResize(int y);
	void EndRowResize();
	int GetTotalContentHeight() const;  // New: calculate total height of all items
	int GetItemYPosition(int index) const;  // New: get Y position of an item
	int GetItemIndexAtY(int y) const;  // New: get item index at Y position
	
	HFONT m_hFont;
	HFONT m_hHeaderFont;
	HBRUSH m_hBackBrush;
	HBRUSH m_hSelBrush;
	HBRUSH m_hHoverBrush;
	
	HCURSOR m_hCursorResize;  // Resize cursor
	
	std::wstring m_fontName;  // Current font name
	int m_fontSize;  // Current font size
	
	void RecreateFont();  // Helper to recreate fonts when settings change
	
	// Search state
	int m_searchResultIndex;
	std::wstring m_lastSearchText;
	bool m_lastCaseSensitive;

	bool m_modified;
};
