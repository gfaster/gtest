#define _GNU_SOURCE
#include "gtest.h"

int 
main(void)
{
	gtest("Test that will pass") {
		gassert(1 + 1 == 2);
	}

	gtest("Test that will fail") {
		gassert(1 + 1 == 3);
	}

	gtest_prntres();

	return !gtest_issuccess();
}
