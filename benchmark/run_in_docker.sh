#!/bin/bash
# Run from anywhere on the Mac: ./benchmark/run_in_docker.sh
# seccomp=unconfined lets io_uring work, SYS_PTRACE lets strace attach.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
docker run --rm \
    --security-opt seccomp=unconfined --cap-add SYS_PTRACE \
    -e CONNS="${CONNS:-64}" -e SECS="${SECS:-5}" \
    -v "$ROOT":/w -w /w gcc bash -c '
        apt-get update -qq >/dev/null &&
        apt-get install -y -qq liburing-dev strace >/dev/null 2>&1 &&
        bash benchmark/run.sh'
