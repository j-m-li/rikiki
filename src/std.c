/*

   This software is dedicated to the public domain.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef var
#define var long
#endif

#define peek(buf, at) ((var)(((unsigned char*)buf)[at]))
#define poke(buf, at, v) ((unsigned char*)buf)[at] = (v)

var startup(var a, var b);

int main(int argc, char *argv[])
{
	return startup(argc, (var)(void*)argv);
}

var print(var str)
{
	printf("%s", (char*)str);
	return 0;
}

var print2(var n)
{
	printf("%ld", n);
	return 0;
}

var file_size(var path)
{
	FILE *fp;
	var si;
	fp = fopen((char*)path, "rb");
	if (!fp) {
		return 0;
	}
	fseek(fp, 0, SEEK_END);
	si = ftell(fp);
	fclose(fp);
	return si;
}

var file_load(var path, var size)
{
	char *buf;
	FILE *fp;
	var ret;

	fp = fopen((char*)path, "rb");
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
	return (var)buf;
}

var file_save(var path, var size, var buf)
{
	FILE *fp;
	var ret;

	fp = fopen((char*)path, "wb");
	if (!fp) {
		return -1;
	}
	ret = fwrite((void*)buf, 1, size, fp);
	fclose(fp);
	return ret;
}


#define strcmp(a,b) strcmp((void*)a, (void*)b)
#define strdup(a) strdup((void*)a)

