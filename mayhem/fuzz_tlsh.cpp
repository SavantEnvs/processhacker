// Fuzz the vendored TLSH (tools/thirdparty/tlsh): hash a buffer, parse a hash string, diff them.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "tlsh.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 2)
        return -1;
    size_t split = data[0] % size;
    const uint8_t *body = data + 1;
    size_t blen = size - 1;
    if (split > blen)
        split = blen;

    Tlsh a;
    a.update(body, (unsigned)split);
    a.final(body + split, (unsigned)(blen - split));
    if (a.isValid())
    {
        char hash[TLSH_STRING_BUFFER_LEN];
        const char *h = a.getHash(hash, sizeof(hash));
        Tlsh b;
        if (h && b.fromTlshStr(h) == 0)
            (void)a.totalDiff(&b);
    }

    char *str = (char *)malloc(blen + 1);
    if (str)
    {
        memcpy(str, body, blen);
        str[blen] = 0;
        Tlsh c;
        if (c.fromTlshStr(str) == 0)
            (void)c.totalDiff(&a);
        free(str);
    }
    return 0;
}
