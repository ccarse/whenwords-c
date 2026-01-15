#!/usr/bin/env ruby
# frozen_string_literal: true

require 'yaml'

def c_escape(str)
  str.gsub('\\', '\\\\').gsub('"', '\\"').gsub("\n", "\\n")
end

data = YAML.load_file('tests.yaml')

File.open('tests.c', 'w') do |f|
  f.puts "#include <math.h>"
  f.puts "#include <stdio.h>"
  f.puts "#include <stdlib.h>"
  f.puts "#include <string.h>"
  f.puts "#include \"whenwords.h\""
  f.puts
  f.puts "typedef struct {"
  f.puts "    const char *name;"
  f.puts "    double timestamp;"
  f.puts "    double reference;"
  f.puts "    const char *output;"
  f.puts "    int error;"
  f.puts "} TimeagoTest;"
  f.puts
  f.puts "typedef struct {"
  f.puts "    const char *name;"
  f.puts "    double seconds;"
  f.puts "    int has_options;"
  f.puts "    int compact;"
  f.puts "    int max_units;"
  f.puts "    const char *output;"
  f.puts "    int error;"
  f.puts "} DurationTest;"
  f.puts
  f.puts "typedef struct {"
  f.puts "    const char *name;"
  f.puts "    const char *input;"
  f.puts "    double output;"
  f.puts "    int error;"
  f.puts "} ParseDurationTest;"
  f.puts
  f.puts "typedef struct {"
  f.puts "    const char *name;"
  f.puts "    double timestamp;"
  f.puts "    double reference;"
  f.puts "    const char *output;"
  f.puts "    int error;"
  f.puts "} HumanDateTest;"
  f.puts
  f.puts "typedef struct {"
  f.puts "    const char *name;"
  f.puts "    double start;"
  f.puts "    double end;"
  f.puts "    const char *output;"
  f.puts "    int error;"
  f.puts "} DateRangeTest;"
  f.puts

  f.puts "static const TimeagoTest TIMEAGO_TESTS[] = {"
  data.fetch('timeago', []).each do |t|
    name = c_escape(t['name'])
    input = t['input'] || {}
    ts = input['timestamp']
    ref = input['reference']
    output = t['output']
    error = t['error'] ? 1 : 0
    output_str = output ? "\"#{c_escape(output)}\"" : 'NULL'
    f.puts "    {\"#{name}\", #{ts}, #{ref}, #{output_str}, #{error}},"
  end
  f.puts "};"
  f.puts

  f.puts "static const DurationTest DURATION_TESTS[] = {"
  data.fetch('duration', []).each do |t|
    name = c_escape(t['name'])
    input = t['input'] || {}
    seconds = input['seconds']
    options = input['options']
    output = t['output']
    error = t['error'] ? 1 : 0
    has_options = options ? 1 : 0
    compact = options && options.key?('compact') ? (options['compact'] ? 1 : 0) : 0
    max_units = options && options.key?('max_units') ? options['max_units'] : 0
    output_str = output ? "\"#{c_escape(output)}\"" : 'NULL'
    f.puts "    {\"#{name}\", #{seconds}, #{has_options}, #{compact}, #{max_units}, #{output_str}, #{error}},"
  end
  f.puts "};"
  f.puts

  f.puts "static const ParseDurationTest PARSE_DURATION_TESTS[] = {"
  data.fetch('parse_duration', []).each do |t|
    name = c_escape(t['name'])
    input = t['input']
    output = t['output']
    error = t['error'] ? 1 : 0
    input_str = input ? "\"#{c_escape(input)}\"" : 'NULL'
    output_val = output ? output : 0
    f.puts "    {\"#{name}\", #{input_str}, #{output_val}, #{error}},"
  end
  f.puts "};"
  f.puts

  f.puts "static const HumanDateTest HUMAN_DATE_TESTS[] = {"
  data.fetch('human_date', []).each do |t|
    name = c_escape(t['name'])
    input = t['input'] || {}
    ts = input['timestamp']
    ref = input['reference']
    output = t['output']
    error = t['error'] ? 1 : 0
    output_str = output ? "\"#{c_escape(output)}\"" : 'NULL'
    f.puts "    {\"#{name}\", #{ts}, #{ref}, #{output_str}, #{error}},"
  end
  f.puts "};"
  f.puts

  f.puts "static const DateRangeTest DATE_RANGE_TESTS[] = {"
  data.fetch('date_range', []).each do |t|
    name = c_escape(t['name'])
    input = t['input'] || {}
    start = input['start']
    ending = input['end']
    output = t['output']
    error = t['error'] ? 1 : 0
    output_str = output ? "\"#{c_escape(output)}\"" : 'NULL'
    f.puts "    {\"#{name}\", #{start}, #{ending}, #{output_str}, #{error}},"
  end
  f.puts "};"
  f.puts

  f.puts "static int expect_string(const char *name, const char *got, const char *expected, int expect_error) {"
  f.puts "    if (expect_error) {"
  f.puts "        if (got != NULL) {"
  f.puts "            fprintf(stderr, \"FAIL: %s (expected error, got '%s')\\n\", name, got);"
  f.puts "            free((void *)got);"
  f.puts "            return 1;"
  f.puts "        }"
  f.puts "        return 0;"
  f.puts "    }"
  f.puts "    if (got == NULL) {"
  f.puts "        fprintf(stderr, \"FAIL: %s (expected '%s', got error)\\n\", name, expected);"
  f.puts "        return 1;"
  f.puts "    }"
  f.puts "    if (strcmp(got, expected) != 0) {"
  f.puts "        fprintf(stderr, \"FAIL: %s (expected '%s', got '%s')\\n\", name, expected, got);"
  f.puts "        free((void *)got);"
  f.puts "        return 1;"
  f.puts "    }"
  f.puts "    free((void *)got);"
  f.puts "    return 0;"
  f.puts "}"
  f.puts

  f.puts "static int expect_number(const char *name, int ok, double got, double expected, int expect_error) {"
  f.puts "    if (expect_error) {"
  f.puts "        if (ok) {"
  f.puts "            fprintf(stderr, \"FAIL: %s (expected error, got %.0f)\\n\", name, got);"
  f.puts "            return 1;"
  f.puts "        }"
  f.puts "        return 0;"
  f.puts "    }"
  f.puts "    if (!ok) {"
  f.puts "        fprintf(stderr, \"FAIL: %s (expected %.0f, got error)\\n\", name, expected);"
  f.puts "        return 1;"
  f.puts "    }"
  f.puts "    if (fabs(got - expected) > 1e-6) {"
  f.puts "        fprintf(stderr, \"FAIL: %s (expected %.0f, got %.0f)\\n\", name, expected, got);"
  f.puts "        return 1;"
  f.puts "    }"
  f.puts "    return 0;"
  f.puts "}"
  f.puts

  f.puts "int main(void) {"
  f.puts "    int failures = 0;"
  f.puts
  f.puts "    for (size_t i = 0; i < sizeof(TIMEAGO_TESTS) / sizeof(TIMEAGO_TESTS[0]); i++) {"
  f.puts "        const TimeagoTest *t = &TIMEAGO_TESTS[i];"
  f.puts "        char *result = timeago(ww_timestamp_from_unix(t->timestamp), ww_timestamp_from_unix(t->reference));"
  f.puts "        failures += expect_string(t->name, result, t->output, t->error);"
  f.puts "    }"
  f.puts
  f.puts "    for (size_t i = 0; i < sizeof(DURATION_TESTS) / sizeof(DURATION_TESTS[0]); i++) {"
  f.puts "        const DurationTest *t = &DURATION_TESTS[i];"
  f.puts "        ww_duration_options options;"
  f.puts "        ww_duration_options *opt_ptr = NULL;"
  f.puts "        if (t->has_options) {"
  f.puts "            options = ww_duration_options_default();"
  f.puts "            if (t->compact == 1) {"
  f.puts "                options.compact = 1;"
  f.puts "            }"
  f.puts "            if (t->max_units > 0) {"
  f.puts "                options.max_units = t->max_units;"
  f.puts "            }"
  f.puts "            opt_ptr = &options;"
  f.puts "        }"
  f.puts "        char *result = duration(t->seconds, opt_ptr);"
  f.puts "        failures += expect_string(t->name, result, t->output, t->error);"
  f.puts "    }"
  f.puts
  f.puts "    for (size_t i = 0; i < sizeof(PARSE_DURATION_TESTS) / sizeof(PARSE_DURATION_TESTS[0]); i++) {"
  f.puts "        const ParseDurationTest *t = &PARSE_DURATION_TESTS[i];"
  f.puts "        double value = 0.0;"
  f.puts "        int ok = parse_duration(t->input, &value);"
  f.puts "        failures += expect_number(t->name, ok, value, t->output, t->error);"
  f.puts "    }"
  f.puts
  f.puts "    for (size_t i = 0; i < sizeof(HUMAN_DATE_TESTS) / sizeof(HUMAN_DATE_TESTS[0]); i++) {"
  f.puts "        const HumanDateTest *t = &HUMAN_DATE_TESTS[i];"
  f.puts "        char *result = human_date(ww_timestamp_from_unix(t->timestamp), ww_timestamp_from_unix(t->reference));"
  f.puts "        failures += expect_string(t->name, result, t->output, t->error);"
  f.puts "    }"
  f.puts
  f.puts "    for (size_t i = 0; i < sizeof(DATE_RANGE_TESTS) / sizeof(DATE_RANGE_TESTS[0]); i++) {"
  f.puts "        const DateRangeTest *t = &DATE_RANGE_TESTS[i];"
  f.puts "        char *result = date_range(ww_timestamp_from_unix(t->start), ww_timestamp_from_unix(t->end));"
  f.puts "        failures += expect_string(t->name, result, t->output, t->error);"
  f.puts "    }"
  f.puts
  f.puts "    if (failures == 0) {"
  f.puts "        printf(\"All tests passed.\\n\");"
  f.puts "        return 0;"
  f.puts "    }"
  f.puts "    printf(\"%d tests failed.\\n\", failures);"
  f.puts "    return 1;"
  f.puts "}"
end
