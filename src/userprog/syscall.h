#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"

void syscall_init (void);

/* 파일 시스템 동기화용 전역 lock */
extern struct lock filesys_lock;

#endif /* userprog/syscall.h */
