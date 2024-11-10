/*

          MMXXIV November 10 PUBLIC DOMAIN by JML

     The authors and contributors disclaim copyright, patents
           and all related rights to this software.

 Anyone is free to copy, modify, publish, use, compile, sell, or
 distribute this software, either in source code form or as a
 compiled binary, for any purpose, commercial or non-commercial,
 and by any means.

 The authors waive all rights to patents, both currently owned
 by the authors or acquired in the future, that are necessarily
 infringed by this software, relating to make, have made, repair,
 use, sell, import, transfer, distribute or configure hardware
 or software in finished or intermediate form, whether by run,
 manufacture, assembly, testing, compiling, processing, loading
 or applying this software or otherwise.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT OF ANY PATENT, COPYRIGHT, TRADE SECRET OR OTHER
 PROPRIETARY RIGHT.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
 ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct pair {
	struct pair *next;
	char *value;
	char name[1];
};

struct rik {
	struct rik *parent;
	char *buf;
	int pos;
	int end;
	int state;
	int depth;
	int line;
	char *file;
	struct pair *structs;
	struct pair *funcs;
};

struct rik *load(char *file);
int rik__delete(struct rik *st);
int k2c(struct rik *st);

int file_size(char *path)
{
	FILE *fp;
	int si;
	fp = fopen(path, "rb");
	if (!fp) {
		return 0;
	}
	fseek(fp, 0, SEEK_END);
	si = ftell(fp);
	fclose(fp);
	return si;
}

char *file_load(char *path, int size)
{
	char *buf;
	FILE *fp;
	int ret;

	fp = fopen(path, "rb");
	if (!fp) {
		return 0;
	}
	buf = malloc(size+1);
	if (!buf) {
		return 0;
	}
	ret = fread(buf, 1, size, fp);
	if (ret != size) {
		free(buf);
		buf = 0;
	}
	buf[size] = '\0';
	fclose(fp);
	return buf;
}

int error(char *txt, struct rik *st)
{
	printf("#error %s @ line %d\n", txt, st->line);
	exit(-1);
}

int whitespaces(struct rik *st, int semi)
{
	int end;
	while (st->pos < st->end) {
		switch (st->buf[st->pos]) {
		case '\n':
			st->line++;
		case ' ':
		case '\t':
		case '\v':
		case '\r':
			break;
		case ';':
			if (!semi) {
				return 0;
			}
			st->pos++;
			end = 0;
			while (!end && st->pos < st->end) {
				switch (st->buf[st->pos]) {
				case '\n':
					st->line++;
					end = 1;
					break;
				default:
					st->pos++;
				}
			}
			break;
		default:
			return 0;
		}
		st->pos++;
	}
	return 1;
}

int id_len(char *b)
{
	int i;
	i = 0;
	while ((b[i] >= 'a' && b[i] <= 'z') ||
		(b[i] >= 'A' && b[i] <= 'Z') ||
		(b[i] == '_') ||
		(i > 0 && b[i] >= '0' && b[i] <= '9')) 
	{
		if (i > 1020) {
			printf("#error identifier too long\n");
		}
		i++;
	}
	return i;
}

struct pair *pair_create(struct rik *st)
{
	char *b;
	int i;
	struct pair *p;

	b = st->buf + st->pos;
	i = id_len(b);
	p = malloc(sizeof(*p) + i + 2);
	p->next = 0;
	p->value = 0;
	p->name[i] = '\0';
	st->pos += i;
	while (i > 0) {
		i--;
		p->name[i] = b[i];
	}
	return p;	
}

struct pair *named_parameters(struct rik *st)
{
	char *b;
	int i;
	struct pair *p;
	char *v;
	int allo;
	int l;

	p = pair_create(st);
	if (whitespaces(st,0) || st->buf[st->pos] != '(') {
		error("syntax error expecting '('", st);
	}
	st->pos++;
	allo = 254;
	v = malloc(allo + 2);
	i = 0;
	while (!whitespaces(st,1) && st->buf[st->pos] != ')') {
		b = st->buf + st->pos;
		l = id_len(b);
		if (l < 1) {
			error("syntax error in struct ", st);
		}
		if (i + l >= allo) {
			allo += 256;
			v = realloc(v, allo + 2); 
		}
		if (i > 0) {
			v[i] = ',';
			i++;
		}
		st->pos += l;
		while (l > 0) {
			v[i] = *b;
			i++;
			b++;
			l--;
		}
		if (whitespaces(st,1) || (st->buf[st->pos] != ',')) 
		{
			if (st->buf[st->pos] != ')') {
				error("syntax error expecting ','", st);
			}
		} else {
			st->pos++;
		}
	}
	v[i] = '\0';
	p->value = v;
	st->pos++;
	return p;
}

int struct_process(struct rik *st)
{
	struct pair *p;
	struct pair *op;

	p = named_parameters(st);
	op = st->structs;
	if (!op) {
		st->structs = p;
	} else {
		while (op->next) {
			op = op->next;
		}
		op->next = p;
	}
	return 0;
}

int declare_func(struct rik *st)
{
	struct pair *p;
	struct pair *op;

	p = named_parameters(st);
	op = st->funcs;
	if (!op) {
		st->funcs = p;
	} else {
		while (op->next) {
			op = op->next;
		}
		op->next = p;
	}
	return 0;
}

int func_process(struct rik *st)
{
	declare_func(st);
	if (whitespaces(st,0) || (st->buf[st->pos] != '{')) {
		if (st->buf[st->pos] != ';') {
				error("expecting ';' or '{'", st);
		} else {
			return 0;
		}
	}
	whitespaces(st,1);
	if (st->buf[st->pos] != '{') {
		error("expecting '{'", st);
	}
	st->depth = 0;	
	st->pos++;
	while (st->depth >= 0 && !whitespaces(st,1)) {
		switch (st->buf[st->pos]) {
		case '$':
			break;
		case '{':
			st->depth++;
			break;
		case '}':
			st->depth--;
			break;
		}
		st->pos++;
	}
	return 0;
}

int include_process(struct rik *st)
{
	char *b;
	int i;
	char *fn;
	struct rik *nst;

	b = st->buf + st->pos;
	if (b[0] != '"') {
		error("expecting '\"' ", st);
	}
	b++;
	st->pos++;
	i = 0;	
	fn = malloc(2048);
	strcpy(fn, st->file);
	i = strlen(fn);
	while (i > 0) {
		i--;
		if (fn[i] == '\\' || fn[i] == '/') {
			i++;
			break;
		}
	}
	while (i < 2040) {
		st->pos++;
		if (*b != '"') {
			fn[i] = *b;
			b++;
			i++;
		} else {
			break;
		}
	}
	if (*b != '"') {
		error("syntax error", st);
	}
	fn[i] = '\0';
	nst = load(fn);
	if (nst) {
		nst->parent = st;
		k2c(nst);
		rik__delete(nst);
	} else {
		printf("#error '%s' ", fn);
		error("cannot load file ", st);
	}
	free(fn);
	return 0;
}

int k2c(struct rik *st)
{
	char *b;
	whitespaces(st,1);
	while (st->pos < st->end) {
		putc(st->buf[st->pos], stdout);
		switch (st->state) {
		case 0:
			switch (st->buf[st->pos]) {
			case '#':
				st->pos++;
				if (whitespaces(st,1)) {
					error("syntax error", st);
				}
				b = st->buf + st->pos;
				switch (b[0]) {
				case 'i':
					if (!strncmp(b, "include",7)) {
						st->state = 2;
						st->pos += 7;
					}
					break;
				case 's':
					if (!strncmp(b, "struct", 6)) {
						st->state = 3;
						st->pos += 6;
					}
					break;
				}
				break;
			case '@':
				if (whitespaces(st,1)) {
					error("syntax error", st);
				}
				st->state = 1;
			}
			st->pos++;
			break;
		case 2: /* include */
			include_process(st);
			st->pos++;
			st->state = 0;
			break;
		case 3: /* struct */
			struct_process(st);
			st->pos++;
			st->state = 0;
			break;
		case 1: /* function */
			func_process(st);
			st->pos++;
			st->state = 0;
			break;
		default:
			st->pos++;
			st->state = 0;
		}
		whitespaces(st,1);
	}
}

struct rik *load(char *file) 
{
	struct rik *st;
	st = malloc(sizeof(*st));
	st->line = 1;
	st->end = file_size(file);
	st->buf = file_load(file, st->end);
	st->state = 0;
	st->pos = 0;
	st->file = strdup(file);
	st->structs = 0;
	st->funcs = 0;
	st->parent = 0;
	if (!st->buf) {
		free(st);
		st = NULL;
	}
	return st;
}

int rik__delete(struct rik *st)
{
	free(st->buf);
	free(st->file);
	free(st);
	return 0;
}

int main(int argc, char *argv[]) 
{
	struct rik *st;
	switch (argc > 1) {
	case 1:
		st = load(argv[1]);
		if (st) {
			k2c(st);
			rik__delete(st);
		}
	}
	return 0;
}
