#include <sys/socket.h>
#include <sys/event.h>
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>

int main()
{

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9090);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverFd, (sockaddr *)&addr, sizeof(addr));
    listen(serverFd, 10);
    std::cout << "Waiting for client...\n";


    int kq = kqueue();

    struct kevent change;
    EV_SET(&change, serverFd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
    int n = kevent(kq, &change, 1, nullptr, 0, nullptr);
    

    struct kevent events[64];

    while (true)
    {
        int count = kevent(kq, nullptr, 0, events, 64, nullptr);

        for(int i=0; i<count; i++)
        {
            if(events[i].ident == serverFd)
            {
                int clientFd = accept(serverFd, nullptr, nullptr);
                std::cout << "Client connected!\n";

                struct kevent change;
                EV_SET(&change, clientFd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
                kevent(kq, &change, 1, nullptr, 0, nullptr);
            }
            else
            {
                int clientFd = events[i].ident;
                char buffer[1024];
                int bytesRead = read(clientFd, buffer, sizeof(buffer) - 1);
                if (bytesRead > 0)
                {
                    buffer[bytesRead] = '\0';
                    std::cout << "Received: " << buffer << "\n";
                }
                else
                {
                    close(clientFd);
                    std::cout << "Client disconnected.\n";
                }
            }
        }

    }

    return 0;
}
