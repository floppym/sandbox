/*
 * sandbox.c
 *
 * Main sandbox related functions.
 *
 * Copyright 1999-2006 Gentoo Foundation
 *
 *
 *      This program is free software; you can redistribute it and/or modify it
 *      under the terms of the GNU General Public License as published by the
 *      Free Software Foundation version 2 of the License.
 *
 *      This program is distributed in the hope that it will be useful, but
 *      WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along
 *      with this program; if not, write to the Free Software Foundation, Inc.,
 *      675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Some parts might have Copyright:
 *
 *   Copyright (C) 2002 Brad House <brad@mainstreetsoftworks.com>
 *
 * $Header$
 */


#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include "sbutil.h"
#include "sandbox.h"

static int print_debug = 0;

volatile static int stop_called = 0;
volatile static pid_t child_pid = 0;

static char log_domain[] = "sandbox";

int setup_sandbox(struct sandbox_info_t *sandbox_info, bool interactive)
{
	if (NULL != getenv(ENV_PORTAGE_TMPDIR)) {
		/* Portage handle setting SANDBOX_WRITE itself. */
		sandbox_info->work_dir[0] = '\0';
	} else {
		if (NULL == getcwd(sandbox_info->work_dir, SB_PATH_MAX)) {
			perror("sandbox:  Failed to get current directory");
			return -1;
		}
		if (interactive)
			setenv(ENV_SANDBOX_WORKDIR, sandbox_info->work_dir, 1);
	}
	
	/* Do not resolve symlinks, etc .. libsandbox will handle that. */
	if (!rc_is_dir(VAR_TMPDIR, TRUE)) {
		perror("sandbox:  Failed to get var_tmp_dir");
		return -1;
	}
	snprintf(sandbox_info->var_tmp_dir, SB_PATH_MAX, "%s", VAR_TMPDIR);

	if (-1 == get_tmp_dir(sandbox_info->tmp_dir)) {
		perror("sandbox:  Failed to get tmp_dir");
		return -1;
	}
	setenv(ENV_TMPDIR, sandbox_info->tmp_dir, 1);

	sandbox_info->home_dir = getenv("HOME");
	if (!sandbox_info->home_dir) {
		sandbox_info->home_dir = sandbox_info->tmp_dir;
		setenv("HOME", sandbox_info->home_dir, 1);
	}

	/* Generate sandbox lib path */
	get_sandbox_lib(sandbox_info->sandbox_lib);

	/* Generate sandbox bashrc path */
	get_sandbox_rc(sandbox_info->sandbox_rc);

	/* Generate sandbox log full path */
	get_sandbox_log(sandbox_info->sandbox_log);
	if (rc_file_exists(sandbox_info->sandbox_log)) {
		if (-1 == unlink(sandbox_info->sandbox_log)) {
			perror("sandbox:  Could not unlink old log file");
			return -1;
		}
	}

	/* Generate sandbox debug log full path */
	get_sandbox_debug_log(sandbox_info->sandbox_debug_log);
	if (rc_file_exists(sandbox_info->sandbox_debug_log)) {
		if (-1 == unlink(sandbox_info->sandbox_debug_log)) {
			perror("sandbox:  Could not unlink old debug log file");
			return -1;
		}
	}

	return 0;
}

