#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "whenwords.h"

typedef struct {
    const char *name;
    double timestamp;
    double reference;
    const char *output;
    int error;
} TimeagoTest;

typedef struct {
    const char *name;
    double seconds;
    int has_options;
    int compact;
    int max_units;
    const char *output;
    int error;
} DurationTest;

typedef struct {
    const char *name;
    const char *input;
    double output;
    int error;
} ParseDurationTest;

typedef struct {
    const char *name;
    double timestamp;
    double reference;
    const char *output;
    int error;
} HumanDateTest;

typedef struct {
    const char *name;
    double start;
    double end;
    const char *output;
    int error;
} DateRangeTest;

static const TimeagoTest TIMEAGO_TESTS[] = {
    {"just now - identical timestamps", 1704067200, 1704067200, "just now", 0},
    {"just now - 30 seconds ago", 1704067170, 1704067200, "just now", 0},
    {"just now - 44 seconds ago", 1704067156, 1704067200, "just now", 0},
    {"1 minute ago - 45 seconds", 1704067155, 1704067200, "1 minute ago", 0},
    {"1 minute ago - 89 seconds", 1704067111, 1704067200, "1 minute ago", 0},
    {"2 minutes ago - 90 seconds", 1704067110, 1704067200, "2 minutes ago", 0},
    {"30 minutes ago", 1704065400, 1704067200, "30 minutes ago", 0},
    {"44 minutes ago", 1704064560, 1704067200, "44 minutes ago", 0},
    {"1 hour ago - 45 minutes", 1704064500, 1704067200, "1 hour ago", 0},
    {"1 hour ago - 89 minutes", 1704061860, 1704067200, "1 hour ago", 0},
    {"2 hours ago - 90 minutes", 1704061800, 1704067200, "2 hours ago", 0},
    {"5 hours ago", 1704049200, 1704067200, "5 hours ago", 0},
    {"21 hours ago", 1703991600, 1704067200, "21 hours ago", 0},
    {"1 day ago - 22 hours", 1703988000, 1704067200, "1 day ago", 0},
    {"1 day ago - 35 hours", 1703941200, 1704067200, "1 day ago", 0},
    {"2 days ago - 36 hours", 1703937600, 1704067200, "2 days ago", 0},
    {"7 days ago", 1703462400, 1704067200, "7 days ago", 0},
    {"25 days ago", 1701907200, 1704067200, "25 days ago", 0},
    {"1 month ago - 26 days", 1701820800, 1704067200, "1 month ago", 0},
    {"1 month ago - 45 days", 1700179200, 1704067200, "1 month ago", 0},
    {"2 months ago - 46 days", 1700092800, 1704067200, "2 months ago", 0},
    {"6 months ago", 1688169600, 1704067200, "6 months ago", 0},
    {"10 months ago - 319 days", 1676505600, 1704067200, "10 months ago", 0},
    {"1 year ago - 320 days", 1676419200, 1704067200, "1 year ago", 0},
    {"1 year ago - 547 days", 1656806400, 1704067200, "1 year ago", 0},
    {"2 years ago - 548 days", 1656720000, 1704067200, "2 years ago", 0},
    {"5 years ago", 1546300800, 1704067200, "5 years ago", 0},
    {"future - in just now (30 seconds)", 1704067230, 1704067200, "just now", 0},
    {"future - in 1 minute", 1704067260, 1704067200, "in 1 minute", 0},
    {"future - in 5 minutes", 1704067500, 1704067200, "in 5 minutes", 0},
    {"future - in 1 hour", 1704070200, 1704067200, "in 1 hour", 0},
    {"future - in 3 hours", 1704078000, 1704067200, "in 3 hours", 0},
    {"future - in 1 day", 1704150000, 1704067200, "in 1 day", 0},
    {"future - in 2 days", 1704240000, 1704067200, "in 2 days", 0},
    {"future - in 1 month", 1706745600, 1704067200, "in 1 month", 0},
    {"future - in 1 year", 1735689600, 1704067200, "in 1 year", 0},
};

