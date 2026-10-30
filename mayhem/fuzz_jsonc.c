// Fuzz the vendored json-c (tools/thirdparty/jsonc): incremental tokener -> serializer -> tree walk.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"

static void walk(struct json_object *o, int depth)
{
    if (depth > 64)
        return;
    switch (json_object_get_type(o))
    {
    case json_type_object:
    {
        struct json_object_iter it;
        json_object_object_foreachC(o, it)
        {
            walk(it.val, depth + 1);
        }
        break;
    }
    case json_type_array:
    {
        size_t n = json_object_array_length(o);
        for (size_t i = 0; i < n; i++)
            walk(json_object_array_get_idx(o, i), depth + 1);
        break;
    }
    case json_type_int:
        (void)json_object_get_int64(o);
        break;
    case json_type_double:
        (void)json_object_get_double(o);
        break;
    case json_type_string:
        (void)json_object_get_string(o);
        break;
    default:
        break;
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 2)
        return -1;

    int tokflags = data[0] & (JSON_TOKENER_STRICT | JSON_TOKENER_ALLOW_TRAILING_CHARS);
    int outflags = data[1] & 0x3f;
    data += 2;
    size -= 2;

    unsigned char *buf = malloc(size + 1);
    if (!buf)
        return -1;
    memcpy(buf, data, size);
    buf[size] = 0;

    struct json_tokener *tok = json_tokener_new();
    if (tok)
    {
        json_tokener_set_flags(tok, tokflags);
        struct json_object *obj = json_tokener_parse_ex(tok, buf, (int)size);
        if (obj)
        {
            (void)json_object_to_json_string_ext(obj, outflags);
            walk(obj, 0);
            json_object_put(obj);
        }
        json_tokener_free(tok);
    }
    free(buf);
    return 0;
}
