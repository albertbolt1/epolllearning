#include <iostream>
#include <vector>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

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

    std::vector<int> clients;

    std::cout << "Listening on 9090...\n";

    while (true) {

        // Accept all pending connections
        while (true) {
            int clientFd = accept(serverFd, nullptr, nullptr);

            if (clientFd == -1) {
                break; // no more clients waiting
            }

            fcntl(clientFd, F_SETFL, O_NONBLOCK);

            clients.push_back(clientFd);

            std::cout << "Client connected. Total clients = "
                      << clients.size() << std::endl;
        }

        // Poll all clients
        for (auto it = clients.begin(); it != clients.end();) {

            int fd = *it;

            char buffer[1024];

            int n = recv(fd, buffer, sizeof(buffer) - 1, 0);

            if (n > 0) {
                buffer[n] = '\0';

                std::cout << "FD "
                          << fd
                          << " sent: "
                          << buffer
                          << std::endl;

                ++it;
            }
            else if (n == 0) {
                std::cout << "FD "
                          << fd
                          << " disconnected"
                          << std::endl;

                close(fd);

                it = clients.erase(it);
            }
            else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // No data available right now
                    ++it;
                } else {
                    std::cout << "FD "
                              << fd
                              << " error"
                              << std::endl;

                    close(fd);
                    it = clients.erase(it);
                }
            }
        }

        usleep(100000); // 100 ms so CPU doesn't spin
    }

    close(serverFd);
}