static const DurationTest DURATION_TESTS[] = {
    {"zero seconds", 0, 0, 0, 0, "0 seconds", 0},
    {"1 second", 1, 0, 0, 0, "1 second", 0},
    {"45 seconds", 45, 0, 0, 0, "45 seconds", 0},
    {"1 minute", 60, 0, 0, 0, "1 minute", 0},
    {"1 minute 30 seconds", 90, 0, 0, 0, "1 minute, 30 seconds", 0},
    {"2 minutes", 120, 0, 0, 0, "2 minutes", 0},
    {"1 hour", 3600, 0, 0, 0, "1 hour", 0},
    {"1 hour 1 minute", 3661, 0, 0, 0, "1 hour, 1 minute", 0},
    {"1 hour 30 minutes", 5400, 0, 0, 0, "1 hour, 30 minutes", 0},
    {"2 hours 30 minutes", 9000, 0, 0, 0, "2 hours, 30 minutes", 0},
    {"1 day", 86400, 0, 0, 0, "1 day", 0},
    {"1 day 2 hours", 93600, 0, 0, 0, "1 day, 2 hours", 0},
    {"7 days", 604800, 0, 0, 0, "7 days", 0},
    {"1 month (30 days)", 2592000, 0, 0, 0, "1 month", 0},
    {"1 year (365 days)", 31536000, 0, 0, 0, "1 year", 0},
    {"1 year 2 months", 36720000, 0, 0, 0, "1 year, 2 months", 0},
    {"compact - 1h 1m", 3661, 1, 1, 0, "1h 1m", 0},
    {"compact - 2h 30m", 9000, 1, 1, 0, "2h 30m", 0},
    {"compact - 1d 2h", 93600, 1, 1, 0, "1d 2h", 0},
    {"compact - 45s", 45, 1, 1, 0, "45s", 0},
    {"compact - 0s", 0, 1, 1, 0, "0s", 0},
    {"max_units 1 - hours only", 3661, 1, 0, 1, "1 hour", 0},
    {"max_units 1 - days only", 93600, 1, 0, 1, "1 day", 0},
    {"max_units 3", 93661, 1, 0, 3, "1 day, 2 hours, 1 minute", 0},
    {"compact max_units 1", 9000, 1, 1, 1, "2h", 0},
    {"error - negative seconds", -100, 0, 0, 0, NULL, 1},
};

static const ParseDurationTest PARSE_DURATION_TESTS[] = {
    {"compact hours minutes", "2h30m", 9000, 0},
    {"compact with space", "2h 30m", 9000, 0},
    {"compact with comma", "2h, 30m", 9000, 0},
    {"verbose", "2 hours 30 minutes", 9000, 0},
    {"verbose with and", "2 hours and 30 minutes", 9000, 0},
    {"verbose with comma and", "2 hours, and 30 minutes", 9000, 0},
    {"decimal hours", "2.5 hours", 9000, 0},
    {"decimal compact", "1.5h", 5400, 0},
    {"single unit minutes verbose", "90 minutes", 5400, 0},
    {"single unit minutes compact", "90m", 5400, 0},
    {"single unit min", "90min", 5400, 0},
    {"colon notation h:mm", "2:30", 9000, 0},
    {"colon notation h:mm:ss", "1:30:00", 5400, 0},
    {"colon notation with seconds", "0:05:30", 330, 0},
    {"days verbose", "2 days", 172800, 0},
    {"days compact", "2d", 172800, 0},
    {"weeks verbose", "1 week", 604800, 0},
    {"weeks compact", "1w", 604800, 0},
    {"mixed verbose", "1 day, 2 hours, and 30 minutes", 95400, 0},
    {"mixed compact", "1d 2h 30m", 95400, 0},
    {"seconds only verbose", "45 seconds", 45, 0},
    {"seconds compact s", "45s", 45, 0},
    {"seconds compact sec", "45sec", 45, 0},
    {"hours hr", "2hr", 7200, 0},
    {"hours hrs", "2hrs", 7200, 0},
    {"minutes mins", "30mins", 1800, 0},
    {"case insensitive", "2H 30M", 9000, 0},
    {"whitespace tolerance", "  2 hours   30 minutes  ", 9000, 0},
    {"error - empty string", "", 0, 1},
    {"error - no units", "hello world", 0, 1},
    {"error - negative", "-5 hours", 0, 1},
    {"error - just number", "42", 0, 1},
};

static const HumanDateTest HUMAN_DATE_TESTS[] = {
    {"today", 1705276800, 1705276800, "Today", 0},
    {"today - same day different time", 1705320000, 1705276800, "Today", 0},
    {"yesterday", 1705190400, 1705276800, "Yesterday", 0},
    {"tomorrow", 1705363200, 1705276800, "Tomorrow", 0},
    {"last Sunday (1 day before Monday)", 1705190400, 1705276800, "Yesterday", 0},
    {"last Saturday (2 days ago)", 1705104000, 1705276800, "Last Saturday", 0},
    {"last Friday (3 days ago)", 1705017600, 1705276800, "Last Friday", 0},
    {"last Thursday (4 days ago)", 1704931200, 1705276800, "Last Thursday", 0},
    {"last Wednesday (5 days ago)", 1704844800, 1705276800, "Last Wednesday", 0},
    {"last Tuesday (6 days ago)", 1704758400, 1705276800, "Last Tuesday", 0},
    {"last Monday (7 days ago) - becomes date", 1704672000, 1705276800, "January 8", 0},
    {"this Tuesday (1 day future)", 1705363200, 1705276800, "Tomorrow", 0},
    {"this Wednesday (2 days future)", 1705449600, 1705276800, "This Wednesday", 0},
    {"this Thursday (3 days future)", 1705536000, 1705276800, "This Thursday", 0},
    {"this Sunday (6 days future)", 1705795200, 1705276800, "This Sunday", 0},
    {"next Monday (7 days future) - becomes date", 1705881600, 1705276800, "January 22", 0},
    {"same year different month", 1709251200, 1705276800, "March 1", 0},
    {"same year end of year", 1735603200, 1705276800, "December 31", 0},
    {"previous year", 1672531200, 1705276800, "January 1, 2023", 0},
    {"next year", 1736121600, 1705276800, "January 6, 2025", 0},
};

