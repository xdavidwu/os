#include "string.h"
#include "stdlib.h"
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

char *strncpy(char *restrict dest, const char *restrict src, size_t n) {
	size_t i = 0;

	for (; i < n && src[i] != '\0'; i++) {
		dest[i] = src[i];
	}
	for (; i < n; i++) {
		dest[i] = '\0';
	}

	return dest;
}

char *strcat(char *restrict dest, const char *restrict src) {
	char *ptr = dest;
	ptr += strlen(ptr);
	strcpy(ptr, src);
	return dest;
}

char *strncat(char *restrict dest, const char *restrict src, size_t n) {
	size_t dest_len = strlen(dest);
	size_t i;

	for (i = 0 ; i < n && src[i] != '\0' ; i++) {
		dest[dest_len + i] = src[i];
	}
	dest[dest_len + i] = '\0';

	return dest;
}

char *strdup(const char *src) {
	char *new = malloc(strlen(src) + 1);
	return strcpy(new, src);
}
