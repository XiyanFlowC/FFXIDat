#pragma once

#ifndef XY_LITEOPT_H__
#define XY_LITEOPT_H__

#include <stdlib.h>
#include <string.h>

#ifndef LITEOPT_TBLSIZ
#define LITEOPT_TBLSIZ 32
#endif

struct opt_reg
{
    int (*callback)(const char *);
    const void *desc;
    char ch_opt;
    char long_opt[26];
    unsigned char flg;
};

#define LOPT_FLG_DESC_VLD 0x10
#define LOPT_FLG_OPT_VLD 0x8  /* This opt is valid */
#define LOPT_FLG_VAL_NEED 0x4 /* This opt needs a value */
#define LOPT_FLG_CH_VLD 0x2   /* This opt has a ch prs */
#define LOPT_FLG_STR_VLD 0x1  /* This opt has a str prs */
#define LOPT_FLG_CHK(val, flg) ((val) & (flg))

static struct opt_reg *_reged_opt = NULL;

static void _lopt_mktbl()
{
    _reged_opt = (struct opt_reg *)calloc(LITEOPT_TBLSIZ, sizeof(struct opt_reg));
}

static int _lopt_callbych(char opt, const char *_str)
{
    int i;
    for (i = 0; i < LITEOPT_TBLSIZ; ++i)
    {
        if (_reged_opt[i].ch_opt == opt)
        {
            if (!LOPT_FLG_CHK(_reged_opt[i].flg, LOPT_FLG_CH_VLD))
                return -i; /* NOT REGED OPT */
            if (LOPT_FLG_CHK(_reged_opt[i].flg, LOPT_FLG_VAL_NEED) && (_str == NULL))
                return -i;
            return _reged_opt[i].callback(_str);
        }
    }
    return -i; /* NOT REGED OPT */
}

static int _lopt_callbystr(const char *opt, const char *_str)
{
    int i;
    for (i = 0; i < LITEOPT_TBLSIZ; ++i)
    {
        if (strcmp(_reged_opt[i].long_opt, opt) == 0)
        {
            if (!LOPT_FLG_CHK(_reged_opt[i].flg, LOPT_FLG_STR_VLD))
                return -i; /* NOT REGED OPT */
            if (LOPT_FLG_CHK(_reged_opt[i].flg, LOPT_FLG_VAL_NEED) && (_str == NULL))
                return -i;
            return _reged_opt[i].callback(_str);
        }
    }
    return -i;
}

static void lopt_regopt(const char *name, char chname, unsigned char flg, int (*callback)(const char *), const void *desc)
{
    static int opt_idx = 0;
    if (_reged_opt == NULL)
        _lopt_mktbl();
    if (_reged_opt == NULL)
        return;
    _reged_opt[opt_idx].callback = callback;
    _reged_opt[opt_idx].ch_opt = chname;
    _reged_opt[opt_idx].desc = desc;
    if (desc != NULL)
    {
        flg |= LOPT_FLG_DESC_VLD;
    }
    if (name != NULL)
    {
        flg |= LOPT_FLG_STR_VLD;
        strcpy(_reged_opt[opt_idx].long_opt, name);
    }
    if (chname != '\0') flg |= LOPT_FLG_CH_VLD;
    _reged_opt[opt_idx].flg = flg;
    _reged_opt[opt_idx++].flg |= LOPT_FLG_OPT_VLD;
}

static int lopt_parse(int argc, const char **argv)
{
    int i, rst;
    for (i = 1; i < argc; ++i)
    {
        if (argv[i][0] != '-')
            continue; /* INVALID OPT FOUND */
        if (argv[i][1] == '-')
        {
            if (argc > i + 1 && argv[i + 1][0] != '-')
                rst = _lopt_callbystr(argv[i] + 2, argv[i + 1]);
            else
                rst = _lopt_callbystr(argv[i] + 2, NULL);

            if (rst < 0)
                return -i;
            if (rst > 0)
                return rst;
        }
        else
        {
            int j = 1;
            while (argv[i][j] != '\0')
            {
                if (argc > i + 1 && argv[i + 1][0] != '-')
                    rst = _lopt_callbych(argv[i][j], argv[i + 1]);
                else
                    rst = _lopt_callbych(argv[i][j], NULL);

                if (rst < 0)
                    return -i;
                if (rst > 0)
                    return rst;
                ++j;
            }
        }
    }
    return 0;
}

static int lopt_finalize()
{
    free(_reged_opt);
    _reged_opt = NULL;
    return 0;
}

#undef LITEOPT_TBLSIZ

#endif

