#include <stdio.h>
#include <sys/time.h> 

void print_local_time(void)
{ 
	struct timeval start, end;
    gettimeofday( &start, NULL );
    printf("localtime : %d.%d\n", start.tv_sec, start.tv_usec);
}

int add(int i)
{
	return i;
}

static inline int inline_add(int i)
{
	return i;
}

int main(void) 
{

	printf("start.\n");
	print_local_time();

	int count = 100000000;
	int i = 0;
	int num = 0;
	
	for(i = 0; i < count; i++) {
		add(i);
	}
	print_local_time();

	for(i = 0; i < count; i++) {
		inline_add(i);
	}
	print_local_time();

	for(i = 0; i < count; i++) {
		num = i;
		num *= 2;
	}
	print_local_time();



	printf("over.\n");
	print_local_time();

	return 0;
}