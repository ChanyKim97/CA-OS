#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>

/*
 * For debug option. If you want to debug, set 1.
 * If not, set 0.
 */
#define DEBUG 0

#define MAX_SYMBOL_TABLE_SIZE   1024
#define MEM_TEXT_START          0x00400000
#define MEM_DATA_START          0x10000000
#define BYTES_PER_WORD          4
#define INST_LIST_LEN           20

/******************************************************
 * Structure Declaration
 *******************************************************/

typedef struct inst_struct {
    char *name;
    char *op;
    char type;
    char *funct;
} inst_t;

typedef struct symbol_struct {
    char name[32];
    uint32_t address;
} symbol_t;

enum section { 
    DATA = 0,
    TEXT,
    MAX_SIZE
};

/******************************************************
 * Global Variable Declaration
 *******************************************************/

inst_t inst_list[INST_LIST_LEN] = {       //  idx
    {"addiu",   "001001", 'I', ""},       //    0
    {"addu",    "000000", 'R', "100001"}, //    1
    {"and",     "000000", 'R', "100100"}, //    2
    {"andi",    "001100", 'I', ""},       //    3
    {"beq",     "000100", 'I', ""},       //    4
    {"bne",     "000101", 'I', ""},       //    5
    {"j",       "000010", 'J', ""},       //    6
    {"jal",     "000011", 'J', ""},       //    7
    {"jr",      "000000", 'R', "001000"}, //    8
    {"lui",     "001111", 'I', ""},       //    9
    {"lw",      "100011", 'I', ""},       //   10
    {"nor",     "000000", 'R', "100111"}, //   11
    {"or",      "000000", 'R', "100101"}, //   12
    {"ori",     "001101", 'I', ""},       //   13
    {"sltiu",   "001011", 'I', ""},       //   14
    {"sltu",    "000000", 'R', "101011"}, //   15
    {"sll",     "000000", 'R', "000000"}, //   16
    {"srl",     "000000", 'R', "000010"}, //   17
    {"sw",      "101011", 'I', ""},       //   18
    {"subu",    "000000", 'R', "100011"}  //   19
};

symbol_t SYMBOL_TABLE[MAX_SYMBOL_TABLE_SIZE]; // Global Symbol Table

uint32_t symbol_table_cur_index = 0; // For indexing of symbol table

/* Temporary file stream pointers */
FILE *data_seg;
FILE *text_seg;

/* Size of each section */
uint32_t data_section_size = 0;
uint32_t text_section_size = 0;

/******************************************************
 * Function Declaration
 *******************************************************/

/* Change file extension from ".s" to ".o" */
char* change_file_ext(char *str) {
    char *dot = strrchr(str, '.');

    if (!dot || dot == str || (strcmp(dot, ".s") != 0))
        return NULL;

    str[strlen(str) - 1] = 'o';
    return "";
}

/* Add symbol to global symbol table */
void symbol_table_add_entry(symbol_t symbol)
{
    SYMBOL_TABLE[symbol_table_cur_index++] = symbol;
#if DEBUG
    printf("%s: 0x%08x\n", symbol.name, symbol.address);
#endif
}

/* Convert integer number to binary string */
char* num_to_bits(unsigned int num, int len) 
{
    char* bits = (char *) malloc(len+1);
    int idx = len-1, i;
    while (num > 0 && idx >= 0) {
        if (num % 2 == 1) {
            bits[idx--] = '1';
        } else {
            bits[idx--] = '0';
        }
        num /= 2;
    }
    for (i = idx; i >= 0; i--){
        bits[i] = '0';
    }
    bits[len] = '\0';
    return bits;
}

//dataseg나 textseg strotk input을 bit화 시키기 위한 정수 변환과정
int change_input_to_int(char* temp) {
    char* check = temp;
    if (check[0] == '0' && check[1] == 'x') { //16진수로 들어오는 경우                                segmentation fault
        return strtoul(check, NULL, 16);
    }
    else if (check[0] == '$') { // register                                                             
        return atoi(check + 1);
    }
    else { // 10진수 상수
        return atoi(check);
    }
    return 0;
}

//주소 찾아 돌려주기
uint32_t returnadd(char* wantadd) {
    for (int i = 0; i < symbol_table_cur_index; i++) {
        if (!strcmp(SYMBOL_TABLE[i].name, wantadd)) {
            return SYMBOL_TABLE[i].address;
            break;
        }
    }
    return 0;
}

