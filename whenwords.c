#include "whenwords.h"

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WW_SECONDS_PER_MINUTE 60.0
#define WW_SECONDS_PER_HOUR 3600.0
#define WW_SECONDS_PER_DAY 86400.0
#define WW_SECONDS_PER_MONTH (30.0 * WW_SECONDS_PER_DAY)
#define WW_SECONDS_PER_YEAR (365.0 * WW_SECONDS_PER_DAY)

static const char *ww_month_names[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"};

static const char *ww_weekday_names[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

static char *ww_strdup(const char *s) {
    size_t len = strlen(s);
    char *out = (char *)malloc(len + 1);
    if (!out) {
        return NULL;
    }
    memcpy(out, s, len + 1);
    return out;
}

static char *ww_strdup_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int needed = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (needed < 0) {
        return NULL;
    }
    char *buf = (char *)malloc((size_t)needed + 1);
    if (!buf) {
        return NULL;
    }
    va_start(ap, fmt);
    vsnprintf(buf, (size_t)needed + 1, fmt, ap);
    va_end(ap);
    return buf;
}

static int64_t ww_days_from_civil(int y, unsigned m, unsigned d) {
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (int64_t)era * 146097 + (int64_t)doe - 719468;
}

static void ww_civil_from_days(int64_t z, int *y, unsigned *m, unsigned *d) {
    z += 719468;
    const int era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = (unsigned)(z - (int64_t)era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const int y_local = (int)yoe + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    const unsigned d_local = doy - (153 * mp + 2) / 5 + 1;
    const unsigned m_local = mp + (mp < 10 ? 3 : -9);
    *y = y_local + (m_local <= 2);
    *m = m_local;
    *d = d_local;
}

static int64_t ww_days_from_unix_seconds(double seconds) {
    return (int64_t)floor(seconds / WW_SECONDS_PER_DAY);
}

static int ww_day_of_week_from_days(int64_t days_since_epoch) {
    int weekday = (int)((days_since_epoch + 4) % 7);
    if (weekday < 0) {
        weekday += 7;
    }
    return weekday;
}

static bool ww_parse_int_fixed(const char *s, size_t len, int *out) {
    int value = 0;
    for (size_t i = 0; i < len; i++) {
        if (!isdigit((unsigned char)s[i])) {
            return false;
        }
        value = value * 10 + (s[i] - '0');
    }
    *out = value;
    return true;
}

static bool ww_parse_iso8601(const char *input, double *out_seconds) {
    if (!input) {
        return false;
    }
    while (isspace((unsigned char)*input)) {
        input++;
    }
    size_t len = strlen(input);
    while (len > 0 && isspace((unsigned char)input[len - 1])) {
        len--;
    }
    if (len < 10) {
        return false;
    }
    int year = 0;
    int month = 0;
    int day = 0;
    if (!ww_parse_int_fixed(input + 0, 4, &year) || input[4] != '-' ||
        !ww_parse_int_fixed(input + 5, 2, &month) || input[7] != '-' ||
        !ww_parse_int_fixed(input + 8, 2, &day)) {
        return false;
    }
    int hour = 0;
    int minute = 0;
    int second = 0;
    if (len > 10) {
        char sep = input[10];
        if (sep != 'T' && sep != ' ') {
            return false;
        }
        if (len < 19) {
            return false;
        }
        if (!ww_parse_int_fixed(input + 11, 2, &hour) || input[13] != ':' ||
            !ww_parse_int_fixed(input + 14, 2, &minute) || input[16] != ':' ||
            !ww_parse_int_fixed(input + 17, 2, &second)) {
            return false;
        }
    }
    int64_t days = ww_days_from_civil(year, (unsigned)month, (unsigned)day);
    double total = (double)days * WW_SECONDS_PER_DAY + (double)hour * WW_SECONDS_PER_HOUR +
                   (double)minute * WW_SECONDS_PER_MINUTE + (double)second;
    *out_seconds = total;
    return true;
}

static bool ww_normalize_timestamp(ww_timestamp ts, double *out_seconds) {
    if (!out_seconds) {
        return false;
    }
    switch (ts.kind) {
    case WW_TS_UNIX:
        *out_seconds = ts.unix_seconds;
        return true;
    case WW_TS_ISO:
        return ww_parse_iso8601(ts.iso, out_seconds);
    case WW_TS_TM: {
        if (!ts.tm) {
            return false;
        }
        int year = ts.tm->tm_year + 1900;
        int month = ts.tm->tm_mon + 1;
        int day = ts.tm->tm_mday;
        int hour = ts.tm->tm_hour;
        int minute = ts.tm->tm_min;
        int second = ts.tm->tm_sec;
        int64_t days = ww_days_from_civil(year, (unsigned)month, (unsigned)day);
        *out_seconds = (double)days * WW_SECONDS_PER_DAY + (double)hour * WW_SECONDS_PER_HOUR +
                       (double)minute * WW_SECONDS_PER_MINUTE + (double)second;
        return true;
    }
    case WW_TS_NONE:
    default:
        return false;
    }
}

static int64_t ww_round_half_up(double value) {
    double floor_value = floor(value);
    double frac = value - floor_value;
    if (frac > 0.5 || fabs(frac - 0.5) < 1e-12) {
        return (int64_t)floor_value + 1;
    }
    return (int64_t)floor_value;
}

static const char *ww_plural(const char *singular, const char *plural, int64_t count) {
    return (count == 1) ? singular : plural;
}

ww_duration_options ww_duration_options_default(void) {
    ww_duration_options options;
    options.compact = 0;
    options.max_units = 2;
    return options;
}

char *timeago(ww_timestamp timestamp, ww_timestamp reference) {
    double ts_seconds = 0.0;
    if (!ww_normalize_timestamp(timestamp, &ts_seconds)) {
        return NULL;
    }
    double ref_seconds = ts_seconds;
    if (reference.kind != WW_TS_NONE) {
        if (!ww_normalize_timestamp(reference, &ref_seconds)) {
            return NULL;
        }
    }
    double diff = ref_seconds - ts_seconds;
    double abs_diff = fabs(diff);
    if (abs_diff < 45.0) {
        return ww_strdup("just now");
    }

    const char *unit = NULL;
    int64_t n = 0;

    if (abs_diff < 90.0) {
        unit = "minute";
        n = 1;
    } else if (abs_diff < 45.0 * WW_SECONDS_PER_MINUTE) {
        unit = "minute";
        n = ww_round_half_up(abs_diff / WW_SECONDS_PER_MINUTE);
    } else if (abs_diff < 90.0 * WW_SECONDS_PER_MINUTE) {
        unit = "hour";
        n = 1;
    } else if (abs_diff < 22.0 * WW_SECONDS_PER_HOUR) {
        unit = "hour";
        n = ww_round_half_up(abs_diff / WW_SECONDS_PER_HOUR);
    } else if (abs_diff < 36.0 * WW_SECONDS_PER_HOUR) {
        unit = "day";
        n = 1;
    } else if (abs_diff < 26.0 * WW_SECONDS_PER_DAY) {
        unit = "day";
        n = ww_round_half_up(abs_diff / WW_SECONDS_PER_DAY);
    } else if (abs_diff < 46.0 * WW_SECONDS_PER_DAY) {
        unit = "month";
        n = 1;
    } else if (abs_diff < 320.0 * WW_SECONDS_PER_DAY) {
        double month_seconds = WW_SECONDS_PER_YEAR / 12.0;
        unit = "month";
        n = ww_round_half_up(abs_diff / month_seconds);
    } else if (abs_diff < 548.0 * WW_SECONDS_PER_DAY) {
        unit = "year";
        n = 1;
    } else {
        unit = "year";
        n = ww_round_half_up(abs_diff / WW_SECONDS_PER_YEAR);
    }

    const char *unit_label = ww_plural(unit, unit, n);
    if (n != 1) {
        if (strcmp(unit, "day") == 0) {
            unit_label = "days";
        } else if (strcmp(unit, "hour") == 0) {
            unit_label = "hours";
        } else if (strcmp(unit, "minute") == 0) {
            unit_label = "minutes";
        } else if (strcmp(unit, "month") == 0) {
            unit_label = "months";
        } else if (strcmp(unit, "year") == 0) {
            unit_label = "years";
        }
    }

    if (diff < 0) {
        return ww_strdup_printf("in %lld %s", (long long)n, unit_label);
    }
    return ww_strdup_printf("%lld %s ago", (long long)n, unit_label);
}

char *duration(double seconds, const ww_duration_options *options) {
    if (seconds < 0.0 || isnan(seconds) || isinf(seconds)) {
        return NULL;
    }
    ww_duration_options opts = ww_duration_options_default();
    if (options) {
        opts = *options;
        if (opts.max_units <= 0) {
            opts.max_units = 2;
        }
    }

    const struct {
        const char *name_singular;
        const char *name_plural;
        const char *compact;
        double unit_seconds;
    } units[] = {
        {"year", "years", "y", WW_SECONDS_PER_YEAR},
        {"month", "months", "mo", WW_SECONDS_PER_MONTH},
        {"day", "days", "d", WW_SECONDS_PER_DAY},
        {"hour", "hours", "h", WW_SECONDS_PER_HOUR},
        {"minute", "minutes", "m", WW_SECONDS_PER_MINUTE},
        {"second", "seconds", "s", 1.0},
    };

    int64_t counts[sizeof(units) / sizeof(units[0])];
    size_t count_len = sizeof(units) / sizeof(units[0]);
    double remaining = seconds;
    for (size_t i = 0; i < count_len; i++) {
        counts[i] = (int64_t)floor(remaining / units[i].unit_seconds);
        remaining -= (double)counts[i] * units[i].unit_seconds;
        if (remaining < 0.0) {
            remaining = 0.0;
        }
    }

    size_t start_index = count_len;
    for (size_t i = 0; i < count_len; i++) {
        if (counts[i] > 0) {
            start_index = i;
            break;
        }
    }
    if (start_index == count_len) {
        return ww_strdup(opts.compact ? "0s" : "0 seconds");
    }

    size_t end_index = start_index + (size_t)opts.max_units - 1;
    if (end_index >= count_len) {
        end_index = count_len - 1;
    }

    char buffer[256];
    size_t pos = 0;
    size_t emitted = 0;
    for (size_t idx = start_index; idx <= end_index; idx++) {
        if (counts[idx] == 0) {
            continue;
        }
        if (emitted > 0) {
            const char *sep = opts.compact ? " " : ", ";
            size_t sep_len = strlen(sep);
            if (pos + sep_len < sizeof(buffer)) {
                memcpy(buffer + pos, sep, sep_len);
                pos += sep_len;
            }
        }
        if (opts.compact) {
            int written = snprintf(buffer + pos, sizeof(buffer) - pos, "%lld%s",
                                   (long long)counts[idx], units[idx].compact);
            if (written < 0) {
                break;
            }
            pos += (size_t)written;
        } else {
            const char *label = ww_plural(units[idx].name_singular, units[idx].name_plural, counts[idx]);
            int written = snprintf(buffer + pos, sizeof(buffer) - pos, "%lld %s",
                                   (long long)counts[idx], label);
            if (written < 0) {
                break;
            }
            pos += (size_t)written;
        }
        emitted++;
        if (pos >= sizeof(buffer)) {
            pos = sizeof(buffer) - 1;
            break;
        }
    }
    buffer[pos] = '\0';
    return ww_strdup(buffer);
}

static const struct {
    const char *name;
    double multiplier;
} ww_unit_map[] = {
    {"s", 1.0},
    {"sec", 1.0},
    {"secs", 1.0},
    {"second", 1.0},
    {"seconds", 1.0},
    {"m", 60.0},
    {"min", 60.0},
    {"mins", 60.0},
    {"minute", 60.0},
    {"minutes", 60.0},
    {"h", 3600.0},
    {"hr", 3600.0},
    {"hrs", 3600.0},
    {"hour", 3600.0},
    {"hours", 3600.0},
    {"d", 86400.0},
    {"day", 86400.0},
    {"days", 86400.0},
    {"w", 604800.0},
    {"wk", 604800.0},
    {"wks", 604800.0},
    {"week", 604800.0},
    {"weeks", 604800.0},
};

static bool ww_is_word(const char *s, const char *word) {
    size_t len = strlen(word);
    for (size_t i = 0; i < len; i++) {
        if (tolower((unsigned char)s[i]) != word[i]) {
            return false;
        }
    }
    return !isalpha((unsigned char)s[len]);
}

static bool ww_parse_colon_duration(const char *input, double *out_seconds) {
    const char *p = input;
    while (isspace((unsigned char)*p)) {
        p++;
    }
    if (!strchr(p, ':')) {
        return false;
    }
    char *copy = ww_strdup(p);
    if (!copy) {
        return false;
    }
    size_t len = strlen(copy);
    while (len > 0 && isspace((unsigned char)copy[len - 1])) {
        copy[len - 1] = '\0';
        len--;
    }

    int parts[3] = {0, 0, 0};
    int count = 0;
    char *save = NULL;
    char *token = strtok_r(copy, ":", &save);
    while (token && count < 3) {
        char *endptr = NULL;
        long value = strtol(token, &endptr, 10);
        if (endptr == token || value < 0) {
            free(copy);
            return false;
        }
        parts[count++] = (int)value;
        token = strtok_r(NULL, ":", &save);
    }
    if (token != NULL || (count != 2 && count != 3)) {
        free(copy);
        return false;
    }
    double total = 0.0;
    if (count == 2) {
        total = (double)parts[0] * WW_SECONDS_PER_HOUR + (double)parts[1] * WW_SECONDS_PER_MINUTE;
    } else {
        total = (double)parts[0] * WW_SECONDS_PER_HOUR + (double)parts[1] * WW_SECONDS_PER_MINUTE +
                (double)parts[2];
    }
    free(copy);
    *out_seconds = total;
    return true;
}

bool parse_duration(const char *input, double *out_seconds) {
    if (!input || !out_seconds) {
        return false;
    }
    if (ww_parse_colon_duration(input, out_seconds)) {
        return true;
    }

    const char *p = input;
    while (isspace((unsigned char)*p)) {
        p++;
    }
    if (*p == '\0') {
        return false;
    }

    double total = 0.0;
    bool found = false;

    while (*p) {
        while (*p && (isspace((unsigned char)*p) || *p == ',')) {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        if (isalpha((unsigned char)*p)) {
            if (ww_is_word(p, "and")) {
                p += 3;
                continue;
            }
            return false;
        }
        char *endptr = NULL;
        double value = strtod(p, &endptr);
        if (endptr == p) {
            return false;
        }
        if (value < 0) {
            return false;
        }
        p = endptr;
        while (*p && isspace((unsigned char)*p)) {
            p++;
        }
        if (!isalpha((unsigned char)*p)) {
            return false;
        }
        char unit_buf[16];
        size_t unit_len = 0;
        while (*p && isalpha((unsigned char)*p) && unit_len + 1 < sizeof(unit_buf)) {
            unit_buf[unit_len++] = (char)tolower((unsigned char)*p);
            p++;
        }
        unit_buf[unit_len] = '\0';
        if (unit_len == 0) {
            return false;
        }
        double multiplier = 0.0;
        bool matched = false;
        for (size_t i = 0; i < sizeof(ww_unit_map) / sizeof(ww_unit_map[0]); i++) {
            if (strcmp(unit_buf, ww_unit_map[i].name) == 0) {
                multiplier = ww_unit_map[i].multiplier;
                matched = true;
                break;
            }
        }
        if (!matched) {
            return false;
        }
        total += value * multiplier;
        found = true;
    }

    if (!found || total < 0) {
        return false;
    }
    *out_seconds = total;
    return true;
}

char *human_date(ww_timestamp timestamp, ww_timestamp reference) {
    double ts_seconds = 0.0;
    if (!ww_normalize_timestamp(timestamp, &ts_seconds)) {
        return NULL;
    }
    double ref_seconds = ts_seconds;
    if (reference.kind != WW_TS_NONE) {
        if (!ww_normalize_timestamp(reference, &ref_seconds)) {
            return NULL;
        }
    }
    int64_t ts_days = ww_days_from_unix_seconds(ts_seconds);
    int64_t ref_days = ww_days_from_unix_seconds(ref_seconds);
    int64_t diff_days = ts_days - ref_days;

    if (diff_days == 0) {
        return ww_strdup("Today");
    }
    if (diff_days == -1) {
        return ww_strdup("Yesterday");
    }
    if (diff_days == 1) {
        return ww_strdup("Tomorrow");
    }

    if (diff_days >= -6 && diff_days <= -2) {
        int weekday = ww_day_of_week_from_days(ts_days);
        return ww_strdup_printf("Last %s", ww_weekday_names[weekday]);
    }
    if (diff_days >= 2 && diff_days <= 6) {
        int weekday = ww_day_of_week_from_days(ts_days);
        return ww_strdup_printf("This %s", ww_weekday_names[weekday]);
    }

    int y = 0;
    unsigned m = 0;
    unsigned d = 0;
    int y_ref = 0;
    unsigned m_ref = 0;
    unsigned d_ref = 0;
    ww_civil_from_days(ts_days, &y, &m, &d);
    ww_civil_from_days(ref_days, &y_ref, &m_ref, &d_ref);

    if (y == y_ref) {
        return ww_strdup_printf("%s %u", ww_month_names[m - 1], d);
    }
    return ww_strdup_printf("%s %u, %d", ww_month_names[m - 1], d, y);
}

char *date_range(ww_timestamp start, ww_timestamp end) {
    double start_seconds = 0.0;
    double end_seconds = 0.0;
    if (!ww_normalize_timestamp(start, &start_seconds)) {
        return NULL;
    }
    if (!ww_normalize_timestamp(end, &end_seconds)) {
        return NULL;
    }
    if (start_seconds > end_seconds) {
        double tmp = start_seconds;
        start_seconds = end_seconds;
        end_seconds = tmp;
    }

    int64_t start_days = ww_days_from_unix_seconds(start_seconds);
    int64_t end_days = ww_days_from_unix_seconds(end_seconds);

    int y1 = 0;
    unsigned m1 = 0;
    unsigned d1 = 0;
    int y2 = 0;
    unsigned m2 = 0;
    unsigned d2 = 0;
    ww_civil_from_days(start_days, &y1, &m1, &d1);
    ww_civil_from_days(end_days, &y2, &m2, &d2);

    const char *en_dash = "\xE2\x80\x93";

    if (y1 == y2 && m1 == m2 && d1 == d2) {
        return ww_strdup_printf("%s %u, %d", ww_month_names[m1 - 1], d1, y1);
    }
    if (y1 == y2 && m1 == m2) {
        return ww_strdup_printf("%s %u%s%u, %d", ww_month_names[m1 - 1], d1, en_dash, d2, y1);
    }
    if (y1 == y2) {
        return ww_strdup_printf("%s %u %s %s %u, %d", ww_month_names[m1 - 1], d1, en_dash,
                                ww_month_names[m2 - 1], d2, y1);
    }
    return ww_strdup_printf("%s %u, %d %s %s %u, %d", ww_month_names[m1 - 1], d1, y1, en_dash,
                            ww_month_names[m2 - 1], d2, y2);
}
