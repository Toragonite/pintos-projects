#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/synch.h"
#include "threads/fixed-point.h"

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

#ifdef USERPROG
/* 전방 선언 추가 - struct thread 정의 전에 */
struct thread;
struct semaphore;

/* 자식 프로세스 정보 구조체 (여기에 추가!) */
struct child_process {
    tid_t tid;                          /* 자식의 tid */
    int exit_status;                    /* 자식의 종료 상태 */
    bool waited;                        /* wait가 이미 호출되었는지 */
    bool exited;                        /* 자식이 종료되었는지 */
    bool orphaned;                       /* 부모가 이 자식을 유기했는지 (userprog(2)) */
    int ref_cnt;                        /* 참조 카운트 (부모 + 자식) (userprog(2)) */
    struct semaphore wait_sema;        /* 대기용 세마포어 */
    struct list_elem child_elem;        /* 자식 리스트 원소 */
};
#endif

/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    struct list_elem allelem;           /* List element for all threads list. */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */

    /* For alarm clock implementation. */
    int64_t wake_tick;                  /* Tick to wake up at (for timer_sleep). */

    /* For priority donation. */
    int base_priority;                  /* Original priority before donation. */
    struct lock *waiting_on;            /* Lock this thread is waiting for. */
    struct list donors;                 /* List of threads donating priority. */
    struct list_elem donor_elem;        /* Element for donors list. */
    struct list locks_held;             /* List of locks held by this thread. */

    /* For MLFQS (Advanced Scheduler). */
    int nice;                           /* Nice value: -20 to 20. */
    fixed_t recent_cpu;                 /* Recent CPU time (fixed-point 17.14). */

#ifdef USERPROG
   /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */
    
    /* Project 2 - 파일 시스템 */
    struct file* fd_table[128];         /* 파일 디스크립터 테이블 */
    int next_fd;                         /* 다음 할당할 fd 번호 (추가) */
    struct file *exec_file;              /* 실행 중인 파일 (추가) */
    
    /* Project 2 - 프로세스 계층 구조 */
    struct thread *parent;               /* 부모 스레드 포인터 (추가) */
    struct list children;                /* 자식 프로세스 리스트 (추가) */
    struct child_process *child_info;   /* 자신의 child_process 정보 (추가) */
    
    /* Project 2 - 동기화 */
    int exit_status;                     /* 종료 상태 */
    struct semaphore load_sema;         /* 로드 완료 대기용 (추가) */
    bool load_success;                   /* 로드 성공 여부 (추가) */
    bool exit_called;  /* exit가 이미 호출되었는지 추가 */
    
    /* Project 2 - 시스템 콜 인자 전달 */
    char *cmd_line;                      /* 명령줄 저장 (추가) */
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

/* If true, use priority aging to prevent starvation.
   Controlled by kernel command-line option "-aging". */
extern bool thread_prior_aging;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

#endif /* threads/thread.h */
