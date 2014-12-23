/*
**   ACSMX.H
**
**
*/
#ifndef ACSMX_H
#define ACSMX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
*   Prototypes
*/
#define ALPHABET_SIZE    256     
#define MAXLEN 256

#define ACSM_FAIL_STATE   -1     

typedef struct _acsm_pattern {

	struct  _acsm_pattern *next;
	unsigned char         *patrn; //一个匹配模式 例如本例使用的test,会转为大写的
	unsigned char         *casepatrn; //原始的匹配模式，不想patrn中会将匹配的模式转为大写
	int      n; //模式的长度 例如 test为4个
	int      nocase;
	void   * id;
	int         nmatch;

} ACSM_PATTERN;


typedef struct  {

	/* Next state - based on input character */
	int      NextState[ALPHABET_SIZE];

	/* Failure state - used while building NFA & DFA  */
	int      FailState;

	/* List of patterns that end here, if any */
	ACSM_PATTERN *MatchList;

}ACSM_STATETABLE;


/*
* State machine Struct
*/
typedef struct {

	int acsmMaxStates; //求出所有模式的长度的和
	int acsmNumStates;

	ACSM_PATTERN    * acsmPatterns; //存储模式的列表
	ACSM_STATETABLE * acsmStateTable; //状态表

}ACSM_STRUCT;

/*
*   Prototypes
*/
ACSM_STRUCT * acsmNew();
int acsmAddPattern(ACSM_STRUCT * p, unsigned char * pat, int n, int nocase);
int acsmCompile(ACSM_STRUCT * acsm);
int acsmSearch(ACSM_STRUCT * acsm, unsigned char * T, int n, void (*Match) (ACSM_PATTERN * pattern, ACSM_PATTERN * mlist, int nline, int index));
void acsmFree(ACSM_STRUCT * acsm);
void PrintMatch(ACSM_PATTERN * pattern, ACSM_PATTERN * mlist, int nline, int index);
void PrintSummary(ACSM_PATTERN * pattern);

#endif