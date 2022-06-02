#include<stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int add_int(int, int);
int add_int(int num1, int num2) {
	return num1+num2;
}

#ifdef __cplusplus
}
#endif

