// Fuzz the vendored Mini-XML (tools/thirdparty/mxml): load a document, save it back, free it.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mxml.h"

static const mxml_type_t kTypes[] = {MXML_TYPE_TEXT, MXML_TYPE_OPAQUE, MXML_TYPE_INTEGER, MXML_TYPE_REAL};

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 1)
        return -1;

    mxml_type_t type = kTypes[data[0] & 3];
    data++;
    size--;

    char *buf = malloc(size + 1);
    if (!buf)
        return -1;
    memcpy(buf, data, size);
    buf[size] = 0;

    mxml_options_t *opts = mxmlOptionsNew();
    if (opts)
    {
        mxmlOptionsSetTypeValue(opts, type);
        mxml_node_t *top = mxmlLoadString(NULL, opts, buf);
        if (top)
        {
            char *out = mxmlSaveAllocString(top, opts);
            free(out);
            mxmlDelete(top);
        }
        mxmlOptionsDelete(opts);
    }
    free(buf);
    return 0;
}
