struct command {
	int cmd_num;
	void (*cmd_func)(int, char **);
	char *cmd_str;
	char *cmd_help;
};

#define CMD_NONUM -1	/* Arguments is parsed in command function. */

