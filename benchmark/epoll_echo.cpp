// Echo server using epoll (Linux only).
// usage: ./epoll_echo [seconds]   (exits by itself after the given time so strace can print its summary)
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>

static volatile sig_atomic_t running = 1;
static void onAlarm(int) { running = 0; }

int main(int argc, char **argv)
{
    int seconds = argc > 1 ? atoi(argv[1]) : 10;

    // no SA_RESTART, so the alarm interrupts epoll_wait and the loop can exit
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

    int ep = epoll_create1(0);
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = serverFd;
    epoll_ctl(ep, EPOLL_CTL_ADD, serverFd, &ev);

    std::cout << "epoll echo server on 9091 for " << seconds << "s\n" << std::flush;
    alarm(seconds);

    epoll_event events[128];
    char buf[1024];

    while (running)
    {
        int n = epoll_wait(ep, events, 128, -1);

        for (int i = 0; i < n; i++)
        {
            int fd = events[i].data.fd;

            if (fd == serverFd)
            {
                int clientFd = accept(serverFd, nullptr, nullptr);
                if (clientFd >= 0)
                {
                    ev.events = EPOLLIN;
                    ev.data.fd = clientFd;
                    epoll_ctl(ep, EPOLL_CTL_ADD, clientFd, &ev);
                }
            }
            else
            {
                ssize_t r = read(fd, buf, sizeof(buf));
                if (r > 0)
                {
                    (void)!write(fd, buf, r);
                }
                else
                {
                    close(fd); // closing also removes it from the epoll set
                }
            }
        }
    }
    return 0;
}
