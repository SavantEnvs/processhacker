// Known-answer tests for the libraries System Informer vendors under tools/thirdparty. Each check
// compares a computed value with a hard-coded expected one; test.sh counts the PASS/FAIL lines.
#define PCRE2_CODE_UNIT_WIDTH 16
#define PCRE2_STATIC 1
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Zydis.h"
#include "json.h"
#include "miniz.h"
#include "mxml.h"
#include "pcre2.h"

#define JSON_PLAIN_EXPECTED "{\"a\":[1,2,{\"b\":\"c\"}],\"n\":null,\"f\":2.5}"

static int passed, failed;

static void check(const char *name, int ok)
{
    printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    ok ? passed++ : failed++;
}

static void test_json(void)
{
    const unsigned char *doc = (const unsigned char *)"{\"a\":[1,2,{\"b\":\"c\"}],\"n\":null,\"f\":2.5}";
    struct json_object *o = json_tokener_parse(doc);
    check("json parses object", o != NULL);
    if (!o)
        return;
    struct json_object *a = NULL, *f = NULL, *b = NULL;
    check("json has key a", json_object_object_get_ex(o, (const unsigned char *)"a", &a) && a);
    check("json array length is 3", a && json_object_array_length(a) == 3);
    check("json array[1] == 2", a && json_object_get_int64(json_object_array_get_idx(a, 1)) == 2);
    struct json_object *inner = a ? json_object_array_get_idx(a, 2) : NULL;
    check("json nested string is c",
          inner && json_object_object_get_ex(inner, (const unsigned char *)"b", &b) &&
              strcmp((const char *)json_object_get_string(b), "c") == 0);
    check("json double is 2.5",
          json_object_object_get_ex(o, (const unsigned char *)"f", &f) && json_object_get_double(f) == 2.5);
    const char *plain = (const char *)json_object_to_json_string_ext(o, JSON_C_TO_STRING_PLAIN);
    check("json plain serialization", strcmp(plain, JSON_PLAIN_EXPECTED) == 0);
    json_object_put(o);
    check("json rejects truncated input", json_tokener_parse((const unsigned char *)"{\"a\":[1,2") == NULL);
    check("json rejects garbage", json_tokener_parse((const unsigned char *)"{]") == NULL);
}

static void test_mxml(void)
{
    mxml_node_t *top = mxmlLoadString(NULL, NULL, "<?xml version=\"1.0\"?><root x=\"1\"><item id=\"7\">hi</item><item id=\"8\"/></root>");
    check("mxml parses document", top != NULL);
    if (!top)
        return;
    mxml_node_t *root = mxmlFindElement(top, top, "root", NULL, NULL, MXML_DESCEND_ALL);
    check("mxml finds root", root != NULL);
    check("mxml root attribute x == 1", root && mxmlElementGetAttr(root, "x") && !strcmp(mxmlElementGetAttr(root, "x"), "1"));
    mxml_node_t *item = root ? mxmlFindElement(root, root, "item", "id", "8", MXML_DESCEND_ALL) : NULL;
    check("mxml finds item by attribute value", item != NULL);
    mxml_node_t *first = root ? mxmlFindElement(root, root, "item", NULL, NULL, MXML_DESCEND_ALL) : NULL;
    const char *text = first ? mxmlGetText(first, NULL) : NULL;
    check("mxml item text is hi", text && !strcmp(text, "hi"));
    mxmlDelete(top);
    check("mxml rejects mismatched tags", mxmlLoadString(NULL, NULL, "<a><b></a></b>") == NULL);
}

static PCRE2_UCHAR *wide(const char *s, size_t *n)
{
    *n = strlen(s);
    PCRE2_UCHAR *w = malloc((*n + 1) * sizeof(PCRE2_UCHAR));
    for (size_t i = 0; i <= *n; i++)
        w[i] = (unsigned char)s[i];
    return w;
}

static void test_pcre2(void)
{
    size_t pn, sn;
    PCRE2_UCHAR *pat = wide("(\\d+)-(\\w+)", &pn);
    PCRE2_UCHAR *subj = wide("abc 123-foo!", &sn);
    int err;
    PCRE2_SIZE eo;
    pcre2_code *re = pcre2_compile(pat, pn, 0, &err, &eo, NULL);
    check("pcre2 compiles pattern", re != NULL);
    if (re)
    {
        pcre2_match_data *md = pcre2_match_data_create_from_pattern(re, NULL);
        int rc = pcre2_match(re, subj, sn, 0, 0, md, NULL);
        PCRE2_SIZE *ov = pcre2_get_ovector_pointer(md);
        check("pcre2 match count is 3", rc == 3);
        check("pcre2 whole match is [4,11)", rc == 3 && ov[0] == 4 && ov[1] == 11);
        check("pcre2 group 1 is [4,7)", rc == 3 && ov[2] == 4 && ov[3] == 7);
        check("pcre2 group 2 is [8,11)", rc == 3 && ov[4] == 8 && ov[5] == 11);
        size_t nn;
        PCRE2_UCHAR *nomatch = wide("no digits here", &nn);
        check("pcre2 reports no match", pcre2_match(re, nomatch, nn, 0, 0, md, NULL) == PCRE2_ERROR_NOMATCH);
        free(nomatch);
        pcre2_match_data_free(md);
        pcre2_code_free(re);
    }
    free(pat);
    pat = wide("(unclosed", &pn);
    re = pcre2_compile(pat, pn, 0, &err, &eo, NULL);
    check("pcre2 rejects unbalanced paren", re == NULL && err == PCRE2_ERROR_MISSING_CLOSING_PARENTHESIS);
    free(pat);
    free(subj);
}

