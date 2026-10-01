# Pintos — Operating Systems course projects

Sogang University, Operating Systems (Fall 2025, Prof. Youngjae Lee). Three individual projects on the Pintos teaching kernel (x86), built on top of each other. The tree under `src/` is the final Project 3 submission and contains the work of all three.

| # | Project | What I implemented |
| --- | --- | --- |
| 1 | User Programs (1) | Argument passing onto the user stack, validation of user pointers, buffers and strings, and 15 system calls. |
| 2 | User Programs (2) | Per-thread file-descriptor table, file-system synchronization, and parent-child synchronization for `exec` and `wait`. |
| 3 | Threads | Alarm clock without busy waiting, priority scheduling with nested priority donation, and the MLFQS advanced scheduler. |

## Project 1 and 2 — user programs

- **Argument passing**: `load()` tokenizes the command line and builds the stack as strings, word alignment, `argv[]` pointers, `argv`, `argc` and a fake return address.
- **User memory access**: every system-call argument is checked before use (null, kernel address, unmapped page, buffers and strings across page boundaries); a bad access ends the process with exit status -1 instead of panicking the kernel.
- **System calls**: `halt`, `exit`, `exec`, `wait`, `create`, `remove`, `open`, `filesize`, `read`, `write`, `seek`, `tell`, `close`, plus two added calls, `fibonacci` and `max_of_four_int`.
- **File descriptors**: a 128-entry table per thread, with 0 and 1 reserved for standard input and output.
- **Synchronization**: one global file-system lock around file operations; semaphores let a parent wait for its child to finish loading (`exec`) and to exit (`wait`); a running executable is protected with `file_deny_write`.

Main files: `src/userprog/process.c`, `src/userprog/syscall.c`, `src/userprog/exception.c`, `src/threads/thread.h`.

## Project 3 — threads

- **Alarm clock**: `timer_sleep()` blocks the thread on a sleep list sorted by wake-up tick, and the timer interrupt wakes threads from the front of the list.
- **Priority scheduling**: the ready list and the waiters of semaphores, locks and condition variables are ordered by priority, and a thread yields as soon as a higher-priority thread becomes ready.
- **Priority donation**: a thread waiting for a lock donates its priority to the holder; donation follows chains of locks up to depth 8 and is withdrawn when the lock is released.
- **MLFQS**: priorities are recomputed from `nice`, `recent_cpu` and `load_avg` in 17.14 fixed-point arithmetic (`src/threads/fixed-point.h`), every tick, every 4 ticks and every second as the Pintos reference specifies. Donation is switched off in this mode.
- **Priority aging**: an optional `-aging` kernel flag raises the priority of waiting threads to prevent starvation.

Main files: `src/devices/timer.c`, `src/threads/thread.c`, `src/threads/synch.c`, `src/threads/fixed-point.h`.

## Running

```bash
cd src/threads && make          # or src/userprog
cd build && make check
```

Pintos itself is from Stanford (see `LICENSE` and `AUTHORS`); the course modifications are mine.
