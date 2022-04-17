#ifndef SYSCALL_H
#define SYSCALL_H

#include "aarch64/registers.h"

void syscall(struct trapframe *trapframe);

#endif
