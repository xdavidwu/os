#ifndef STRING_H
#define STRING_H

#include <stddef.h>

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strlen(const char *s);
char *strcpy(char *restrict dest, const char *src);
char *strcat(char *restrict dest, const char *restrict src);

#endif

