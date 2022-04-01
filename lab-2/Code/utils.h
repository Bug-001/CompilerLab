#ifndef UTILS_H
#define UTILS_H

#define container_of(ptr, type, member) ({				\
	void *__mptr = (void *)(ptr);					\
	((type *)(__mptr - (void *)&((type *)0)->member)); })


#endif