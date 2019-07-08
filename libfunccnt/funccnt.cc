#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "libfunccnt.h"

using namespace std;

void funcc_count_pre(unsigned int func)
{
	fprintf(stdout, "pre %u\n", func);
}

void funcc_count_post(unsigned int func)
{
	fprintf(stdout, "post %u\n", func);
}
