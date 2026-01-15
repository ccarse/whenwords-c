#ifndef WHENWORDS_H
#define WHENWORDS_H

#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WW_TS_UNIX,
    WW_TS_ISO,
    WW_TS_TM,
    WW_TS_NONE
} ww_timestamp_kind;

typedef struct {
    ww_timestamp_kind kind;
    double unix_seconds;
    const char *iso;
    const struct tm *tm;
} ww_timestamp;

static inline ww_timestamp ww_timestamp_from_unix(double seconds) {
    ww_timestamp ts;
    ts.kind = WW_TS_UNIX;
    ts.unix_seconds = seconds;
    ts.iso = NULL;
    ts.tm = NULL;
    return ts;
}

static inline ww_timestamp ww_timestamp_from_iso(const char *iso) {
    ww_timestamp ts;
    ts.kind = WW_TS_ISO;
    ts.unix_seconds = 0.0;
    ts.iso = iso;
    ts.tm = NULL;
    return ts;
}

static inline ww_timestamp ww_timestamp_from_tm(const struct tm *tm) {
    ww_timestamp ts;
    ts.kind = WW_TS_TM;
    ts.unix_seconds = 0.0;
    ts.iso = NULL;
    ts.tm = tm;
    return ts;
}

static inline ww_timestamp ww_timestamp_unset(void) {
    ww_timestamp ts;
    ts.kind = WW_TS_NONE;
    ts.unix_seconds = 0.0;
    ts.iso = NULL;
    ts.tm = NULL;
    return ts;
}

typedef struct {
    int compact;
    int max_units;
} ww_duration_options;

ww_duration_options ww_duration_options_default(void);

char *timeago(ww_timestamp timestamp, ww_timestamp reference);
char *duration(double seconds, const ww_duration_options *options);
bool parse_duration(const char *input, double *out_seconds);
char *human_date(ww_timestamp timestamp, ww_timestamp reference);
char *date_range(ww_timestamp start, ww_timestamp end);

#ifdef __cplusplus
}
#endif

#endif
