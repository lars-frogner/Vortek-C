#include "hash_map.h"

#include "error.h"
#include "extra_math.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>


#define HASH_MAP_PRIME_1 100151
#define HASH_MAP_PRIME_2 100153
#define INITIAL_HASH_MAP_BASE_SIZE 50


typedef struct MapEntry
{
    char* key;
    MapItem item;
} MapEntry;


static void allocate_map(HashMap* map, size_t base_size);
static void increase_map_size(HashMap* map);
static void decrease_map_size(HashMap* map);
static void resize_map(HashMap* map, size_t base_size);
static MapEntry* create_entry(const char* key, size_t item_size);
static void insert_entry(HashMap* map, MapEntry* entry);
static void free_entry(MapEntry* entry);
static long find_key_location(const HashMap* map, const char* key);
static size_t compute_map_key_index(const char* key, size_t upper_limit, unsigned int attempt);
static size_t compute_key_hash(const char* key, unsigned int prime_number, size_t upper_limit);


static MapEntry DELETED_MAP_ENTRY = {NULL, {NULL, 0}};


void print_map_content_as_strings(const HashMap* map)
{
    check(map);

    if (map->length == 0)
    {
        printf("{}\n");
        return;
    }

    printf("{");

    size_t i;
    size_t entry_num = 0;

    for (i = 0; i < map->size; i++)
    {
        if (map->entries[i] != NULL && map->entries[i] != &DELETED_MAP_ENTRY)
        {
            if (entry_num > 0)
                printf(", ");

            printf("\"%s\": \"%s\"", map->entries[i]->key, (char*)map->entries[i]->item.data);

            entry_num++;
        }
    }

    printf("}\n");
}

HashMap create_map(void)
{
    HashMap map;
    allocate_map(&map, INITIAL_HASH_MAP_BASE_SIZE);
    return map;
}

MapItem insert_new_map_item(HashMap* map, const char* key, size_t item_size)
{
    check(map);
    check(key);
    check(map->length < map->size);
    assert(item_size > 0);

    const size_t load = (100*map->length)/map->size;

    if (load > 70)
        increase_map_size(map);

    MapEntry* entry = create_entry(key, item_size);
    insert_entry(map, entry);

    return entry->item;
}

MapItem get_map_item(const HashMap* map, const char* key)
{
    check(map);
    check(key);

    const long location = find_key_location(map, key);

    if (location >= 0)
    {
        return map->entries[location]->item;
    }
    else
    {
        MapItem empty_item;
        empty_item.data = NULL;
        empty_item.size = 0;
        return empty_item;
    }
}

void remove_map_item(HashMap* map, const char* key)
{
    check(map);
    check(key);

    const size_t load = (100*map->length)/map->size;

    if (load < 10)
        decrease_map_size(map);

    const long location = find_key_location(map, key);

    if (location >= 0)
    {
        free_entry(map->entries[location]);
        map->entries[location] = &DELETED_MAP_ENTRY;
        map->length--;
        map->iterator = NULL;
    }
}

void clear_map(HashMap* map)
{
    check(map);
    check(map->entries);

    size_t idx;
    for (idx = 0; idx < map->size; idx++)
    {
        free_entry(map->entries[idx]);
        map->entries[idx] = NULL;
    }

    map->length = 0;
    map->iterator = NULL;
}

void destroy_map(HashMap* map)
{
    check(map);

    if (map->entries)
    {
        clear_map(map);
        free(map->entries);
        map->entries = NULL;
    }

    map->size = 0;
    map->length = 0;
}

void insert_string_in_map(HashMap* map, const char* key, const char* string, ...)
{
    check(string);

    va_list args;

    va_start(args, string);
    const int string_length = vsnprintf(NULL, 0, string, args);
    va_end(args);

    check(string_length > 0);
    const size_t item_size = sizeof(char)*((size_t)string_length + 1);

    MapItem item = insert_new_map_item(map, key, item_size);

    va_start(args, string);
    vsprintf((char*)item.data, string, args);
    va_end(args);
}

const char* get_string_from_map(const HashMap* map, const char* key)
{
    MapItem item = get_map_item(map, key);
    return (const char*)item.data;
}

void reset_map_iterator(HashMap* map)
{
    check(map);
    map->iterator = NULL;

    size_t i;
    for (i = 0; i < map->size; i++)
    {
        if (map->entries[i] != NULL && map->entries[i] != &DELETED_MAP_ENTRY)
        {
            map->iterator = map->entries[i]->key;
            break;
        }
    }
}

int valid_map_iterator(const HashMap* map)
{
    check(map);
    return map->iterator != NULL;
}

void advance_map_iterator(HashMap* map)
{
    check(map);

    if (!map->iterator)
        return;

    const long location = find_key_location(map, map->iterator);
    check(location >= 0);

    size_t i;
    for (i = (size_t)location + 1; i < map->size; i++)
    {
        if (map->entries[i] != NULL && map->entries[i] != &DELETED_MAP_ENTRY)
        {
            map->iterator = map->entries[i]->key;
            break;
        }
    }

    if (i == map->size)
        map->iterator = NULL;
}

