#ifndef GAVTEST
#define GAVTEST

#ifndef _GNU_SOURCE
#error "test library requires _GNU_SOURCE"
#endif

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <err.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/sendfile.h>

// #define CAT(x, y) CAT_(x, y)
// #define CAT_(x, y) x ## y

/* "✓" character */
#define PASS_STR "\033[32m\u2713 PASS\033[0m"

/* "✗" character */
#define FAIL_STR "\033[1;31m\u2717 FAIL\033[0m"


#define gassert(test) do {                      \
	if (test) {                             \
	} else {                                \
		err(1, "\t \033[1;31mAssert failed\033[0m: \"%s\"", #test);\
	}                                       \
} while (0)

#define gtest(name) for (int igtest = gtest_inner(name); igtest == 0; exit(0))

struct gavtest_failed_test {
	int outfd;
	int errfd;
	char *name;
};

static unsigned int gavtest_passcnt = 0;
static unsigned int gavtest_failcnt = 0;

static struct gavtest_failed_test *gavtest_failures = NULL;
static unsigned int gavtest_failures_cap = 0;

static void
gtest_handle_fail(int outfd, int errfd, char *name)
{
	if (gavtest_failures_cap == gavtest_failcnt) {
		if (gavtest_failures_cap) {
			gavtest_failures_cap *= 2;
		} else {
			gavtest_failures_cap = 8;
		}
		gavtest_failures = realloc(gavtest_failures,
			     gavtest_failures_cap * 
			     sizeof(struct gavtest_failed_test));
	}
	gavtest_failures[gavtest_failcnt] = (struct gavtest_failed_test) {
		.outfd = outfd,
		.errfd = errfd,
		.name = name
	};
	printf(FAIL_STR " %s\n", name);
	gavtest_failcnt += 1;
}

static int
gtest_inner(char* name)
{
	int ingtest;
	int outfd = memfd_create("test_stdout", 0);
	int errfd = memfd_create("test_stderr", 0);
	int pid = fork();
	if (outfd == -1 || errfd == -1) {
		err(1, "could not create files for stdout and stderr");
	}
	if (pid == 0) {
		/* child */
		ingtest = 0;
		dup2(outfd, STDOUT_FILENO);
		dup2(errfd, STDERR_FILENO);
	} else if (pid == -1) {
		warn(FAIL_STR " Test %s failed to run: fork returned -1",
			name);
		ingtest = 1;
	} else {
		/* parent */
		ingtest = 1;
		int wstatus = 0;
		do {
			waitpid(pid, &wstatus, 0);
		} while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
		if (WIFEXITED(wstatus)) {
			if (WEXITSTATUS(wstatus) == 0) {
				printf(PASS_STR " %s\n", name);
				gavtest_passcnt += 1;
				close(outfd);
				close(errfd);
			} else {
				gtest_handle_fail(outfd, errfd, name);
			}
		} else {
			gtest_handle_fail(outfd, errfd, name);
		}
	}
	return ingtest;
}

static int
gtest_prntres(void)
{
	unsigned int i;
	ssize_t e;
	off_t offset;
	struct gavtest_failed_test fail;
	char buf[4096];
	

	printf("\n\n");
	printf("%u tests were run:\n", gavtest_failcnt + gavtest_passcnt);
	printf(PASS_STR ": %u\n", gavtest_passcnt);
	if (gavtest_failcnt > 0) {
		printf(FAIL_STR ": %u\n", gavtest_failcnt);
	}
	i = 0;
	for (; i < gavtest_failcnt; i++) {
		fail = gavtest_failures[i];
		printf("\n\n\n\033[1m--------- stdout of %s ---------\033[0m\n", fail.name);
		fflush(NULL);
		offset = 0;
		lseek(fail.outfd, 0, SEEK_SET);
		while ((e = read(fail.outfd, buf, 4096))) {
			if (e == -1) {
				err(1, "reading test stdout failed");
			}
			if (write(STDOUT_FILENO, buf, e) != e) {
				err(1, "incomplete write");
			}
		}
		if (close(fail.outfd)) {
			warn("could not close test stdout fd %i", fail.outfd);
		}

		printf("\n\033[1m--------- stderr of %s ---------\033[0m\n", fail.name);
		fflush(NULL);

		lseek(fail.errfd, 0, SEEK_SET);
		while ((e = read(fail.errfd, buf, 4096))) {
			if (e == -1) {
				err(1, "reading test stderr failed");
			}
			if (write(STDERR_FILENO, buf, e) != e) {
				err(1, "incomplete write");
			}
		}
		if (close(fail.errfd)) {
			warn("could not close test stderr fd %i", fail.errfd);
		}
	}
	if (gavtest_failcnt) {
		free(gavtest_failures);
	}
	return gavtest_failcnt;
}

static int 
gtest_issuccess(void)
{
	return gavtest_failcnt == 0 && gavtest_passcnt > 0;
}


#endif
