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

static int __process_command(char* command); // error 처리용
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
	//exit시 나감
	if (strcmp(tokens[0], "exit") == 0) return 0; //if (!ret) break;
	
	
	/*
	* pa1-04 pipe
	* Hint:
	* pipe()	성공적으로 호출하면 0반환 틀리면 -1반환
	* dup()		dup는 매개변수로 전달받은 파일 서술자를 복제하여 반환
	* dup(fd1, fd2)	dup2는 새 서술자의 값을 fd2로 지정합니다. 만일 fd2가 이미 열려있으면 fd2를 닫은 후 복제
	* Implement incrementally. First check whether the pipe symbol exists in the tokens.
	* If not, just do execute the command.
	* If exists, split the tokens into two pars and feed them to two different processes which are connected through a pipe.
	*/
	int status;
	pid_t pid;

	//pip 있는지 check
	int pip = 0;
	int pip_index = 0;

	for (int i = 0; i < nr_tokens; i++) {
		if (strcmp(tokens[i], "|") == 0) {
			pip++;
			pip_index = i;
		}
	}

	//pip 있는 경우
	if (pip) {
		int fd[2]; //fd[1]에 데이터를 쓴다고 가정하면 fd[0]에서 데이터를 읽을 수 있음
		pipe(fd);
		pid = fork();

		//| 기준으로 나누기 
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







	/** pa1-03 history 구현
	*  Hint : PA0
	**/
	//histroy part
	else if (strcmp(tokens[0], "history") == 0) {
		struct list_head* pos;

		//list_head타입의 pos를 포함하는 구조체 entry를 찾아서
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





	/**pa1-02 cd 구현
	* chdir()	현재 작업 디렉토리를 변경 반환값 정상일 때 0 에러 -1
	* getenv()	varname에 해당하는 항목에 대한 환경 변수 리스트를 검색
	**/
	else if (strcmp(tokens[0], "cd") == 0) {
		if (nr_tokens == 1 || strcmp(tokens[1], "~") == 0) { //token이 하나인 경우(cd만 있는경우), token[1]이 ~ 인 경우 home으로
			chdir(getenv("HOME"));
		}
		else { //cd 뒤에있는 문 수행
			chdir(tokens[1]);
		}
		return 1; //ret 0이면 break
	}
	



	//pa1-01 external command
	else {
		pid = fork(); //프로세스 복제해서 exdternal command 수행

		//child
		if(pid == 0) {
			if (execvp(tokens[0], tokens) < 0) { //지정 tokens[0]을 tokens로 전달하여 실행시키는데 결과 값이 없는 경우는 error 라는 것
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
	//history 기록
	struct entry* new = (struct entry*)malloc(sizeof(struct entry));

	new->command = (char*)malloc(sizeof(char) * MAX_COMMAND_LEN);
	strcpy(new->command, command); //command 저장

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