/* Record .text section to output file */
void record_text_section(FILE *output) 
{
    uint32_t cur_addr = MEM_TEXT_START;
    char line[1024];

    /* Point to text_seg stream */
    rewind(text_seg);

    /* Print .text section */
    while (fgets(line, 1024, text_seg) != NULL) {
        //char inst[0x1000] = {0};
        char op[32] = {0};
        char label[32] = {0};
        char type = '0';
        int i, idx = 0;
        int rs, rt, rd, imm, shamt;
        int addr;
        char* bits;

        rs = rt = rd = imm = shamt = addr = 0;
#if DEBUG
        printf("0x%08x: ", cur_addr);
#endif
        /* Find the instruction type that matches the line */
        /* blank */
        char* temp = strtok(line, "\t\n, ");
        strcpy(label, temp);
        for (i = 0; i < INST_LIST_LEN; i++) {
            if (!strcmp(temp, inst_list[i].name)) {
                idx = i;
                break;
            }
        }
        type = inst_list[idx].type;
        strcpy(op, inst_list[idx].op);
        fputs(op, output);      //6

        switch (type) {
            case 'R':
                /* blank */
                /*
                R구조 op  rs  rt  rd  shamt   funct
                       6   5   5   5    5       6

                R에 해당 addu, and, jr, nor, or, sltu, sll, srl, subu
                addu    rd = rs + rt
                and     rd = rs & rt
                jr      PC = rs
                nor     rd = !(rs | rt)
                or      rd = rs | rt
                sltu    rd = (rs < rt)? 1: 0
                sll     rd = rt << shamt
                srl     rd = rt >> shamt
                subu    rd = rs - rt

                대입순서와 해당 목록
                rd, rs, rt  {addu, and, nor, or, sltu, subu}
                rd, rt      {sll, srl}
                rs          {jr}
                */
                if (!strcmp(label, "jr")) {
                    temp = strtok(NULL, "\t\n, ");
                    rs = change_input_to_int(temp);
                }
                else if (!strcmp(label, "sll") || !strcmp(label, "srl")) {
                    temp = strtok(NULL, "\t\n, ");
                    rd = change_input_to_int(temp);
                    temp = strtok(NULL, "\t\n, ");
                    rt = change_input_to_int(temp);
                    temp = strtok(NULL, "\t\n, ");
                    shamt = change_input_to_int(temp);
                }
                else { //addu add nor or sltu subu
                    temp = strtok(NULL, "\t\n, ");
                    rd = change_input_to_int(temp);
                    temp = strtok(NULL, "\t\n, ");
                    rs = change_input_to_int(temp);
                    temp = strtok(NULL, "\t\n, ");
                    rt = change_input_to_int(temp);
                }

                bits = num_to_bits(rs, 5);
                fputs(bits, output);            //5
                free(bits);
                bits = num_to_bits(rt, 5);
                fputs(bits, output);            //5
                free(bits);
                bits = num_to_bits(rd, 5);
                fputs(bits, output);            //5
                free(bits);
                bits = num_to_bits(shamt, 5);
                fputs(bits, output);            //5
                free(bits);
                fputs(inst_list[idx].funct, output);//6     총합 : 32
                //fputc('\n', output);
#if DEBUG
                printf("op:%s rs:$%d rt:$%d rd:$%d shamt:%d funct:%s\n",
                        op, rs, rt, rd, shamt, inst_list[idx].funct);
#endif
                break;

            case 'I':
                /* blank */
                /*
                I format
                    op  rs  rt  imm
                    6   5   5   16

                I type - addiu, andi, beq, bne, lui, lw, ori, sltiu, sw
                addiu       rt = rs + SEI
                andi        rt = rs & ZEI
                beq         rs == rt -> PC = PC + 4 + branchadd
                bne         rs != rt ->
                lui         rt = imm
                lw          rt = imm(rs)
                ori         rt = rs | ZEI
                sltiu       rt = (rs<SEI)? 1 : 0
                sw          rt = imm(rs)

                rt rs imm       {addiu, andi,ori, sltiu}
                rs rt imm       {beq, bne}
                rt imm          {lui}
                rt imm rs       {lw, sw}
                */

                if (!strcmp(label, "lui")) {
                    rs = 0;
                    temp = strtok(NULL, "\t\n, ");
                    rt = change_input_to_int(temp);
                    temp = strtok(NULL, "\t\n, ");
                    imm = change_input_to_int(temp);
                }
                else if (!strcmp(label, "beq") || !strcmp(label, "bne")) {
                    temp = strtok(NULL, "\t\n, ");
                    rs = change_input_to_int(temp);
                    temp = strtok(NULL, "\t\n, ");
                    rt = change_input_to_int(temp);
                    temp = strtok(NULL, "\t\n, ");              //lab1 꼴
                                                                //imm은 lab1 등으로 들어와서 주소 넘겨줘야함
                                                                //imm*4 = (가야할주소 - 현주소) - 4// 떨어진 칸수 만큼이니 절댓값 붙여야하나? 교수님이나 조교님께 여쭤볼것.
                    imm = returnadd(temp);
                    imm = imm - cur_addr - BYTES_PER_WORD;
                    imm /= BYTES_PER_WORD;
                }
                else if (!strcmp(label, "lw") || !strcmp(label, "sw")) {
                    temp = strtok(NULL, "\t\n, ");
                    rt = change_input_to_int(temp);
                    temp = strtok(NULL, "( ");
                    imm = change_input_to_int(temp);
                    temp = strtok(NULL, "\n) ");
                    rs = change_input_to_int(temp);
                }
                else {// addiu andi ori sltiu
                    temp = strtok(NULL, "\t\n, ");
                    rt = change_input_to_int(temp);
                    temp = strtok(NULL, "\t\n, ");
                    rs = change_input_to_int(temp);
                    temp = strtok(NULL, "\t\n, ");
//#if DEBUG
//    printf("++++++++++++HERE++++++++++++++ temp is %s\n", temp);
//#endif
                    imm = change_input_to_int(temp); //                                                           segmentation fault // sw가 rw로 오타나있었음..................
//#if DEBUG
//    printf("++++++++++++HERE++++++++++++++ imm is %d\n", imm);
//#endif
                }
                bits = num_to_bits(rs, 5);
                fputs(bits, output);        //5
                free(bits);
                bits = num_to_bits(rt, 5);
                fputs(bits, output);        //5
                free(bits);
                bits = num_to_bits(imm, 16);
                fputs(bits, output);        //16    총 32
                free(bits);

#if DEBUG
                printf("op:%s rs:$%d rt:$%d imm:0x%x\n",
                        op, rs, rt, imm);
#endif
                break;

            case 'J':
                /* blank */
                /*
                J format    op      add
                            6       26

                J type j, jal
                j   PC = jumpadd
                jal PC = jumpadd
                */
                temp = strtok(NULL, "\t\n, "); //lab1 꼴
                addr = returnadd(temp) / BYTES_PER_WORD;

                bits = num_to_bits(addr, 26);
                fputs(bits, output);    //26
                free(bits);

#if DEBUG
                printf("op:%s addr:%i\n", op, addr);
#endif
                break;

            default:
                break;
        }
        fprintf(output, "\n");

        cur_addr += BYTES_PER_WORD;
    }
}

