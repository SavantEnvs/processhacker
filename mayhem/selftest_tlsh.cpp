// Known-answer tests for the vendored TLSH. Prints PASS/FAIL lines and a RESULT line for test.sh.
#include <stdio.h>
#include <string.h>

#include "tlsh.h"

#define TLSH_EXPECTED "D0815CFA132DF6A19448F05091F94BAC3B28DAF29AC93D2E5410496059A4383C2FE559"
#define TLSH_EXPECTED_DIFF 16

static int passed, failed;

static void check(const char *name, bool ok)
{
    printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    ok ? passed++ : failed++;
}

static void fill(unsigned char *buf, unsigned n, unsigned seed)
{
    for (unsigned i = 0; i < n; i++)
    {
        seed = seed * 1103515245u + 12345u;
        buf[i] = (unsigned char)(seed >> 16);
    }
}

int main()
{
    unsigned char a[4096], b[4096];
    fill(a, sizeof(a), 1);
    memcpy(b, a, sizeof(b));
    for (int i = 0; i < 64; i++)
        b[1000 + i] ^= 0x5a;

    Tlsh ta, ta2, tb;
    ta.update(a, sizeof(a)); ta.final();
    ta2.update(a, 1000); ta2.update(a + 1000, sizeof(a) - 1000); ta2.final();
    tb.update(b, sizeof(b)); tb.final();

    check("tlsh hash is valid", ta.isValid());
    check("tlsh known hash", strcmp(ta.getHash(), TLSH_EXPECTED) == 0);
    check("tlsh incremental update matches one-shot", strcmp(ta.getHash(), ta2.getHash()) == 0);
    check("tlsh identical data has distance 0", ta.totalDiff(&ta2) == 0);
    check("tlsh perturbed data has known distance", ta.totalDiff(&tb) == TLSH_EXPECTED_DIFF);

    Tlsh parsed;
    check("tlsh parses its own hash string", parsed.fromTlshStr(ta.getHash()) == 0 && parsed.totalDiff(&ta) == 0);
    Tlsh bad;
    check("tlsh rejects malformed hash string", bad.fromTlshStr("not-a-tlsh-hash") != 0);

    printf("RESULT passed=%d failed=%d\n", passed, failed);
    return failed ? 1 : 0;
}
