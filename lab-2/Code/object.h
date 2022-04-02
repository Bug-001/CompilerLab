#ifndef OBJECT_H
#define OBJECT_H

#include "rbtree.h"

struct object {
    struct rb_node node;
	char* id;
	int ref_count;
};

/* id must NOT be NULL */
void* __alloc_object(int _size, const char* id);

inline void* __get_object(void* _obj) {
    ((struct object*) _obj)->ref_count += 1;
    return _obj;
}

void put_object(void* _obj);

void* search_object(struct rb_root* root, const char* str);

bool insert_object(struct rb_root* root, void* _obj);

inline void erase_object(void* del, struct rb_root* root) {
    struct object* obj = (struct object*) del;
    rb_erase(&obj->node, root);
}

#define INIT_OBJECT(type, name, id)     \
    type* name = (type*)__alloc_object(sizeof(type), id)

#define alloc_object(type, id)          \
    (type*)__alloc_object(sizeof(type), id)

#define get_object(type, ref)           \
    (type*)__get_object(ref)

#endif