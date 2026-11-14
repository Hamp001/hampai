#define NOMINMAX
#include <Windows.h>
#include <conio.h>
#include <intrin.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

namespace {

struct CpuTimes {
    std::uint64_t idle = 0;
    std::uint64_t kernel = 0;
    std::uint64_t user = 0;
};

std::uint64_t filetime_to_u64(const FILETIME& value) {
    ULARGE_INTEGER result{};
    result.LowPart = value.dwLowDateTime;
    result.HighPart = value.dwHighDateTime;
    return result.QuadPart;
}

bool read_cpu_times(CpuTimes& result) {
    FILETIME idle{}, kernel{}, user{};
    if (!GetSystemTimes(&idle, &kernel, &user)) {
        return false;
    }

    result.idle = filetime_to_u64(idle);
    result.kernel = filetime_to_u64(kernel);
    result.user = filetime_to_u64(user);
    return true;
}

double calculate_cpu_usage(const CpuTimes& previous, const CpuTimes& current) {
    const auto idle_delta = current.idle - previous.idle;
    const auto kernel_delta = current.kernel - previous.kernel;
    const auto user_delta = current.user - previous.user;
    const auto total_delta = kernel_delta + user_delta;

    if (total_delta == 0 || total_delta < idle_delta) {
        return 0.0;
    }

    const double busy = static_cast<double>(total_delta - idle_delta);
    return std::clamp(100.0 * busy / static_cast<double>(total_delta), 0.0, 100.0);
}

struct MemoryInfo {
    std::uint64_t total_mb = 0;
    std::uint64_t available_mb = 0;
    std::uint64_t used_mb = 0;
    double usage_percent = 0.0;
};

bool read_memory_info(MemoryInfo& result) {
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);

    if (!GlobalMemoryStatusEx(&status)) {
        return false;
    }

    constexpr std::uint64_t bytes_per_mb = 1024ULL * 1024ULL;
    result.total_mb = status.ullTotalPhys / bytes_per_mb;
    result.available_mb = status.ullAvailPhys / bytes_per_mb;
    result.used_mb = result.total_mb - result.available_mb;
    result.usage_percent = static_cast<double>(status.dwMemoryLoad);
    return true;
}

bool has_avx2_support() {
    int registers[4]{};
    __cpuid(registers, 0);
    const int max_leaf = registers[0];

    if (max_leaf < 7) {
        return false;
    }

    __cpuidex(registers, 1, 0);
    constexpr int osxsave_bit = 1 << 27;
    constexpr int avx_bit = 1 << 28;
    if ((registers[2] & osxsave_bit) == 0 || (registers[2] & avx_bit) == 0) {
        return false;
    }

    const unsigned __int64 xcr0 = _xgetbv(0);
    constexpr unsigned __int64 xmm_ymm_state = (1ULL << 1) | (1ULL << 2);
    if ((xcr0 & xmm_ymm_state) != xmm_ymm_state) {
        return false;
    }

    __cpuidex(registers, 7, 0);
    constexpr int avx2_bit = 1 << 5;
    return (registers[1] & avx2_bit) != 0;
}

void enable_virtual_terminal() {
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD mode = 0;
    if (GetConsoleMode(output, &mode)) {
        SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}

std::string make_bar(double percent, int width =  thirty) = delete;

std::string progress_bar(double percent, int width = 28) {
    const int filled = static_cast<int>(std::round((percent / 100.0) * width));
    std::string bar = "[";
    for (int i = 0; i < width; ++i) {
        bar += (i < filled) ? '#' : '-';
    }
    bar += "]";
    return bar;
}

void render(double cpu_usage, const MemoryInfo& memory, bool avx2,
            unsigned int logical_processors) {
    std::cout << "\x1b[H";
    std::cout << "  MicroAI | LOCAL SYSTEM MONITOR\n";
    std::cout << "  ---------------------------------------------\n\n";

    std::cout << "  CPU\n";
    std::cout << "  Usage       " << std::fixed << std::setprecision(1)
              << std::setw(5) << cpu_usage << "%  "
              << progress_bar(cpu_usage) << "\n";
    std::cout << "  Logical CPUs " << logical_processors << "\n";
    std::cout << "  AVX2         " << (avx2 ? "available" : "not available") << "\n\n";

    std::cout << "  MEMORY\n";
    std::cout << "  Usage       " << std::setw(5) << memory.usage_percent << "%  "
              << progress_bar(memory.usage_percent) << "\n";
    std::cout << "  Used        " << memory.used_mb << " MB / "
              << memory.total_mb << " MB\n";
    std::cout << "  Available   " << memory.available_mb << " MB\n\n";

    std::cout << "  INFERENCE (not connected yet)\n";
    std::cout << "  Tokens/sec  --\n";
    std::cout << "  TTFT        --\n";
    std::cout << "  Model RAM   --\n\n";

    std::cout << "  ---------------------------------------------\n";
    std::cout << "  Refresh: 500 ms                         Q: quit\n";
    std::cout.flush();
}

} // namespace

int main() {
    enable_virtual_terminal();
    std::cout << "\x1b[2J\x1b[H";

    CpuTimes previous{};
    if (!read_cpu_times(previous)) {
        std::cerr << "Error: GetSystemTimes failed.\n";
        return 1;
    }

    const bool avx2 = has_avx2_support();
    const unsigned int logical_processors = std::thread::hardware_concurrency();

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (_kbhit()) {
            const int key = _getch();
            if (key == 'q' || key == 'Q') {
                break;
            }
        }

        CpuTimes current{};
        MemoryInfo memory{};

        if (!read_cpu_times(current) || !read_memory_info(memory)) {
            std::cerr << "\nError: unable to read system metrics.\n";
            return 1;
        }

        const double cpu_usage = calculate_cpu_usage(previous, current);
        previous = current;

        render(cpu_usage, memory, avx2, logical_processors);
    }

    std::cout << "\x1b[0m\n";
    return 0;
}
