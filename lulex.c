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

enum { TcSemi = 0, TcDot, TcWiCard, Tc0, Tc1, Tc2, Tc3, Tc4, Tc5, Tc6, Tc7, Tc8, TcEEq, TcNEq, TcLt, TcGe, TcLe, TcGt };

char tcInit[] = "; . !!* 0 1 2 3 4 5 6 7 8 == != < >= <= >";

// ppc is ptr pc1, wpc[] wildcard ptr token 
int phrCmp_tc[32 * 100], ppc1, wpc[9];

int phrCmp(int pid, String phr, int pc) {
    int i0 = pid * 32, i, i1, j;
    if(phrCmp_tc[i0 + 31] == 0) {
        i1 = lexer(phr, &phrCmp_tc[i0]);
        phrCmp_tc[i0 + 31] = i1;
    }
    i1 = phrCmp_tc[i0 + 31];
    for(i = 0; i < i1; i++) {
        if(phrCmp_tc[i0 + i] == TcWiCard) {
            i++;
            j = phrCmp_tc[i0 + i] - Tc0; // get number of next token
            wpc[j] = pc;
            pc++;
            continue;
        }
        if(phrCmp_tc[i0 + i] != tc[pc]) return 0; // not match
        pc++;
    }
    ppc1 = pc;
    return 1;
}

/***************************************************************************/

enum { OpCpy = 0, OpAdd, OpSub, OpPrint, OpGoto, OpJeq, OpJne, OpzJlt, OpJlt, OpJge, OpJle, OpJgt, OpTime, OpEnd };

int *ic[10000], **icq;

void putIc(int op, int *p1, int *p2, int *p3, int *p4) {
    icq[0] = (int *)op;
    icq[1] = p1;
    icq[2] = p2;
    icq[3] = p3;
    icq[4] = p4;
    icq += 5;
}

/***************************************************************************/

int compile(String s){
    int pc, pc1, i;
    int **icq1;
    pc1 = lexer(s, tc) ;
    tc[pc1++] = TcSemi;
    tc[pc1] = tc[pc1 + 1] = tc[pc1 + 2] = tc[pc1 + 3] = TcDot;
    icq = ic;
    for(pc = 0; pc < pc1; ) {
        if(phrCmp(1,"!!*0 = !!*1;", pc)){
            putIc(OpCpy, &var[tc[wpc[0]]], &var[tc[wpc[1]]], 0, 0);
        } else if (phrCmp(2, "!!*0 = !!*1 + !!*2;", pc)){
            putIc(OpAdd, &var[tc[wpc[0]]], &var[tc[wpc[1]]], &var[tc[wpc[2]]], 0);
        } else if (phrCmp(3, "!!*0 = !!*1 - !!*2;", pc)){
            putIc(OpSub, &var[tc[wpc[0]]], &var[tc[wpc[1]]], &var[tc[wpc[2]]], 0);
        } else if (phrCmp(4, "print !!*0;", pc)) {
            putIc(OpPrint, &var[tc[wpc[0]]], 0, 0, 0);
        } else if (phrCmp(5, "goto !!*0;", pc)) {
            putIc(OpGoto, &var[tc[wpc[0]]], 0, 0, 0);
        } else if (phrCmp(6, "if (!!*0 !!*1 !!*2) goto !!*3;", pc) && TcEEq <= tc[wpc[1]] && tc[wpc[1]] <= TcLt) {
            putIc(OpJeq + (tc[wpc[1]] - TcEEq), &var[tc[wpc[3]]], &var[tc[wpc[0]]], &var[tc[wpc[2]]], 0);
        } else if (phrCmp(7, "time;", pc)) {
            putIc(OpTime, 0, 0, 0, 0);
        } else if (phrCmp(8, ";", pc)) {
        } else {
            goto err;
        }
        pc = ppc1;
    }
    putIc(OpEnd, 0, 0, 0, 0);
    icq1 = icq;
    for(icq = ic; icq < icq1; icq += 5) {
        i = (int)icq[0];
        if (OpGoto <= i && i <= OpJgt) {
            icq[1] = (int *) (*icq[1] + ic);
        }
    }
    return icq1 - ic;
err:
    printf("syntax error : %s %s %s %s\n", ts[tc[pc]], ts[tc[pc + 1]], ts[tc[pc + 2]], ts[tc[pc + 3]]);
    return -1;
}

/***************************************************************************/

void exec(){
    clock_t t0 = clock();
    int **icp = ic;
    while(1){
        switch((int)icp[0]){
            case OpCpy:
                *icp[1] = *icp[2];
                icp += 5;
                continue;
            case OpAdd:
                *icp[1] = *icp[2] + *icp[3];
                icp += 5;
                continue;
            case OpSub:
                *icp[1] = *icp[2] - *icp[3];
                icp += 5;
                continue;
            case OpPrint:
                printf("%d\n", *icp[1]);
                icp += 5;
                continue;
            case OpGoto:
                icp = (int **) icp[1];
                continue;
            case OpJeq:
                if(*icp[2] == *icp[3]) {
                    icp = (int **) icp[1];
                    continue;
                }
                icp += 5;
                continue;
            case OpJne:
                if(*icp[2] != *icp[3]) {
                    icp = (int **) icp[1];
                    continue;
                }
                icp += 5;
                continue;
            case OpJlt:
                if(*icp[2] < *icp[3]) {
                    icp = (int **) icp[1];
                    continue;
                }
                icp += 5;
                continue;
            case OpJgt:
                if(*icp[2] > *icp[3]) {
                    icp = (int **) icp[1];
                    continue;
                }
                icp += 5;
                continue;
            case OpJle:
                if(*icp[2] <= *icp[3]) {
                    icp = (int **) icp[1];
                    continue;
                }
                icp += 5;
                continue;
            case OpJge:
                if(*icp[2] >= *icp[3]) {
                    icp = (int **) icp[1];
                    continue;
                }
                icp += 5;
                continue;
            case OpTime:
                printf("time : %.3f[sec]\n", (clock() - t0) / (double) CLOCKS_PER_SEC);
                icp += 5;
                continue;
            case OpEnd:
                return;
        }
    }
}


/***************************************************************************/

int run(String s) {
    if(compile(s) < 0){
        return 1;
    }
    exec();
    return 0;
}

/***************************************************************************/

int main(int argc, const char **argv) {
    unsigned char txt[N];
    int i;
    lexer(tcInit, tc);
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