/* Record .data section to output file */
void record_data_section(FILE *output)
{
    uint32_t cur_addr = MEM_DATA_START;
    char line[1024];

    /* Point to data segment stream */
    rewind(data_seg);

    /* Print .data section */
    while (fgets(line, 1024, data_seg) != NULL) {
        /* blank */
        char* temp = strtok(line, "\t\n, ");
        temp = strtok(NULL, "\t\n, "); //상수나 16진수 형태
        char* bits = num_to_bits(change_input_to_int(temp) , 32);
        fprintf(output, "%s\n", bits);
        free(bits);

#if DEBUG
        printf("0x%08x: ", cur_addr);
        printf("%s", line);
#endif
        cur_addr += BYTES_PER_WORD;
    }
}

/* Fill the blanks */
void make_binary_file(FILE *output)
{
#if DEBUG
    char line[1024] = {0};
    rewind(text_seg);
    /* Print line of text segment */
    while (fgets(line, 1024, text_seg) != NULL) {
        printf("%s",line);
    }
    printf("text section size: %d, data section size: %d\n",
            text_section_size, data_section_size);
#endif

    /* Print text section size and data section size */
    /* blank */
    //text data 순서
    char* bits = num_to_bits(text_section_size, 32);
    fprintf(output, "%s\n", bits);
    free(bits);
    bits = num_to_bits(data_section_size, 32);
    fprintf(output, "%s\n", bits);
    free(bits);

    /* Print .text section */
    record_text_section(output);

    /* Print .data section */
    record_data_section(output);
}

