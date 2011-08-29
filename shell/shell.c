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
extern void pkb(int, char **);
extern void route(int, char **);
extern void ping(int, char **);
extern void ping2(int, char **);
extern void udp_test(int, char **);

static char *prompt = "[net shell]";
static int quit;

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
	{ 0, 1, pkb, "pkb", "display pkb information" },
	/* new thread command */
	{ 1, CMD_NONUM, ping, "ping", "ping [OPTIONS] ipaddr" },
	{ 1, CMD_NONUM, udp_test, "udp_test", "test udp recv and send" },
	/* last one */
	{ 0, 0, NULL, NULL, NULL }
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
	quit = 1;
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

static void *command_thread_func(void *data)
{
	struct cmd_args *arg = (struct cmd_args *)data;
	arg->cmd->cmd_func(arg->argc, arg->argv);
	pthread_exit(NULL);
}

static void command_new_thread(struct command *cmd, int argc, char **argv)
{
	pthread_t tid;
	struct cmd_args arg;
	/* FIXME:  copy @argv for new thread */
	arg.argc = argc;
	arg.argv = argv;
	arg.cmd = cmd;
	if (pthread_create(&tid, NULL, command_thread_func, (void *)&arg))
		perrx("pthread_create");
	pthread_join(tid, NULL);
	signal_init();
}

static void parse_args(int argc, char **argv)
{
	struct command *cmd;
	for (cmd = &cmds[0]; cmd->cmd_num; cmd++) {
		if (strcmp(cmd->cmd_str, argv[0]) == 0)
			goto handle_command;
	}

	ferr("-shell: %s: command not found\n", argv[0]);
	return;

handle_command:
	if (cmd->cmd_num != CMD_NONUM && cmd->cmd_num != argc) {
		ferr("shell: %s needs %d commands\n", cmd->cmd_str, cmd->cmd_num);
		ferr("       %s: %s\n", cmd->cmd_str, cmd->cmd_help);
	} else {
		if (cmd->cmd_new)
			command_new_thread(cmd, argc, argv);
		else
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

void test_shell(char *prompt_str)
{
	char linebuf[256];
	int linelen;
	char *argv[16];
	int argc;

	if (prompt_str && *prompt_str)
		prompt = prompt_str;

	quit = 0;
	signal_init();

	while (1) {
		print_prompt();
		linelen = get_line(linebuf, 256);
		argc = parse_line(linebuf, linelen, argv);
		if (argc > 0)
			parse_args(argc, argv);
		else if (argc < 0)
			ferr("-shell: too many arguments\n");
		if (quit)
			break;
	}
}

#ifdef TEST_SHELL
int main(int argc, char **argv)
{
	test_shell("[test]");
	return 0;
}
#endif
