/*
 * simple shell for net stack debug and monitor
 */
#include "lib.h"
#include "arp.h"

static void signal_init(void);
/* extern builtin command handlers */
static void builtin_help(int argc, char **argv);
static void builtin_exit(int argc, char **argv);
static void builtin_clear(int argc, char **argv);

/* extern net stack command handlers */
extern void arpcache(int, char **);
extern void netdebug(int, char **);
extern void ifconfig(int, char **);
extern void stat(int, char **);
extern void route(int, char **);
extern void ping(int, char **);
extern void ping2(int, char **);
extern void snc(int, char **);

struct command {
	int cmd_new;	/* new thread flag */
	int cmd_num;
	void (*cmd_func)(int, char **);
	char *cmd_str;
	char *cmd_help;
};

struct cmd_args {
	int argc;
	char **argv;
	struct command *cmd;
};

static char *prompt = "[net shell]";
static pthread_cond_t master_cond;
static pthread_mutex_t master_mutex;
static int master_quit;

static struct cmd_args work, *pending_work;
static pthread_cond_t worker_cond;
static pthread_mutex_t worker_mutex;
static int work_quit;

void shell_init(void)
{
	pthread_cond_init(&worker_cond, NULL);
	pthread_mutex_init(&worker_mutex, NULL);
	pthread_cond_init(&master_cond, NULL);
	pthread_mutex_init(&master_mutex, NULL);
	/* No work at boot-up stage */
	pending_work = NULL;
	work_quit = 0;
	master_quit = 0;
}

#define CMD_NONUM -1	/* Arguments are checked in command function. */

static struct command cmds[] = {
	/* built-in command */
	{ 0, CMD_NONUM, builtin_help, "help", "display shell command information" },
	{ 0, CMD_NONUM, builtin_clear, "clear", "clear the terminal screen" },
	{ 0, CMD_NONUM, builtin_exit, "exit", "exit shell" },
	/* net stack command */
	{ 0, CMD_NONUM, netdebug, "debug", "debug dev|l2|arp|ip|icmp|udp|tcp|all" },
	{ 0, CMD_NONUM, ping2, "ping2", "ping [OPTIONS] ipaddr(Internal stack implementation)" },
	{ 0, 1, arpcache, "arpcache", "see arp cache" },
	{ 0, 1, route, "route", "show / manipulate the IP routing table" },
	{ 0, 1, ifconfig, "ifconfig", "configure a network interface" },
	{ 0, 1, stat, "stat", "display pkb/sock information" },
	/* new thread command */
	{ 1, CMD_NONUM, ping, "ping", "ping [OPTIONS] ipaddr" },
	{ 1, CMD_NONUM, snc, "snc", "Simplex Net Cat" },
	/* last one */
	{ 0, 0, NULL, NULL, NULL }	/* can also use sizeof(cmds)/sizeof(cmds[0]) for cmds number */
};

static void builtin_clear(int argc, char **argv)
{
	printf("\033[1H\033[2J");
}

static void builtin_help(int argc, char **argv)
{
	struct command *cmd;
	int i;
	for (i = 1, cmd = &cmds[0]; cmd->cmd_num; i++, cmd++)
		printf(" %d  %s: %s\n", i, cmd->cmd_str, cmd->cmd_help);
}

static void builtin_exit(int argc, char **argv)
{
	master_quit = 1;
	work_quit = 1;
	pthread_cond_signal(&worker_cond);
	/* Should we destroy it during exit? */
	pthread_cond_destroy(&worker_cond);
	pthread_cond_destroy(&master_cond);
}

static int get_line(char *buf, int bufsz)
{
	char *p;
	int len;
	p = fgets(buf, bufsz - 1, stdin);
	if (!p) {
		/*
		 * Ctrl+D also cause interrupt.
		 * Because int signal is set RESTART, so other interrupts
		 * will not return from fgets.
		 */
		if (errno && errno != EINTR)
			perrx("fgets");
		/* EOF (Ctrl + D) is set as exit command */
		printf("exit\n");
		strcpy(buf, "exit");
		p = buf;
	}
	len = strlen(p);
	if (len == 0)
		return 0;
	if (p[len - 1] == '\n') {
		p[len - 1] = '\0';
		len--;
	}
	return len;
}

