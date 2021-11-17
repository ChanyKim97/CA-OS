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

    //dir 없으면 true로 바꿔줌
    //link 하기위해 필요
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

    //name에 맞는 file 찾은 경우
    //검사후 내보내기
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

    assert(dir != NULL); //aseert는 개발자가 error 검출용 코드로 심어놓는 것

    file = find_file(dir, name); // 있는 경우

    //file return NULL로 name에 맞는 file 없는 경우
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
    계층구조를 만들어라.. 경로들어오면
    전체적으로 linked list 
    num_token 3이면 num_token[2]은 file [1][0] directory // num_token -1 은 항상 file
    root_dir에 "root" 있는 상태

    root - home(dir) - user(dir) - text.txt(file)
                     |          ㅏ main.c(file)
                     |          ㄴ main(file)
                     |
                     ㄴhost_list(file)
    함수가 있는 위치가 while문안 하나씩 처리
    root - home(dir) - user(dir) - text.txt(file)
    */
    directory_t* upperdir;
    directory_t* dir;
    file_t* file;

    //find_create_file 함수에 매개변수로 들어갈 bool형
    //file이나 dir없으면 만들어서 연결해줘야함 //판별용
    bool create;

    if (num_token) {
        upperdir = root_dir;
        //dir경우와 file 경우 나눠줘야함
        //for문으로 dir까지하고 file은 링크중하나니 별개처리

        //num_token -1 까지가 dir처리
        for (int i = 0; i < num_token - 1; i++) {
            create = false;
            dir = find_create_dir(upperdir, token_list[i], &create);

            //file 없는 경우 새로 create하면서 true로 바꿔주면서 if문 실행
            if (create) {
                //create_dir에서 num_dir 0으로 초기화 되어있음
                (*upperdir).dir_list[(*upperdir).num_dir++] = dir;
            }

            //파일속으로 들어가면 여기서 처리한 하위폴더가 하위하위폴더의 상위 폴더가됨 처리필요
            upperdir = dir;
        }

        //num-token -1 file형 처리
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
