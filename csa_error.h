#ifndef CPT_SONAR_ASSIST_ERROR_H
#define CPT_SONAR_ASSIST_ERROR_H

int _csa_log(const char *prefix, const char *file, const int line, const char *func, const char *fmt, ...);

#define csa_error(...) _csa_log("\033[1;31mError", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define csa_warning(...) _csa_log("\033[1;33mWarning", __FILE__, __LINE__, __func__, __VA_ARGS__)

#endif
