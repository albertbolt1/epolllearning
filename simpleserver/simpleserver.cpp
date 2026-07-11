#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>




int main() {
/*
create a socker
attach socket to a port 
listen for incoming connections
accept incoming connections
read what the connection sends
*/
int serverFd = socket(AF_INET, SOCK_STREAM, 0);
sockaddr_in addr{};
addr.sin_family = AF_INET;
addr.sin_port = htons(9090);
addr.sin_addr.s_addr = INADDR_ANY;

bind(serverFd, (sockaddr*)&addr, sizeof(addr));
listen(serverFd, 10);

std::cout << "Waiting for client...\n";
int clientFd = accept(serverFd, nullptr, nullptr);

std::cout << "Client connected!\n";
char buffer[1024];
int bytesRead = read(clientFd, buffer, sizeof(buffer) - 1);
if (bytesRead > 0) {
    buffer[bytesRead] = '\0';
    std::cout << "Received: " << buffer << "\n";
}
close(clientFd);
close(serverFd);
}