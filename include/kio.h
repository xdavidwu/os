#ifndef KIO_H
#define KIO_H

#include "console.h"

extern const struct console *kconsole;

#define kputs(str)	cputs(kconsole, str)
#define kgets(str, sz)	cgets(kconsole, str, sz)

#endif
