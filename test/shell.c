/*
 * simple shell for net stack debug and monitor
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "lib.h"
#include "arp.h"

/* extern net stack command handler */
extern void arpcache(int, char **);
extern void netdebug(int, char **);
extern void ifconfig(int, char **);
extern void route(int, char **);
extern void ping(int, char **);

static char *prompt = "[net shell]";
static int quit;

struct command {
	int cmd_num;
	void (*cmd_func)(int, char **);
	char *cmd_str;
	char *cmd_help;
};

#define CMD_NONUM -1	/* Arguments is parsed in command function. */

static void builtin_exit(int argc, char **argv)
{
	quit = 1;
}

extern void builtin_help(int argc, char **argv);
static struct command cmds[] = {
	/* built-in command */
	{ CMD_NONUM, builtin_help, "help", "display shell command information" },
	{ CMD_NONUM, builtin_exit, "exit", "exit shell" },
	/* net stack command */
	{ CMD_NONUM, netdebug, "debug", "debug dev|l2|arp|ip|icmp|udp|tcp|all" },
	{ CMD_NONUM, ping, "ping", "ping [OPTIONS] ipaddr" },
	{ 1, arpcache, "arpcache", "see arp cache" },
	{ 1, route, "route", "show / manipulate the IP routing table" },
	{ 1, ifconfig, "ifconfig", "configure a network interface" },
	{ 0, NULL, NULL, NULL }	/* last one flag */
};

void builtin_help(int argc, char **argv)
{
	struct command *cmd;
	int i;
	for (i = 1, cmd = &cmds[0]; cmd->cmd_num; i++, cmd++)
		printf(" %d  %s: %s\n", i, cmd->cmd_str, cmd->cmd_help);

}

static int get_line(char *buf, int bufsz)
{
	char *p;
	int len;
	p = fgets(buf, bufsz - 1, stdin);
	if (!p) {
		if (errno)
			perrx("fgets");
		/* EOF (Ctrl + D) */
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
	while (p = get_arg(&pp))
		argv[argc++] = p;
	return argc;
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
