#include "ChsToSJis.h"

#include "ChsToSJis.h"

#include "StringBuilder.h"
#include "xystring.h"

std::u8string ChsToSJis::ReplaceHanzi(std::u8string in)
{
    xybase::StringBuilder<char8_t> sb;

    int leng = 1;
    for (int i = 0; i < in.length(); i += leng)
    {
        int cp = xybase::string::to_codepoint(in, i, leng);
        if (repMap.contains(cp))
        {
            sb += repMap[cp];
        }
        else
        {
            sb += xybase::string::to_utf8(cp);
        }
    }
    return sb.ToString();
}

#include "CsvFile.h"

void ChsToSJis::Init(std::filesystem::path csvPath)
{
    CsvFile csv(csvPath, std::ios::in | std::ios::binary);

    while (!csv.IsEof())
    {
        std::u8string ori = csv.NextCell();
        std::u8string rep = csv.NextCell();
        csv.NextLine();
        repMap[xybase::string::to_codepoint(ori)] = rep;
    }
}

ChsToSJis &ChsToSJis::Instance()
{
    static ChsToSJis _inst;
    return _inst;
}
