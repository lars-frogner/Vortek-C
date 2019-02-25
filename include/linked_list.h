#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stddef.h>

typedef struct Link Link;

typedef struct ListItem
{
    void* data;
    size_t size;
} ListItem;

typedef struct LinkedList
{
    Link* start;
    Link* end;
    size_t length;
} LinkedList;

void print_list_content_as_strings(const LinkedList* list);

LinkedList create_linked_list(void);
ListItem append_new_list_item(LinkedList* list, size_t item_size);
ListItem prepend_new_list_item(LinkedList* list, size_t item_size);
ListItem insert_new_list_item(LinkedList* list, size_t item_size, size_t idx);

size_t get_list_length(const LinkedList* list);
ListItem get_list_item(const LinkedList* list, size_t idx);
ListItem get_first_list_item(const LinkedList* list);
ListItem get_last_list_item(const LinkedList* list);

void join_lists(LinkedList* list, LinkedList* other_list);

void remove_list_item(LinkedList* list, size_t idx);
void clear_list(LinkedList* list);

void append_string_to_list(LinkedList* list, const char* string);
void prepend_string_to_list(LinkedList* list, const char* string);
void insert_string_in_list(LinkedList* list, const char* string, size_t idx);
const char* get_string_from_list(LinkedList* list, size_t idx);

#endif
