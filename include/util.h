// Purpose: Utility helpers for safe I/O, parsing, and OS helpers. Author: GitHub Copilot
#ifndef CONTACTS_UTIL_H
#define CONTACTS_UTIL_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UTIL_MAX_LINE 1024

    int util_read_line(FILE* in, char* buf, size_t len);
    void util_trim(char* s);
    int util_parse_long(const char* s, long* out, long min, long max);
    int util_parse_i64(const char* s, int64_t* out, int64_t min, int64_t max);
    int util_parse_double(const char* s, double* out, double min, double max);
    int util_random_bytes(uint8_t* buf, size_t len);
    int util_file_exists(const char* path);
    int util_copy_file(const char* src, const char* dst);
    int util_make_backup(const char* path, char* out_path, size_t out_len);
    void util_print_json_string(FILE* out, const char* s);
    int util_parse_iso_date(const char* input, struct tm* out);
    int util_due_days(const char* due_date, int* days_out);
    void util_format_iso_date(time_t when, char* out, size_t len);
    void util_copy_str(char* dest, size_t dest_len, const char* src);

#ifdef __cplusplus
}
#endif

#endif
