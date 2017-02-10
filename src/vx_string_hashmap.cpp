#include "vx_string_hashmap.hpp"
#include <string.h>
#include "um.hpp"

u64
hash_string(const char *str, u32 numBuckets)
{
    // djb2 algorithm
    u64 hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash % numBuckets;
}

vx::StringHashmapBucket*
bucket_new(const char *key, void *value)
{
    vx::StringHashmapBucket *bucket = (vx::StringHashmapBucket*)malloc(sizeof(vx::StringHashmapBucket));
    bucket->key = (char*)malloc(strlen(key) + 1);
    bucket->value = value;
    strcpy(bucket->key, key);
    bucket->next = NULL;

    ASSERT(bucket != NULL);
    return bucket;
}

void
free_bucket_list(vx::StringHashmapBucket* bucketList, void (*free_fn)(void *data))
{
    vx::StringHashmapBucket* search = bucketList;
    while (search != NULL)
    {
        vx::StringHashmapBucket* next = search->next;
        free(search->key);
        if (free_fn != NULL)
        {
            free_fn(search->value);
        }
        free(search);
        search = next;
    }
}

vx::StringHashmap*
vx::string_hashmap_new(u32 numBuckets)
{
    vx::StringHashmap *hashmap = (vx::StringHashmap*)malloc(sizeof(vx::StringHashmap));
    hashmap->num_buckets = numBuckets;
    hashmap->buckets = (vx::StringHashmapBucket**)calloc(numBuckets, sizeof(vx::StringHashmapBucket));
    return hashmap;
}

void
vx::string_hashmap_free_with_fn(vx::StringHashmap *hashmap, void (*freeFn)(void *data))
{
    for (size_t i = 0; i < hashmap->num_buckets; ++i)
    {
        if (hashmap->buckets[i] != NULL)
        {
            free_bucket_list(hashmap->buckets[i], freeFn);
        }
    }
    free(hashmap->buckets);
    free(hashmap);
}

void
vx::string_hashmap_free(vx::StringHashmap* hashmap)
{
    for (size_t i = 0; i < hashmap->num_buckets; ++i)
    {
        if (hashmap->buckets[i] != NULL)
        {
            free_bucket_list(hashmap->buckets[i], NULL);
        }
    }
    free(hashmap->buckets);
    free(hashmap);
}

void
vx::string_hashmap_insert(vx::StringHashmap *hashmap, const char *key, void *value)
{
    u64 hash = hash_string(key, hashmap->num_buckets);
    vx::StringHashmapBucket* firstBucket = hashmap->buckets[hash];

    if (firstBucket == NULL)
    {
        hashmap->buckets[hash] = bucket_new(key, value);
    }
    else
    {
        vx::StringHashmapBucket* search = firstBucket;

        while (search->next != NULL)
        {
            ASSERT(strcmp(search->key, key) != 0);
            search = search->next;
        }
        ASSERT(search->next == NULL);
        search->next = bucket_new(key, value);
    }
}

bool
vx::string_hashmap_is_present(vx::StringHashmap* hashmap, const char *key)
{
    u64 hash = hash_string(key, hashmap->num_buckets);
    vx::StringHashmapBucket* firstBucket = hashmap->buckets[hash];

    if (firstBucket != NULL)
    {
        vx::StringHashmapBucket* search = firstBucket;
        while (search != NULL)
        {
            if (strcmp(search->key, key) == 0) return true;
            search = search->next;
        }
    }
    return false;
}

void *
vx::string_hashmap_get(vx::StringHashmap* hashmap, const char *key)
{
    u64 hash = hash_string(key, hashmap->num_buckets);
    ASSERT(hash < hashmap->num_buckets);
    vx::StringHashmapBucket* firstBucket = hashmap->buckets[hash];
    if (firstBucket != NULL)
    {
        vx::StringHashmapBucket* search = firstBucket;
        while (search != NULL)
        {
            if (strcmp(search->key, key) == 0)
            {
                return search->value;
            }
            search = search->next;
        }
    }
    return NULL;
}
