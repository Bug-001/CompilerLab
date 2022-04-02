#ifndef OBJECT_H
#define OBJECT_H

#include "rbtree.h"

struct object {
    struct rb_node rb_node;
	const char* name;
	int ref_count;
};

#endif