# whenwords for C

Human-friendly time formatting and parsing.

## Installation

Add `whenwords.c` and `whenwords.h` to your project and compile them with your sources.

## Quick start

```c
#include <stdio.h>
#include <stdlib.h>
#include "whenwords.h"

int main(void) {
    char *ago = timeago(ww_timestamp_from_unix(1704065400),
                        ww_timestamp_from_unix(1704067200));
    char *dur = duration(3661, NULL);

    double seconds = 0.0;
    if (parse_duration("2h 30m", &seconds)) {
        printf("2h 30m = %.0f seconds\n", seconds);
    }

    char *human = human_date(ww_timestamp_from_unix(1705276800),
                             ww_timestamp_from_unix(1705276800));
    char *range = date_range(ww_timestamp_from_unix(1705276800),
                             ww_timestamp_from_unix(1705363200));

    printf("%s\n%s\n%s\n%s\n", ago, dur, human, range);

    free(ago);
    free(dur);
    free(human);
    free(range);
    return 0;
}
```

## Functions

### timeago(timestamp, reference?) -> string

```c
char *timeago(ww_timestamp timestamp, ww_timestamp reference);
```

- `timestamp`: Unix seconds, ISO 8601 string, or `struct tm` via `ww_timestamp` helpers.
- `reference`: Optional comparison timestamp. Use `ww_timestamp_unset()` to default to `timestamp`.
- Returns a newly allocated string. Caller must `free()` it. Returns `NULL` on error.

Example:

```c
char *s = timeago(ww_timestamp_from_iso("2024-01-01T00:00:00Z"),
                  ww_timestamp_from_unix(1704067200));
```

### duration(seconds, options?) -> string

```c
char *duration(double seconds, const ww_duration_options *options);
```

- `seconds`: Non-negative duration in seconds.
- `options`: Optional formatting options.
  - `compact`: `1` for compact style (e.g., `2h 30m`).
  - `max_units`: Maximum units to display.
- Returns a newly allocated string. Caller must `free()` it. Returns `NULL` on error.

Example:

```c
ww_duration_options opts = ww_duration_options_default();
opts.compact = 1;
char *s = duration(9000, &opts); // "2h 30m"
```

### parse_duration(string) -> number

```c
bool parse_duration(const char *input, double *out_seconds);
```

- Parses inputs like `"2h 30m"`, `"1.5h"`, `"2:30"`, `"2 hours and 30 minutes"`.
- Returns `true` on success and writes seconds to `out_seconds`.
- Returns `false` on error.

Example:

```c
double seconds = 0.0;
if (parse_duration("1:30:00", &seconds)) {
    // seconds == 5400
}
```

### human_date(timestamp, reference?) -> string

```c
char *human_date(ww_timestamp timestamp, ww_timestamp reference);
```

- Produces contextual dates like `"Today"`, `"Yesterday"`, or `"March 1"`.
- Returns a newly allocated string. Caller must `free()` it. Returns `NULL` on error.

### date_range(start, end) -> string

```c
char *date_range(ww_timestamp start, ww_timestamp end);
```

- Formats a date range with smart abbreviation.
- Returns a newly allocated string. Caller must `free()` it. Returns `NULL` on error.

## Error handling

- String-returning functions (`timeago`, `duration`, `human_date`, `date_range`) return `NULL` on error.
- `parse_duration` returns `false` on error.

## Accepted types

Use these helpers to create timestamps:

```c
ww_timestamp ww_timestamp_from_unix(double seconds);
ww_timestamp ww_timestamp_from_iso(const char *iso8601);
ww_timestamp ww_timestamp_from_tm(const struct tm *tm);
ww_timestamp ww_timestamp_unset(void);
```

All calendar computations are UTC-based.
