#include "json.h"

#define nil ((void*)0)
#define must(b) do { if (!(b)) return 0; } while (0)

typedef struct Parser Parser;

struct Parser {
	char *s;
	int  n;
	JSON *j;
	int  nj;
};

static int parsevalue(Parser*, JSON*, JSON**);


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
inititem(Parser *p, JSON *parent, JSON **prev, char type)
{
	p->n++;
	if (p->nj > 0) {
		JSON *v = p->j;
		p->j++;
		p->nj--;
		v->type = type;
		v->src = p->s;
		v->parent = parent;
		v->next = nil;
		v->prev = *prev;
		if (*prev) {
			(*prev)->next = v;
		}
		*prev = v;
		return v;
	}
	return nil;
}


static int
parsenull(Parser *p, JSON *parent, JSON **prev)
{
	JSON *v = inititem(p, parent, prev, 'n');
	must(consume(p, "null"));
	if (v) {
		v->end = p->s;
	}
	return 1;
}


static int
parsetrue(Parser *p, JSON *parent, JSON **prev)
{
	JSON *v = inititem(p, parent, prev, 't');
	must(consume(p, "true"));
	if (v) {
		v->end = p->s;
	}
	return 1;
}


static int
parsefalse(Parser *p, JSON *parent, JSON **prev)
{
	JSON *v = inititem(p, parent, prev, 'f');
	must(consume(p, "false"));
	if (v) {
		v->end = p->s;
	}
	return 1;
}


static int
parsestring(Parser *p, JSON *parent, JSON **prev)
{
	JSON *v = inititem(p, parent, prev, '"');
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
parsenumber(Parser *p, JSON *parent, JSON **prev)
{
	char c;
	JSON *v = inititem(p, parent, prev, '0');
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
parsepair(Parser *p, JSON *parent, JSON **kprev, JSON **vprev)
{
	must(*p->s == '"');
	must(parsestring(p, parent, kprev));
	skipws(p);
	must(consume(p, ":"));
	skipws(p);
	must(parsevalue(p, parent, vprev));
	return 1;
}


static int
parseobject(Parser *p, JSON *parent, JSON **prev)
{
	JSON *kprev = nil, *vprev = nil;
	JSON *v = inititem(p, parent, prev, '{');
	must(consume(p, "{"));
	skipws(p);
	if (*p->s == '}') {
		p->s++;
		if (v) {
			v->end = p->s;
		}
		return 1;
	}

	must(parsepair(p, parent, &kprev, &vprev));
	for (skipws(p); *p->s == ','; skipws(p)) {
		p->s++; // consume ,
		skipws(p);
		must(parsepair(p, parent, &kprev, &vprev));
	}

	must(consume(p, "}"));
	if (v) {
		v->end = p->s;
	}
	return 1;
}


static int
parsearray(Parser *p, JSON *parent, JSON **prev)
{
	JSON *v = inititem(p, parent, prev, '[');
	must(consume(p, "["));
	skipws(p);
	if (*p->s == ']') {
		p->s++;
		if (v) {
			v->end = p->s;
		}
		return 1;
	}

	JSON *aprev = nil;
	must(parsevalue(p, parent, &aprev));
	for (skipws(p); *p->s == ','; skipws(p)) {
		p->s++; // consume ,
		skipws(p);
		must(parsevalue(p, parent, &aprev));
	}

	must(consume(p, "]"));
	if (v) {
		v->end = p->s;
	}
	return 1;
}


static int
parsevalue(Parser *p, JSON *parent, JSON **prev)
{
	switch (*p->s) {
	case '{': return parseobject(p, parent, prev);
	case '[': return parsearray(p, parent, prev);
	case '"': return parsestring(p, parent, prev);
	case 't': return parsetrue(p, parent, prev);
	case 'f': return parsefalse(p, parent, prev);
	case 'n': return parsenull(p, parent, prev);
	case '-':
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return parsenumber(p, parent, prev);
	}
	return 0;
}


static int
parsetext(Parser *p)
{
	JSON *prev = nil;
	switch (*p->s) {
	case '{': return parseobject(p, nil, &prev);
	case '[': return parsearray(p, nil, &prev);
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
