#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "util.h"

int is_large(int num1, int num2)
{
	return num1 > num2;
}

int sum_x(int x1, int x2)
{
	int sum = 0;
	/* Fill this function */
	sum = x1 + x2;

	return sum;
}

void sub_y(int y1, int y2, int *sub)
{
	/* Fill this function */
	*sub = y1 - y2;
}

// You have to allocate memory for pointer members of "struct Point_ref"
// Hint: Use malloc()
void Point_val_to_Point_ref(struct Point_val *P1, struct Point_ref *P2)
{
	/* Fill this function */
	(*P2).x = (int*)malloc(sizeof(int));
	(*P2).y = (int*)malloc(sizeof(int));
	*(*P2).x = (*P1).x;
	*(*P2).y = (*P1).y;
}

void Point_ref_to_Point_val(struct Point_ref *P1, struct Point_val *P2)
{
	/* Fill this function */
	(*P2).x = *(*P1).x;
	(*P2).y = *(*P1).y;
}

int calc_area1(struct Point_val *P1, struct Point_val *P2)
{
	/* Fill this function */
	int area = 0;
	int side = (*P2).x - (*P1).x;
	int high = (*P2).y - (*P1).y;
	area = abs(side * high);
	return area;
}

void calc_area2(struct Point_ref *P1, struct Point_ref *P2, int *area)
{
	/* Fill this function */
	int side = *(*P2).x - *(*P1).x;
	int high = *(*P2).y - *(*P1).y;
	*area = abs(side * high);
}

char* reverse(char *word)
{
	/* Fill this function */
	int temp;
	int len = strlen(word);
	for (int i = 0; i < len / 2; i++) {
		temp = word[len - 1 - i];
		word[len - 1 - i] = word[i];
		word[i] = temp;
	}
	return word;
}
