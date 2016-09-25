#include <stdio.h>    //printf(3)
#include <stdlib.h>   //exit(3)
#include <unistd.h>   //fork(3), chdir(3), sysconf(3)
#include <signal.h>   //signal(3)
#include <sys/stat.h> //umask(3)
#include <fcntl.h>    //open
#include <errno.h>    //errno
#include <syslog.h>   //syslog(3), openlog(3), closelog(3)
#include <getopt.h>   //getopt_long
#include <string.h>   //strlen

void daemonize();
void print_help();
void handle_signal(int sig);

int g_pid_fd = -1;
FILE *gp_log_stream;
char *gp_app_name, *gp_conf_file, *gp_log_file, *gp_pid_file;

int main(int argc, char *argv[])
{
    // Command line arguments definition
	struct option long_options[] = {
		{"conf_file", required_argument, 0, 'c'},
		{"log_file", required_argument, 0, 'l'},
        {"pid_file", required_argument, 0, 'p'},
		{"help", no_argument, 0, 'h'},
		{NULL, 0, 0, 0}
	};

    // Read app name from command line
    gp_app_name = argv[0];

    // Try to process all command line arguments
	int value, option_index = 0;
	while ((value = getopt_long(argc, argv, "c:l:p:h", long_options, &option_index)) != -1) {
		switch(value) {
			case 'c':
				gp_conf_file = strdup(optarg);
				break;
			case 'l':
				gp_log_file = strdup(optarg);
				break;
			case 'p':
				gp_pid_file = strdup(optarg);
				break;
			case 'h':
                print_help();
				return EXIT_SUCCESS;
			case '?':
				print_help();
				return EXIT_FAILURE;
			default:
				break;
		}
    }

    // Daemozine process
    daemonize();

    // Daemon will handle two signals
	signal(SIGINT, handle_signal);
    signal(SIGHUP, handle_signal);

    // Open the system log file
    openlog(gp_app_name, LOG_PID|LOG_CONS, LOG_DAEMON);

    // Write start message
    syslog(LOG_NOTICE, "%s started.", gp_app_name);

    // Try to app specific log
	if (gp_log_file != NULL) {
		gp_log_stream = fopen(gp_log_file, "a+");
		if (gp_log_stream == NULL) {
			syslog(LOG_ERR, "Can not open log file: %s, error: %s", gp_log_file, strerror(errno));
		}
	}

    while (1) {
        // Write to log
		fprintf(gp_log_stream, "%s started.\n", gp_app_name);
		fflush(gp_log_stream);

        sleep(20);
        break;
    }

    syslog(LOG_NOTICE, "%s terminated.", gp_app_name);
    closelog();

    // Close log file
	if (gp_log_stream != NULL) {
		fclose(gp_log_stream);
	}

	// Free allocated memory
	if (gp_conf_file != NULL) {
        free(gp_conf_file);
	}
	if (gp_log_file != NULL) {
        free(gp_log_file);
	}
    if (gp_pid_file != NULL) {
         free(gp_pid_file);
    }

    return EXIT_SUCCESS;
}

void daemonize()
{
    pid_t pid;

    // Fork off the parent process
    pid = fork();

    // An error occurred
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    // Success: Let the parent terminate
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // On success: The child process becomes session leader
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    // Ignore signal sent from child to parent process
    signal(SIGCHLD, SIG_IGN);

    // Fork off for the second time
    pid = fork();

    // An error occurred
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    // Success: Let the parent terminate
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Set new file permissions
    umask(0);

    // Change the working directory to the root directory
    chdir("/");

    // Close all open file descriptors
    for (int x = sysconf(_SC_OPEN_MAX); x > 0; --x) {
        close (x);
    }

    // Reopen stdin (fd = 0), stdout (fd = 1), stderr (fd = 2)
	stdin = fopen("/dev/null", "r");
	stdout = fopen("/dev/null", "w+");
	stderr = fopen("/dev/null", "w+");

	// Try to write PID of daemon to lockfile
	if (gp_pid_file != NULL) {
		g_pid_fd = open(gp_pid_file, O_RDWR|O_CREAT, 0640);
		if (g_pid_fd < 0) {
			// Can't open lockfile
			exit(EXIT_FAILURE);
		}

		if (lockf(g_pid_fd, F_TLOCK, 0) < 0) {
			// Can't lock file
			exit(EXIT_FAILURE);
		}

		// Get current PID
        char str[256];
		sprintf(str, "%d\n", getpid());

		// Write PID to lockfile
		write(g_pid_fd, str, strlen(str));
    }
}

void print_help()
{
	printf("\n Usage: %s [OPTIONS]\n\n", gp_app_name);
	printf("  Options:\n");
	printf("   -h --help                 Print this help\n");
	printf("   -c --conf_file filename   Path to the configuration file\n");
	printf("   -l --log_file  filename   Path to the log file\n");
	printf("   -p --pid_file  filename   Path to file containing PID value\n");
    printf("\n");
}

void handle_signal(int sig)
{
	if(sig == SIGINT) {
		fprintf(gp_log_stream, "Debug: stopping daemon ...\n");

		// Unlock and close lockfile
		if (g_pid_fd != -1) {
			lockf(g_pid_fd, F_ULOCK, 0);
			close(g_pid_fd);
		}

		// Try to delete lockfile
		if (gp_pid_file != NULL) {
			unlink(gp_pid_file);
		}

		// Reset signal handling to default behavior
		signal(SIGINT, SIG_DFL);
	} else if(sig == SIGHUP) {
		fprintf(gp_log_stream, "Debug: reloading daemon config file ...\n");
		// TO DO conf read
	} else if(sig == SIGCHLD) {
		fprintf(gp_log_stream, "Debug: received SIGCHLD signal\n");
	}
}

