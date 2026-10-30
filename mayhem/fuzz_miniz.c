// Fuzz the vendored miniz (tools/thirdparty/miniz): raw inflate, zlib uncompress, and the zip reader.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "miniz.h"

#define MAX_OUT (1u << 20)

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 2)
        return -1;
    uint8_t mode = data[0] % 3;
    data++;
    size--;

    if (mode == 0)
    {
        size_t out_len = 0;
        void *out = tinfl_decompress_mem_to_heap(data, size, &out_len, TINFL_FLAG_PARSE_ZLIB_HEADER);
        mz_free(out);
    }
    else if (mode == 1)
    {
        static unsigned char dst[MAX_OUT];
        mz_ulong dlen = sizeof(dst);
        (void)mz_uncompress(dst, &dlen, data, size);
    }
    else
    {
        mz_zip_archive zip;
        memset(&zip, 0, sizeof(zip));
        if (mz_zip_reader_init_mem(&zip, data, size, 0))
        {
            mz_uint n = mz_zip_reader_get_num_files(&zip);
            if (n > 32)
                n = 32;
            for (mz_uint i = 0; i < n; i++)
            {
                mz_zip_archive_file_stat st;
                if (!mz_zip_reader_file_stat(&zip, i, &st) || st.m_uncomp_size > MAX_OUT)
                    continue;
                size_t out_len = 0;
                void *out = mz_zip_reader_extract_to_heap(&zip, i, &out_len, 0);
                mz_free(out);
            }
            mz_zip_reader_end(&zip);
        }
    }
    return 0;
}
