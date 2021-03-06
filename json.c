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


static void
skipws(Parser *p)
{
	while (*p->s == ' ' || *p->s == '\t' || *p->s == '\n' || *p->s == '\r') {
		p->s++;
	}
}


static int
scan(Parser *p, char *s)
{
	while (*s) {
		must(*p->s == *s);
		p->s++;
		s++;
	}
	return 1;
}


static int
scandigits(Parser *p)
{
	int d = 0;
	while ('0' <= *p->s && *p->s <= '9') {
		p->s++;
		d = 1;
	}
	return d;
}


static JSON *
inititem(Parser *p, JSON *parent, JSON **prev, char type)
{
	p->n++;
	if (p->nj < 1) {
		return nil;
	}
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


static void
setend(Parser *p, JSON *v)
{
	if (v) {
		v->end = p->s;
	}
}


static int
parseword(Parser *p, JSON *parent, JSON **prev, char *lit)
{
	JSON *v = inititem(p, parent, prev, lit[0]);
	must(scan(p, lit));
	setend(p, v);
	return 1;
}


static int
parsestring(Parser *p, JSON *parent, JSON **prev)
{
	JSON *v = inititem(p, parent, prev, '"');
	p->s++; /* scan " */
	while (*p->s != '"') {
		char c = *p->s;
		must(c >= ' '); /* no control chars */
		p->s++;
		if (c == '\\') {
			switch (*p->s++) {
			case 'b': case 'f': case 'n': case 'r':
			case 't': case '"': case '\\': case '/':
				continue;
			case 'u':
				{
					int i;
					for (i = 0; i < 4; i++) {
						char c = *p->s++;
						must(('0'<=c && c<='9') ||
								('a'<=c && c<='f') ||
								('A'<=c && c<='F'));
					}
				}
				continue;
			}
			return 0;
		}
	}
	p->s++; /* scan " */
	setend(p, v);
	return 1;
}


static int
parsenumber(Parser *p, JSON *parent, JSON **prev)
{
	JSON *v = inititem(p, parent, prev, '0');
	scan(p, "-"); /* optional */
	if (*p->s == '0') {
		p->s++; /* special case: single 0, no more digits allowed */
	} else {
		must(scandigits(p));
	}
	if (*p->s == '.') {
		p->s++;
		must(scandigits(p));
	}
	if (*p->s == 'e' || *p->s == 'E') {
		p->s++;
		if (*p->s == '+' || *p->s == '-') {
			p->s++;
		}
		must(scandigits(p));
	}
	setend(p, v);
	return 1;
}


static int
parsepair(Parser *p, JSON *parent, JSON **kprev, JSON **vprev)
{
	must(*p->s == '"');
	must(parsestring(p, parent, kprev));
	skipws(p);
	must(scan(p, ":"));
	skipws(p);
	must(parsevalue(p, parent, vprev));
	return 1;
}


static int
parseobject(Parser *p, JSON *parent, JSON **prev)
{
	JSON *v = inititem(p, parent, prev, '{');
	must(scan(p, "{"));
	skipws(p);
	if (*p->s != '}') {
		JSON *kprev = nil, *vprev = nil;
		must(parsepair(p, v, &kprev, &vprev));
		for (skipws(p); *p->s == ','; skipws(p)) {
			p->s++; /* scan , */
			skipws(p);
			must(parsepair(p, v, &kprev, &vprev));
		}
	}
	must(scan(p, "}"));
	setend(p, v);
	return 1;
}


static int
parsearray(Parser *p, JSON *parent, JSON **prev)
{
	JSON *v = inititem(p, parent, prev, '[');
	must(scan(p, "["));
	skipws(p);
	if (*p->s != ']') {
		JSON *aprev = nil;
		must(parsevalue(p, v, &aprev));
		for (skipws(p); *p->s == ','; skipws(p)) {
			p->s++; /* scan , */
			skipws(p);
			must(parsevalue(p, v, &aprev));
		}
	}
	must(scan(p, "]"));
	setend(p, v);
	return 1;
}


static int
parsevalue(Parser *p, JSON *parent, JSON **prev)
{
	switch (*p->s) {
	case '{': return parseobject(p, parent, prev);
	case '[': return parsearray(p, parent, prev);
	case '"': return parsestring(p, parent, prev);
	case 't': return parseword(p, parent, prev, "true");
	case 'f': return parseword(p, parent, prev, "false");
	case 'n': return parseword(p, parent, prev, "null");
	case '-':
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return parsenumber(p, parent, prev);
	}
	return 0;
}


int
jsonparse(char *src, JSON *part, int npart)
{
	JSON *prev = nil;
	Parser p = {};
	p.s = src;
	p.j = part;
	p.nj = npart;
	skipws(&p);
	/* a "json text" must be an array or object */
	must(*p.s == '{' || *p.s == '[');
	must(parsevalue(&p, nil, &prev));
	skipws(&p);
	must(*p.s == '\0');
	if (part) {
		if (p.n < npart) {
			npart = p.n;
		}
		int i;
		for (i = 0; i < npart; i++) {
			part[i].len = part[i].end - part[i].src;
		}
	}
	return p.n;
}
