#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N 10000

typedef unsigned char* String;	// define String = unsigned char * 

int loadText(String path, String t, int size) { // load text from file
    FILE *fp;
    unsigned char s[1000];
    int i = 0, j;
    if (path[0] == 34) { i = 1; } 
    for (j = 0; ; j++) {
        if (path[i + j] == 0 || path[j + i] == 34)
            break;
        s[j] = path[i + j];
    }
    s[j] = 0;
    fp = fopen(s, "rt"); 
    if (fp == 0) {  
        printf("fopen error : %s\n", path);
        return -1; 
    }
    i = fread(t, 1, size - 1, fp);
    fclose(fp);
    t[i] = 0; //  \n
    return 0; 
}

/***************************************************************************/

#define MAX_TC  255 // max token code
String ts[MAX_TC + 1]; // token string
int tl[MAX_TC + 1]; // token length
unsigned char tcBuf[(MAX_TC + 1) * 10]; // 1 token about 10 bytes
int tcs = 0, tcb = 0;

int var[MAX_TC + 1];	// variable

int getTn(String s, int len) { // get token number 
    int i;
    // find token number
    for (i = 0; i < tcs; i++) { 
        if (len == tl[i] && strncmp(s, ts[i], len) == 0)
            break; 
    }
    if (i == tcs) {
        if (tcs >= MAX_TC) {
            printf("too many tokens\n");
            exit(1);
        }
        // create new token number
        /********** malloc *************/
        strncpy(&tcBuf[tcb], s, len); 
        tcBuf[tcb + len] = 0; 
        ts[i] = &tcBuf[tcb];
        tl[i] = len;
        tcb += len + 1;
        /*******************************/
        tcs++;
        var[i] = strtol(ts[i], 0, 0);	// set 0
    }
    return i;
}

/***************************************************************************/

int isAlphabetOrNumber(unsigned char c)	{ // available variable number
    if ('0' <= c && c <= '9') return 1;
    if ('a' <= c && c <= 'z') return 1;
    if ('A' <= c && c <= 'Z') return 1;
    if (c == '_') return 1;
    return 0;
}

/***************************************************************************/

int lexer(String s, int tc[])	{ // program to token
    int i = 0, j = 0, len; // i:今s[]のどこを読んでいるか、j:これまでに変換したトークン列の長さ.
    while(1) {
        if (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r') {	// スペース、タブ、改行.
            i++;
            continue;
        }
        if (s[i] == 0)	// end file
            return j;
        len = 0;
        if (strchr("(){}[];,", s[i]) != 0) {	
            len = 1;
        } else if (isAlphabetOrNumber(s[i])) {  
            while (isAlphabetOrNumber(s[i + len]))
                len++;
        } else if (strchr("=+-*/!%&~|<>?:.#", s[i]) != 0) {  
            while (strchr("=+-*/!%&~|<>?:.#", s[i + len]) != 0 && s[i + len] != 0)
                len++;
        } else {
            printf("syntax error : %.10s\n", &s[i]);
            exit(1);
        }
        tc[j] = getTn(&s[i], len);
        i += len;
        j++;
    }
}

/***************************************************************************/

int tc[N];	// token code

int run(String s)
{
    clock_t t0 = clock();
    int pc, pc1;
    pc1 = lexer(s, tc);
    /********* debug ***********/
    for(int i=0; i < pc1; i++){
        printf("%d %s\n", tc[i], ts[tc[i]]);
    }
    /***************************/
    tc[pc1++] = getTn(";", 1);	// add ;
    tc[pc1] = tc[pc1+1] = tc[pc1+2] = tc[pc1+3] = getTn(".", 1);	// err to .
    int semi = getTn(";", 1);
    for (pc = 0; pc < pc1; pc++) { // find and register label
        if (tc[pc+1] == getTn(":", 1)) {
            var[tc[pc]] = pc+2; // store label
        }
    }
    // program start
    for (pc = 0; pc < pc1;) { 
        if (tc[pc+1] == getTn("=", 1) && tc[pc+3] == semi) { // substitute
            var[tc[pc]] = var[tc[pc + 2]];
        } else if (tc[pc+1] == getTn("=", 1) && tc[pc+3] == getTn("+", 1) && tc[pc+5] == semi) {  // sum
            var[tc[pc]] = var[tc[pc+2]] + var[tc[pc+4]];
        } else if (tc[pc+1] == getTn("=", 1) && tc[pc+3] == getTn("-", 1) && tc[pc+5] == semi) {  // sub 
            var[tc[pc]] = var[tc[pc+2]] - var[tc[pc+4]];
        } else if (tc[pc] == getTn("print", 5) && tc[pc+2] == semi) { // print.
            printf("%d\n", var[tc[pc+1]]);
        } else if (tc[pc+1] == getTn(":", 1)) {	// define label
            pc += 2; 
            continue;
        } else if (tc[pc] == getTn("goto", 4) && tc[pc+2] == semi) { // goto.
            pc = var[tc[pc+1]];
            continue;
        } else if (tc[pc] == getTn("if", 2) && tc[pc+1] == getTn("(", 1) && tc[pc+5] == getTn(")", 1) && tc[pc+6] == getTn("goto", 4) && tc[pc+8] == semi) {	// if (...) goto.
            int gpc = var[tc[pc+7]], v0 = var[tc[pc+2]], v1 = var[tc[pc+4]];
            if (tc[pc + 3] == getTn("!=", 2) && v0 != v1) { pc = gpc; continue; } 
            if (tc[pc + 3] == getTn("==", 2) && v0 == v1) { pc = gpc; continue; } 
            if (tc[pc + 3] == getTn("<",  1) && v0 <  v1) { pc = gpc; continue; } 
        } else if (tc[pc] == getTn("time", 4) && tc[pc+1] == semi) {
            printf("time: %.3f[sec]\n", (clock() - t0) / (double) CLOCKS_PER_SEC);
        } else if (tc[pc] == semi) {
        } else
            goto err;
        while (tc[pc] != semi)
            pc++;
        pc++; // jump semi
    }
    return 0;
err:
    printf("syntax error : %s %s %s %s\n", ts[tc[pc]], ts[tc[pc+1]], ts[tc[pc+2]], ts[tc[pc+3]]);
    return 1;
}

/***************************************************************************/

int main(int argc, const char **argv) {
    unsigned char txt[N];
    int i;
    if (argc >= 2) { // command line
        if (loadText((String) argv[1], txt, N) == 0) {
            run(txt);
        }
        exit(0);
    }
    while(1) { // Read-Eval-Print Loop.
        printf("\n>");
        fgets(txt, N, stdin);
        i = strlen(txt);
        if (txt[i - 1] == '\n') { // \n to null
            txt[i - 1] = 0;
        }
        if (strncmp(txt, "run ", 4) == 0) {  // run command
            if (loadText(&txt[4], txt, N) == 0) {
                run(txt);
            }
        } else if (strcmp(txt, "exit") == 0) { // exit command
            exit(0);
        } else {
            run(txt);
        }
    }
    
}
