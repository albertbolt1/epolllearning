#include <iostream>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main() {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9090);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverFd, (sockaddr*)&addr, sizeof(addr));
    listen(serverFd, 10);

    // Make listening socket non-blocking
    fcntl(serverFd, F_SETFL, O_NONBLOCK);

    while (true) {

        int clientFd = accept(serverFd, nullptr, nullptr);

        if (clientFd >= 0) {
            std::cout << "Client connected" << std::endl;
            close(clientFd);
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            std::cout << "No clients waiting" << std::endl;
        }

        sleep(1);
    }

    close(serverFd);
};
