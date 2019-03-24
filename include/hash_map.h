#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stddef.h>

typedef struct MapEntry MapEntry;

typedef struct MapItem
{
    void* data;
    size_t size;
} MapItem;

typedef struct HashMap
{
    MapEntry** entries;
    size_t base_size;
    size_t size;
    size_t length;
    char* iterator;
} HashMap;

void print_map_content_as_strings(const HashMap* map);

HashMap create_map(void);
MapItem insert_new_map_item(HashMap* map, const char* key, size_t item_size);
MapItem get_map_item(const HashMap* map, const char* key);

int map_has_key(const HashMap* map, const char* key);

void remove_map_item(HashMap* map, const char* key);
void clear_map(HashMap* map);
void destroy_map(HashMap* map);

void insert_string_in_map(HashMap* map, const char* key, const char* string, ...);
const char* get_string_from_map(const HashMap* map, const char* key);

void reset_map_iterator(HashMap* map);
int valid_map_iterator(const HashMap* map);
void advance_map_iterator(HashMap* map);

const char* get_current_map_key(const HashMap* map);

#endif