static void test_miniz(void)
{
    check("miniz crc32(123456789)", mz_crc32(MZ_CRC32_INIT, (const unsigned char *)"123456789", 9) == 0xCBF43926u);
    check("miniz adler32(Wikipedia)", mz_adler32(MZ_ADLER32_INIT, (const unsigned char *)"Wikipedia", 9) == 0x11E60398u);

    static const unsigned char hello_z[] = {0x78, 0x9c, 0xcb, 0x48, 0xcd, 0xc9, 0xc9, 0x07, 0x00, 0x06, 0x2c, 0x02, 0x15};
    unsigned char out[64];
    mz_ulong olen = sizeof(out);
    int rc = mz_uncompress(out, &olen, hello_z, sizeof(hello_z));
    check("miniz inflates known zlib stream", rc == MZ_OK && olen == 5 && memcmp(out, "hello", 5) == 0);

    unsigned char src[2000], comp[2100], back[2000];
    for (int i = 0; i < 2000; i++)
        src[i] = (unsigned char)("abcabcabd"[i % 9] + (i / 300));
    mz_ulong clen = sizeof(comp), blen = sizeof(back);
    rc = mz_compress(comp, &clen, src, sizeof(src));
    check("miniz compresses repetitive data", rc == MZ_OK && clen < 400);
    rc = mz_uncompress(back, &blen, comp, clen);
    check("miniz roundtrip", rc == MZ_OK && blen == sizeof(src) && memcmp(src, back, sizeof(src)) == 0);

    unsigned char bad[sizeof(hello_z)];
    memcpy(bad, hello_z, sizeof(bad));
    bad[sizeof(bad) - 1] ^= 0xff;
    olen = sizeof(out);
    check("miniz detects adler corruption", mz_uncompress(out, &olen, bad, sizeof(bad)) != MZ_OK);
}

static void test_zydis(void)
{
    struct { const char *name; ZydisMachineMode mode; ZydisStackWidth sw; const unsigned char *bytes; size_t n; unsigned len; const char *text; } cases[] = {
        {"zydis 89 d8", ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64, (const unsigned char *)"\x89\xd8", 2, 2, "mov eax, ebx"},
        {"zydis 48 83 ec 28", ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64, (const unsigned char *)"\x48\x83\xec\x28", 4, 4, "sub rsp, 0x28"},
        {"zydis c3", ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64, (const unsigned char *)"\xc3", 1, 1, "ret"},
        {"zydis 32-bit push ebp", ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32, (const unsigned char *)"\x55", 1, 1, "push ebp"},
        {"zydis e8 call rel32", ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64, (const unsigned char *)"\xe8\x10\x00\x00\x00", 5, 5, "call 0x0000000000401015"},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        ZydisDecoder d;
        ZydisFormatter f;
        ZydisDecodedInstruction insn;
        ZydisDecodedOperand ops[ZYDIS_MAX_OPERAND_COUNT];
        char text[128] = "";
        ZydisDecoderInit(&d, cases[i].mode, cases[i].sw);
        ZydisFormatterInit(&f, ZYDIS_FORMATTER_STYLE_INTEL);
        int ok = ZYAN_SUCCESS(ZydisDecoderDecodeFull(&d, cases[i].bytes, cases[i].n, &insn, ops)) &&
                 ZYAN_SUCCESS(ZydisFormatterFormatInstruction(&f, &insn, ops, insn.operand_count_visible, text, sizeof(text), 0x401000, NULL)) &&
                 insn.length == cases[i].len && strcmp(text, cases[i].text) == 0;
        if (!ok)
            fprintf(stderr, "  zydis got len=%u text='%s'\n", insn.length, text);
        check(cases[i].name, ok);
    }
    ZydisDecoder d;
    ZydisDecodedInstruction insn;
    ZydisDecodedOperand ops[ZYDIS_MAX_OPERAND_COUNT];
    ZydisDecoderInit(&d, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
    check("zydis rejects truncated instruction", !ZYAN_SUCCESS(ZydisDecoderDecodeFull(&d, "\x48\x8b", 2, &insn, ops)));
}

int main(void)
{
    test_json();
    test_mxml();
    test_pcre2();
    test_miniz();
    test_zydis();
    printf("RESULT passed=%d failed=%d\n", passed, failed);
    return failed ? 1 : 0;
}