static const DateRangeTest DATE_RANGE_TESTS[] = {
    {"same day", 1705276800, 1705276800, "January 15, 2024", 0},
    {"same day different times", 1705276800, 1705320000, "January 15, 2024", 0},
    {"consecutive days same month", 1705276800, 1705363200, "January 15–16, 2024", 0},
    {"same month range", 1705276800, 1705881600, "January 15–22, 2024", 0},
    {"same year different months", 1705276800, 1707955200, "January 15 – February 15, 2024", 0},
    {"different years", 1703721600, 1705276800, "December 28, 2023 – January 15, 2024", 0},
    {"full year span", 1704067200, 1735603200, "January 1 – December 31, 2024", 0},
    {"swapped inputs - should auto-correct", 1705881600, 1705276800, "January 15–22, 2024", 0},
    {"multi-year span", 1672531200, 1735689600, "January 1, 2023 – January 1, 2025", 0},
};

static int expect_string(const char *name, const char *got, const char *expected, int expect_error) {
    if (expect_error) {
        if (got != NULL) {
            fprintf(stderr, "FAIL: %s (expected error, got '%s')\n", name, got);
            free((void *)got);
            return 1;
        }
        return 0;
    }
    if (got == NULL) {
        fprintf(stderr, "FAIL: %s (expected '%s', got error)\n", name, expected);
        return 1;
    }
    if (strcmp(got, expected) != 0) {
        fprintf(stderr, "FAIL: %s (expected '%s', got '%s')\n", name, expected, got);
        free((void *)got);
        return 1;
    }
    free((void *)got);
    return 0;
}

static int expect_number(const char *name, int ok, double got, double expected, int expect_error) {
    if (expect_error) {
        if (ok) {
            fprintf(stderr, "FAIL: %s (expected error, got %.0f)\n", name, got);
            return 1;
        }
        return 0;
    }
    if (!ok) {
        fprintf(stderr, "FAIL: %s (expected %.0f, got error)\n", name, expected);
        return 1;
    }
    if (fabs(got - expected) > 1e-6) {
        fprintf(stderr, "FAIL: %s (expected %.0f, got %.0f)\n", name, expected, got);
        return 1;
    }
    return 0;
}

int main(void) {
    int failures = 0;

    for (size_t i = 0; i < sizeof(TIMEAGO_TESTS) / sizeof(TIMEAGO_TESTS[0]); i++) {
        const TimeagoTest *t = &TIMEAGO_TESTS[i];
        char *result = timeago(ww_timestamp_from_unix(t->timestamp), ww_timestamp_from_unix(t->reference));
        failures += expect_string(t->name, result, t->output, t->error);
    }

    for (size_t i = 0; i < sizeof(DURATION_TESTS) / sizeof(DURATION_TESTS[0]); i++) {
        const DurationTest *t = &DURATION_TESTS[i];
        ww_duration_options options;
        ww_duration_options *opt_ptr = NULL;
        if (t->has_options) {
            options = ww_duration_options_default();
            if (t->compact == 1) {
                options.compact = 1;
            }
            if (t->max_units > 0) {
                options.max_units = t->max_units;
            }
            opt_ptr = &options;
        }
        char *result = duration(t->seconds, opt_ptr);
        failures += expect_string(t->name, result, t->output, t->error);
    }

    for (size_t i = 0; i < sizeof(PARSE_DURATION_TESTS) / sizeof(PARSE_DURATION_TESTS[0]); i++) {
        const ParseDurationTest *t = &PARSE_DURATION_TESTS[i];
        double value = 0.0;
        int ok = parse_duration(t->input, &value);
        failures += expect_number(t->name, ok, value, t->output, t->error);
    }

    for (size_t i = 0; i < sizeof(HUMAN_DATE_TESTS) / sizeof(HUMAN_DATE_TESTS[0]); i++) {
        const HumanDateTest *t = &HUMAN_DATE_TESTS[i];
        char *result = human_date(ww_timestamp_from_unix(t->timestamp), ww_timestamp_from_unix(t->reference));
        failures += expect_string(t->name, result, t->output, t->error);
    }

    for (size_t i = 0; i < sizeof(DATE_RANGE_TESTS) / sizeof(DATE_RANGE_TESTS[0]); i++) {
        const DateRangeTest *t = &DATE_RANGE_TESTS[i];
        char *result = date_range(ww_timestamp_from_unix(t->start), ww_timestamp_from_unix(t->end));
        failures += expect_string(t->name, result, t->output, t->error);
    }

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    printf("%d tests failed.\n", failures);
    return 1;
}
