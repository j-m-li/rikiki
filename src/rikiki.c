/*
         rikiki programing language to C translator


          MMXXIV November 11 PUBLIC DOMAIN by JML

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
	int retok;
	int state;
	int depth;
	int line;
	int indent;
	char *file;
	struct pair *structs;
	struct pair *funcs;
	struct pair *consts;
	struct pair *current;
	char *vars;
	int vars_allo;
	char tmp[128];
};

struct rik *load(char *file);
int rik__delete(struct rik *st);
int k2c(struct rik *st);
int expression(struct rik *st);
int statement(struct rik *st);

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
	printf("\n#error \"%s @ line %d in %s\"\n", txt, st->line, st->file);
	exit(-1);
}

int indent(struct rik *st)
{
	int i = st->indent;
	while (i > 0) {
		printf("\t");
		i--;
	}
	return 0;
}

int whitespaces(struct rik *st)
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
			st->pos++;
			break;
		case '/':
			if (st->buf[st->pos+1] != '/') {
				return 0;
			}
			st->pos++;
			st->pos++;
			end = 0;
			while (!end && st->pos < st->end) {
				switch (st->buf[st->pos]) {
				case '\n':
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
	}
	return 1;
}

int string_len(char *b)
{
	int e = b[0];
	int i;
	i = 1;
	while (b[i] != e) {
		if (b[i] == '\\') {
			i++;
		}
		if (i > 1020) {
			printf("#error string too long\n");
		}
		i++;
	}
	i++;
	return i;
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

int printsub(char *name, int len)
{
	int o;
	o = name[len];
	name[len] = '\0';
	printf("%s", name);
	name[len] = o;
	return 0;
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
	if (whitespaces(st) || st->buf[st->pos] != '(') {
		error("syntax error expecting '('", st);
	}
	st->pos++;
	allo = 254;
	v = malloc(allo + 2);
	i = 0;
	while (!whitespaces(st) && st->buf[st->pos] != ')') {
		b = st->buf + st->pos;
		l = id_len(b);
		if (l < 1) {
			error("expecting identifier ", st);
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
		if (whitespaces(st) || (st->buf[st->pos] != ',')) 
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

char *constant(struct rik *st)
{
	int i;
	char *b;
	b = st->buf + st->pos;
	i = 0; /* FIXME buffer overflow */
	if (b[i] == '\'') {
		st->tmp[i] = b[i];
		i++;
		st->tmp[i] = b[i];
		while (b[i] != '\'' && i < 100) {
			if (b[i] == '\\') {
				i++;
				st->tmp[i] = b[i];
			}
			i++;
			st->tmp[i] = b[i];
		}
		i++;
	} else if (b[i] == '+' || b[i] == '-') {
		st->tmp[i] = b[i];
		i++;
	}
	while (b[i] >= '0' && b[i] <= '9') {
		st->tmp[i] = b[i];
		if (i > 100) {
			error("constant too large", st);
		}
		i++;
	}
	if (i < 1) {
		return NULL;
	}
	st->tmp[i] = '\0';
	st->pos += i;
	return st->tmp;	
}

int const_process(struct rik *st)
{
	struct pair *p;
	struct pair *op;
	char *c;
	int l;
	int o;

	p = pair_create(st);
	whitespaces(st);
	c = st->buf + st->pos;
	l = 0;
	while (st->pos < st->end && c[l] != ';') {
		l++;
		st->pos++;
	}
	o = c[l];
	c[l] = '\0';
	p->value = strdup(c);
	c[l] = o;
	printf("#define %s %s\n", p->name, p->value);
	whitespaces(st);
	while (st->parent) {
		st = st->parent;
	}
	op = st->consts;
	if (!op) {
		st->consts = p;
	} else {
		while (op->next) {
			op = op->next;
		}
		op->next = p;
	}
	return 0;
}


int struct_process(struct rik *st)
{
	struct pair *p;
	struct pair *op;
	char *b;
	char *e;
	int o;
	p = named_parameters(st);
	printf("struct %s {\n", p->name);
	b = p->value;
	e = b;
	while (*e) {
		e++;
		if (*e == ',' || *e == '\0') {
			o = *e;
			*e = '\0';
			printf("\tvar %s;\n", b);
			*e = o;
			if (o) {
				b = e + 1;
			}
		}
	}
	printf("};\n", p->name);

	while (st->parent) {
		st = st->parent;
	}
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
	char *b;
	char *e;
	int o;

	p = named_parameters(st);
	printf("var %s(", p->name);
	b = p->value;
	e = b;
	while (*e) {
		e++;
		if (*e == ',' || *e == '\0') {
			o = *e;
			*e = '\0';
			printf("var %s", b);
			*e = o;
			if (o) {
				b = e + 1;
				printf(", ");
			}
		}
	}
	if (e == b) {
		printf("void)");
	} else {
		printf(")");
	}

	while (st->parent) {
		st = st->parent;
	}
	op = st->funcs;
	if (!op) {
		st->funcs = p;
	} else {
		while (op->next) {
			op = op->next;
		}
		op->next = p;
	}
	st->current = p;
	return 0;
}

