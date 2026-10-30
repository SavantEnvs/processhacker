/* Linux stand-in for the MSVC <io.h> that tools/thirdparty/mxml/config.h includes.
 * config.h then #defines close/open/read/write/... to their MSVC "_name" spellings, so provide
 * those names as thin wrappers over libc. */
#ifndef MAYHEM_SHIM_IO_H
#define MAYHEM_SHIM_IO_H
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define _inline __inline

static __inline int _close(int fd) { return close(fd); }
static __inline int _open(const char *path, int flags, ...) { return open(path, flags, 0644); }
static __inline int _read(int fd, void *buf, unsigned int n) { return (int)read(fd, buf, n); }
static __inline int _write(int fd, const void *buf, unsigned int n) { return (int)write(fd, buf, n); }
static __inline int _vsnprintf(char *s, size_t n, const char *f, va_list ap) { return vsnprintf(s, n, f, ap); }
static __inline int _snprintf(char *s, size_t n, const char *f, ...)
{
    va_list ap; int r;
    va_start(ap, f); r = vsnprintf(s, n, f, ap); va_end(ap);
    return r;
}
static __inline char *_strdup(const char *s) { return strdup(s); }
#endif
