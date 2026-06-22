#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>

int main()
{
/*
AF_INET means give an ipv4 address 
SOCK_STREAM - TCP
SOCK_DGRAM is for UDP
*/
int serverFd = socket(AF_INET,SOCK_STREAM,0);
sockaddr_in addr{};
addr.sin_family = AF_INET;
addr.sin_port = htons(9090);
addr.sin_addr.s_addr = INADDR_ANY;


bind(serverFd,(sockaddr*)&addr, sizeof(addr));

listen(serverFd,10);
std::cout << "Waiting for client...\n";

int clientFd=accept(serverFd,nullptr,nullptr);

std::cout << "Client connected!\n";

close(clientFd);
close(serverFd);
}
