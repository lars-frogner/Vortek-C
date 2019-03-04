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
    Link* iterator;
} LinkedList;

void print_list_content_as_strings(const LinkedList* list);

LinkedList create_list(void);
ListItem append_new_list_item(LinkedList* list, size_t item_size);
ListItem prepend_new_list_item(LinkedList* list, size_t item_size);
ListItem insert_new_list_item(LinkedList* list, size_t item_size, size_t idx);

size_t get_list_length(const LinkedList* list);
ListItem get_list_item(const LinkedList* list, size_t idx);
ListItem get_first_list_item(const LinkedList* list);
ListItem get_last_list_item(const LinkedList* list);

void join_lists(LinkedList* list, LinkedList* other_list);

void remove_list_item(LinkedList* list, size_t idx);
void remove_first_list_item(LinkedList* list);
void remove_last_list_item(LinkedList* list);
void clear_list(LinkedList* list);

void reset_list_iterator(LinkedList* list);
int valid_list_iterator(const LinkedList* list);
void advance_list_iterator(LinkedList* list);

void append_int_to_list(LinkedList* list, int value);
void append_unsigned_int_to_list(LinkedList* list, unsigned int value);
void append_size_t_to_list(LinkedList* list, size_t value);
void append_float_to_list(LinkedList* list, float value);
void append_double_to_list(LinkedList* list, double value);
void append_string_to_list(LinkedList* list, const char* string, ...);

void prepend_int_to_list(LinkedList* list, int value);
void prepend_unsigned_int_to_list(LinkedList* list, unsigned int value);
void prepend_size_t_to_list(LinkedList* list, size_t value);
void prepend_float_to_list(LinkedList* list, float value);
void prepend_double_to_list(LinkedList* list, double value);
void prepend_string_to_list(LinkedList* list, const char* string, ...);

void insert_int_in_list(LinkedList* list, size_t idx, int value);
void insert_unsigned_int_in_list(LinkedList* list, size_t idx, unsigned int value);
void insert_size_t_in_list(LinkedList* list, size_t idx, size_t value);
void insert_float_in_list(LinkedList* list, size_t idx, float value);
void insert_double_in_list(LinkedList* list, size_t idx, double value);
void insert_string_in_list(LinkedList* list, size_t idx, const char* string, ...);

int get_int_from_list(const LinkedList* list, size_t idx);
unsigned int get_unsigned_int_from_list(const LinkedList* list, size_t idx);
size_t get_size_t_from_list(const LinkedList* list, size_t idx);
float get_float_from_list(const LinkedList* list, size_t idx);
double get_double_from_list(const LinkedList* list, size_t idx);
const char* get_string_from_list(const LinkedList* list, size_t idx);

ListItem get_current_list_item(const LinkedList* list);
int get_current_int_from_list(const LinkedList* list);
unsigned int get_current_unsigned_int_from_list(const LinkedList* list);
size_t get_current_size_t_from_list(const LinkedList* list);
float get_current_float_from_list(const LinkedList* list);
double get_current_double_from_list(const LinkedList* list);
const char* get_current_string_from_list(const LinkedList* list);

#endif
