#include "hash_map.h"

#include "error.h"
#include "extra_math.h"

#include <stdlib.h>
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


static void allocate_hash_map(HashMap* map, size_t base_size);
static void increase_hash_map_size(HashMap* map);
static void decrease_hash_map_size(HashMap* map);
static void resize_hash_map(HashMap* map, size_t base_size);
static MapEntry* create_entry(const char* key, size_t item_size);
static void insert_entry(HashMap* map, MapEntry* entry);
static void free_entry(MapEntry* entry);
static size_t compute_map_key_index(const char* key, size_t upper_limit, unsigned int attempt);
static size_t compute_key_hash(const char* key, unsigned int prime_number, size_t upper_limit);


static MapEntry DELETED_MAP_ENTRY = {NULL, {NULL, 0}};


void print_map_content_as_strings(const HashMap* map)
{
    check(map);

    if (map->n_entries == 0)
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

HashMap create_hash_map(void)
{
    HashMap map;
    allocate_hash_map(&map, INITIAL_HASH_MAP_BASE_SIZE);
    return map;
}

MapItem insert_new_map_item(HashMap* map, const char* key, size_t item_size)
{
    check(map);
    check(key);
    check(map->n_entries < map->size);
    assert(item_size > 0);

    const size_t load = (100*map->n_entries)/map->size;

    if (load > 70)
        increase_hash_map_size(map);

    MapEntry* entry = create_entry(key, item_size);
    insert_entry(map, entry);

    return entry->item;
}

MapItem get_map_item(const HashMap* map, const char* key)
{
    check(map);
    check(key);

    unsigned int attempt = 0;
    size_t hash = compute_map_key_index(key, map->size, attempt++);

    while (map->entries[hash] != NULL)
    {
        if (map->entries[hash] != &DELETED_MAP_ENTRY && strcmp(map->entries[hash]->key, key) == 0)
            return map->entries[hash]->item;

        hash = compute_map_key_index(key, map->size, attempt++);
    }

    MapItem empty_item;
    empty_item.data = NULL;
    empty_item.size = 0;

    return empty_item;
}

void remove_map_item(HashMap* map, const char* key)
{
    check(map);
    check(key);

    const size_t load = (100*map->n_entries)/map->size;

    if (load < 10)
        decrease_hash_map_size(map);

    unsigned int attempt = 0;
    size_t hash = compute_map_key_index(key, map->size, attempt++);

    while (map->entries[hash] != NULL)
    {
        if (map->entries[hash] != &DELETED_MAP_ENTRY && strcmp(map->entries[hash]->key, key) == 0)
        {
            free_entry(map->entries[hash]);
            map->entries[hash] = &DELETED_MAP_ENTRY;
            map->n_entries--;
        }

        hash = compute_map_key_index(key, map->size, attempt++);
    }
}

void clear_hash_map(HashMap* map)
{
    check(map);
    check(map->entries);

    size_t idx;
    for (idx = 0; idx < map->size; idx++)
    {
        free_entry(map->entries[idx]);
        map->entries[idx] = NULL;
    }

    map->n_entries = 0;
}

void destroy_hash_map(HashMap* map)
{
    check(map);

    if (map->entries)
    {
        clear_hash_map(map);
        free(map->entries);
        map->entries = NULL;
    }

    map->size = 0;
    map->n_entries = 0;
}

void insert_string_in_map(HashMap* map, const char* key, const char* string)
{
    check(string);
    const size_t item_size = sizeof(char)*(strlen(string) + 1);
    MapItem item = insert_new_map_item(map, key, item_size);
    strcpy((char*)item.data, string);
}

const char* get_string_from_map(const HashMap* map, const char* key)
{
    MapItem item = get_map_item(map, key);
    return (const char*)item.data;
}

static void allocate_hash_map(HashMap* map, size_t base_size)
{
    assert(map);

    map->base_size = base_size;
    map->size = next_prime(base_size);
    map->n_entries = 0;

    map->entries = (MapEntry**)calloc(map->size, sizeof(MapEntry*));
    check(map->entries);
}

static void increase_hash_map_size(HashMap* map)
{
    assert(map);
    const size_t new_base_size = map->base_size*2;
    resize_hash_map(map, new_base_size);
}

static void decrease_hash_map_size(HashMap* map)
{
    assert(map);
    const size_t new_base_size = map->base_size/2;
    resize_hash_map(map, new_base_size);
}

static void resize_hash_map(HashMap* map, size_t base_size)
{
    assert(map);

    if (base_size < INITIAL_HASH_MAP_BASE_SIZE)
        return;

    HashMap old_map = *map;
    allocate_hash_map(map, base_size);

    MapEntry* entry = NULL;

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
    assert(map->n_entries < map->size);

    unsigned int attempt = 0;
    size_t hash = compute_map_key_index(entry->key, map->size, attempt++);

    while (map->entries[hash] != NULL && map->entries[hash] != &DELETED_MAP_ENTRY)
    {
        if (strcmp(map->entries[hash]->key, entry->key) == 0)
        {
            free_entry(map->entries[hash]);
            map->n_entries--;
            break;
        }

        hash = compute_map_key_index(entry->key, map->size, attempt++);
    }

    map->entries[hash] = entry;
    map->n_entries++;
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

static size_t compute_map_key_index(const char* key, size_t upper_limit, unsigned int attempt)
{
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
