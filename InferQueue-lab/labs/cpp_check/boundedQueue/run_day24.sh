#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

# 本脚本应与用户当前的三个源码文件放在同一个目录。
for source in bounded_queue.hpp thread_pool.hpp test_thread_pool.cpp benchmark.cpp; do
    if [[ ! -f "$source" ]]; then
        printf 'Missing required file: %s\n' "$source" >&2
        exit 1
    fi
done

mkdir -p build
result_dir=$(mktemp -d -p . day24-results-XXXXXXXX)
printf 'Results directory: %s\n' "$result_dir"
trap 'status=$?; if (( status != 0 )); then printf "Stopped with exit code %s. Inspect %s; do not mark checks as passed.\n" "$status" "$result_dir" >&2; fi' EXIT

{
    date -Is
    uname -srmo
    g++ --version
    printf '\nAvailable processors: '
    nproc
    if command -v lscpu >/dev/null; then
        LC_ALL=C lscpu
    fi
    printf '\nSource hashes:\n'
    sha256sum bounded_queue.hpp thread_pool.hpp test_thread_pool.cpp benchmark.cpp run_day24.sh
} > "$result_dir/environment.txt"

common=(-std=c++17 -Wall -Wextra -Wpedantic -pthread)

printf '\n[1/3] Build and run normal regression tests, five repetitions\n'
g++ "${common[@]}" -O0 -g test_thread_pool.cpp -o build/test_thread_pool_day24 \
    > "$result_dir/build-normal.log" 2>&1
for round in 1 2 3 4 5; do
    timeout 30s ./build/test_thread_pool_day24 \
        > "$result_dir/regression-$round.log" 2>&1
done
cat "$result_dir/regression-1.log"

printf '\n[2/3] Build and run ThreadSanitizer\n'
# -fsanitize=thread：启用数据竞争检测。
# -g：让报告更容易定位源码。
# 检查的是程序实际运行到的路径；注释放在完整命令前。
g++ "${common[@]}" -O1 -g -fsanitize=thread -fno-omit-frame-pointer \
    test_thread_pool.cpp -o build/test_thread_pool_tsan_day24 \
    > "$result_dir/build-tsan.log" 2>&1
TSAN_OPTIONS=halt_on_error=1 timeout 60s ./build/test_thread_pool_tsan_day24 \
    > "$result_dir/tsan.log" 2>&1
cat "$result_dir/tsan.log"

printf '\n[3/3] Build optimized benchmark and compare 1/2/4 workers\n'
g++ "${common[@]}" -O2 benchmark.cpp -o build/benchmark_day24 \
    > "$result_dir/build-benchmark.log" 2>&1
timeout 120s ./build/benchmark_day24 \
    > "$result_dir/benchmark.csv" 2> "$result_dir/benchmark-summary.txt"
cat "$result_dir/benchmark-summary.txt"

printf '\nAll commands completed successfully. Results: %s\n' "$result_dir"
printf 'TSan covers this execution only; review synchronization and lifetimes too.\n'
