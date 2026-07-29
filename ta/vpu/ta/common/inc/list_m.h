#ifndef _VPU_LIST_H
#define _VPU_LIST_H

#include <sys/types.h>

struct list_head {
    struct list_head *next, *prev;
};

static inline void list_init_head(struct list_head *list) {
    list->next = list;
    list->prev = list;
}

static inline void list_add_entry(struct list_head *new,
                                  struct list_head *head) {
    struct list_head *prev = head;
    struct list_head *next = head->next;

    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_del_entry(struct list_head *entry) {
    struct list_head *prev = entry->prev;
    struct list_head *next = entry->next;

    next->prev = prev;
    prev->next = next;

    entry->next = entry;
    entry->prev = entry;
}

#define list_entry(ptr, type, member) container_of(ptr, type, member)

#define list_for_each(pos, head)                                               \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head)                                       \
    for (pos = (head)->next, n = pos->next; pos != (head);                     \
         pos = n, n = pos->next)

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
#define container_of(ptr, type, member)                                        \
    ({                                                                         \
        const typeof(((type *)0)->member) *__mptr = (ptr);                     \
        (type *)((char *)__mptr - offsetof(type, member));                     \
    })
#endif
#endif