int var_decl(struct rik *st)
{
	char *b;
	int i;
	int l;
	char *v;
	struct rik *pst;

	pst = st;
	while (pst->parent) {
		pst = pst->parent;
	}

	st->pos++;
	if (pst->vars) {
		i = strlen(pst->vars);
	} else {
		i = 0;
		pst->vars_allo = 128;
		pst->vars = malloc(pst->vars_allo);
		pst->vars[0] == '\0';
	}
	whitespaces(st);
	b = st->buf + st->pos;
	while (*b != ';' && st->pos < st->end) {
		l = id_len(b);
		if (l < 1) {
			error("identifier expected", st);
		}
		while (pst->vars_allo < i + l + 2) {
			pst->vars_allo += 128;
			pst->vars = realloc(pst->vars, pst->vars_allo);
		}
		v = pst->vars + i;
		st->pos += l;
		while (l > 0) {
			pst->vars[i] = *b;
			i++;
			b++;
			l--;
		}	
		whitespaces(st);
		pst->vars[i] = '\0';
		printf("\tvar %s;\n", v);
		if (st->buf[st->pos] == ',') {
			pst->vars[i] = ',';
			i++;
			pst->vars[i] = '\0';
			st->pos++;
			whitespaces(st);
		}
		b = st->buf + st->pos;
	}
	whitespaces(st);
	if (st->buf[st->pos] == ';') {
		st->pos++;
	}
	return 0;
}

int op_group(struct rik *st)
{
	int end = 0;
	char *b;
	printf("(");
	st->pos++;
	while (!end && st->pos < st->end) {
		whitespaces(st);
		if (st->buf[st->pos] == ')') {
			end = 1;
		} else if (st->buf[st->pos] == ';') {
			st->pos++;
		} else {
			expression(st);
			whitespaces(st);
		}
	}
	if (st->buf[st->pos] != ')') {
		error("missing ')'", st);
	}
	printf(")");
	st->pos++;
	return 0;
}


int op_call(struct rik *st, char *name, int len)
{
	int end = 0;
	char *b;
	printsub(name, len);
	printf("(");
	st->pos++;
	while (!end && st->pos < st->end) {
		whitespaces(st);
		if (st->buf[st->pos] == ')') {
			end = 1;
		} else if (st->buf[st->pos] == ';') {
			st->pos++;
		} else {
			expression(st);
			whitespaces(st);
			if (st->buf[st->pos] == ',') {
				printf(", ");
				st->pos++;
			}
		}
	}
	if (st->buf[st->pos] != ')') {
		error("missing ')'", st);
	}
	printf(")");
	st->pos++;
	return 0;
}


int op_dot(struct rik *st, char *name, int len)
{
	int l;
	char *b;
	st->pos++;
	whitespaces(st);
	b = st->buf + st->pos;
	if (*b == '(') {
		printf("((var (*)())");
		printsub(name, len);
		printf(")");
		return op_call(st, name, 0);
	}
	l = id_len(b);
	if (l < 1) {
		error("expecting struct identifier after '.'", st);
	}
	printf("(((struct ");
	printsub(b,l);
	printf("*)");
	printsub(name, len);
	st->pos += l;
	whitespaces(st);
	if (st->buf[st->pos] != '.') {
		printf("))");
		return 0;
	}
	printf(")->");
	st->pos++;
	whitespaces(st);
	b = st->buf + st->pos;
	l = id_len(b);
	if (l < 1) {
		error("struct member after '. .'", st);
	}
	printsub(b,l);
	printf(")");
	st->pos += l;
	return 0;
}

int op_index(struct rik *st, char *name, int len)
{	int l;
	char *b;
	st->pos++;
	if (len < 1) {
		error("expecting identifier", st);
	}
	printf("(((var*)");
	printsub(name, len);
	printf(")[");
	whitespaces(st);
	expression(st);
	whitespaces(st);
	if (st->buf[st->pos] != ']') {
		error("expecting ']'", st);
	}
	printf("])");
	st->pos++;
	return 0;
}

