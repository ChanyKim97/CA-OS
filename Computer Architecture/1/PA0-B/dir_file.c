#include "utils.h"
#include "dir_file.h"

directory_t* create_dir(char* name) {
    directory_t *dir;

    dir = (directory_t*)malloc(sizeof(directory_t));

    memcpy(dir->name, name, MAX_NAME_SIZE);

    dir->num_file = 0;
    dir->num_dir = 0;

    return dir;

}

directory_t* find_dir(directory_t* dir, char* name) {
    int i;

    assert(dir != NULL);
    assert(name != NULL);
    assert(strlen(name) > 0);

    for (i = 0; i < dir->num_dir; i++) {
        if (strcmp(dir->dir_list[i]->name, name) == 0)
            return dir->dir_list[i];
    }

    return NULL;
}

// This find_create_dir() find directory_t matched with name variable, in parent_dir.
// Otherwise, if this directory not exists, creates directory.
directory_t* find_create_dir(directory_t* parent_dir, char* name, bool* is_create) {
    directory_t *dir;

    assert(parent_dir != NULL);

    dir = find_dir(parent_dir, name);

    //dir ������ true�� �ٲ���
    //link �ϱ����� �ʿ�
    if (dir == NULL) {
        dir = create_dir(name);
        *is_create = true;
    }

    return dir;
}

file_t* create_file(char* name) {
    file_t *file;

    file = (file_t*)malloc(sizeof(file_t));

    memcpy(file->name, name, MAX_NAME_SIZE);

    return file;
}

file_t* find_file(directory_t* dir, char* name) {
    int i;

    assert(dir != NULL);
    assert(name != NULL);
    assert(strlen(name) > 0);

    //name�� �´� file ã�� ���
    //�˻��� ��������
    for (i = 0; i < dir->num_file; i++) {
        if (strcmp(dir->file_list[i]->name, name) == 0)
            return dir->file_list[i];
    }

    return NULL;
}

// This find_create_file() find file matched with name variable, in dir.
// Otherwise, if this file not exists, create the file.
file_t* find_create_file(directory_t* dir, char* name, bool* is_create) {
    file_t *file;

    assert(dir != NULL); //aseert�� �����ڰ� error ����� �ڵ�� �ɾ���� ��

    file = find_file(dir, name); // �ִ� ���

    //file return NULL�� name�� �´� file ���� ���
    if (file == NULL) {
        file = create_file(name);
        *is_create = true;
    }

    return file;
}

// This make_dir_and_file() make hierarchy of file and directory which is indicated by token_list.
// Everything starts in root_dir. You can implement this function using find_create_dir() and find_create_file().
void make_dir_and_file(directory_t* root_dir, char** token_list, int num_token) {
    /* Fill this function */
    /*
    ���������� ������.. ��ε�����
    ��ü������ linked list 
    num_token 3�̸� num_token[2]�� file [1][0] directory // num_token -1 �� �׻� file
    root_dir�� "root" �ִ� ����

    root - home(dir) - user(dir) - text.txt(file)
                     |          �� main.c(file)
                     |          �� main(file)
                     |
                     ��host_list(file)
    �Լ��� �ִ� ��ġ�� while���� �ϳ��� ó��
    root - home(dir) - user(dir) - text.txt(file)
    */
    directory_t* upperdir;
    directory_t* dir;
    file_t* file;

    //find_create_file �Լ��� �Ű������� �� bool��
    //file�̳� dir������ ���� ����������� //�Ǻ���
    bool create;

    if (num_token) {
        upperdir = root_dir;
        //dir���� file ��� ���������
        //for������ dir�����ϰ� file�� ��ũ���ϳ��� ����ó��

        //num_token -1 ������ diró��
        for (int i = 0; i < num_token - 1; i++) {
            create = false;
            dir = find_create_dir(upperdir, token_list[i], &create);

            //file ���� ��� ���� create�ϸ鼭 true�� �ٲ��ָ鼭 if�� ����
            if (create) {
                //create_dir���� num_dir 0���� �ʱ�ȭ �Ǿ�����
                (*upperdir).dir_list[(*upperdir).num_dir++] = dir;
            }

            //���ϼ����� ���� ���⼭ ó���� ���������� �������������� ���� �������� ó���ʿ�
            upperdir = dir;
        }

        //num-token -1 file�� ó��
        create = false;
        file = find_create_file(upperdir, token_list[num_token - 1], &create);
        if (create) {
            (*upperdir).file_list[(*upperdir).num_file++] = file;
        }
    }
}

void free_dir_and_file(directory_t* dir) {
    int index;

    for (index = 0; index < dir->num_file; index++)
        free(dir->file_list[index]);

    for (index = 0; index < dir->num_dir; index++)
        free_dir_and_file(dir->dir_list[index]);

    free(dir);
}

// This find_target_dir() find the directoy which is indicated as full path by token_list
// and return directory_t* entry of the found directory or NULL when such directory not exists.
directory_t* find_target_dir(directory_t* root_dir, char** token_list, int num_token) {
    directory_t *current_dir, *child_dir;
    char *token;
    char path[MAX_BUFFER_SIZE];
    int index;

    memset(path, 0, MAX_BUFFER_SIZE);

    current_dir = root_dir;

    for (index = 0; index < num_token; index++) {
        strcat(path, "/");
        token = token_list[index];
        child_dir = find_dir(current_dir, token);

        if (child_dir == NULL) {
            fprintf(stderr, "Not found '%s' directory in %s\n", token, path);
            return NULL;
        }

        current_dir = child_dir;
        strcat(path, token);
        child_dir = NULL;
    }

    return current_dir;
}

void print_file_on_dir(directory_t* dir) {
    int index = 0;

    for (index = 0; index < dir->num_file; index++)
        printf("%s\n", dir->file_list[index]->name);
}
