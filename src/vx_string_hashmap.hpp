#ifndef VX_STRING_HASHMAP_HPP
#define VX_STRING_HASHMAP_HPP

#include <stdbool.h>
#include <GLFW/glfw3.h>
#include "um.hpp"

namespace vx
{

struct StringHashmapBucket
{
    void*                value;
    char*                key;
    StringHashmapBucket* next;
};

struct StringHashmap
{
    u32                        num_buckets;
    StringHashmapBucket**      buckets;
};

StringHashmap*     string_hashmap_new(u32 numBuckets);
void               string_hashmap_free(StringHashmap* hashmap);
void               string_hashmap_free_with_fn(StringHashmap* hashmap, void (*free_fn)(void *data));

void               string_hashmap_insert(StringHashmap* hashmap, const char *key, void *value);
bool               string_hashmap_is_present(StringHashmap* hashmap, const char *key);
void*              string_hashmap_get(StringHashmap* hashmap, const char *key);

}

#endif // VX_STRING_HASHMAP_HPP