int print_sandbox_log(char *sandbox_log)
{
	int sandbox_log_file = -1;
	char *beep_count_env = NULL;
	int i, color, beep_count = 0;
	off_t len = 0;
	char *buffer = NULL;

	if (!rc_is_file(sandbox_log, FALSE)) {
		perror("sandbox:  Log file is not a regular file");
		return 0;
	}
	
	len = rc_get_size(sandbox_log, TRUE);
	if (0 == len)
		return 0;

	sandbox_log_file = sb_open(sandbox_log, O_RDONLY, 0);
	if (-1 == sandbox_log_file) {
		perror("sandbox:  Could not open Log file");
		return 0;
	}

	buffer = (char *)xmalloc((len + 1) * sizeof(char));
	if (NULL == buffer) {
		perror("sandbox:  Could not allocate buffer for Log file");
		return 0;
	}
	memset(buffer, 0, len + 1);
	if (-1 == sb_read(sandbox_log_file, buffer, len)) {
		perror("sandbox:  Could read Log file");
		return 0;
	}
	sb_close(sandbox_log_file);

	color = ((is_env_on(ENV_NOCOLOR)) ? 0 : 1);

	SB_EERROR(color,
	       "--------------------------- ACCESS VIOLATION SUMMARY ---------------------------",
	       "\n");
	SB_EERROR(color, "LOG FILE = \"%s\"", "\n\n", sandbox_log);
	fprintf(stderr, "%s", buffer);
	free(buffer);
	SB_EERROR(color,
	       "--------------------------------------------------------------------------------",
	       "\n");

	beep_count_env = getenv(ENV_SANDBOX_BEEP);
	if (beep_count_env)
		beep_count = atoi(beep_count_env);
	else
		beep_count = DEFAULT_BEEP_COUNT;

	for (i = 0; i < beep_count; i++) {
		fputc('\a', stderr);
		if (i < beep_count - 1)
			sleep(1);
	}
	
	return 1;
}

void stop(int signum)
{
	if (0 == stop_called) {
		stop_called = 1;
		printf("sandbox:  Caught signal %d in pid %d\n",
		       signum, getpid());
	} else {
		fprintf(stderr,
			"sandbox:  Signal already caught and busy still cleaning up!\n");
	}
}

void usr1_handler(int signum, siginfo_t *siginfo, void *ucontext)
{
	if (0 == stop_called) {
		stop_called = 1;
		printf("sandbox:  Caught signal %d in pid %d\n",
		       signum, getpid());

		/* FIXME: This is really bad form, as we should kill the whole process
		 *        tree, but currently that is too much work and not worth the
		 *        effort.  Thus we only kill the calling process and our child
		 *        for now.
		 */		
		if (siginfo->si_pid > 0)
			kill(siginfo->si_pid, SIGKILL);
		kill(child_pid, SIGKILL);
	} else {
		fprintf(stderr,
			"sandbox:  Signal already caught and busy still cleaning up!\n");
	}
}

int spawn_shell(char *argv_bash[], char **env, int debug)
{
	int status = 0;
	int ret = 0;

	child_pid = fork();

	/* Child's process */
	if (0 == child_pid) {
		execve(argv_bash[0], argv_bash, env);
		_exit(EXIT_FAILURE);
	} else if (child_pid < 0) {
		if (debug)
			fprintf(stderr, "Process failed to spawn!\n");
		return 0;
	}

	/* fork() creates a copy of this, so no need to use more memory than
	 * absolutely needed. */
	str_list_free(argv_bash);
	str_list_free(env);

	ret = waitpid(child_pid, &status, 0);
	if ((-1 == ret) || (status > 0)) {
		if (debug)
			fprintf(stderr, "Process returned with failed exit status!\n");
		return 0;
	}

	return 1;
}

