/**********************************************************************
 * Copyright (c) 2021
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>

#include "types.h"
#include "list_head.h"
#include "parser.h"

#include <sys/wait.h>

static int __process_command(char* command); // error Ã³¸®¿ë
 /***********************************************************************
  * struct list_head history
  *
  * DESCRIPTION
  *   Use this list_head to store unlimited command history.
  */
LIST_HEAD(history);

typedef struct entry {
	struct list_head list;
	char* command;
	int index;
}entry;



/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */
static int run_command(int nr_tokens, char *tokens[])
{
	//exit½Ã ³ª°¨
	if (strcmp(tokens[0], "exit") == 0) return 0; //if (!ret) break;
	
	
	/*
	* pa1-04 pipe
	* Hint:
	* pipe()	¼º°øÀûÀ¸·Î È£ÃâÇÏ¸é 0¹ÝÈ¯ Æ²¸®¸é -1¹ÝÈ¯
	* dup()		dup´Â ¸Å°³º¯¼ö·Î Àü´Þ¹ÞÀº ÆÄÀÏ ¼­¼úÀÚ¸¦ º¹Á¦ÇÏ¿© ¹ÝÈ¯
	* dup(fd1, fd2)	dup2´Â »õ ¼­¼úÀÚÀÇ °ªÀ» fd2·Î ÁöÁ¤ÇÕ´Ï´Ù. ¸¸ÀÏ fd2°¡ ÀÌ¹Ì ¿­·ÁÀÖÀ¸¸é fd2¸¦ ´ÝÀº ÈÄ º¹Á¦
	* Implement incrementally. First check whether the pipe symbol exists in the tokens.
	* If not, just do execute the command.
	* If exists, split the tokens into two pars and feed them to two different processes which are connected through a pipe.
	*/
	int status;
	pid_t pid;

	//pip ÀÖ´ÂÁö check
	int pip = 0;
	int pip_index = 0;

	for (int i = 0; i < nr_tokens; i++) {
		if (strcmp(tokens[i], "|") == 0) {
			pip++;
			pip_index = i;
		}
	}

	//pip ÀÖ´Â °æ¿ì
	if (pip) {
		int fd[2]; //fd[1]¿¡ µ¥ÀÌÅÍ¸¦ ¾´´Ù°í °¡Á¤ÇÏ¸é fd[0]¿¡¼­ µ¥ÀÌÅÍ¸¦ ÀÐÀ» ¼ö ÀÖÀ½
		pipe(fd);
		pid = fork();

		//| ±âÁØÀ¸·Î ³ª´©±â 
		char* first[MAX_NR_TOKENS] = { NULL };
		char* second[MAX_NR_TOKENS] = { NULL };
		for (int i = 0; i < nr_tokens; i++) {
			if (i < pip_index) {
				first[i] = tokens[i];
			}
			else if (i == pip_index) continue;
			else { 
				second[i-pip_index-1] = tokens[i];
			}
		}

		//child
		if (pid == 0) {
			close(fd[0]); 
			dup2(fd[1], STDOUT_FILENO);
			execvp(first[0], first);
			close(fd[1]);
			exit(1);
		}
		// parent
		else {
			pid = fork();
			if (pid == 0) {
				close(fd[1]);
				dup2(fd[0], STDIN_FILENO);
				execvp(second[0], second);
				close(fd[0]);
				exit(1);
			}
			else {
				close(fd[1]);
				wait(&status);
				wait(&status);
			}
		}
		return 1;
	}







	/** pa1-03 history ±¸Çö
	*  Hint : PA0
	**/
	//histroy part
	else if (strcmp(tokens[0], "history") == 0) {
		struct list_head* pos;

		//list_headÅ¸ÀÔÀÇ pos¸¦ Æ÷ÇÔÇÏ´Â ±¸Á¶Ã¼ entry¸¦ Ã£¾Æ¼­
		list_for_each(pos, &history) { 
			fprintf(stderr, "%2d: %s", container_of(pos, entry, list)->index, container_of(pos, entry, list)->command);
		}

		return 1;
	}
	//! part
	else if (strcmp(tokens[0], "!") == 0) {
		struct list_head* pos;
		char* temp = malloc(sizeof(char) * MAX_COMMAND_LEN);

		list_for_each(pos, &history) {
			if (container_of(pos, entry, list)->index == atoi(tokens[1])) { 
				strcpy(temp, container_of(pos, entry, list)->command);
				__process_command(temp); // error  implicit declaration of function
				break;
			}
		}
		free(temp);

		return 1;
	}





	/**pa1-02 cd ±¸Çö
	* chdir()	ÇöÀç ÀÛ¾÷ µð·ºÅä¸®¸¦ º¯°æ ¹ÝÈ¯°ª Á¤»óÀÏ ¶§ 0 ¿¡·¯ -1
	* getenv()	varname¿¡ ÇØ´çÇÏ´Â Ç×¸ñ¿¡ ´ëÇÑ È¯°æ º¯¼ö ¸®½ºÆ®¸¦ °Ë»ö
	**/
	else if (strcmp(tokens[0], "cd") == 0) {
		if (nr_tokens == 1 || strcmp(tokens[1], "~") == 0) { //tokenÀÌ ÇÏ³ªÀÎ °æ¿ì(cd¸¸ ÀÖ´Â°æ¿ì), token[1]ÀÌ ~ ÀÎ °æ¿ì homeÀ¸·Î
			chdir(getenv("HOME"));
		}
		else { //cd µÚ¿¡ÀÖ´Â ¹® ¼öÇà
			chdir(tokens[1]);
		}
		return 1; //ret 0ÀÌ¸é break
	}
	



	//pa1-01 external command
	else {
		pid = fork(); //ÇÁ·Î¼¼½º º¹Á¦ÇØ¼­ exdternal command ¼öÇà

		//child
		if(pid == 0) {
			if (execvp(tokens[0], tokens) < 0) { //ÁöÁ¤ tokens[0]À» tokens·Î Àü´ÞÇÏ¿© ½ÇÇà½ÃÅ°´Âµ¥ °á°ú °ªÀÌ ¾ø´Â °æ¿ì´Â error ¶ó´Â °Í
				fprintf(stderr, "Unable to execute %s\n", tokens[0]);
				exit(1);
			}
		}
		//parent
		else {
			wait(&status);
		}
	}
	

	return -EINVAL;
}





