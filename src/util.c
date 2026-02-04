// Purpose: Utility helpers for safe I/O, parsing, and OS helpers. Author: GitHub Copilot
#include "util.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#include <wincrypt.h>
#endif

int util_read_line(FILE* in, char* buf, size_t len) {
    if (!in || !buf || len == 0) {
        return 0;
    }
    if (!fgets(buf, (int)len, in)) {
        return 0;
    }
    size_t n = strnlen(buf, len);
    if (n > 0 && buf[n - 1] == '\n') {
        buf[n - 1] = '\0';
    }
    return 1;
}

void util_trim(char* s) {
    if (!s) {
        return;
    }
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
    size_t i = 0;
    while (s[i] && isspace((unsigned char)s[i])) {
        i++;
    }
    if (i > 0) {
        memmove(s, s + i, strlen(s + i) + 1);
    }
}

int util_parse_long(const char* s, long* out, long min, long max) {
    if (!s || !out) {
        return 0;
    }
    char* end = NULL;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') {
        return 0;
    }
    if (v < min || v > max) {
        return 0;
    }
    *out = v;
    return 1;
}

int util_parse_i64(const char* s, int64_t* out, int64_t min, int64_t max) {
    if (!s || !out) {
        return 0;
    }
    char* end = NULL;
    errno = 0;
    long long v = strtoll(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') {
        return 0;
    }
    if (v < min || v > max) {
        return 0;
    }
    *out = (int64_t)v;
    return 1;
}

int util_parse_double(const char* s, double* out, double min, double max) {
    if (!s || !out) {
        return 0;
    }
    char* end = NULL;
    errno = 0;
    double v = strtod(s, &end);
    if (errno != 0 || end == s || *end != '\0') {
        return 0;
    }
    if (v < min || v > max) {
        return 0;
    }
    *out = v;
    return 1;
}

void util_copy_str(char* dest, size_t dest_len, const char* src) {
    if (!dest || dest_len == 0) {
        return;
    }
    if (!src || !src[0]) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, dest_len - 1);
    dest[dest_len - 1] = '\0';
}

int util_random_bytes(uint8_t* buf, size_t len) {
    if (!buf || len == 0) {
        return 0;
    }
#if defined(_WIN32)
    HCRYPTPROV hProv = 0;
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        return 0;
    }
    BOOL ok = CryptGenRandom(hProv, (DWORD)len, buf);
    CryptReleaseContext(hProv, 0);
    return ok ? 1 : 0;
#else
    FILE* urand = fopen("/dev/urandom", "rb");
    if (!urand) {
        return 0;
    }
    size_t r = fread(buf, 1, len, urand);
    fclose(urand);
    return r == len ? 1 : 0;
#endif
}

int util_file_exists(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        return 0;
    }
    fclose(f);
    return 1;
}

int util_copy_file(const char* src, const char* dst) {
    FILE* in = fopen(src, "rb");
    if (!in) {
        return 0;
    }
    FILE* out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return 0;
    }
    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            fclose(in);
            fclose(out);
            return 0;
        }
    }
    fclose(in);
    fclose(out);
    return 1;
}

int util_make_backup(const char* path, char* out_path, size_t out_len) {
    if (!path || !out_path || out_len == 0) {
        return 0;
    }
    time_t t = time(NULL);
    struct tm tmv;
#if defined(_WIN32)
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    char stamp[32];
    snprintf(stamp, sizeof(stamp), "%04d%02d%02d_%02d%02d%02d",
        tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
        tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
    snprintf(out_path, out_len, "%s.%s.bak", path, stamp);
    return util_copy_file(path, out_path);
}

void util_print_json_string(FILE* out, const char* s) {
    if (!out) {
        return;
    }
    fputc('"', out);
    if (s) {
        for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
            switch (*p) {
            case '"': fputs("\\\"", out); break;
            case '\\': fputs("\\\\", out); break;
            case '\b': fputs("\\b", out); break;
            case '\f': fputs("\\f", out); break;
            case '\n': fputs("\\n", out); break;
            case '\r': fputs("\\r", out); break;
            case '\t': fputs("\\t", out); break;
            default:
                if (*p < 0x20) {
                    fprintf(out, "\\u%04x", *p);
                }
                else {
                    fputc(*p, out);
                }
                break;
            }
        }
    }
    fputc('"', out);
}

int util_parse_iso_date(const char* input, struct tm* out) {
    if (!input || !out) {
        return 0;
    }
    int year = 0;
    int month = 0;
    int day = 0;
    if (sscanf(input, "%4d-%2d-%2d", &year, &month, &day) != 3) {
        return 0;
    }
    if (year < 1900 || month < 1 || month > 12 || day < 1 || day > 31) {
        return 0;
    }
    memset(out, 0, sizeof(*out));
    out->tm_year = year - 1900;
    out->tm_mon = month - 1;
    out->tm_mday = day;
    out->tm_hour = 0;
    out->tm_min = 0;
    out->tm_sec = 0;
    out->tm_isdst = -1;
    if (mktime(out) == (time_t)-1) {
        return 0;
    }
    return 1;
}

int util_due_days(const char* due_date, int* days_out) {
    if (!due_date || !due_date[0] || !days_out) {
        return 0;
    }
    struct tm due_tm = { 0 };
    if (!util_parse_iso_date(due_date, &due_tm)) {
        return 0;
    }
    time_t due_time = mktime(&due_tm);
    if (due_time == (time_t)-1) {
        return 0;
    }
    time_t now = time(NULL);
    long long seconds = (long long)(due_time - now);
    long days = (long)(seconds / 86400LL);
    if (seconds < 0 && (seconds % 86400LL) != 0) {
        days -= 1;
    }
    *days_out = (int)days;
    return 1;
}

void util_format_iso_date(time_t when, char* out, size_t len) {
    if (!out || len < 11) {
        return;
    }
    struct tm tmv;
#if defined(_WIN32)
    localtime_s(&tmv, &when);
#else
    localtime_r(&when, &tmv);
#endif
    snprintf(out, len, "%04d-%02d-%02d",
        tmv.tm_year + 1900,
        tmv.tm_mon + 1,
        tmv.tm_mday);
}