int main(int argc, char **argv)
{
	struct sigaction act_new;
    
	int success = 1;
	int sandbox_log_presence = 0;

	struct sandbox_info_t sandbox_info;

	char **sandbox_environ;
	char **argv_bash = NULL;

	char *run_str = "-c";

	rc_log_domain(log_domain);

	/* Only print info if called with no arguments .... */
	if (argc < 2)
		print_debug = 1;

	if (print_debug)
		printf("========================== Gentoo linux path sandbox ===========================\n");

	/* check if a sandbox is already running */
	if (NULL != getenv(ENV_SANDBOX_ACTIVE)) {
		fprintf(stderr, "Not launching a new sandbox instance\n");
		fprintf(stderr, "Another one is already running in this process hierarchy.\n");
		exit(EXIT_FAILURE);
	}

	/* determine the location of all the sandbox support files */
	if (print_debug)
		printf("Detection of the support files.\n");

	if (-1 == setup_sandbox(&sandbox_info, print_debug)) {
		fprintf(stderr, "sandbox:  Failed to setup sandbox.");
		exit(EXIT_FAILURE);
	}
	
	/* verify the existance of required files */
	if (print_debug)
		printf("Verification of the required files.\n");

#ifndef SB_HAVE_MULTILIB
	if (!rc_file_exists(sandbox_info.sandbox_lib)) {
		perror("sandbox:  Could not open the sandbox library");
		exit(EXIT_FAILURE);
	}
#endif
	if (!rc_file_exists(sandbox_info.sandbox_rc)) {
		perror("sandbox:  Could not open the sandbox rc file");
		exit(EXIT_FAILURE);
	}

	/* set up the required environment variables */
	if (print_debug)
		printf("Setting up the required environment variables.\n");

	/* If not in portage, cd into it work directory */
	if ('\0' != sandbox_info.work_dir[0])
		chdir(sandbox_info.work_dir);

	/* Setup the child environment stuff.
	 * XXX:  We free this in spawn_shell(). */
	sandbox_environ = setup_environ(&sandbox_info, print_debug);
	if (NULL == sandbox_environ)
		goto oom_error;

	/* Setup bash argv */
	str_list_add_item_copy(argv_bash, "/bin/bash", oom_error);
	str_list_add_item_copy(argv_bash, "-rcfile", oom_error);
	str_list_add_item_copy(argv_bash, sandbox_info.sandbox_rc, oom_error);
	if (argc >= 2) {
		int i;

		str_list_add_item_copy(argv_bash, run_str, oom_error);
		str_list_add_item_copy(argv_bash, argv[1], oom_error);
		for (i = 2; i < argc; i++) {
			char *tmp_ptr;

			tmp_ptr = xrealloc(argv_bash[4],
					   (strlen(argv_bash[4]) +
					    strlen(argv[i]) + 2) *
					   sizeof(char));
			if (NULL == tmp_ptr)
				goto oom_error;
			argv_bash[4] = tmp_ptr;

			snprintf(argv_bash[4] + strlen(argv_bash[4]),
				 strlen(argv[i]) + 2, " %s",
				 argv[i]);
		}
	}

	/* set up the required signal handlers */
	signal(SIGHUP, &stop);
	signal(SIGINT, &stop);
	signal(SIGQUIT, &stop);
	signal(SIGTERM, &stop);
	act_new.sa_sigaction = usr1_handler;
	sigemptyset (&act_new.sa_mask);
	act_new.sa_flags = SA_SIGINFO | SA_RESTART;
	sigaction (SIGUSR1, &act_new, NULL);

	/* STARTING PROTECTED ENVIRONMENT */
	if (print_debug) {
		printf("The protected environment has been started.\n");
		printf("--------------------------------------------------------------------------------\n");
	}

	if (print_debug)
		printf("Process being started in forked instance.\n");

	/* Start Bash */
	if (!spawn_shell(argv_bash, sandbox_environ, print_debug))
		success = 0;

	/* As spawn_shell() free both argv_bash and sandbox_environ, make sure
	 * we do not run into issues in future if we need a OOM error below
	 * this ... */
	argv_bash = NULL;
	sandbox_environ = NULL;

	if (print_debug)
		printf("Cleaning up sandbox process\n");

	if (print_debug) {
		printf("========================== Gentoo linux path sandbox ===========================\n");
		printf("The protected environment has been shut down.\n");
	}

	if (rc_file_exists(sandbox_info.sandbox_log)) {
		sandbox_log_presence = 1;
		print_sandbox_log(sandbox_info.sandbox_log);
	} else if (print_debug) {
		printf("--------------------------------------------------------------------------------\n");
	}

	if ((sandbox_log_presence) || (!success))
		return 1;
	else
		return 0;

oom_error:
	if (NULL != argv_bash)
		str_list_free(argv_bash);

	perror("sandbox:  Out of memory (environ)");
	exit(EXIT_FAILURE);
}

