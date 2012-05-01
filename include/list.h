/*
 * Simple double linked list and hash list implementation
 */
#ifndef __LIST_H
#define __LIST_H

#include "compile.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

/*
 * Double linked list
 */
/* list head */
struct list_head {
	struct list_head *prev, *next;
};

static _inline void list_init(struct list_head *head)
{
	head->prev = head->next = head;
}

static _inline void __list_add(struct list_head *list,
			struct list_head *prev, struct list_head *next)
{
	list->prev = prev;
	list->next = next;
	next->prev = list;
	prev->next = list;
}

static _inline void list_add(struct list_head *list, struct list_head *head)
{
	__list_add(list, head, head->next);
}

static _inline void list_add_tail(struct list_head *list, struct list_head *head)
{
	__list_add(list, head->prev, head);
}

static _inline void __list_del(struct list_head *prev, struct list_head *next)
{
	prev->next = next;
	next->prev = prev;
}

static _inline void list_del(struct list_head *list)
{
	__list_del(list->prev, list->next);
	list->prev = NULL;
	list->next = NULL;
}

static _inline void list_del_init(struct list_head *list)
{
	__list_del(list->prev, list->next);
	list->prev = list;
	list->next = list;
}

#define LIST_HEAD(name)\
	struct list_head name = { &name, &name };

#define list_empty(head) ((head) == (head)->next)

#define list_entry(ptr, type, member) containof(ptr, type, member)
#define list_first_entry(head, type, member)\
	list_entry((head)->next, type, member)
#define list_last_entry(head, type, member)\
	list_entry((head)->prev, type, member)

#define list_for_each_entry(entry, head, member)\
	for (entry = list_first_entry(head, typeof(*entry), member);\
		&entry->member != (head);\
		entry = list_first_entry(&entry->member, typeof(*entry), member))

#define list_for_each_entry_continue(entry, head, member)\
	for (; &entry->member != (head);\
		entry = list_first_entry(&entry->member, typeof(*entry), member))

#define list_for_each_entry_safe(entry, next, head, member)\
	for (entry = list_first_entry(head, typeof(*entry), member),\
		next = list_first_entry(&entry->member, typeof(*entry), member);\
		&entry->member != (head);\
		entry = next, next = list_first_entry(&next->member, typeof(*entry), member))

#define list_for_each_entry_safe_continue(entry, next, head, member)\
	for (next = list_first_entry(&entry->member, typeof(*entry), member);\
		&entry->member != (head);\
		entry = next, next = list_first_entry(&next->member, typeof(*entry), member))


#define list_for_each_entry_reverse(entry, head, member)\
	for (entry = list_last_entry(head, typeof(*entry), member);\
		&entry->member != (head);\
		entry = list_last_entry(&entry->member, typeof(*entry), member))

/*
 * Hash list
 */
/* hash list head */
struct hlist_head {
	struct hlist_node *first;
};

struct hlist_node {
	struct hlist_node *next;
	struct hlist_node **pprev;
};

static _inline int hlist_unhashed(struct hlist_node *node)
{
	return !node->pprev;
}

static _inline int hlist_empty(struct hlist_head *head)
{
	return !head->first;
}

static _inline void hlist_head_init(struct hlist_head *head)
{
	head->first = NULL;
}

static _inline void hlist_node_init(struct hlist_node *node)
{
	node->next = NULL;
	node->pprev = NULL;
}

static _inline void __hlist_del(struct hlist_node *n)
{
	*n->pprev = n->next;
	if (n->next)
		n->next->pprev = n->pprev;
}

static _inline void hlist_del(struct hlist_node *n)
{
	__hlist_del(n);
	n->next = NULL;
	n->pprev = NULL;
}

static _inline void hlist_add_head(struct hlist_node *n, struct hlist_head *h)
{
	n->next = h->first;
	n->pprev = &h->first;
	if (h->first)
		h->first->pprev = &n->next;
	h->first = n;
}

/* add @n before @next */
static _inline void hlist_add_before(struct hlist_node *n, struct hlist_node *next)
{
	n->next = next;
	n->pprev = next->pprev;
	*next->pprev = n;
	next->pprev = &n->next;
}

/* add @next after @n */
static _inline void hlist_add_after(struct hlist_node *n, struct hlist_node *next)
{
	next->next = n->next;
	next->pprev = &n->next;
	if (n->next)
		n->next->pprev = &next->next;
	n->next = next;
}

#define hlist_entry(ptr, type, member) list_entry(ptr, type, member)
/* non-node implementation */
#define hlist_for_each_entry2(entry, head, member)\
	for (entry = ((head)->first) ? hlist_entry((head)->first, typeof(*entry), member) : NULL;\
		entry;\
		entry = (entry->member.next) ? hlist_entry(entry->member.next, typeof(*entry), member) : NULL)

#define hlist_for_each_entry(entry, node, head, member)\
	for (node = (head)->first;\
		node && (entry = hlist_entry(node, typeof(*entry), member));\
		node = node->next)

#endif	/* list.h */
