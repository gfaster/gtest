## gtest
gtest is a simple, single-header C testing library for GNU/Linux.

gtest runs each test in separate child process, so bugs that cause segmentation
faults do not interrupt testing. It also does not print test stdout and stderr
unless a test failed. This means that log outputs do not clutter output.

## Example
This example code can be found in [`example.c`](./example.c) and can be run
simply with `gcc example.c -o example && ./example`.

```c 
#define _GNU_SOURCE
#include "gtest.h"

int 
main(void)
{
	gtest("Test that will pass") {
		gassert(1 + 1 == 2);
	}

	gtest_ignore = 1;
	gtest("Test that is ignored") {
		gassert(0);
	}

	gtest("Test that will fail") {
		gassert(1 + 1 == 3);
	}

	gtest_prntres();

	return !gtest_issuccess();
}
```

The output of this test (minus the fancy colors) is:

```
✓ PASS Test that will pass
IGNORE Test that is ignored
✗ FAIL Test that will fail

2 tests were run:
✓ PASS: 1
✗ FAIL: 1
IGNORE: 1

--------- stdout of Test that will fail ---------

--------- stderr of Test that will fail ---------
example: 	 Assert failed: "1 + 1 == 3"
```