int keyw_bytes(struct rik *st)
{
	whitespaces(st);
	if (st->buf[st->pos] != '(') {
		error("missing '('", st);
	}
	st->pos++;
	whitespaces(st);
	printf("malloc(((");
	expression(st);
	whitespaces(st);
	printf(") & ~(sizeof(var) - 1)) + sizeof(var))");
	if (st->buf[st->pos] != ')') {
		error("missing ')'", st);
	}
	st->pos++;
	return 0;
}


int keyw_array(struct rik *st)
{
	whitespaces(st);
	if (st->buf[st->pos] != '(') {
		error("missing '('", st);
	}
	st->pos++;
	whitespaces(st);
	printf("malloc((");
	expression(st);
	whitespaces(st);
	printf(") * sizeof(var))");
	if (st->buf[st->pos] != ')') {
		error("missing ')'", st);
	}
	st->pos++;
	return 0;
}

int keyw_free(struct rik *st)
{
	whitespaces(st);
	if (st->buf[st->pos] != '(') {
		error("missing '('", st);
	}
	st->pos++;
	printf("free((void*)");
	expression(st);
	if (st->buf[st->pos] != ')') {
		error("missing )", st);
	}
	printf(")");
	st->pos++;
	return 0;
}

int keyw_return(struct rik *st)
{
	printf("return ");
	expression(st);
	if (st->buf[st->pos] != ';') {
		error("missing ;", st);
	}
	return 0;
}

int keyw_struct(struct rik *st)
{
	char *b;
	int l;
	whitespaces(st);
	if (st->buf[st->pos] != '(') {
		error("missing '('", st);
	}
	st->pos++;
	whitespaces(st);
	b = st->buf + st->pos;
	l = id_len(b);
	st->pos += l;
	whitespaces(st);
	printf("malloc(sizeof(struct ");
	while (l > 0) {
		printf("%c", *b);
		b++;
		l--;
	}
	printf("))");
	if (st->buf[st->pos] != ')') {
		error("missing ')'", st);
	}
	st->pos++;
	return 0;
}

int body(struct rik *st)
{
	int end = 0;
	if (st->buf[st->pos] != '(') {
		error(" ( ", st);
	}
	st->pos++;
	st->indent++;
	while (!end && st->pos < st->end) {
		whitespaces(st);
		switch(st->buf[st->pos]) {
		case ')':
			end = 1;
			st->pos++;
			break;
		case ';':
			st->pos++;
			break;
		default:
			statement(st);
		}
	}
	st->indent--;
	return 0;
}

int keyw_switch(struct rik *st)
{
	char *b;
	int l;
	int end = 0;

	whitespaces(st);
	if (st->buf[st->pos] != '(') {
		error("missing '('", st);
	}
	st->pos++;
	printf("switch ((var)(");
	expression(st);
	if (st->buf[st->pos] != ')') {
		error("missing ')'", st);
	}
	st->pos++;
	whitespaces(st);
	if (st->buf[st->pos] != '(') {
		error("missing ') ('", st);
	}
	st->pos++;
	printf(")) {\n");
	while (!end && st->pos < st->end) {
		whitespaces(st);
		indent(st);
		printf("case ");
		constant(st);
		printf("%s:", st->tmp);
		whitespaces(st);
		if (st->buf[st->pos] != ':') {
			error("missing ':'", st);
		}
		st->pos++;
		whitespaces(st);
		switch (st->buf[st->pos]) {
		case '(':
			printf(" {\n");
			body(st);
			indent(st);
			printf("\tbreak;\n");
			indent(st);
			whitespaces(st);
			printf("}\n");
			if (st->buf[st->pos] != ',') {
				end = 1;
			} else {
				st->pos++;
			}
			break;
		case ')':
			end = 1;
		 	break;
		default:
			printf("\n");
			break;
		}
	}
	if (st->buf[st->pos] != ')') {
		error("missing ')'", st);
	}
	st->pos++;
	indent(st);
	printf("}");
	return 0;
}
int keyw_while(struct rik *st)
{
	whitespaces(st);
	if (st->buf[st->pos] != '(') {
		error("missing '('", st);
	}
	st->pos++;
	printf("while (");
	expression(st);
	if (st->buf[st->pos] != ')') {
		error("missing ')'", st);
	}
	st->pos++;
	whitespaces(st);
	printf(") {\n");
	body(st);
	if (st->buf[st->pos] != ';') {
		error("missing ';'", st);
	}
	indent(st);
	printf("}");
	st->pos++;
	return 0;
}

