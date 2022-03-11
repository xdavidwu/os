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
