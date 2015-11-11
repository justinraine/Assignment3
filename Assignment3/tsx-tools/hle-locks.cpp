#include <immintrin.h>

int main() {
    int lock;
    
    // Attempt to secure lock (write 0 to lock)
    // __atomic_exchange_n returns previous value of lock
    
    // While locked
    while (__atomic_exchange_n(&lock, 0, __ATOMIC_ACQUIRE|__ATOMIC_HLE_ACQUIRE) != 1) {
        int lockStatus; // 0: locked; 1: unlocked
        
        // Wait for lock to become free again before retrying.
        do {
            _mm_pause();
            
            // Abort speculation
            __atomic_load(&lock, &val, __ATOMIC_CONSUME);
            
        } while (val == 0); // while locked
    }
    
    return 1;
}


