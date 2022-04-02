#ifndef SEMANTIC_H
#define SEMANTIC_H

struct node;

struct value {
  union {
    int i_val;
    float f_val;
  };
};

enum relop_type {
  GT,
  LT,
  GE,
  LE,
  EQ,
  NE
};

struct lexical_attr {
  char* info;
  union {
    struct value value;
    enum relop_type relop_type;
  };
};

struct syntax_attr {
  int dummy;
};

void semantic_analysis(struct node* tree);

#endif /* SEMANTIC_H */