int statement(struct rik *st)
{
	indent(st);
	expression(st);
	printf(";\n");
	return 0;
}

int keyword(struct rik *st, char *b, int idl)
{
	struct pair *p;
	int r;

	if (idl < 1) {
		return -1;
	}

	st->retok = 0;
	switch(*b) {
	case 'r':
		if (idl == 6) {
			if (!strncmp(b, "return", 6)) {
				r = keyw_return(st);
				st->retok = 1;
				return r;
			}
		}
		break;
	}
	while (st->parent) {
		st = st->parent;
	}
	p = st->consts;
	while (p) {
		if (!strncmp(p->name,b, idl) && p->name[idl] == '\0') {
			printf("%s", p->value);
			return 0;
		}
		p = p->next;	
	}
	return -1;
}


int func_call(struct rik *st, char *b, int idl)
{
	struct pair *p;
	int r;

	if (idl < 1) {
		error("expecting statement", st);
	}

	st->retok = 0;
	switch(*b) {
	case 'a':
		if (idl == 5) {
			if (!strncmp(b, "array", 5)) {
				return keyw_array(st);
			}
		}
		break;
	case 'b':
		if (idl == 5) {
			if (!strncmp(b, "bytes", 5)) {
				return keyw_bytes(st);
			}
		}
		break;
	case 'f':
		if (idl == 4) {
			if (!strncmp(b, "free", 4)) {
				return keyw_free(st);
			}
		}
		break;
	case 's':
		if (idl == 6) {
			if (!strncmp(b, "switch", 6)) {
				return keyw_switch(st);
			} else 	if (!strncmp(b, "struct", 6)) {
				return keyw_struct(st);
			}
		}
		break;
	case 'w':
		if (idl == 5) {
			if (!strncmp(b, "while",5)) {
				return keyw_while(st);
			}
		}
		break;
	}
	return -1;
}

int expression(struct rik *st)
{
	int idl = 0;
	int idi;
	int sl;
	int end = 0;
	char *b;
	int op = 0;

	while(!end && st->pos < st->end && !whitespaces(st)) {
		idi = st->pos;
		idl = id_len(st->buf + st->pos);
		st->pos += idl;
		whitespaces(st);
		b = st->buf + st->pos;
		if (!keyword(st, st->buf + idi, idl)) {
			idl = 0;
			continue;
		}
		switch(*b) {
		case ')':
		case ']':
		case ',':
			printsub(st->buf + idi, idl);
			idl = 0;
			end = 1;
			break;
		case ';':
			printsub(st->buf + idi, idl);
			idl = 0;
			end = 1;
			break;
		case '.':
			op_dot(st, st->buf + idi, idl);
			idl = 0;
			break;
		case '(':
			if (idl == 0) {
				op_group(st);
				break;
			}
			if (func_call(st, st->buf + idi, idl) == 0) {
				whitespaces(st);
			} else {
				op_call(st, st->buf + idi, idl);
				whitespaces(st);
			}
			return 0;
			break;
		case '[':
			op_index(st, st->buf + idi, idl);
			idl = 0;
			break;
		case '&':
			printsub(st->buf + idi, idl);
			idl = 0;
			if (b[1] == '&') {
				st->pos++;
				op = ('&' << 8) | '&';
				printf(" && ");
			} else {
				error("& ", st);
			}
			st->pos++;
			break;
		case '|':
			printsub(st->buf + idi, idl);
			idl = 0;
			if (b[1] == '|') {
				st->pos++;
				op = ('|' << 8) | '|';
				printf(" || ");
			} else {
				error("| ", st);
			}
			st->pos++;
			break;
	
		case '>':
			printsub(st->buf + idi, idl);
			idl = 0;
			if (b[1] == '=') {
				st->pos++;
				op = ('>' << 8) | '=';
				printf(" >= ");
			} else {
				op = '>';
				printf(" > ");
			}
			st->pos++;
			break;
		case '<':
			printsub(st->buf + idi, idl);
			idl = 0;
			if (b[1] == '=') {
				st->pos++;
				op = ('<' << 8) | '=';
				printf(" <= ");
			} else {
				op = '<';
				printf(" < ");
			}
			st->pos++;
			break;
		case '!':
			printsub(st->buf + idi, idl);
			idl = 0;
			if (b[1] == '=') {
				st->pos++;
				op = ('!' << 8) | '=';
				printf(" != ");
			} else {
				op = '!';
				printf(" ! ");
			}
			st->pos++;
			break;
		case '=':
			printsub(st->buf + idi, idl);
			idl = 0;
			if (b[1] != '=') {
				st->pos++;
				printf(" = (var)(void*)(");
				expression(st);
				printf(")");
				if (st->buf[st->pos] == ';') {
					return 0;
				}
				break;
			}
			st->pos++;
			op = ('=' << 8) | '=';
			op = ('=' << 8) | '=';
			printf(" == ");
			st->pos++;
			break;
		case '*':
		case '/':
		case '%':
			printsub(st->buf + idi, idl);
			idl = 0;
			printf(" %c ", *b);
			st->pos++;
			break;
		case '+':
		case '-':
			printsub(st->buf + idi, idl);
			idl = 0;
			printf(" %c ", *b);
			st->pos++;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			constant(st);
			printf("%s", st->tmp);
			op = '1';
			break;
		case '\'':
			sl = string_len(b);
			printsub(b, sl);
			idl = 0;
			st->pos += sl;
			op = '1';
			break;
		case '"':
			sl = string_len(b);
			printf("((var)(void*)");
			printsub(b, sl);
			printf(")");
			idl = 0;
			st->pos += sl;
			op = '"';
			break;
		default:
			error("syntax error.", st);
			break;	
		}
	}
	whitespaces(st);
	return 0;
}

