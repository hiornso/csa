#include <stdio.h>
#include <stdarg.h>

#include "csa_error.h"

int _csa_log(const char *prefix, const char *file, const int line, const char *func, const char *fmt, ...)
{ // should we ever switch over to using g_critical/g_warning?
	FILE *out = stderr;
	int len = fprintf(out, "%s: \033[1;34m%s@%s:%i: \033[0m", prefix, func, file, line);
	va_list args;
	va_start(args, fmt);
	len += vfprintf(out, fmt, args);
	va_end(args);
	return len;
}
