# epolllearning
for learning and experimenting with epoll




blocking means it will sleep there and continue when picked up, for example blocking accept will sleep till there is a new connection
and then it will be woken up again.

## epoll / kqueue
epoll (Linux) and kqueue (macOS/BSD) still sleep, but they sleep on all the registered fds at once instead of on one.
The listening socket and every client socket are registered in the same queue, so the thread wakes up when a new
connection arrives or when any client sends data. Reads from existing clients keep being served even when no new
clients are connecting, which a blocking accept() cannot do.

The wait call (epoll_wait / kevent) returns only the fds that are ready. We branch on the fd: if it is the server fd,
accept and register the new client; otherwise read from that client.