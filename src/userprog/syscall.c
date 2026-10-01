#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "threads/malloc.h" 
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

// pid_t 정의
typedef int pid_t;

// 시스템 콜 함수 선언
void halt(void);
void exit(int status);
pid_t exec(const char *cmd_line);
int wait(pid_t pid);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
void seek(int fd, unsigned position);
unsigned tell(int fd);
void close(int fd);
int fibonacci(int n);
int max_of_four_int(int a, int b, int c, int d);

static void syscall_handler (struct intr_frame *);
struct lock filesys_lock;  // 파일시스템 동기화용 (전역으로 변경)

// ========== 포인터 검증 함수들 ==========
static bool 
is_valid_ptr(const void *ptr)
{
  if (ptr == NULL)
    return false;
    
  if (!is_user_vaddr(ptr))
    return false;
    
  void *page = pagedir_get_page(thread_current()->pagedir, ptr);
  if (page == NULL)
    return false;
    
  return true;
}

static bool
is_valid_buffer(const void *buffer, unsigned size)
{
  const char *buf = (const char *)buffer;
  
  if (!is_valid_ptr(buf))
    return false;
    
  if (size > 0 && !is_valid_ptr(buf + size - 1))
    return false;
    
  /* 페이지 경계마다 검증 */
  unsigned i;
  for (i = PGSIZE; i < size; i += PGSIZE) {
    if (!is_valid_ptr(buf + i))
      return false;
  }
  
  return true;
}

static bool
is_valid_string(const char *str)
{
    if (!is_valid_ptr(str))
        return false;
    
    /* NULL 문자를 찾을 때까지 각 페이지 검증 */
    for (int i = 0; ; i++) {
        /* 페이지 경계마다 확인 */
        if (i % PGSIZE == 0 && i != 0) {
            if (!is_valid_ptr(str + i))
                return false;
        }
        if (str[i] == '\0')
            return true;
    }
    return false;
}

static int get_arg(struct intr_frame *f, int n) {
    int *ptr = (int *)f->esp + n + 1;
    
    if (!is_valid_ptr(ptr)) {
        exit(-1);
        return -1;  /* Never reached */
    }
    
    return *ptr;
}

/* 자식 프로세스 찾기 헬퍼 함수 */
static struct child_process *
find_child(tid_t child_tid) 
{
    struct thread *curr = thread_current();
    struct list_elem *e;
    
    for (e = list_begin(&curr->children); 
         e != list_end(&curr->children);
         e = list_next(e)) {
        struct child_process *cp = list_entry(e, struct child_process, child_elem);
        if (cp->tid == child_tid) {
            return cp;
        }
    }
    return NULL;
}

// ========== 초기화 ==========
void syscall_init (void) {
    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
    lock_init(&filesys_lock);
}

