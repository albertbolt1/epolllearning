#!/bin/bash
# Runs inside a Linux container (see run_in_docker.sh). Needs g++, liburing-dev and strace.
#   CONNS=64 SECS=5 ./run.sh
cd "$(dirname "$0")"
CONNS=${CONNS:-64}
SECS=${SECS:-5}
SERVER_LIFE=$((SECS + 4))

mkdir -p build
g++ -O2 -std=c++17 -pthread -o build/loadgen loadgen.cpp || exit 1
g++ -O2 -std=c++17 -o build/epoll_echo epoll_echo.cpp || exit 1
g++ -O2 -std=c++17 -o build/uring_echo uring_echo.cpp -luring || exit 1

# 1) throughput, no tracing, so strace does not slow anything down
run_plain() {
    ./build/$1 $SERVER_LIFE > build/$1.log 2>&1 &
    sleep 1
    ./build/loadgen 127.0.0.1 9091 $CONNS $SECS > build/$1.load
    wait
}

# 2) syscall count, with strace attached to the server only
run_strace() {
    strace -f -c -o build/$1.strace ./build/$1 $SERVER_LIFE > build/$1.log 2>&1 &
    sleep 2
    ./build/loadgen 127.0.0.1 9091 $CONNS $SECS > build/$1.loadtrace
    wait
}

for s in epoll_echo uring_echo; do
    run_plain $s
    if grep -q "failed" build/$s.log; then
        echo "$s failed to start:"; cat build/$s.log; exit 1
    fi
    run_strace $s
done

echo
echo "=============== $CONNS connections, ${SECS}s each ==============="
for s in epoll_echo uring_echo; do
    echo
    echo "---- $s ----"
    grep -E "throughput|avg latency" build/$s.load
    trips=$(grep ROUNDTRIPS build/$s.loadtrace | cut -d= -f2)
    calls=$(awk '$NF=="total"{print $4}' build/$s.strace)
    echo "syscalls (server, traced run): $calls for $trips round trips"
    awk -v c="$calls" -v t="$trips" 'BEGIN{ printf "syscalls per round trip     : %.2f\n", c/t }'
    echo "top syscalls:"
    grep -vE "^-|total|% time|calls" build/$s.strace | sort -k4 -nr | head -4 | awk '{printf "   %-16s %s calls\n", $NF, $4}'
done
