// Fuzz the vendored PCRE2 (tools/thirdparty/pcre, built 16-bit like the product): compile a
// pattern, then match it with the backtracking matcher and the DFA matcher.
#define PCRE2_CODE_UNIT_WIDTH 16
#define PCRE2_STATIC 1
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pcre2.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 7)
        return -1;

    uint32_t copts;
    uint16_t split;
    memcpy(&copts, data, 4);
    copts &= ~PCRE2_NO_UTF_CHECK;
    memcpy(&split, data + 4, 2);
    uint8_t mflags = data[6];
    data += 7;
    size -= 7;

    size_t units = size / sizeof(PCRE2_UCHAR);
    if (units == 0)
        return -1;
    PCRE2_UCHAR *w = malloc(units * sizeof(PCRE2_UCHAR));
    if (!w)
        return -1;
    memcpy(w, data, units * sizeof(PCRE2_UCHAR));

    size_t plen = split % (units + 1);
    if (plen == 0)
        plen = units;

    pcre2_compile_context *cc = pcre2_compile_context_create(NULL);
    pcre2_match_context *mc = pcre2_match_context_create(NULL);
    pcre2_set_match_limit(mc, 10000);
    pcre2_set_depth_limit(mc, 1000);
    pcre2_set_heap_limit(mc, 1000);
    pcre2_set_parens_nest_limit(cc, 50);

    int errcode;
    PCRE2_SIZE erroff;
    pcre2_code *re = pcre2_compile(w, plen, copts, &errcode, &erroff, cc);
    if (re)
    {
        pcre2_match_data *md = pcre2_match_data_create_from_pattern(re, NULL);
        const PCRE2_UCHAR *subj = w + plen;
        size_t slen = units - plen;
        uint32_t mopts = (mflags & 1 ? PCRE2_NOTBOL : 0) | (mflags & 2 ? PCRE2_NOTEOL : 0) |
                         (mflags & 4 ? PCRE2_NOTEMPTY : 0) | (mflags & 8 ? PCRE2_NO_UTF_CHECK : 0);
        if (md)
        {
            (void)pcre2_match(re, subj, slen, 0, mopts & ~PCRE2_NO_UTF_CHECK, md, mc);
            int ws[200];
            (void)pcre2_dfa_match(re, subj, slen, 0, mopts & ~PCRE2_NO_UTF_CHECK, md, mc, ws, 200);
            pcre2_match_data_free(md);
        }
        pcre2_code_free(re);
    }
    pcre2_match_context_free(mc);
    pcre2_compile_context_free(cc);
    free(w);
    return 0;
}
