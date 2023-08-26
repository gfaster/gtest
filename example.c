#define _GNU_SOURCE
#include "gavtest.h"

int 
main(void)
{
	gtest("Test one") {
		gassert(1);
	}

	gtest("Test two") {
		gassert(1 + 1 == 2);
	}

	gtest_prntres();

	return !gtest_issuccess();
}
