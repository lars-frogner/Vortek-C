#include "linked_list.h"

#include "error.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


typedef struct Link
{
    ListItem item;
    Link* previous;
    Link* next;
} Link;


static Link* get_link(const LinkedList* list, size_t idx);
static Link* get_link_from_start(const LinkedList* list, size_t idx);
static Link* get_link_from_end(const LinkedList* list, size_t idx);
static Link* allocate_link(size_t item_size);
static void free_link(Link* link);
static void free_next_links(Link* start_link);


void print_list_content_as_strings(const LinkedList* list)
{
    check(list);

    if (list->length == 0)
    {
        printf("[]\n");
        return;
    }

    Link* link = list->start;

    printf("[\"%s\"", (char*)link->item.data);

    size_t idx;
    for (idx = 1; idx < list->length; idx++)
    {
        link = link->next;
        printf(", \"%s\"", (char*)link->item.data);
    }

    printf("]\n");
}

LinkedList create_linked_list(void)
{
    LinkedList list;
    list.start = NULL;
    list.end = NULL;
    list.length = 0;
    return list;
}

ListItem append_new_list_item(LinkedList* list, size_t item_size)
{
    check(list);
    assert(item_size > 0);

    if (list->end)
    {
        list->end->next = allocate_link(item_size);
        list->end->next->previous = list->end;
        list->end = list->end->next;
    }
    else
    {
        list->start = allocate_link(item_size);
        list->end = list->start;
    }

    list->length++;

    return list->end->item;
}

ListItem prepend_new_list_item(LinkedList* list, size_t item_size)
{
    check(list);
    assert(item_size > 0);

    if (list->start)
    {
        list->start->previous = allocate_link(item_size);
        list->start->previous->next = list->start;
        list->start = list->start->previous;
    }
    else
    {
        list->start = allocate_link(item_size);
        list->end = list->start;
    }

    list->length++;

    return list->start->item;
}

ListItem insert_new_list_item(LinkedList* list, size_t item_size, size_t idx)
{
    check(list);
    check(idx <= list->length);
    assert(item_size > 0);

    if (idx == 0)
    {
        return prepend_new_list_item(list, item_size);
    }
    else if (idx == list->length)
    {
        return append_new_list_item(list, item_size);
    }
    else
    {
        Link* link_after = get_link(list, idx);
        Link* link_before = link_after->previous;

        link_after->previous = allocate_link(item_size);
        link_after->previous->next = link_after;
        link_after->previous->previous = link_before;
        link_before->next = link_after->previous;

        list->length++;

        return link_after->previous->item;
    }
}

size_t get_list_length(const LinkedList* list)
{
    check(list);
    return list->length;
}

ListItem get_list_item(const LinkedList* list, size_t idx)
{
    check(list);
    check(idx < list->length);
    if (idx == 0)
        return get_first_list_item(list);
    else if (idx == list->length - 1)
        return get_last_list_item(list);
    else
        return get_link(list, idx)->item;
}

ListItem get_first_list_item(const LinkedList* list)
{
    check(list);
    check(list->length > 0);
    return list->start->item;
}

ListItem get_last_list_item(const LinkedList* list)
{
    check(list);
    check(list->length > 0);
    return list->end->item;
}

void join_lists(LinkedList* list, LinkedList* other_list)
{
    list->end->next = other_list->start;
    other_list->start->previous = list->end;
    list->end = other_list->end;
    list->length += other_list->length;

    other_list->start = NULL;
    other_list->end = NULL;
    other_list->length = 0;
}

void remove_list_item(LinkedList* list, size_t idx)
{
    check(list);
    check(idx < list->length);

    Link* link = NULL;

    if (idx == 0)
    {
        link = list->start;
        list->start = list->start->next;

        if (list->start)
            list->start->previous = NULL;
        else
            list->end = NULL;
    }
    else if (idx == list->length - 1)
    {
        link = list->end;
        list->end = list->end->previous;

        if (list->end)
            list->end->next = NULL;
        else
            list->start = NULL;
    }
    else
    {
        link = get_link(list, idx);
        link->previous->next = link->next;
        link->next->previous = link->previous;
    }

    free_link(link);

    list->length--;
}

void remove_first_list_item(LinkedList* list)
{
    check(list);
    check(list->length > 0);
    remove_list_item(list, 0);
}

void remove_last_list_item(LinkedList* list)
{
    check(list);
    check(list->length > 0);
    remove_list_item(list, list->length-1);
}

void clear_list(LinkedList* list)
{
    check(list);

    free_next_links(list->start);
    list->start = NULL;
    list->end = NULL;
    list->length = 0;
}

void append_string_to_list(LinkedList* list, const char* string)
{
    check(string);
    const size_t item_size = sizeof(char)*(strlen(string) + 1);
    ListItem item = append_new_list_item(list, item_size);
    strcpy((char*)item.data, string);
}

void prepend_string_to_list(LinkedList* list, const char* string)
{
    check(string);
    const size_t item_size = sizeof(char)*(strlen(string) + 1);
    ListItem item = prepend_new_list_item(list, item_size);
    strcpy((char*)item.data, string);
}

void insert_string_in_list(LinkedList* list, const char* string, size_t idx)
{
    check(string);
    const size_t item_size = sizeof(char)*(strlen(string) + 1);
    ListItem item = insert_new_list_item(list, item_size, idx);
    strcpy((char*)item.data, string);
}

const char* get_string_from_list(LinkedList* list, size_t idx)
{
    ListItem item = get_list_item(list, idx);
    return (const char*)item.data;
}

static Link* get_link(const LinkedList* list, size_t idx)
{
    assert(list);
    assert(idx < list->length);
    return (idx <= list->length/2) ? get_link_from_start(list, idx) : get_link_from_end(list, idx);
}

static Link* get_link_from_start(const LinkedList* list, size_t idx)
{
    Link* link = list->start;

    size_t i;
    for (i = 0; i  < idx; i++)
        link = link->next;

    return link;
}

static Link* get_link_from_end(const LinkedList* list, size_t idx)
{
    Link* link = list->end;

    size_t i;
    for (i = list->length - 1; i > idx; i--)
        link = link->previous;

    return link;
}

static Link* allocate_link(size_t item_size)
{
    Link* link = (Link*)malloc(sizeof(Link));
    link->item.data = malloc(item_size);
    link->item.size = item_size;
    link->previous = NULL;
    link->next = NULL;
    return link;
}

static void free_link(Link* link)
{
    if (!link)
        return;

    if (link->item.data)
        free(link->item.data);

    free(link);
}

static void free_next_links(Link* start_link)
{
    if (!start_link)
        return;

    Link* next_link = start_link->next;
    free_link(start_link);
    free_next_links(next_link);
}