// ========== 시스템 콜 핸들러 ==========
static void syscall_handler(struct intr_frame *f) {
    if (!is_valid_ptr(f->esp)) {
        exit(-1);
        return;
    }
    
    int syscall_num = *(int *)f->esp;
    
    switch (syscall_num) {
        case SYS_HALT:
            halt();
            break;
            
        case SYS_EXIT:
            exit(get_arg(f, 0));
            break;
            
        case SYS_EXEC:
            {
                const char *cmd_line = (const char *)get_arg(f, 0);
                if (!is_valid_ptr(cmd_line) || !is_valid_string(cmd_line)) {
                    exit(-1);
                    break;
                }
                f->eax = exec(cmd_line);
            }
            break;
            
        case SYS_WAIT:
            f->eax = wait((pid_t)get_arg(f, 0));
            break;
            
        case SYS_CREATE:
            {
                const char *file = (const char *)get_arg(f, 0);
                unsigned initial_size = (unsigned)get_arg(f, 1);
                
                /* NULL이나 잘못된 포인터면 종료 */
                if (!is_valid_ptr(file) || !is_valid_string(file)) {
                    exit(-1);
                    break;
                }
                
                /* 빈 문자열은 실패 반환 */
                if (strlen(file) == 0) {
                    f->eax = false;
                    break;
                }
                
                f->eax = create(file, initial_size);
            }
            break;
            
        case SYS_REMOVE:
            {
                const char *file = (const char *)get_arg(f, 0);
                if (!is_valid_ptr(file) || !is_valid_string(file)) {
                    exit(-1);
                    break;
                }
                f->eax = remove(file);
            }
            break;
            
        case SYS_OPEN:
            {
                const char *file = (const char *)get_arg(f, 0);
                if (!is_valid_ptr(file) || !is_valid_string(file)) {
                    exit(-1);
                    break;
                }
                f->eax = open(file);
            }
            break;
            
        case SYS_FILESIZE:
            f->eax = filesize(get_arg(f, 0));
            break;
            
        case SYS_READ:
            {
                int fd = get_arg(f, 0);
                void *buffer = (void *)get_arg(f, 1);
                unsigned size = (unsigned)get_arg(f, 2);
                
                if (!is_valid_buffer(buffer, size)) {
                    exit(-1);
                    break;
                }
                
                f->eax = read(fd, buffer, size);
            }
            break;
            
        case SYS_WRITE:
            {
                int fd = get_arg(f, 0);
                const void *buffer = (const void *)get_arg(f, 1);
                unsigned size = (unsigned)get_arg(f, 2);
                
                if (!is_valid_buffer(buffer, size)) {
                    exit(-1);
                    break;
                }
                
                f->eax = write(fd, buffer, size);
            }
            break;
            
        case SYS_SEEK:
            seek(get_arg(f, 0), (unsigned)get_arg(f, 1));
            break;
            
        case SYS_TELL:
            f->eax = tell(get_arg(f, 0));
            break;
            
        case SYS_CLOSE:
            close(get_arg(f, 0));
            break;
            
        case SYS_FIBONACCI:
            f->eax = fibonacci(get_arg(f, 0));
            break;
            
        case SYS_MAX_OF_FOUR:
            f->eax = max_of_four_int(get_arg(f, 0), get_arg(f, 1), 
                                     get_arg(f, 2), get_arg(f, 3));
            break;
            
        default:
            exit(-1);
            break;
    }
}

// ========== 시스템 콜 구현 ==========

void halt(void) {
    shutdown_power_off();
}

void exit(int status) {
    struct thread *cur = thread_current();
    cur->exit_status = status;
    
    /* process_exit()에서 출력하므로 여기서는 출력 안함 */
    
    /* 열린 파일들 모두 닫기 */
    for (int i = 2; i < 128; i++) {
        if (cur->fd_table[i] != NULL) {
            close(i);
        }
    }
    
    /* 실행 파일 닫기 */
    if (cur->exec_file != NULL) {
        lock_acquire(&filesys_lock);
        file_allow_write(cur->exec_file);
        file_close(cur->exec_file);
        lock_release(&filesys_lock);
        cur->exec_file = NULL;
    }
    
    thread_exit();
}

pid_t exec(const char *cmd_line) {
    /* 핸들러에서 이미 검증됨 */
    pid_t pid = process_execute(cmd_line);

    if (pid == TID_ERROR) {
        return -1;  // process_execute 실패 시 즉시 -1 반환
    }

    /* 자식이 로드 완료할 때까지 대기 */
    struct thread *parent = thread_current();
    sema_down(&parent->load_sema);

    /* 로드 실패 시 자식을 orphan으로 만들고 -1 반환 */
    if (!parent->load_success) {
        struct child_process *child = find_child(pid);
        if (child != NULL) {
            /* orphan으로 표시하면 자식이 종료될 때 스스로 free함 */
            child->orphaned = true;
            list_remove(&child->child_elem);
            /* 자식이 이미 종료했으면 직접 free */
            if (child->exited) {
                free(child);
            }
        }
        return -1;
    }

    return pid;
}

int wait(pid_t pid) {
    return process_wait(pid);
}

bool create(const char *file, unsigned initial_size) {
    /* 핸들러에서 이미 검증됨 */
    lock_acquire(&filesys_lock);
    bool success = filesys_create(file, initial_size);
    lock_release(&filesys_lock);
    return success;
}

bool remove(const char *file) {
    /* 핸들러에서 이미 검증됨 */
    lock_acquire(&filesys_lock);
    bool success = filesys_remove(file);
    lock_release(&filesys_lock);
    return success;
}

