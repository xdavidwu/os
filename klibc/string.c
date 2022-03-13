#include "string.h"
#include <stddef.h>

int strcmp(const char *s1, const char *s2) {
	while (*s1 || *s2) {
		if (*s1 != *s2) {
			return *s1 - *s2;
		}
		s1++;
		s2++;
	}
	return 0;
}

int strncmp(const char *s1, const char *s2, size_t n) {
	while (n-- && (*s1 || *s2)) {
		if (*s1 != *s2) {
			return *s1 - *s2;
		}
		s1++;
		s2++;
	}
	return 0;
}

size_t strlen(const char *s) {
	size_t res = 0;
	while (*(s++)) {
		res++;
	}
	return res;
}

char *strcpy(char *restrict dest, const char *src) {
	char *ptr = dest;
	while (*src) {
		*(ptr++) = *(src++);
	}
	*ptr = '\0';
	return dest;
}

char *strcat(char *restrict dest, const char *restrict src) {
	char *ptr = dest;
	ptr += strlen(ptr);
	strcpy(ptr, src);
	return dest;
}
