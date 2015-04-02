#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

char *socket_path = "./socket";

int setNonblocking(int fd)
{
    int flags;

    /* If they have O_NONBLOCK, use the Posix way to do it */
#if defined(O_NONBLOCK)
    /* Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    /* Otherwise, use the old way of doing it */
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
}


int main(int argc, char *argv[]) {
  struct sockaddr_un addr;
  char buf[100];
  int fd,rc;

  if (argc > 1) socket_path=argv[1];

  if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(-1);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);

  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("connect error");
    exit(-1);
  }

  //setNonblocking(fd);

  while(1) {
    if ((rc = recv(fd, buf, sizeof(buf), MSG_DONTWAIT)) != sizeof(buf)) {
      if (rc > 0) fprintf(stderr,"partial read %d\n", rc);
      else {
        int lasterrno = errno;
        perror("read error: ");
        fprintf(stderr, "read err: %d, errno: %d\n", rc, lasterrno);
        if (errno != EAGAIN && errno != EINTR) {
          fprintf(stderr, "broken socket, exiting\n");
          exit(1);
        } else {
          sleep(1);
        }
      }
    } else {
      fprintf(stdout, "Got %d bytes\n", rc);
    }
    struct pollfd pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fd;
    pfd.events |= ((POLLERR | POLLHUP | POLLNVAL) | (POLLIN) | (POLLIN | POLLPRI));
    while((rc = poll(&pfd, 1, 1000)) == 0) {
      fprintf(stdout, "try polling...\n");
    }
    if (rc < 0) {
      fprintf(stderr, "poll err %d, errno: %d\n", rc, errno);
      exit(1);
    } else {
      fprintf(stdout, "poll returns 0x%x\n", pfd.revents);
    }
  }

  return 0;
}
