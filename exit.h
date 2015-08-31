#ifndef EXIT_H_
#define EXIT_H_

#include <stdlib.h>
#include <stdio.h>

#define EXIT(x)					\
    do {					\
    perror(x);					\
    exit(EXIT_FAILURE);				\
    } while (0);

#endif /* EXIT_H_ */
