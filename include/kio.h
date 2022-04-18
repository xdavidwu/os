#ifndef KIO_H
#define KIO_H

#include "console.h"

extern struct console *kconsole;

#define kputc(c)	cputc(kconsole, c)
#define kputs(str)	cputs(kconsole, str)
#define kgetc()	cgetc(kconsole)
#define kgets(str, sz)	cgets(kconsole, str, sz)

#endif
