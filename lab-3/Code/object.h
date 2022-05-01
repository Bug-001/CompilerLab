#ifndef OBJECT_H
#define OBJECT_H

#include "rbtree.h"

struct object {
	struct rb_node node;
	char *id;
};

/* id must NOT be NULL */
void *__alloc_object(int _size, const char *id);

void free_object(void *_obj);

void *search_object(struct rb_root *root, const char *str);

bool insert_object(struct rb_root *root, void *_obj);

void erase_object(void *del, struct rb_root *root);

#define INIT_OBJECT(type, name, id)                                            \
	type *name = (type *)__alloc_object(sizeof(type), id)

#define alloc_object(type, id) (type *)__alloc_object(sizeof(type), id)

#endif