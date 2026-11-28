// Echo server using io_uring (Linux only, needs liburing).
// usage: ./uring_echo [seconds]   (exits by itself after the given time so strace can print its summary)
#include <liburing.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <iostream>

static volatile sig_atomic_t running = 1;
static void onAlarm(int) { running = 0; }

enum Type { ACCEPT, RECV, SEND };

struct Conn {
    Type type;
    int fd;
    int len;
    char buf[1024];
};

static void queueRecv(io_uring &ring, Conn *c)
{
    c->type = RECV;
    io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_recv(sqe, c->fd, c->buf, sizeof(c->buf), 0);
    io_uring_sqe_set_data(sqe, c);
}

static void queueSend(io_uring &ring, Conn *c)
{
    c->type = SEND;
    io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_send(sqe, c->fd, c->buf, c->len, 0);
    io_uring_sqe_set_data(sqe, c);
}

static void queueAccept(io_uring &ring, Conn *c)
{
    c->type = ACCEPT;
    io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_accept(sqe, c->fd, nullptr, nullptr, 0);
    io_uring_sqe_set_data(sqe, c);
}

int main(int argc, char **argv)
{
    int seconds = argc > 1 ? atoi(argv[1]) : 10;

    // no SA_RESTART, so the alarm interrupts the wait and the loop can exit
    struct sigaction sa{};
    sa.sa_handler = onAlarm;
    sigaction(SIGALRM, &sa, nullptr);

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9091);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(serverFd, (sockaddr *)&addr, sizeof(addr));
    listen(serverFd, 128);

    io_uring ring;
    int rc = io_uring_queue_init(256, &ring, 0);
    if (rc < 0)
    {
        std::cerr << "io_uring_queue_init failed: " << -rc << " (EPERM = blocked by seccomp)\n";
        return 1;
    }

    Conn *acceptConn = new Conn{ACCEPT, serverFd, 0, {}};
    queueAccept(ring, acceptConn);

    std::cout << "io_uring echo server on 9091 for " << seconds << "s\n" << std::flush;
    alarm(seconds);

    while (running)
    {
        // one syscall: submit everything queued so far AND wait for at least one completion
        int ret = io_uring_submit_and_wait(&ring, 1);
        if (ret < 0)
        {
            if (ret == -EINTR) continue;
            break;
        }

        // drain every completion that is ready, without any further syscall
        unsigned head, count = 0;
        io_uring_cqe *cqe;
        io_uring_for_each_cqe(&ring, head, cqe)
        {
            count++;
            Conn *c = (Conn *)io_uring_cqe_get_data(cqe);
            int res = cqe->res;

            if (c->type == ACCEPT)
            {
                if (res >= 0)
                {
                    Conn *client = new Conn{RECV, res, 0, {}};
                    queueRecv(ring, client);
                }
                queueAccept(ring, c); // accept completes only once, so queue the next one
            }
            else if (c->type == RECV)
            {
                if (res > 0)
                {
                    c->len = res;
                    queueSend(ring, c);
                }
                else
                {
                    close(c->fd);
                    delete c;
                }
            }
            else // SEND done, wait for the client's next message
            {
                queueRecv(ring, c);
            }
        }
        io_uring_cq_advance(&ring, count);
    }

    io_uring_queue_exit(&ring);
    return 0;
}
