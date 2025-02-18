// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

// 添加要在此处预编译的标头
#include "framework.h"

#define HINT_FILE_CHANGED (45556)
#define HINT_FOCUS_ITEM_CHANGED (45559)
#define HINT_PROPERTIES_CHANGED (45560)

#define WM_TREE_UPDATE_MSG (WM_USER + 100) 
#define WM_NODE_FOCUS_CHANGE_MSG (WM_USER + 110)
#define WM_PROPERTIES_CHANGE_MSG (WM_USER + 120)
#define WM_PROP_FREQ_CHANGE_END_MSG (WM_USER + 121)
#define WM_PROP_FREQ_CHANGE_BEGIN_MSG (WM_USER + 122)
#define WM_TEXTURE_UPDATE_MSG (WM_USER + 130)

#endif //PCH_H
