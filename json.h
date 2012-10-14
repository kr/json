typedef struct JSON JSON;

struct JSON {
	char type; // one of: { [ " 0 t f n
	int  len;
	char *src;
	char *end; // src + len
	JSON *parent;
	JSON *next;
	JSON *prev;
};

// Scans src and fills in at most nvalue elements of value
// with pointers to the lexical bounds of JSON values.
// Values are written in the order they appear in src.
//
// Returns the total number of values in src, regardless of
// nvalue. If src is not well-formed JSON, returns 0.
int jsonparse(char *src, JSON *value, int nvalue);
