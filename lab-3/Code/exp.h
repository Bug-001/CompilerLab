#ifndef EXP_H
#define EXP_H

#include "object.h"
#include "rbtree.h"

enum exp_type {
	EXP_INVALID = 0,
	EXP_SYMBOL,
	EXP_FIELD_ACCESS,
	EXP_ARRAY_ACCESS,
	EXP_ARITHMETIC
};

enum value_type { VALUE_UNDEF = 0, VALUE_INT, VALUE_FLOAT, VALUE_NAC };

enum relop_type { GT, LT, GE, LE, EQ, NE };

struct value {
	union {
		int i_val;
		float f_val;
	};
};

// struct field_access {
//   struct type* parent_type;
//   struct type* type;
//   struct var_list* field;
// };

// struct array_access {
//   struct type* parent_type;
//   struct type* type;
//   int index;
// };

/* Inherited from struct object */
struct symbol {
	struct object obj;
	struct type *type;
	enum value_type val_kind;
	struct value val;
};

// struct arithmetic {
//   struct type* type;
//   bool value_is_avail;
//   struct value val;
// };

struct exp_attr {
	enum exp_type kind;
	struct type *type;
	struct node *parent;
	enum value_type value_kind;
	struct value val;
	union {
		/* for symbol */
		struct symbol *sym;
		/* for field access */
		struct var_list *field;
		/* for array access */
		int index;
	};
};

struct symbol *search_symbol(const char *id);

struct symbol *search_symbol_within_compst(const char *id);

struct symbol *insert_new_symbol(const char *id, struct type *type);

void push_symbol_table();

void pop_symbol_table();

#endif