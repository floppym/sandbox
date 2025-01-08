AT_FILES = $(wildcard $(top_srcdir)/%D%/*.at)
DISTCLEANFILES += %D%/atconfig

# Use top_srcdir for dependencies, and abs_top_srcdir to execute it.
TESTSUITE = $(top_srcdir)/%D%/testsuite
ABS_TESTSUITE = $(abs_top_srcdir)/%D%/testsuite

EXTRA_DIST += \
	$(AT_FILES) \
	$(TESTSUITE) \
	$(TESTSUITE_LIST) \
	%D%/atlocal.in \
	%D%/package.m4.in \
	%D%/test-skel-0.c \
	%D%/tests.h \
	%D%/xattr-0

check_PROGRAMS += \
	%D%/get-group \
	%D%/get-user \
	%D%/sb_true \
	%D%/sb_true_static \
	\
	%D%/access-0 \
	%D%/chmod-0 \
	%D%/chown-0 \
	%D%/creat-0 \
	%D%/creat64-0 \
	%D%/execv-0 \
	%D%/execvp-0 \
	%D%/faccessat-0 \
	%D%/faccessat_static-0 \
	%D%/fchmod-0 \
	%D%/fchmodat-0 \
	%D%/fchown-0 \
	%D%/fchownat-0 \
	%D%/fopen-0 \
	%D%/fopen64-0 \
	%D%/futimesat-0 \
	%D%/lchown-0 \
	%D%/link-0 \
	%D%/linkat-0 \
	%D%/linkat_static-0 \
	%D%/lremovexattr-0 \
	%D%/lsetxattr-0 \
	%D%/lutimes-0 \
	%D%/mkdtemp-0 \
	%D%/mkdir-0 \
	%D%/mkdir_static-0 \
	%D%/mkdirat-0 \
	%D%/mkfifo-0 \
	%D%/mkfifoat-0 \
	%D%/mknod-0 \
	%D%/mknodat-0 \
	%D%/mkostemp-0 \
	%D%/mkostemp64-0 \
	%D%/mkostemps-0 \
	%D%/mkostemps64-0 \
	%D%/mkstemp-0 \
	%D%/mkstemp64-0 \
	%D%/mkstemps-0 \
	%D%/mkstemps64-0 \
	%D%/open-0 \
	%D%/open_static-0 \
	%D%/open64-0 \
	%D%/openat-0 \
	%D%/openat_static-0 \
	%D%/openat64-0 \
	%D%/opendir-0 \
	%D%/remove-0 \
	%D%/removexattr-0 \
	%D%/rename-0 \
	%D%/renameat-0 \
	%D%/renameat2-0 \
	%D%/rmdir-0 \
	%D%/setxattr-0 \
	%D%/signal_static-0 \
	%D%/symlink-0 \
	%D%/symlinkat-0 \
	%D%/truncate-0 \
	%D%/truncate64-0 \
	%D%/unlink-0 \
	%D%/unlink_static-0 \
	%D%/unlinkat-0 \
	%D%/utime-0 \
	%D%/utimensat-0 \
	%D%/utimensat64-0 \
	%D%/utimensat_static-0 \
	%D%/utimensat64_static-0 \
	%D%/utimes-0 \
	%D%/vfork-0 \
	\
	%D%/getcwd-gnulib_tst \
	%D%/libsigsegv_tst \
	%D%/malloc_hooked_tst \
	%D%/malloc_mmap_tst \
	%D%/pipe-fork_tst \
	%D%/pipe-fork_static_tst \
	%D%/sb_printf_tst \
	%D%/sigsuspend-zsh_tst \
	%D%/sigsuspend-zsh_static_tst \
	%D%/trace-memory_static_tst

dist_check_SCRIPTS += \
	$(wildcard $(top_srcdir)/%D%/*-[0-9]*.sh) \
	%D%/malloc-0 \
	%D%/script-0 \
	%D%/trace-0

# This will be used by all programs, not just tests/ ...
AM_LDFLAGS = `expr $@ : .*_static >/dev/null && echo -all-static`

%C%_get_group_CPPFLAGS = $(SIXTY_FOUR_FLAGS)
%C%_get_user_CPPFLAGS = $(SIXTY_FOUR_FLAGS)
%C%_trace_memory_static_tst_CPPFLAGS = $(SIXTY_FOUR_FLAGS)

%C%_sb_printf_tst_CFLAGS = -I$(top_srcdir)/libsbutil -I$(top_srcdir)/libsbutil/include
%C%_sb_printf_tst_LDADD = libsbutil/libsbutil.la

%C%_malloc_hooked_tst_LDFLAGS = $(AM_LDFLAGS) -pthread

%C%_libsigsegv_tst_CPPFLAGS = ${AM_CPPFLAGS}
if HAVE_LIBSIGSEGV
%C%_libsigsegv_tst_CPPFLAGS += -DHAVE_LIBSIGSEGV
%C%_libsigsegv_tst_LDADD = -lsigsegv
endif

TESTSUITEFLAGS = --jobs=`getconf _NPROCESSORS_ONLN || echo 1`

# Helper target for devs to precompile.
tests: $(check_PROGRAMS) $(TESTSUITE)

check-local: %D%/atconfig %D%/atlocal $(TESTSUITE)
	cd %D% && $(SHELL) '$(ABS_TESTSUITE)' AUTOTEST_PATH='src:tests' $(TESTSUITEFLAGS)

installcheck-local: %D%/atconfig %D%/atlocal $(TESTSUITE)
	cd %D% && $(SHELL) '$(ABS_TESTSUITE)' AUTOTEST_PATH='src:tests:$(bindir)' $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)' || { cd %D% && $(SHELL) '$(ABS_TESTSUITE)' --clean; }

TESTSUITE_LIST = $(top_srcdir)/%D%/testsuite.list
AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): $(AT_FILES) $(TESTSUITE_LIST)
	@$(MKDIR_P) '$(top_srcdir)/%D%'
	$(AM_V_GEN)$(AUTOTEST) -I'%D%' -I'$(top_srcdir)/%D%' -o '$@.tmp' '$@.at'
	$(AM_V_at)mv '$@.tmp' '$@'

$(TESTSUITE_LIST): $(AT_FILES)
	@$(MKDIR_P) $(top_srcdir)/%D%
	$(AM_V_GEN)( echo "dnl DO NOT EDIT: GENERATED BY MAKEFILE.AM"; \
	$(GREP) -l -e '^SB_CHECK' -e '^AT_CHECK' $(AT_FILES) | LC_ALL=C sort | \
		$(SED) -e 's:^.*/%D%/:sb_inc([:' -e 's:[.]at$$:]):' ) > $@
