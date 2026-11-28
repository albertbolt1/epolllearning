#include<liburing.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <unistd.h>

enum Type { ACCEPT, RECV };

struct Request {
    Type type;
    int fd;
    char buffer[1024];
};

int main() {


    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9090);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverFd, (sockaddr *)&addr, sizeof(addr));
    listen(serverFd, 10);
    std::cout << "Waiting for client...\n";

    io_uring ring;
    io_uring_queue_init(32, &ring, 0);

    Request *req = new Request{ACCEPT, serverFd};

    io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_accept(sqe, serverFd, nullptr, nullptr, 0);
    io_uring_sqe_set_data(sqe, req);
    io_uring_submit(&ring);

    io_uring_cqe *cqe;

    while(io_uring_wait_cqe(&ring, &cqe) == 0) {
        // Process the completed event

        Request *done = (Request *)io_uring_cqe_get_data(cqe);
        int res = cqe->res;

        if (done->type == ACCEPT) {
            if (res >= 0) {
                int clientFd = res;
                std::cout << "Client connected!\n";

                Request *r = new Request{RECV, clientFd};
                io_uring_sqe *s = io_uring_get_sqe(&ring);
                io_uring_prep_recv(s, clientFd, r->buffer, sizeof(r->buffer) - 1, 0);
                io_uring_sqe_set_data(s, r);
            } else {
                std::cerr << "accept failed: " << strerror(-res) << "\n";
            }

            // accept fires only once, so queue the next one (reusing the same request)
            io_uring_sqe *a = io_uring_get_sqe(&ring);
            io_uring_prep_accept(a, serverFd, nullptr, nullptr, 0);
            io_uring_sqe_set_data(a, done);
            io_uring_submit(&ring);
        } else if (done->type == RECV) {
            if (res > 0) {
                done->buffer[res] = '\0';
                std::cout << "Received: " << done->buffer << "\n";

                // recv fires only once per client message, so queue the next one
                io_uring_sqe *s = io_uring_get_sqe(&ring);
                io_uring_prep_recv(s, done->fd, done->buffer, sizeof(done->buffer) - 1, 0);
                io_uring_sqe_set_data(s, done);
                io_uring_submit(&ring);
            } else {
                close(done->fd);
                std::cout << "Client disconnected.\n";
                delete done;
            }
        }

        io_uring_cqe_seen(&ring, cqe);
    }

    return 0;
}