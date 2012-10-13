#include "json.h"

#include <stdio.h>

#define nil ((void*)0)
#define must(b) do { if (!(b)) return 0; } while (0)

typedef struct Parser Parser;

struct Parser {
	char *s;
	int  n;
	JSON *j;
	int  nj;
};

static int parsevalue(Parser*, JSON*);


static int
consume(Parser *p, char *s)
{
	while (*s) {
		must(*p->s == *s);
		p->s++;
		s++;
	}
	return 1;
}


static void
skipws(Parser *p)
{
	while (*p->s == ' ' || *p->s == '\n') {
		p->s++;
	}
}


static void
scandigits(Parser *p)
{
	while ('0' <= *p->s && *p->s <= '9') {
		p->s++;
	}
}


static int
scanhex4(Parser *p)
{
	for (int i = 0; i < 4; i++) {
		char c = *p->s++;
		if (!(('0'<=c && c<='9') || ('a'<=c && c<='f') || ('A'<=c && c<='F'))) {
			return 0;
		}
	}
	return 1;
}


static JSON *
inititem(Parser *p, JSON *parent, char type)
{
	p->n++;
	if (p->nj > 0) {
		JSON *v = p->j;
		p->j++;
		p->nj--;
		v->type = type;
		v->src = p->s;
		v->parent = parent;
		return v;
	}
	return nil;
}


static int
parsenull(Parser *p, JSON *parent)
{
	JSON *v = inititem(p, parent, 'n');
	must(consume(p, "null"));
	if (v) {
		v->end = p->s;
	}
	return 1;
}


static int
parsetrue(Parser *p, JSON *parent)
{
	JSON *v = inititem(p, parent, 't');
	must(consume(p, "true"));
	if (v) {
		v->end = p->s;
	}
	return 1;
}


static int
parsefalse(Parser *p, JSON *parent)
{
	JSON *v = inititem(p, parent, 'f');
	must(consume(p, "false"));
	if (v) {
		v->end = p->s;
	}
	return 1;
}


static int
parsestring(Parser *p, JSON *parent)
{
	JSON *v = inititem(p, parent, '"');
	p->s++; // consume "
	while (*p->s != '"') {
		char c = *p->s;
		must(c >= ' '); // no control chars
		p->s++;
		if (c == '\\') {
			switch (c = *p->s++) {
			case 'b': case 'f': case 'n': case 'r':
			case 't': case '"': case '\\': case '/':
				continue;
			case 'u':
				must(scanhex4(p));
				continue;
			}
			return 0;
		}
	}
	p->s++; // consume "
	if (v) {
		v->end = p->s;
	}
	return 1;
}


static int
parsenumber(Parser *p, JSON *parent)
{
	char c;
	JSON *v = inititem(p, parent, '0');
	if (*p->s == '-') {
		p->s++;
	}
	c = *p->s;
	if ('0' <= c && c <= '9') {
		p->s++;
		if (c != '0') {
			scandigits(p);
		}
	} else {
		return 0;
	}
	if (*p->s == '.') {
		p->s++;
		scandigits(p);
	}
	if (*p->s == 'e' || *p->s == 'E') {
		p->s++;
		if (*p->s == '+' || *p->s == '-') {
			p->s++;
		}
		scandigits(p);
	}
	if (v) {
		v->end = p->s;
	}
	return 1;
}


static int
parsepair(Parser *p, JSON *parent)
{
	must(*p->s == '"');
	must(parsestring(p, parent));
	skipws(p);
	must(consume(p, ":"));
	skipws(p);
	must(parsevalue(p, parent));
	return 1;
}


static int
parseobject(Parser *p, JSON *parent)
{
	JSON *v = inititem(p, parent, '{');
	must(consume(p, "{"));
	skipws(p);
	if (*p->s == '}') {
		p->s++;
		if (v) {
			v->end = p->s;
		}
		return 1;
	}

	must(parsepair(p, parent));
	for (skipws(p); *p->s == ','; skipws(p)) {
		p->s++; // consume ,
		skipws(p);
		must(parsepair(p, parent));
	}

	must(consume(p, "}"));
	if (v) {
		v->end = p->s;
	}
	return 1;
}


static int
parsearray(Parser *p, JSON *parent)
{
	JSON *v = nil;
	if (p->nj > 0) {
		v = p->j;
		p->j++;
		p->nj--;
	}

	p->n++;
	if (v) {
		v->type = '[';
		v->src = p->s;
	}
	must(consume(p, "["));
	skipws(p);
	if (*p->s == ']') {
		p->s++;
		if (v) {
			v->end = p->s;
		}
		return 1;
	}

	must(parsevalue(p, parent));
	for (skipws(p); *p->s == ','; skipws(p)) {
		p->s++; // consume ,
		skipws(p);
		must(parsevalue(p, parent));
	}

	must(consume(p, "]"));
	if (v) {
		v->end = p->s;
	}
	return 1;
}


static int
parsevalue(Parser *p, JSON *parent)
{
	switch (*p->s) {
	case '{': return parseobject(p, parent);
	case '[': return parsearray(p, parent);
	case '"': return parsestring(p, parent);
	case 't': return parsetrue(p, parent);
	case 'f': return parsefalse(p, parent);
	case 'n': return parsenull(p, parent);
	case '-':
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return parsenumber(p, parent);
	}
	return 0;
}


static int
parsetext(Parser *p)
{
	switch (*p->s) {
	case '{': return parseobject(p, nil);
	case '[': return parsearray(p, nil);
	}
	return 0;
}


// Scans src and writes pointers to the lexical bounds of JSON values
// to elements of part.
//
// Returns the total number of values in src, regardless of npart.
// If src is not well-formed JSON, returns 0.
int
jsonparse(char *src, JSON *part, int npart)
{
	Parser p = {};
	p.s = src;
	p.j = part;
	p.nj = npart;
	skipws(&p);
	must(parsetext(&p));
	skipws(&p);
	must(*p.s == '\0');
	if (part) {
		if (p.n < npart) {
			npart = p.n;
		}
		for (int i = 0; i < npart; i++) {
			part[i].len = part[i].end - part[i].src;
		}
	}
	return p.n;
}
