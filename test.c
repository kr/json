#include <string.h>
#include "ct/ct.h"
#include "json.h"

#define nil ((void*)0)

typedef struct Valid Valid;

struct Valid {
	char *src;
	int  n;
	JSON parts[100];
};

static int r;
static JSON x[100];


char *invalid[] = {
	"\"A JSON payload should be an object or array, not a string.\"",
	"0",
	"true",
	"false",
	"nil",
	"[\"Unclosed array\"",
	"{unquoted_key: \"keys must be quoted\"}",
	"[\"extra comma\",]",
	"[\"double extra comma\",,]",
	"[   , \"<-- missing value\"]",
	"[\"Comma after the close\"],",
	"[\"Extra close\"]]",
	"{\"Extra comma\": true,}",
	"{\"Extra value after close\": true} \"misplaced quoted value\"",
	"{\"Illegal expression\": 1 + 2}",
	"{\"Illegal invocation\": alert()}",
	"{\"Numbers cannot have leading zeroes\": 013}",
	"{\"Numbers cannot be hex\": 0x14}",
	"[\"Illegal backslash escape: \\x15\"]",
	"[\"Illegal backslash escape: \\'\"]",
	"[\"Illegal backslash escape: \\017\"]",
	"{\"Missing colon\" null}",
	"{\"Double colon\":: null}",
	"{\"Comma instead of colon\", null}",
	"[\"Colon instead of comma\": false]",
	"[\"Bad value\", truth]",
	"['single quote']",
	"[\"tab	character	in	string	\"]",
	"[\"tab\\	character\\	in\\	string\\	\"]",
	"[\"line\nbreak\"]",
	"[\"line\\\nbreak\"]",
	"{3: \"invalid key\"}",
	"[\"\\u\", \"\\u1\", \"\\u01\", \"\\u001\", \"\\u0001\", \"\\uxxxx\"]",
	"%",
	"{",
	"{[]:0}",
	"{}x",
	"[01]",
	"",
	nil,
};