const char* get_current_map_key(const HashMap* map)
{
    check(map);
    check(map->iterator);
    return map->iterator;
}

static void allocate_map(HashMap* map, size_t base_size)
{
    assert(map);

    map->base_size = base_size;
    map->size = next_prime(base_size); // Use a prime number as the actual size
    map->length = 0;
    map->iterator = NULL;

    map->entries = (MapEntry**)calloc(map->size, sizeof(MapEntry*));
    check(map->entries);
}

static void increase_map_size(HashMap* map)
{
    assert(map);
    const size_t new_base_size = map->base_size*2;
    resize_map(map, new_base_size);
}

static void decrease_map_size(HashMap* map)
{
    assert(map);
    const size_t new_base_size = map->base_size/2;
    resize_map(map, new_base_size);
}

static void resize_map(HashMap* map, size_t base_size)
{
    assert(map);

    if (base_size < INITIAL_HASH_MAP_BASE_SIZE)
        return;

    HashMap old_map = *map;
    allocate_map(map, base_size);

    MapEntry* entry = NULL;

    // Loop through the old map and insert all existing non-deleted entries into the new map
    size_t i;
    for (i = 0; i < old_map.size; i++)
    {
        entry = old_map.entries[i];

        if (entry != NULL && entry != &DELETED_MAP_ENTRY)
            insert_entry(map, entry);
    }

    free(old_map.entries);
}

static MapEntry* create_entry(const char* key, size_t item_size)
{
    assert(key);

    MapEntry* entry = (MapEntry*)malloc(sizeof(MapEntry));

    const size_t key_size = sizeof(char)*(strlen(key) + 1);
    entry->key = (char*)malloc(key_size);
    strcpy(entry->key, key);

    entry->item.data = malloc(item_size);
    entry->item.size = item_size;

    return entry;
}

static void insert_entry(HashMap* map, MapEntry* entry)
{
    assert(map);
    assert(entry);
    assert(map->length < map->size);

    unsigned int attempt = 0;
    size_t idx = compute_map_key_index(entry->key, map->size, attempt++);

    // Visit entries until we encounter an empty or deleted one
    while (map->entries[idx] != NULL && map->entries[idx] != &DELETED_MAP_ENTRY)
    {
        // If an existing entry already uses the key we want to insert for, remove it before the new entry is inserted
        if (strcmp(map->entries[idx]->key, entry->key) == 0)
        {
            free_entry(map->entries[idx]);
            map->length--;
            break;
        }

        idx = compute_map_key_index(entry->key, map->size, attempt++);
    }

    // Insert the new entry when we have found a free location
    map->entries[idx] = entry;
    map->length++;
    map->iterator = NULL;
}

static void free_entry(MapEntry* entry)
{
    if (!entry || entry == &DELETED_MAP_ENTRY)
        return;

    if (entry->key)
        free(entry->key);

    if (entry->item.data)
        free(entry->item.data);

    free(entry);
}

static long find_key_location(const HashMap* map, const char* key)
{
    assert(map);
    assert(key);

    long location = -1; // We will return -1 if the entry for the given key does not exist
    unsigned int attempt = 0;
    size_t idx = compute_map_key_index(key, map->size, attempt++);

    // Visit entries until we encounter an empty one (if we do, the requested entry does not exist)
    while (map->entries[idx] != NULL)
    {
        // If the occupied entry is not deleted and has the right key, return the index
        if (map->entries[idx] != &DELETED_MAP_ENTRY && strcmp(map->entries[idx]->key, key) == 0)
        {
            location = (long)idx;
            break;
        }

        // If the occupied entry was not the right one, increment the attempt number and compute the next index
        idx = compute_map_key_index(key, map->size, attempt++);
    }

    return location;
}

static size_t compute_map_key_index(const char* key, size_t upper_limit, unsigned int attempt)
{
    /*
    Uses double hashing to compute a new index from the same key each time the
    attempt number is incremented. This addresses the problem of collisions when
    multiple keys give the same hash.
    */
    assert(attempt < upper_limit*upper_limit);
    const size_t hash_1 = compute_key_hash(key, HASH_MAP_PRIME_1, upper_limit);
    const size_t hash_2 = compute_key_hash(key, HASH_MAP_PRIME_2, upper_limit - 1);
    return (hash_1 + attempt*(hash_2 + 1)) % upper_limit;
}

static size_t compute_key_hash(const char* key, unsigned int prime_number, size_t upper_limit)
{
    const size_t key_length = strlen(key);

    size_t i;
    size_t hash = 0;
    float scale;

    for (i = 0; i < key_length; i++)
    {
        scale = powf((float)prime_number, (float)(key_length - (i + 1)));
        hash += (size_t)key[i]*(size_t)scale;
        hash = hash % upper_limit;
    }

    return hash;
}