static char *get_arg(char **pp)
{
	char *ret, *p;
	ret = NULL;
	p = *pp;
	/* skip white space and fill \0 */
	while (isblank(*p)) {
		*p = '\0';
		p++;
	}
	if (*p == '\0')
		goto out;
	ret = p;
	/* skip normal character */
	while (*p && !isblank(*p))
		p++;
out:
	*pp = p;
	return ret;
}

static int parse_line(char *line, int len, char **argv)
{
	int argc;
	char *p, *pp;
	/* assert len >= 0 */
	if (len == 0)
		return 0;

	p = pp = line;
	argc = 0;
	while ((p = get_arg(&pp)) != NULL)
		argv[argc++] = p;
	return argc;
}

void *shell_worker(void *none)
{
	while (!work_quit) {
		while (!pending_work) {
			pthread_cond_wait(&worker_cond, &worker_mutex);
			if (work_quit)
				goto out;
		}
		pending_work->cmd->cmd_func(pending_work->argc, pending_work->argv);
		pending_work = NULL;
		pthread_cond_signal(&master_cond);
	}
out:
	dbg("shell worker exit");
	return NULL;
}

static void parse_args(int argc, char **argv)
{
	struct command *cmd;
	for (cmd = &cmds[0]; cmd->cmd_num; cmd++) {
		if (strcmp(cmd->cmd_str, argv[0]) == 0)
			goto runcmd;
	}

	ferr("-shell: %s: command not found\n", argv[0]);
	return;

runcmd:
	if (cmd->cmd_num != CMD_NONUM && cmd->cmd_num != argc) {
		ferr("shell: %s needs %d commands\n", cmd->cmd_str, cmd->cmd_num);
		ferr("       %s: %s\n", cmd->cmd_str, cmd->cmd_help);
	} else if (cmd->cmd_new) {
		work.argc = argc;
		work.argv = argv;
		work.cmd = cmd;
		pending_work = &work;
		pthread_cond_signal(&worker_cond);
		while (pending_work)
			pthread_cond_wait(&master_cond, &master_mutex);
		signal_init();
	} else {
		cmd->cmd_func(argc, argv);
	}
}

static void print_prompt(void)
{
	printf("%s: ", prompt);
	fflush(stdout);
}

/* interrupt and quit signals use the same handler */
static void signal_int(int nr, siginfo_t *si, void *p)
{
	if (!net_debug) {
		printf("\n");
		print_prompt();
	}
}

static void signal_init(void)
{
	struct sigaction act;
	/* interrupt signal */
	memset(&act, 0x0, sizeof(act));
	act.sa_flags = SA_RESTART;
	act.sa_sigaction = signal_int;
	if ((sigaction(SIGINT, &act, NULL)) == -1)
		perrx("sigaction SIGINT");

	/* quit signal */
	memset(&act, 0x0, sizeof(act));
	act.sa_flags = SA_RESTART;
	act.sa_sigaction = signal_int;
	if ((sigaction(SIGQUIT, &act, NULL)) == -1)
		perrx("sigaction SIGOUT");
}

void shell_master(char *prompt_str)
{
	char linebuf[256];
	int linelen;
	char *argv[16];
	int argc;

	/* shell master */
	if (prompt_str && *prompt_str)
		prompt = prompt_str;
	signal_init();
	while (!master_quit) {
		print_prompt();
		linelen = get_line(linebuf, 256);
		argc = parse_line(linebuf, linelen, argv);
		if (argc > 0)
			parse_args(argc, argv);
		else if (argc < 0)
			ferr("-shell: too many arguments\n");
	}
}

#ifdef TEST_SHELL
int main(int argc, char **argv)
{
	test_shell("[test]");
	return 0;
}
#endif
