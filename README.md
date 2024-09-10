Explanation:
CacheLevel Class: Manages the cache entries, size, and eviction policy (either LRU or LFU). It uses an unordered_map for storing cache data, and eviction policy is managed through unique classes for LRU and LFU.

Eviction Policies:

LRU: Implements Least Recently Used policy using a list to track usage order and an unordered_map to provide fast lookup.
LFU: Implements Least Frequently Used policy by tracking the frequency of each key along with a time counter to break ties.
MultilevelCache Class: Manages multiple cache levels dynamically and ensures thread-safety using std::mutex. It supports adding/removing cache levels and handles data promotion across levels.

Main Memory Simulation: If a key is not found in any cache level, it simulates fetching from the main memory and stores the result in the L1 cache.

Concurrency: The system is thread-safe by using std::mutex to protect shared cache structures during read/write operations.

Usage Example: The example in the main function demonstrates the cache system behavior with two levels (L1 and L2), using different eviction policies and showcasing insertion, retrieval, and eviction scenarios.

Output:
1
L1 Cache: 1: A 4: D 3: C 
L2 Cache: 2: B 