/***********************************************************************
 * append_history()
 *
 * DESCRIPTION
 *   Append @command into the history. The appended command can be later
 *   recalled with "!" built-in command
 */
int history_index = 0;

static void append_history(char * const command)
{
	//history ±â·Ï
	struct entry* new = (struct entry*)malloc(sizeof(struct entry));

	new->command = (char*)malloc(sizeof(char) * MAX_COMMAND_LEN);
	strcpy(new->command, command); //command ÀúÀå

	new->index = history_index++;

	INIT_LIST_HEAD(&new->list);
	list_add_tail(&new->list, &history);
}


/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Call-back function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
static int initialize(int argc, char * const argv[])
{
	return 0;
}


/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
static void finalize(int argc, char * const argv[])
{

}


/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING BELOW THIS LINE ******      */
/*          ****** BUT YOU MAY CALL SOME IF YOU WANT TO.. ******      */
static int __process_command(char * command)
{
	char *tokens[MAX_NR_TOKENS] = { NULL };
	int nr_tokens = 0;

	if (parse_command(command, &nr_tokens, tokens) == 0)
		return 1;

	return run_command(nr_tokens, tokens);
}

static bool __verbose = true;
static const char *__color_start = "[0;31;40m";
static const char *__color_end = "[0m";

static void __print_prompt(void)
{
	char *prompt = "$";
	if (!__verbose) return;

	fprintf(stderr, "%s%s%s ", __color_start, prompt, __color_end);
}

/***********************************************************************
 * main() of this program.
 */
int main(int argc, char * const argv[])
{
	char command[MAX_COMMAND_LEN] = { '\0' };
	int ret = 0;
	int opt;

	while ((opt = getopt(argc, argv, "qm")) != -1) {
		switch (opt) {
		case 'q':
			__verbose = false;
			break;
		case 'm':
			__color_start = __color_end = "\0";
			break;
		}
	}

	if ((ret = initialize(argc, argv))) return EXIT_FAILURE;

	/**
	 * Make stdin unbuffered to prevent ghost (buffered) inputs during
	 * abnormal exit after fork()
	 */
	setvbuf(stdin, NULL, _IONBF, 0);

	while (true) {
		__print_prompt();

		if (!fgets(command, sizeof(command), stdin)) break;

		append_history(command);
		ret = __process_command(command);

		if (!ret) break;
	}

	finalize(argc, argv);

	return EXIT_SUCCESS;
}
