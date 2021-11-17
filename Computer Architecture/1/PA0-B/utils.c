#include "dir_file.h"
#include "utils.h"

int open_file(char* fname, FILE** input) {
    if (access(fname, F_OK) == -1) {
        ERR_PRINT("The '%s' file does not exists\n", fname);
        return -1;
    }

    *input = fopen(fname, "r");
    if (input == NULL) {
        ERR_PRINT("Failed open '%s' file\n", fname);
        return -1;
    }

    return 1;
}

// This parse_str_to_list() split string to the tokens, and put the tokens in token_list.
// The value of return is the number of tokens.
int parse_str_to_list(const char* str, char** token_list) {
    /* Fill this function */
   /* int num_token = 0;
    char delimiter[] = "/\n";
    char* token = strtok(str, delimiter);
    char* p; //�ӽ�
    
    while (token != NULL) {
        int len = strlen(token);
        p = token_list[num_token++] = (char*)malloc((len + 1) * sizeof(char));
        for (int i = 0; i < len; i++) {
            p[i] = token[i];
            if (i == len - 1) p[i + 1] = '\0';
        }

        token = strtok(NULL, delimiter);
    }
    return num_token;*/

    //�Ű������� const���̶� strtok �� �� ���ٰ� error �߻���...

    int num_token = 0; //��ȯ��
    char* token; //�߸�����
    int wsindex = 0; //wordseperate ����

    int len = strlen(str);
    for (int i = 0; i< len; i++) {
        switch (str[i]) {
        case '/':
        case '\n':
            if (i > wsindex) {
                token = token_list[num_token++] = (char*)malloc((i - wsindex + 1) * sizeof(char));
                //�ܾ� ũ�⸸ŭ �����Ҵ� \0������ +1����
                //��ū���� +1��
                
                for (int j = wsindex; j < i; j++) {
                    token[j - wsindex] = str[j];
                }
                token[i - wsindex] = '\0';
            }
            wsindex = i + 1; //�����ܾ� �������� index�̵�
            break;
        }
    }
    return num_token;
}

void free_token_list(char** token_list, int num_token) {
    int index;

    for (index = 0; index < num_token; index++) {
        free(token_list[index]);
    }

    free(token_list);
}
