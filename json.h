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

int jsonparse(char *src, JSON *val, int nval);
