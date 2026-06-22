#include <iostream>
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

    while (true) {
        std::cout << "Before accept" << std::endl;

        int clientFd = accept(serverFd, nullptr, nullptr);

        std::cout << "After accept" << std::endl;

        close(clientFd);
    }

    close(serverFd);
};
