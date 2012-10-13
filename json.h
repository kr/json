typedef struct JSON JSON;

struct JSON {
	char type; // one of: { [ " 0 t f n
	int  len;
	char *src;
	char *end; // src + len
	JSON *parent;
};

// Scans src and fills in elements of part with pointers to the lexical
// bounds of JSON values.
//
// Returns the total number of values in src, regardless of npart.
// If src is not well-formed JSON, returns 0.
int jsonparse(char *src, JSON *part, int npart);
