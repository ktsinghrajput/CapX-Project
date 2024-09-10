#include <iostream>
#include <unordered_map>
#include <list>
#include <vector>
#include <memory>
#include <mutex>
#include <algorithm>
#include <limits>

using namespace std;

// Abstract base class for cache eviction policies
class EvictionPolicy
{
public:
    virtual void access(int key) = 0;
    virtual void insert(int key) = 0;
    virtual void evict() = 0;
    virtual bool exists(int key) = 0;
    virtual ~EvictionPolicy() {}
};

// LRU eviction policy implementation
class LRU : public EvictionPolicy
{
private:
    list<int> usageOrder; // stores key usage order (most recent at the front)
    unordered_map<int, list<int>::iterator> keyIterMap;
    int capacity;

public:
    LRU(int capacity) : capacity(capacity) {}

    void access(int key) override
    {
        if (keyIterMap.find(key) != keyIterMap.end())
        {
            usageOrder.erase(keyIterMap[key]);
            usageOrder.push_front(key);
            keyIterMap[key] = usageOrder.begin();
        }
    }

    void insert(int key) override
    {
        if (usageOrder.size() >= capacity)
        {
            evict();
        }
        usageOrder.push_front(key);
        keyIterMap[key] = usageOrder.begin();
    }

    void evict() override
    {
        int evictedKey = usageOrder.back();
        usageOrder.pop_back();
        keyIterMap.erase(evictedKey);
    }

    bool exists(int key) override
    {
        return keyIterMap.find(key) != keyIterMap.end();
    }
};

// LFU eviction policy implementation
class LFU : public EvictionPolicy
{
private:
    unordered_map<int, int> freqMap;    // key -> frequency
    unordered_map<int, int> keyTimeMap; // key -> insertion order (to break ties)
    int timeCounter;
    int capacity;

public:
    LFU(int capacity) : capacity(capacity), timeCounter(0) {}

    void access(int key) override
    {
        freqMap[key]++;
    }

    void insert(int key) override
    {
        if (freqMap.size() >= capacity)
        {
            evict();
        }
        freqMap[key] = 1;
        keyTimeMap[key] = timeCounter++;
    }

    void evict() override
    {
        int minFreq = numeric_limits<int>::max();
        int oldestTime = numeric_limits<int>::max();
        int keyToEvict = -1;

        for (auto &pair : freqMap)
        {
            int key = pair.first;
            int freq = pair.second;
            int time = keyTimeMap[key];

            if (freq < minFreq || (freq == minFreq && time < oldestTime))
            {
                minFreq = freq;
                oldestTime = time;
                keyToEvict = key;
            }
        }

        if (keyToEvict != -1)
        {
            freqMap.erase(keyToEvict);
            keyTimeMap.erase(keyToEvict);
        }
    }

    bool exists(int key) override
    {
        return freqMap.find(key) != freqMap.end();
    }
};

// CacheLevel class which holds the data, eviction policy, and size
class CacheLevel
{
public:
    unordered_map<int, string> cacheData;
    unique_ptr<EvictionPolicy> evictionPolicy;
    int capacity;

    CacheLevel(int size, const string &policy) : capacity(size)
    {
        if (policy == "LRU")
        {
            evictionPolicy = make_unique<LRU>(size);
        }
        else if (policy == "LFU")
        {
            evictionPolicy = make_unique<LFU>(size);
        }
    }

    bool exists(int key)
    {
        return evictionPolicy->exists(key);
    }

    string get(int key)
    {
        return cacheData[key];
    }

    void put(int key, const string &value)
    {
        if (cacheData.size() >= capacity && !evictionPolicy->exists(key))
        {
            evictionPolicy->evict();
        }
        cacheData[key] = value;
        evictionPolicy->insert(key);
    }

    void promote(int key, const string &value)
    {
        if (!evictionPolicy->exists(key))
        {
            put(key, value);
        }
        evictionPolicy->access(key);
    }

    void display()
    {
        for (auto &pair : cacheData)
        {
            cout << pair.first << ": " << pair.second << " ";
        }
    }
};

// MultilevelCache class which manages multiple cache levels
class MultilevelCache
{
private:
    vector<shared_ptr<CacheLevel>> cacheLevels;
    mutex mtx;

public:
    void addCacheLevel(int size, const string &policy)
    {
        lock_guard<mutex> lock(mtx);
        cacheLevels.push_back(make_shared<CacheLevel>(size, policy));
    }

    void removeCacheLevel(int level)
    {
        lock_guard<mutex> lock(mtx);
        if (level >= 1 && level <= cacheLevels.size())
        {
            cacheLevels.erase(cacheLevels.begin() + (level - 1));
        }
    }

    string get(int key)
    {
        lock_guard<mutex> lock(mtx);
        for (size_t i = 0; i < cacheLevels.size(); ++i)
        {
            if (cacheLevels[i]->exists(key))
            {
                string value = cacheLevels[i]->get(key);
                // Promote to higher levels
                for (int j = i - 1; j >= 0; --j)
                {
                    cacheLevels[j]->promote(key, value);
                }
                return value;
            }
        }
        // Simulate fetching from main memory if not found
        string fetchedValue = "Data_from_main_memory";
        put(key, fetchedValue); // Insert into L1
        return fetchedValue;
    }

    void put(int key, const string &value)
    {
        lock_guard<mutex> lock(mtx);
        if (!cacheLevels.empty())
        {
            cacheLevels[0]->put(key, value);
        }
    }

    void displayCache()
    {
        lock_guard<mutex> lock(mtx);
        for (size_t i = 0; i < cacheLevels.size(); ++i)
        {
            cout << "L" << (i + 1) << " Cache: ";
            cacheLevels[i]->display();
            cout << endl;
        }
    }
};

int main()
{
    MultilevelCache cache;

    // Adding cache levels
    cache.addCacheLevel(3, "LRU"); // L1 with size 3, LRU policy
    cache.addCacheLevel(2, "LFU"); // L2 with size 2, LFU policy

    cache.put(1, "A");
    cache.put(2, "B");
    cache.put(3, "C");

    cout << cache.get(1) << endl; // Access from L1

    cache.put(4, "D"); // LRU eviction in L1

    cache.get(3); // Move from L2 to L1

    cache.displayCache();

    return 0;
}
