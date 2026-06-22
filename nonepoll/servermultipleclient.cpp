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

std::cout<<"server listening at 9090"<<std::endl;

while(true)
{
int clientFd=accept(serverFd,nullptr,nullptr);
std::cout<<"client connected"<<std::endl;
char buffer[1024];
int bytesread=recv(clientFd,buffer,sizeof(buffer)-1,0);
if(bytesread >0)
{
buffer[bytesread]='\0';
std::cout << "Received: " << buffer << "\n";
}
close(clientFd);
}

}
