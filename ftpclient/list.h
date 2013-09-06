#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

struct ListNode {
    struct ListNode *next;
    void *data;
};

struct List {
    struct ListNode *head, *tail;
    unsigned int size;
};

int add_to_list(struct List *list, void *data) {
    if (list == NULL) {
        return 1;
    }
    struct ListNode *n = malloc(sizeof(struct ListNode));
    if (n == NULL) {
        fprintf(stderr, "Malloc just failed, check that out\n");
        return 1;
    }
    n->data = data;
    n->next = NULL;
    if (list->head == NULL) {
        list->head = list->tail = n;
        list->size = 1;
    } else {
        list->tail->next = n;
        list->tail = n;
        list->size += 1;
    }
    return 0;
}

void init_list(struct List *list) {
    if (list == NULL) {
        return;
    }
    list->head = list->tail = NULL;
    list->size = 0;
}

void remove_front(struct List *list) {
    if (list == NULL || list->head == NULL) {
        return;
    }
    struct ListNode *f = list->head;
    list->head = f->next;
    free(f);
    list->size--;
}

#endif
