#include "object.h"
#include "rbtree.h"
#include "stdlib.h"
#include "string.h"
#include "utils.h"

void* __alloc_object(int _size, const char* id) {
    if (id == NULL)
        return NULL;
    struct object* obj = (struct object*) malloc(_size);
    memset(obj, 0, _size);
    obj->id = (char*) malloc(strlen(id) + 1);
    strcpy(obj->id, id);

    return obj;
}

void free_object(void* _obj) {
    struct object* obj = (struct object*) _obj;
    free(obj->id);
    free(obj);
}

void* search_object(struct rb_root* root, const char* str) {
    struct rb_node* node = root->rb_node;
    while (node) {
        struct object* obj = container_of(node, struct object, node);
        int result = strcmp(str, obj->id);
        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return obj;
    }
    return NULL;
}

bool insert_object(struct rb_root* root, void* _obj) {
    struct object* obj = (struct object*) _obj;
    struct rb_node** new = &(root->rb_node), *parent = NULL;

    while (*new) {
        struct object* this = container_of(*new, struct object, node);
        int result = strcmp(obj->id, this->id);

        parent = *new;
        if (result < 0) {
            new = &((*new)->rb_left);
        } else if (result > 0) {
            new = &((*new)->rb_right);
        } else {
            return false;
        }
    }

    rb_link_node(&obj->node, parent, new);
    rb_insert_color(&obj->node, root);

    return true;
}

void erase_object(void* del, struct rb_root* root) {
    struct object* obj = (struct object*) del;
    rb_erase(&obj->node, root);
}

/* To test rbtree code with following code, build with
 * `gcc -o a.out -fsanitize=address -fno-omit-frame-pointer ./object.c ./rbtree.c`
 * and then run `./a.out`
 */

// #include <assert.h>
// #include <stdio.h>

// struct derived {
//     struct object obj;
//     int data;
// };

// int main() {
//     struct rb_root mytree = RB_ROOT;
    
//     struct derived* arr[128];
//     char name[129] = {0};
//     for (int i = 0; i < 128; ++i) {
//         name[i] = '1';
//         arr[i] = alloc_object(struct derived, name);
//         assert(insert_object(&mytree, arr[i]) == true);
//     }

//     INIT_OBJECT(struct derived, d, "special");
//     struct derived ref = d;

//     assert(insert_object(&mytree, d) == true);
//     assert(insert_object(&mytree, ref) == false);

//     assert(search_object(&mytree, "11111111"));
//     assert(search_object(&mytree, "111111112") == NULL);
//     printf("%p", search_object(&mytree, "1111111111"));

//     for (int i = 0; i < 128; ++i) {
//         free_object(arr[i]);
//     }
// }