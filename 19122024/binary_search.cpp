#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <sys/time.h>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cerrno>
#include <cstring>
#include <chrono>
#include <cpuid.h>
#include <immintrin.h>

#define PERF_START do { \
    int ctl_fifo_fd = open("./ctl_fifo", O_WRONLY); \
    if (ctl_fifo_fd < 0) { perror("open"); exit(1); } \
    (void) write(ctl_fifo_fd, "enable", sizeof("enable")); \
    close(ctl_fifo_fd); \
} while(0)

#define PERF_END do { \
    int ctl_fifo_fd = open("./ctl_fifo", O_WRONLY); \
    if (ctl_fifo_fd < 0) { perror("open"); exit(1); } \
    (void) write(ctl_fifo_fd, "disable", sizeof("disable")); \
    close(ctl_fifo_fd); \
} while(0)


static inline void speculation_barrier(void) {
	uint32_t unused;
	__builtin_ia32_rdtscp(&unused);
	__builtin_ia32_lfence();
}

// Original binary search implementation
int binary_search(const int arr[], int size, int target) {
    int left = 0;
    int right = size - 1;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        
        if (arr[mid] == target)
            return mid;
        
        if (arr[mid] < target)
            left = mid + 1;
        else
            right = mid - 1;
    }
    
    return -1;  // Element not found
}

// Branchless binary search implementation
int binary_search_branchless(const int arr[], int size, int target) {
    int *base = const_cast<int*>(arr);
    int len = size;
    
    while (len > 1) {
        int half = len / 2;
        base += (base[half - 1] < target) * half;
        len -= half;
    }
    
    return (base - arr);
}

// Generate sorted random array with a more sophisticated pattern
void generate_sorted_array(int arr[], int size) {
    // Start with a base value
    const int base = 17;  // A prime number
    const int step = 3;   // Another prime number
    
    for (int i = 0; i < size; i++) {
        // Create a pattern that combines:
        // 1. Base value
        // 2. Step increment
        // 3. A small prime-based variation
        arr[i] = base + (i * step) + ((i % 7) * 5);  // 7 and 5 are primes
    }
}

// Generate random search keys
void generate_search_keys(int arr[], int size, int max_value) {
    for (int i = 0; i < size; i++) {
        arr[i] = rand() % max_value;
    }
}

// Get current time in microseconds
uint64_t get_time_us() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (uint64_t)(tv.tv_sec) * 1000000 + (uint64_t)(tv.tv_usec);
}

// Format size to human readable format
std::string format_size(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    double size = bytes;
    
    while (size >= 1024 && unit < 3) {
        size /= 1024;
        unit++;
    }
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unit]);
    return std::string(buffer);
}

// Format time to human readable format
std::string format_time(uint64_t us) {
    char buffer[32];
    if (us < 1000) {
        snprintf(buffer, sizeof(buffer), "%lu ns", us * 1000);  // Convert to nanoseconds
    } else if (us < 1000000) {
        snprintf(buffer, sizeof(buffer), "%.2f µs", us / 1000.0);
    } else if (us < 1000000000) {
        snprintf(buffer, sizeof(buffer), "%.2f ms", us / 1000000.0);
    } else {
        snprintf(buffer, sizeof(buffer), "%.2f s", us / 1000000000.0);
    }
    return std::string(buffer);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <array_size> <search_keys_size> <search_type>\n";
        std::cerr << "search_type: 'regular' for regular binary search, 'branchless' for branchless binary search\n";
        return 1;
    }

    const int array_size = std::stoi(argv[1]);
    const int search_keys_size = std::stoi(argv[2]);
    const std::string search_type = argv[3];
    
    if (search_type != "regular" && search_type != "branchless") {
        std::cerr << "Invalid search type. Use 'regular' or 'branchless'\n";
        return 1;
    }
    
    // Allocate sorted array using 1GB hugepages
    int* sorted_array = static_cast<int*>(mmap(nullptr, 
                           array_size * sizeof(int),
                           PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | (30 << MAP_HUGE_SHIFT),
                           -1,
                           0));
    
    if (sorted_array == MAP_FAILED) {
        if (errno == ENOMEM) {
            std::cerr << "Failed to allocate hugepage. Make sure hugepages are configured:\n";
            std::cerr << "Check /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages\n";
            std::cerr << "You can set it with: echo 1 > /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages\n";
        } else {
            std::cerr << "mmap failed: " << strerror(errno) << "\n";
        }
        return 1;
    }

    // Allocate search keys array normally
    int* search_keys = new int[search_keys_size];
    
    if (!search_keys) {
        std::cerr << "Memory allocation failed for search keys\n";
        munmap(sorted_array, array_size * sizeof(int));
        return 1;
    }
    
    // Initialize random seed
    srand(time(nullptr));
    
    // Generate arrays
    generate_sorted_array(sorted_array, array_size);
    generate_search_keys(search_keys, search_keys_size, array_size * 2);
    

    
    // Run selected search type
    
    // Warmup phase
    volatile int warmup_sum = 0;
    if (search_type == "regular") {
        for (int i = 0; i < search_keys_size; i++) {
            warmup_sum += binary_search(sorted_array, array_size, search_keys[i]);
        }
    } else {
        for (int i = 0; i < search_keys_size; i++) {
            warmup_sum += binary_search_branchless(sorted_array, array_size, search_keys[i]);
        }
    }
    
    // Use high-resolution clock for timing
    PERF_START;

    auto start = std::chrono::high_resolution_clock::now();
    
    // Use volatile to prevent optimization
    volatile int dummy_sum = 0;
    
    if (search_type == "regular") {
        for (int i = 0; i < search_keys_size; i++) {
            dummy_sum += binary_search(sorted_array, array_size, search_keys[i]);
            speculation_barrier();
        }
    } else {
        for (int i = 0; i < search_keys_size; i++) {
            dummy_sum += binary_search_branchless(sorted_array, array_size, search_keys[i]);
            speculation_barrier();

        }
    }
    
    // Prevent compiler from optimizing out dummy_sum
    if (dummy_sum == 0) {
        std::cout << "Dummy sum: " << dummy_sum << std::endl;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    PERF_END;
    
    // Calculate duration in nanoseconds
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    uint64_t total_time = duration.count();
    
    // Print statistics
    std::cout << "\nBinary Search Benchmark Results:\n";
    std::cout << "--------------------------------\n";
    std::cout << "Array size: " << array_size << " elements (" << format_size(array_size * sizeof(int)) << ")\n";
    std::cout << "Number of searches: " << search_keys_size << "\n";
    std::cout << "Search type: " << search_type << "\n";
    std::cout << "Total time: " << format_time(total_time / 1000) << "\n";  // Convert ns to µs
    
    // Cleanup
    munmap(sorted_array, array_size * sizeof(int));
    delete[] search_keys;
    
    return 0;
} 