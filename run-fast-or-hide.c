/*
 * run-fast-or-hide:
 *	Waits some time for child process to finish and returns its
 * exit code. If child process keeps running just exit with code 250.
 *
 * 2011 (c) Przemysław Iskra <sparky@pld-linux.org>
 *	This program is free software,
 *  you may distribute it under GPL v2 or newer.
 *
 * $Id$
 */

#include <signal.h> /* sigaction */
#include <stdlib.h> /* exit */
#include <unistd.h> /* fork, usleep, set*id */
#include <sys/types.h> /* waitpid */
#include <sys/wait.h> /* waitpid */
#include <sys/time.h> /* gettimeofday */
#include <getopt.h> /* getopt_long */
#include <pwd.h> /* getpwnam */
#include <grp.h> /* initgroups */
#include <stdio.h>
#include <fcntl.h> /* O_RDWR */

#ifndef SLEEP_TIME
/* 100 msec should be more than enough */
# define SLEEP_TIME 100000
#endif

static pid_t our_child = 0;

static void
die( const char *msg )
{
	fprintf(stderr, "ERROR: %s\n", msg);
	exit( 127 );
}

static void
cb_sigchld( int signum )
{
	int status;
	pid_t got_child;

	while ( ( got_child = waitpid( 0, &status, WNOHANG ) ) > 0 ) {
		if ( got_child == our_child ) {
			int code = WEXITSTATUS( status );
			exit( code == 250 ? 251 : code );
		}
	}
}

static void
set_sigchld( void )
{
	struct sigaction action_child;
	action_child.sa_handler = cb_sigchld;
	action_child.sa_flags = SA_NOCLDSTOP;
	sigemptyset( &action_child.sa_mask );

	sigaction( SIGCHLD, &action_child, NULL );
}

/* usleep will be interrupted by sigchld, this thing makes sure we wait
 * specified ammount of time if there was no child, or it wasn't the right one */
static void
mysleep( long int sleep_usec )
{
	struct timeval t_start, t_test;
	long int slept_usec = 0;

	if ( gettimeofday( &t_start, NULL ) != 0 ) {
		usleep( sleep_usec );
		return;
	}

	do {
		usleep( sleep_usec - slept_usec );
		if ( gettimeofday( &t_test, NULL ) != 0 )
			return;

		slept_usec = t_test.tv_usec - t_start.tv_usec
			+ 1000000 * ( t_test.tv_sec - t_start.tv_sec );
	} while ( sleep_usec > slept_usec + 10000 );

	return;
}

static int
run_child( int verbose, struct passwd *pw, int nicelevel, char * const *argv )
{
	if ( nicelevel )
		nice( nicelevel );

	if ( pw != NULL ) {
 		if ( setgid( pw->pw_gid ) )
			die( "cannot set gid" );
		if ( initgroups( pw->pw_name, pw->pw_gid ) )
			die( "cannot init group list" );
		if ( setuid( pw->pw_uid ) )
			die( "cannot set uid" );
		if ( chdir( pw->pw_dir ) )
			die( "cannot change directory" );
	}

	if ( ! verbose ) {
		int null_fd = open( "/dev/null", O_RDWR );
		dup2( null_fd, 0);
		dup2( null_fd, 1);
		dup2( null_fd, 2);
	}

	execvp( argv[0], argv );
	return 127;
}

static void
show_help( void )
{
	printf(
"Usage: run-fast-or-hide [options] -- <command> <arguments>\n"
" Run command and detach if it takes too long to execute.\n"
"\n"
"\n"
"Options:\n"
"  -h, --help                     show this help\n"
"  -u, --user <username>          run as <username>, chdir to its home directory\n"
"  -n, --nice <incr>              add incr to the process's nice level\n"
"  -s, --sleep <usec>             wait <usec> microseconds before detaching\n"
"  -v, --verbose                  don't close standard input and output\n"
"\n"
"Exit status:\n"
"     0 - child exited normally\n"
"   250 - child didn't exit yet\n"
"   127 - execution failed\n"
" other - exit status of the child process (it exited before timeout)\n"
"   [*] - if child exits with code 250 it is modified to 251.\n"
"\n"
"Przemyslaw Iskra <sparky@pld-linux.org>, GPL v2+.\n"
);

}

int
main( int argc, char **argv )
{
	struct passwd *pw = NULL;
	int nicelevel = 0;
	int verbose = 0;
	long int sleep_time = SLEEP_TIME;

	static const struct option longopts[] = {
		{ "help",	  0, NULL, 'h' },
		{ "sleep",	  1, NULL, 's' },
		{ "user",	  1, NULL, 'u' },
		{ "nice",	  1, NULL, 'n' },
		{ "verbose",	  0, NULL, 'v' },
		{ NULL,		0, NULL, 0 }
	};

	int c;

	for (;;) {
		c = getopt_long(argc, argv, "hvs:u:n:", longopts, NULL);
		if (c == -1)
			break;
		switch (c) {
			case 'h':
				show_help();
				exit( 0 );
			case 's':
				sleep_time = atol( optarg );
				if ( sleep_time < 1000 )
					die( "invalid sleep time" );
				break;
			case 'u':
				pw = getpwnam( optarg );
				if ( pw == NULL )
					die( "invalid user" );
				break;
			case 'n':
				nicelevel = atoi( optarg );
				break;
			case 'v':
				verbose = 1;
				break;
			default:
				die( "unknown option" );
		}
	}

	if ( argc <= optind ) {
		die( "command is missing" );
	}

	set_sigchld();
	our_child = fork();

	if ( our_child ) {
		mysleep( sleep_time );
		return 250;
	} else {
		signal( SIGCHLD, SIG_IGN );
		return run_child( verbose, pw, nicelevel, argv + optind );
	}
}