int open(const char *file) {
    /* 핸들러에서 이미 검증됨 */
    lock_acquire(&filesys_lock);
    struct file *f = filesys_open(file);
    
    if (f == NULL) {
        lock_release(&filesys_lock);
        return -1;
    }
    
    struct thread *cur = thread_current();
    
    /* 현재 실행 중인 파일이면 쓰기 금지 */
    if (cur->exec_file != NULL) {
        struct inode *exec_inode = file_get_inode(cur->exec_file);
        struct inode *open_inode = file_get_inode(f);
        if (exec_inode == open_inode) {
            file_deny_write(f);
        }
    }
    
    /* fd 할당 */
    int fd = -1;
    for (int i = 2; i < 128; i++) {
        if (cur->fd_table[i] == NULL) {
            cur->fd_table[i] = f;
            fd = i;
            break;
        }
    }
    
    if (fd == -1) {
        file_close(f);
    }
    
    lock_release(&filesys_lock);
    return fd;
}

int filesize(int fd) {
    if (fd < 2 || fd >= 128)
        return -1;
    
    struct file *f = thread_current()->fd_table[fd];
    if (f == NULL)
        return -1;
    
    lock_acquire(&filesys_lock);
    int size = file_length(f);
    lock_release(&filesys_lock);
    return size;
}

int read(int fd, void *buffer, unsigned size) {
    /* 핸들러에서 버퍼 검증됨 */
    
    /* stdout에서는 읽기 불가 */
    if (fd == 1)
        return -1;
    
    if (fd == 0) {  /* STDIN */
        unsigned i;
        char *buf = (char *)buffer;
        for (i = 0; i < size; i++) {
            buf[i] = input_getc();
        }
        return size;
    }
    
    if (fd < 2 || fd >= 128)
        return -1;
    
    struct file *f = thread_current()->fd_table[fd];
    if (f == NULL)
        return -1;
    
    lock_acquire(&filesys_lock);
    int bytes_read = file_read(f, buffer, size);
    lock_release(&filesys_lock);
    return bytes_read;
}

int write(int fd, const void *buffer, unsigned size) {
    /* 핸들러에서 버퍼 검증됨 */
    
    if (fd == 1) {  /* STDOUT */
        putbuf(buffer, size);
        return size;
    }
    
    /* stdin에는 쓰기 불가 */
    if (fd == 0)
        return -1;
    
    if (fd < 2 || fd >= 128)
        return -1;
    
    struct file *f = thread_current()->fd_table[fd];
    if (f == NULL)
        return -1;
    
    lock_acquire(&filesys_lock);
    int bytes_written = file_write(f, buffer, size);
    lock_release(&filesys_lock);
    return bytes_written;
}

void seek(int fd, unsigned position) {
    if (fd < 2 || fd >= 128)
        return;
    
    struct file *f = thread_current()->fd_table[fd];
    if (f == NULL)
        return;
    
    lock_acquire(&filesys_lock);
    file_seek(f, position);
    lock_release(&filesys_lock);
}

unsigned tell(int fd) {
    if (fd < 2 || fd >= 128)
        return 0;
    
    struct file *f = thread_current()->fd_table[fd];
    if (f == NULL)
        return 0;
    
    lock_acquire(&filesys_lock);
    unsigned pos = file_tell(f);
    lock_release(&filesys_lock);
    return pos;
}

void close(int fd) {
    if (fd < 2 || fd >= 128)
        return;
    
    struct file *f = thread_current()->fd_table[fd];
    if (f == NULL)
        return;
    
    thread_current()->fd_table[fd] = NULL;
    
    lock_acquire(&filesys_lock);
    file_close(f);
    lock_release(&filesys_lock);
}

int fibonacci(int n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    
    int *fib = malloc(sizeof(int) * (n + 1));
    if (fib == NULL)
        return -1;
        
    fib[0] = 0;
    fib[1] = 1;
    
    for (int i = 2; i <= n; i++) {
        fib[i] = fib[i-1] + fib[i-2];
    }
    
    int result = fib[n];
    free(fib);
    
    return result;
}

int max_of_four_int(int a, int b, int c, int d) {
    int max = a;
    if (b > max) max = b;
    if (c > max) max = c;
    if (d > max) max = d;
    return max;
}