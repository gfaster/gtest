#ifndef GTEST_HEADER
#define GTEST_HEADER

#ifndef _GNU_SOURCE
#error "gtest requires _GNU_SOURCE"
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>

#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/wait.h>

// #define CAT(x, y) CAT_(x, y)
// #define CAT_(x, y) x ## y

/* "✓" character */
#define PASS_STR "\033[32m\u2713 PASS\033[0m"

/* "✗" character */
#define FAIL_STR "\033[1;31m\u2717 FAIL\033[0m"

#define IGN_STR "\033[1;90mIGNORE\033[0m"


#define gassert(test) do {                      \
	if (test) {                             \
	} else {                                \
		errx(1, "\t \033[1;31mAssert failed\033[0m: \"%s\"", #test);\
	}                                       \
} while (0)

#define gtest(name) \
for (int gtest__igtest = (!gtest_ignore) ? gtest__inner(name) : gtest__ignore(name); gtest__igtest == 0; exit(0))

struct gtest__failed_test {
	int outfd;
	int errfd;
	char *name;
};

static int gtest_ignore = 0;

static unsigned int gtest__passcnt = 0;
static unsigned int gtest__igncnt = 0;
static unsigned int gtest__failcnt = 0;

static struct gtest__failed_test *gtest__failures = NULL;
static unsigned int gtest__failures_cap = 0;

static void
gtest__handle_fail(int outfd, int errfd, char *name)
{
	if (gtest__failures_cap == gtest__failcnt) {
		if (gtest__failures_cap) {
			gtest__failures_cap *= 2;
		} else {
			gtest__failures_cap = 8;
		}
		gtest__failures = realloc(gtest__failures,
			     gtest__failures_cap * 
			     sizeof(struct gtest__failed_test));
	}
	gtest__failures[gtest__failcnt] = (struct gtest__failed_test) {
		.outfd = outfd,
		.errfd = errfd,
		.name = name
	};
	printf(FAIL_STR " %s\n", name);
	gtest__failcnt += 1;
}

static int
gtest__ignore(char *name) 
{
	printf(IGN_STR " %s\n", name);
	gtest__igncnt += 1;
	gtest_ignore = 0;
	return 1;
}

static int
gtest__inner(char* name)
{
	int ingtest;
	int outfd = memfd_create("test_stdout", 0);
	int errfd = memfd_create("test_stderr", 0);
	if (outfd == -1 || errfd == -1) {
		err(1, "could not create files for stdout and stderr");
	}
	fflush(NULL); // needed so children don't save stdout
	int pid = fork();
	if (pid == 0) {
		/* child */
		ingtest = 0;
		dup2(outfd, STDOUT_FILENO);
		dup2(errfd, STDERR_FILENO);
	} else if (pid == -1) {
		warn("fork failed");
		gtest__ignore(name);
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
				gtest__passcnt += 1;
				close(outfd);
				close(errfd);
			} else {
				gtest__handle_fail(outfd, errfd, name);
			}
		} else {
			gtest__handle_fail(outfd, errfd, name);
		}
	}
	return ingtest;
}


static int
gtest_prntres(void)
{
	unsigned int i;
	ssize_t e;
	struct gtest__failed_test fail;
	char buf[4096];
	

	printf("\n%u tests were run:\n", gtest__failcnt + gtest__passcnt);
	printf(PASS_STR ": %u\n", gtest__passcnt);
	if (gtest__failcnt > 0) {
		printf(FAIL_STR ": %u\n", gtest__failcnt);
	}
	if (gtest__igncnt > 0) {
		printf(IGN_STR ": %u\n", gtest__igncnt);
	}
	i = 0;
	for (; i < gtest__failcnt; i++) {
		fail = gtest__failures[i];
		printf("\n\033[1m--------- stdout of %s ---------\033[0m\n", fail.name);
		fflush(NULL);
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
	if (gtest__failcnt) {
		free(gtest__failures);
	}
	return gtest__failcnt;
}

static int 
gtest_issuccess(void)
{
	return gtest__failcnt == 0 && gtest__passcnt > 0;
}


#endif
