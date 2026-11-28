// Load generator: N connections, each does send 64 bytes -> wait for the echo -> repeat.
// usage: ./loadgen <host> <port> <connections> <seconds>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

struct Result {
    unsigned long long trips = 0;
    double latencySumUs = 0;
};

static std::atomic<bool> stopFlag{false};

static void worker(const char *host, int port, Result *out)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);

    if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        return;
    }

    char msg[64];
    char reply[64];
    memset(msg, 'x', sizeof(msg));

    while (!stopFlag)
    {
        auto t0 = std::chrono::steady_clock::now();

        if (send(fd, msg, sizeof(msg), 0) != (ssize_t)sizeof(msg)) break;

        size_t got = 0;
        while (got < sizeof(reply))
        {
            ssize_t r = recv(fd, reply + got, sizeof(reply) - got, 0);
            if (r <= 0) { close(fd); return; }
            got += r;
        }

        auto t1 = std::chrono::steady_clock::now();
        out->trips++;
        out->latencySumUs += std::chrono::duration<double, std::micro>(t1 - t0).count();
    }
    close(fd);
}

int main(int argc, char **argv)
{
    if (argc < 5)
    {
        fprintf(stderr, "usage: %s <host> <port> <connections> <seconds>\n", argv[0]);
        return 1;
    }
    const char *host = argv[1];
    int port = atoi(argv[2]);
    int conns = atoi(argv[3]);
    int seconds = atoi(argv[4]);

    std::vector<Result> results(conns);
    std::vector<std::thread> threads;
    for (int i = 0; i < conns; i++)
        threads.emplace_back(worker, host, port, &results[i]);

    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    stopFlag = true;
    for (auto &t : threads) t.join();

    unsigned long long trips = 0;
    double latency = 0;
    for (auto &r : results) { trips += r.trips; latency += r.latencySumUs; }

    printf("connections=%d seconds=%d\n", conns, seconds);
    printf("round trips : %llu\n", trips);
    printf("throughput  : %.0f round trips/sec\n", (double)trips / seconds);
    printf("avg latency : %.1f us\n", trips ? latency / trips : 0.0);
    printf("ROUNDTRIPS=%llu\n", trips);
    return 0;
}