int func_process(struct rik *st)
{
	int idl;
	int idi;
	int p;
	int end = 0;
	st->retok = 0;
	declare_func(st);
	if (whitespaces(st) || (st->buf[st->pos] != '(')) {
		if (st->buf[st->pos] != ';') {
				error("expecting ';' or '('", st);
		} else {
			printf(";\n");
			return 0;
		}
	}
	whitespaces(st);
	if (st->buf[st->pos] != '(') {
		error("expecting '('", st);
	}
	st->indent = 1;
	st->pos++;
	printf("\n{\n");
	while (!end && !whitespaces(st)) {
		switch (st->buf[st->pos]) {
		case '%':
			var_decl(st);
			break;
		case ')':
			end = 1;
			st->pos++;
			break;
		case ';':
			st->pos++;
			break;
		default:
			statement(st);
			break;
		}
	}
	whitespaces(st);
	if (!st->retok) {
		printf("\treturn 0;\n");
	}
	printf("}\n");
	st->indent = 0;

	while (st->parent) {
		st = st->parent;
	}
	if (st->vars) {
		st->vars[0] = '\0';
	}
	return 0;
}

int include_process(struct rik *st)
{
	char *b;
	int i;
	char *fn;
	struct rik *nst;
	char *name;
	int size;

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
	name = fn + i;
	while (i < 2040) {
		if (*b != '"') {
			st->pos++;
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
	st->pos++;
	whitespaces(st);
	if (st->buf[st->pos] != ';') {
		error("syntax error ;", st);
	}

	fn[i] = '\0';
	nst = load(fn);
	if (nst) {
		if (!strcmp(name, "std.ri")) {
			fn[i-1] = '\0';
			fn[i-2] = 'c';
			size = file_size(fn);
			if (size > 0) {
				b = file_load(fn, size);
				if (b) {
					fwrite(b, 1, size, stdout);
					free(b);
				}
			}
			fn[i-1] = 'i';
			fn[i-2] = 'r';
		}
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
	whitespaces(st);
	while (st->pos < st->end) {
		whitespaces(st);
		switch (st->state) {
		case 0:
			b = st->buf + st->pos;
			if (*b == '_' || (*b >= 'a' && *b <= 'z') ||
				(*b >= 'A' && *b <= 'Z')) 
			{
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
				case 'c':
					if (!strncmp(b, "const", 5)) {
						st->state = 4;
						st->pos += 5;
					}
					break;
				}
				if (st->state == 0) {
					if (whitespaces(st)) {
						error("syntax error", st);
					}
					st->state = 1;
				}
			} else if (*b == ';') {
				st->pos++;
			} else if (st->pos < st->end) {
				error("syntax error...", st);
			}
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
		case 4: /* const */
			const_process(st);
			st->state = 0;
			break;
		case 1: /* function */
			func_process(st);
			st->state = 0;
			break;
		default:
			st->pos++;
			st->state = 0;
		}
		whitespaces(st);
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
	st->indent = 0;
	st->file = strdup(file);
	st->structs = 0;
	st->funcs = 0;
	st->consts = 0;
	st->parent = 0;
	st->vars = 0;
	st->vars_allo = 0;
	st->current = 0;
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

