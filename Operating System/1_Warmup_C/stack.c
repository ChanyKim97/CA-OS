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

/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING FROM THIS LINE ******       */
#include <stdio.h>
#include "types.h"
#include "list_head.h"

/* Declaration for the stack instance defined in pa0.c */
extern struct list_head stack;

/* Entry for the stack */
struct entry {
	struct list_head list;
	char *string;
};
/*          ****** DO NOT MODIFY ANYTHING ABOVE THIS LINE ******      */
/*====================================================================*/

/*====================================================================*
 * The rest of this file is all yours. This implies that you can      *
 * include any header files if you want to ...                        */

#include <stdlib.h>                    /* like this */
#include <string.h> //strcpy

/**
 * push_stack()
 *
 * DESCRIPTION
 *   Push @string into the @stack. The @string should be inserted into the top
 *   of the stack. You may use either the head or tail of the list for the top.
 */
void push_stack(char *string)
{
	/* TODO: Implement this function */

	struct entry* data = (struct entry*)malloc(sizeof(struct entry));
	INIT_LIST_HEAD(&data->list);
	data->string = malloc(sizeof(char) * 80); //MAX_BUFFER의 공간만큼 생성
	strcpy(data->string, string);
	list_add_tail(&data->list, &stack);
}


/**
 * pop_stack()
 *
 * DESCRIPTION
 *   Pop a value from @stack and return it through @buffer. The value should
 *   come from the top of the stack, and the corresponding entry should be
 *   removed from @stack.
 *
 * RETURN
 *   If the stack is not empty, pop the top of @stack, and return 0
 *   If the stack is empty, return -1
 */
int pop_stack(char *buffer)
{
	/* TODO: Implement this function */
	struct entry* current_node;
	struct entry* next;

	//스택 안 빈경우
	if (!list_empty(&stack)) {
		list_for_each_entry_safe(current_node, next, &stack, list) {
			if (list_is_last(&current_node->list, &stack)) { //top of stack
				strcpy(buffer, current_node->string);
				list_del(&current_node->list);
				free(current_node);
				free(current_node->string);
			}
		}
		return 0;
	}

	//스택 빈경우
	return -1; /* Must fix to return a proper value when @stack is not empty */
}


/**
 * dump_stack()
 *
 * DESCRIPTION
 *   Dump the contents in @stack. Print out @string of stack entries while
 *   traversing the stack from the bottom to the top. Note that the value
 *   should be printed out to @stderr to get properly graded in pasubmit.
 */
void dump_stack(void)
{
	/* TODO: Implement this function */
	struct entry* node;
	struct list_head* ptr;

	list_for_each(ptr, &stack) {
		node = list_entry(ptr, struct entry, list);
		fprintf(stderr, "%s\n", node->string);/* Example.
										Print out values in this form */
	}

}