Valid valid[] = {
	{"{}", 1, {{'{', 2, "{}"}}},
	{"[]", 1, {{'[', 2, "[]"}}},
	{" {}", 1, {{'{', 2, "{}"}}},
	{"{} ", 1, {{'{', 2, "{}"}}},
	{"{\"k\":\"v\"}", 3, {
		{'{', 9, "{\"k\":\"v\"}"},
		{'"', 3, "\"k\""},
		{'"', 3, "\"v\""},
	}},
	{"[0]", 2, {{'[', 3, "[0]"}, {'0', 1, "0"}}},
	{"[1]", 2, {{'[', 3, "[1]"}, {'0', 1, "1"}}},
	{"[1234567890]", 2, {{'[', 12, "[1234567890]"}, {'0', 10, "1234567890"}}},
	{"[23456789012E666]", 2, {{'[', 17, "[23456789012E666]"}, {'0', 15, "23456789012E666"}}},
	{"[\"// /* <!-- --\"]", 2, {{'[', 17, "[\"// /* <!-- --\"]"}, {'"', 15, "\"// /* <!-- --\""}}},
	{"[\"# -- --> */\"]", 2, {{'[', 15, "[\"# -- --> */\"]"}, {'"', 13, "\"# -- --> */\""}}},
	{"[\"a\"]", 2, {{'[', 5, "[\"a\"]"}, {'"', 3, "\"a\""}}},
	{"[\"\\\"\"]", 2, {{'[', 6, "[\"\\\"\"]"}, {'"', 4, "\"\\\"\""}}},
	{"[\"\\\\\"]", 2, {{'[', 6, "[\"\\\\\"]"}, {'"', 4, "\"\\\\\""}}},
	{"[\"\\b\\f\\n\\r\\t\"]", 2, {
		{'[', 14, "[\"\\b\\f\\n\\r\\t\"]"},
		{'"', 12, "\"\\b\\f\\n\\r\\t\""},
	}},
	{"[false]", 2, {{'[', 7, "[false]"}, {'f', 5, "false"}}},
	{"[true]", 2, {{'[', 6, "[true]"}, {'t', 4, "true"}}},
	{"[null]", 2, {{'[', 6, "[null]"}, {'n', 4, "null"}}},
	{"[-9876.543210]", 2, {{'[', 14, "[-9876.543210]"}, {'0', 12, "-9876.543210"}}},
	{"[1.23456789E+34]", 2, {{'[', 16, "[1.23456789E+34]"}, {'0', 14, "1.23456789E+34"}}},
	{"[0.123456789e-12]", 2, {{'[', 17, "[0.123456789e-12]"}, {'0', 15, "0.123456789e-12"}}},
	{"[1,2,3,4,5,6,7]", 8, {
		{'[', 15, "[1,2,3,4,5,6,7]"},
		{'0', 1, "1"},
		{'0', 1, "2"},
		{'0', 1, "3"},
		{'0', 1, "4"},
		{'0', 1, "5"},
		{'0', 1, "6"},
		{'0', 1, "7"},
	}},
	{"[\"\\u0024\\u00a2\\u20ac\\u5712\\uD834\\uDD1E\"]", 2, {
		{'[', 40, "[\"\\u0024\\u00a2\\u20ac\\u5712\\uD834\\uDD1E\"]"},
		{'"', 38, "\"\\u0024\\u00a2\\u20ac\\u5712\\uD834\\uDD1E\""},
	}},
	{"[\"abcdefghijklmnopqrstuvwyzABCDEFGHIJKLMNOPQRSTUVWYZ0123456789\"]", 2, {
		{'[', 64, "[\"abcdefghijklmnopqrstuvwyzABCDEFGHIJKLMNOPQRSTUVWYZ0123456789\"]"},
		{'"', 62, "\"abcdefghijklmnopqrstuvwyzABCDEFGHIJKLMNOPQRSTUVWYZ0123456789\""},
	}},
	{"[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]", 19, {
		{'[', 38, "[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]"},
		{'[', 36, "[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]"},
		{'[', 34, "[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]"},
		{'[', 32, "[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]"},
		{'[', 30, "[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]"},
		{'[', 28, "[[[[[[[[[[[[[[]]]]]]]]]]]]]]"},
		{'[', 26, "[[[[[[[[[[[[[]]]]]]]]]]]]]"},
		{'[', 24, "[[[[[[[[[[[[]]]]]]]]]]]]"},
		{'[', 22, "[[[[[[[[[[[]]]]]]]]]]]"},
		{'[', 20, "[[[[[[[[[[]]]]]]]]]]"},
		{'[', 18, "[[[[[[[[[]]]]]]]]]"},
		{'[', 16, "[[[[[[[[]]]]]]]]"},
		{'[', 14, "[[[[[[[]]]]]]]"},
		{'[', 12, "[[[[[[]]]]]]"},
		{'[', 10, "[[[[[]]]]]"},
		{'[', 8, "[[[[]]]]"},
		{'[', 6, "[[[]]]"},
		{'[', 4, "[[]]"},
		{'[', 2, "[]"},
	}},
	{},
};


void
cttestcount()
{
	int i;

	for (i = 0; valid[i].src; i++) {
		Valid v;

		v = valid[i];
		r = jsonparse(v.src, nil, 0);
		assertf(r == v.n, "jsonparse(%s) = %d want %d", v.src, r, v.n);
	}
}


void
cttestparse()
{
	int i, j;

	for (i = 0; valid[i].src; i++) {
		Valid v;

		v = valid[i];
		r = jsonparse(v.src, x, 100);
		assertf(r == v.n, "r is %d, exp %d", r, v.n);
		for (j = 0; j < r; j++) {
			JSON exp = v.parts[j], got = x[j];
			assert(exp.len == strlen(exp.src)); // sanity check test data
			assertf(got.type == exp.type, "part[%d].type = '%c' want '%c'", j, got.type, exp.type);
			assertf(strncmp(got.src, exp.src, got.len) == 0, "part[%d].src = %.*s want %s", j, got.len, got.src, exp.src);
			assertf(got.end == got.src + got.len, "part[%d]", j);
		}
	}
}


void
cttestinvalid()
{
	int i;

	for (i = 0; invalid[i]; i++) {
		r = jsonparse(invalid[i], nil, 0);
		assertf(r == 0, "jsonparse(%s) = %d want 0", invalid[i], r);
	}
}
