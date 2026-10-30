/* Force-included (-include) when compiling tools/thirdparty/jsonc on Linux. The vendored copy is
 * the Windows-adapted json-c: it spells libc calls the MSVC way (_strdup/_read/_close/_O_RDONLY)
 * and routes strerror() through _json_c_strerror. Map them back to libc; the self-referential
 * defines are intentional (a macro is not re-expanded inside its own expansion). */
#ifndef MAYHEM_SHIM_JSONC_COMPAT_H
#define MAYHEM_SHIM_JSONC_COMPAT_H
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define _strdup strdup
#define _read read
#define _close close
#define _open open
#define _O_RDONLY O_RDONLY
#define _O_WRONLY O_WRONLY
#define _O_TRUNC O_TRUNC
#define _O_CREAT O_CREAT
#define _write write
#define _json_c_strerror strerror
#endif