/* Fill the blanks */
void make_symbol_table(FILE* input)
{
    char line[1024] = { 0 };
    uint32_t address = 0;
    enum section cur_section = MAX_SIZE;

    /* Read each section and put the stream */
    while (fgets(line, 1024, input) != NULL) {
        char* temp;
        char _line[1024] = { 0 };
        strcpy(_line, line);
        temp = strtok(_line, "\t\n");

        /* Check section type */
        if (!strcmp(temp, ".data")) {
            /* blank */
            cur_section = DATA;
            data_seg = tmpfile(); //dats_seg도 채울것
                                  //임시로 만들었다가 프로그램 종료시 삭제되는 파일
            continue;
        }

        else if (!strcmp(temp, ".text")) {
            /* blank */
            cur_section = TEXT;
            address = 0; //data 공간차이 초기화
            text_seg = tmpfile();
            continue;
        }

        /* Put the line into each segment stream */
        if (cur_section == DATA) {
            /* blank */
            char* search = strchr(temp, ':');
            if (search) {
                symbol_t datasym;
                strcpy(datasym.name, temp);
                datasym.name[strlen(temp) - 1] = '\0';
                datasym.address = MEM_DATA_START + address; //0x1000 0000
                symbol_table_add_entry(datasym);

                //temp 재설정
                temp = strtok(NULL, "\n");
                fprintf(data_seg, "%s\n", temp);
            }
            else {
                fputs(line, data_seg);
            }
            data_section_size += BYTES_PER_WORD;
        }
        else if (cur_section == TEXT) {
            /* blank */
            char* search = strchr(temp, ':');
            if (search) {
                symbol_t textsym;
                strcpy(textsym.name, temp);
                textsym.name[strlen(temp) - 1] = '\0';
                textsym.address = MEM_TEXT_START + address; //0x0040 0000
                symbol_table_add_entry(textsym);
                continue;
                // address 더해지면 안됨
            }
            else {
                if (!strcmp(temp, "la")) {
                    temp = strtok(NULL, "\t\n, ");
                    char* temptemp = temp;
                    temp = strtok(NULL, "\t\n, ");
                    uint32_t labeladd = returnadd(temp); //0x1000 0000
//#if DEBUG
//    printf("%s label add is %08x\n", temp,  labeladd);
//#endif
                    fprintf(text_seg, "\tlui\t%s, 0x%4x\n", temptemp, labeladd >> 16);
                    text_section_size += BYTES_PER_WORD;

                    if (labeladd & 0xFFFF) {
                        fprintf(text_seg, "\tori\t%s, %s, 0x%04x\n", temptemp, temptemp, labeladd & 0xFFFF);
                        //중간값 더해주는 temptemp 필요
                        text_section_size += BYTES_PER_WORD;
                        address += BYTES_PER_WORD;
                    }
                }
                else {
                    fputs(line, text_seg);
                    text_section_size += BYTES_PER_WORD;
                }
            }
        }

        address += BYTES_PER_WORD;
    }/*
#if debug
    for (int i = 0; i < symbol_table_cur_index; i++) {
        printf("%s\t%08x\n", symbol_table[i].name, symbol_table[i].address);
    }
#endif*/

}
        

/******************************************************
 * Function: main
 *
 * Parameters:
 *  int
 *      argc: the number of argument
 *  char*
 *      argv[]: array of a sting argument
 *
 * Return:
 *  return success exit value
 *
 * Info:
 *  The typical main function in C language.
 *  It reads system arguments from terminal (or commands)
 *  and parse an assembly file(*.s).
 *  Then, it converts a certain instruction into
 *  object code which is basically binary code.
 *
 *******************************************************/

int main(int argc, char* argv[])
{
    FILE *input, *output;
    char *filename;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <*.s>\n", argv[0]);
        fprintf(stderr, "Example: %s sample_input/example?.s\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Read the input file */
    input = fopen(argv[1], "r");
    if (input == NULL) {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }

    /* Create the output file (*.o) */
    filename = strdup(argv[1]); // strdup() is not a standard C library but fairy used a lot.
    if(change_file_ext(filename) == NULL) {
        fprintf(stderr, "'%s' file is not an assembly file.\n", filename);
        exit(EXIT_FAILURE);
    }

    output = fopen(filename, "w");
    if (output == NULL) {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }

    /******************************************************
     *  Let's complete the below functions!
     *
     *  make_symbol_table(FILE *input)
     *  make_binary_file(FILE *output)
     *  ├── record_text_section(FILE *output)
     *  └── record_data_section(FILE *output)
     *
     *******************************************************/
    make_symbol_table(input);
    make_binary_file(output);

    fclose(input);
    fclose(output);

    return 0;
}
