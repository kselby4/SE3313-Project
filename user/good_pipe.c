#include "kernel/types.h"      /* xv6 type definitions */
#include "kernel/stat.h"       /* file stat struct */
#include "kernel/param.h"      /* constants (e.g. MAXARG) */
#include "user/user.h"         /* user syscalls (pipe, fork, write, etc.) */

#define PIPESIZE 32            /* not used for buffer; kept for layout match with original bad_pipe */

struct good_pipe {
  int readfd;   /* fd for reading from pipe (-1 in writer process) */
  int writefd;  /* fd for writing to pipe (-1 in reader process) */
};

void
pipe_write(struct good_pipe *pi, char ch)
{
  if (write(pi->writefd, &ch, 1) != 1) {  /* write one byte to pipe write end */
    exit(1);                                 /* exit on write failure */
  }
}

int
pipe_read(struct good_pipe *pi)
{
  char ch;
  int n = read(pi->readfd, &ch, 1);   /* read one byte from pipe read end */
  if (n <= 0) {                        /* 0 = writer closed, -1 = error */
    return -1;
  }
  return (unsigned char)ch;            /* return byte as 0..255, -1 for done */
}

int
main(void)
{
  int fd[2];                  /* fd[0]=read end, fd[1]=write end */
  struct good_pipe gp;        /* our pipe descriptor (only one end used per process) */

  if (pipe(fd) < 0) {         /* create kernel pipe; get two fds */
    fprintf(2, "pipe() failed\n");
    exit(1);
  }

  int pid = fork();           /* create child; parent and child both run from here */
  if (pid < 0) {
    fprintf(2, "fork() failed\n");
    exit(1);
  }

  if (pid == 0) {             /* child process: we are the reader */
    close(fd[1]);              /* child closes write end so read() sees EOF when parent closes */
    gp.readfd = fd[0];         /* child uses read end */
    gp.writefd = -1;           /* child does not write */

    printf("\n--- Stored output (good pipe) ---\n");

    int x;
    while ((x = pipe_read(&gp)) != -1) {  /* read bytes until pipe closed (-1) */
      char out = (char)x;
      write(1, &out, 1);                   /* print each byte to stdout */
    }

    printf("\n\nDone.\n");
    close(fd[0]);              /* close read end before exit */
    exit(0);
  }

  close(fd[0]);                /* parent process: we are the writer; close read end */                /* parent closes read end */
  gp.readfd = -1;              /* parent does not read */
  gp.writefd = fd[1];          /* parent uses write end */

  const char *haiku =          /* haiku string with title (5-7-5 + title) */
    "A Haiku about getting out of bed\n"
    "No no no no no,\n"
    "No no no no no no no,\n"
    "No no no no no.\n";

  const char *p = haiku;
  while (*p) {                 /* send each character to pipe */
    pipe_write(&gp, *p);
    p++;
  }

  close(fd[1]);                /* close write end so child's read() returns 0 and exits loop */
  wait(0);                     /* wait for child to finish printing */
  exit(0);
}
