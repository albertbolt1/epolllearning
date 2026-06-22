#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

void handleClient(int clientFd) {
    char buffer[1024];

    int bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';

        std::cout
            << "Thread "
            << std::this_thread::get_id()
            << " received: "
            << buffer
            << std::endl;
    }

    close(clientFd);
}

int main() {
int serverFd = socket(AF_INET,SOCK_STREAM,0);
sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_port = htons(9090);
addr.sin_addr.s_addr = INADDR_ANY;

bind (serverFd,(sockaddr*)&addr,sizeof(addr));
listen(serverFd,10);

std::cout << "Listening on 9090..." << std::endl;

while(true)
{
int clientFd=accept(serverFd,nullptr,nullptr);

if (clientFd < 0) {
            perror("accept");
            continue;
        }

        std::cout << "Client connected" << std::endl;

std::thread t(handleClient, clientFd);
t.detach();
}

}
