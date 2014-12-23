#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "acsm.h"

#define MEMASSERT(p,s) if(!p){fprintf(stderr,"ACSM-No Memory: %s!\n",s);exit(0);}

/*Define the number of the line,when match a keyword*/
extern int nline = 1;

/*
* Malloc the AC Memory
*/
static void *AC_MALLOC(int n)
{
	void *p;
	p = malloc(n); 

	return p;
}

/*
*Free the AC Memory
*/
static void AC_FREE(void *p)
{
	if (p)
		free(p);
}


/*
*    Simple QUEUE NODE
*/
typedef struct _qnode
{
	int state;
	struct _qnode *next;
}QNODE;

/*
*    Simple QUEUE Structure
*/
typedef struct _queue
{
	QNODE * head, *tail;
	int count;
}QUEUE;

/*
*Init the Queue
*/
static void queue_init(QUEUE * s)
{
	s->head = s->tail = 0;
	s->count = 0;
}


/*
*  Add Tail Item to queue
*/
static void queue_add(QUEUE * s, int state)
{
	QNODE * q;
	/*Queue is empty*/
	if (!s->head)
	{
		q = s->tail = s->head = (QNODE *)AC_MALLOC(sizeof (QNODE));
		/*if malloc failed,exit the problom*/
		MEMASSERT(q, "queue_add");
		q->state = state;
		q->next = 0; /*Set the New Node's Next Null*/
	}
	else
	{
		q = (QNODE *)AC_MALLOC(sizeof (QNODE));
		MEMASSERT(q, "queue_add");
		q->state = state;
		q->next = 0;
		/*Add the new Node into the queue*/
		s->tail->next = q;
		/*set the new node is the Queue's Tail*/
		s->tail = q;
	}
	s->count++;
}


/*
*  Remove Head Item from queue
*/
static int queue_remove(QUEUE * s)
{
	int state = 0;
	QNODE * q;
	/*Remove A QueueNode From the head of the Queue*/
	if (s->head)
	{
		q = s->head;
		state = q->state;
		s->head = s->head->next;
		s->count--;

		/*If Queue is Empty,After Remove A QueueNode*/
		if (!s->head)
		{
			s->tail = 0;
			s->count = 0;
		}
		/*Free the QueNode Memory*/
		AC_FREE(q);
	}
	return state;
}


/*
*Return The count of the Node in the Queue
*/
static int queue_count(QUEUE * s)
{
	return s->count;
}


/*
*Free the Queue Memory
*/
static void queue_free(QUEUE * s)
{
	while (queue_count(s))
	{
		queue_remove(s);
	}
}


/*
** Case Translation Table
*/
static unsigned char xlatcase[256];

/*
* Init the xlatcase Table,Trans alpha to UpperMode
* Just for the NoCase State
*/
static void init_xlatcase()
{
	int i;
	for (i = 0; i < 256; i++)
	{
		xlatcase[i] = toupper(i);
	}
}

/*
*Convert the pattern string into upper
*/
static void ConvertCaseEx(unsigned char *d, unsigned char *s, int m)
{
	int i;
	for (i = 0; i < m; i++)
	{
		d[i] = xlatcase[s[i]];
	}
}

/*
*  Add a pattern to the list of patterns terminated at this state.
*  Insert at front of list.
*/
static void AddMatchListEntry(ACSM_STRUCT * acsm, int state, ACSM_PATTERN * px)  //将模式添加到匹配管理
{
	ACSM_PATTERN * p;
	p = (ACSM_PATTERN *)AC_MALLOC(sizeof (ACSM_PATTERN));
	MEMASSERT(p, "AddMatchListEntry");
	memcpy(p, px, sizeof (ACSM_PATTERN));

	/*Add the new pattern to the pattern  list*/
	p->next = acsm->acsmStateTable[state].MatchList;
	acsm->acsmStateTable[state].MatchList = p;
}

/*
* Add Pattern States
*/
static void AddPatternStates(ACSM_STRUCT * acsm, ACSM_PATTERN * p)
{
	unsigned char *pattern;
	int state = 0, next, n;
	n = p->n; /*The number of alpha in the pattern string*/
	pattern = p->patrn;

	/*
	*  Match up pattern with existing states
	*/
	for (; n > 0; pattern++, n--)
	{
		next = acsm->acsmStateTable[state].NextState[*pattern];
		if (next == ACSM_FAIL_STATE)
			break;
		state = next;
	}

	/*
	*   Add new states for the rest of the pattern bytes, 1 state per byte
	*/
	for (; n > 0; pattern++, n--)
	{
		acsm->acsmNumStates++;
		acsm->acsmStateTable[state].NextState[*pattern] = acsm->acsmNumStates; //这里的NextState[*pattern]需要编写代码测试
		state = acsm->acsmNumStates;
	}
	/*Here,An accept state,just add into the MatchListof the state*/
	AddMatchListEntry(acsm, state, p);
}


/*
*   Build Non-Deterministic Finite Automata
*/
static void Build_NFA(ACSM_STRUCT * acsm)
{
	int r, s;
	int i;
	QUEUE q, *queue = &q;
	ACSM_PATTERN * mlist = 0;
	ACSM_PATTERN * px = 0;

	/* Init a Queue */
	queue_init(queue);

	/* Add the state 0 transitions 1st */
	/*1st depth Node's FailState is 0, fail(x)=0 */
	for (i = 0; i < ALPHABET_SIZE; i++)
	{
		s = acsm->acsmStateTable[0].NextState[i];
		if (s)
		{
			queue_add(queue, s);
			acsm->acsmStateTable[s].FailState = 0;
		}
	}

	/* Build the fail state transitions for each valid state */
	while (queue_count(queue) > 0)
	{
		r = queue_remove(queue);

		/* Find Final States for any Failure */
		for (i = 0; i < ALPHABET_SIZE; i++)
		{
			int fs, next;
			/*** Note NextState[i] is a const variable in this block ***/
			if ((s = acsm->acsmStateTable[r].NextState[i]) != ACSM_FAIL_STATE)
			{
				queue_add(queue, s);
				fs = acsm->acsmStateTable[r].FailState;

				/*
				*  Locate the next valid state for 'i' starting at s
				*/
				/**** Note the  variable "next" ****/
				/*** Note "NextState[i]" is a const variable in this block ***/
				while ((next = acsm->acsmStateTable[fs].NextState[i]) ==
					ACSM_FAIL_STATE)
				{
					fs = acsm->acsmStateTable[fs].FailState;
				}

				/*
				*  Update 's' state failure state to point to the next valid state
				*/
				acsm->acsmStateTable[s].FailState = next;
			}
			else
			{
				acsm->acsmStateTable[r].NextState[i] =
					acsm->acsmStateTable[acsm->acsmStateTable[r].FailState].NextState[i];
			}
		}
	}

	/* Clean up the queue */
	queue_free(queue);
}


/*
*   Build Deterministic Finite Automata from NFA
*/
static void Convert_NFA_To_DFA(ACSM_STRUCT * acsm)
{
	int r, s;
	int i;
	QUEUE q, *queue = &q;

	/* Init a Queue */
	queue_init(queue);

	/* Add the state 0 transitions 1st */
	for (i = 0; i < ALPHABET_SIZE; i++)
	{
		s = acsm->acsmStateTable[0].NextState[i];
		if (s)
		{
			queue_add(queue, s);
		}
	}

	/* Start building the next layer of transitions */
	while (queue_count(queue) > 0)
	{
		r = queue_remove(queue);

		/* State is a branch state */
		for (i = 0; i < ALPHABET_SIZE; i++)
		{
			if ((s = acsm->acsmStateTable[r].NextState[i]) != ACSM_FAIL_STATE)
			{
				queue_add(queue, s);
			}
			else
			{
				acsm->acsmStateTable[r].NextState[i] =
					acsm->acsmStateTable[acsm->acsmStateTable[r].FailState].NextState[i];
			}
		}
	}

	/* Clean up the queue */
	queue_free(queue);
}


/*
* Init the acsm DataStruct
*/
ACSM_STRUCT * acsmNew()
{
	ACSM_STRUCT * p;
	init_xlatcase();
	p = (ACSM_STRUCT *)AC_MALLOC(sizeof (ACSM_STRUCT));
	MEMASSERT(p, "acsmNew");
	if (p)
		memset(p, 0, sizeof (ACSM_STRUCT));
	return p;
}


/*
*   Add a pattern to the list of patterns for this state machine
*/
int acsmAddPattern(ACSM_STRUCT * p, unsigned char *pat, int n, int nocase)
{
	ACSM_PATTERN * plist;
	plist = (ACSM_PATTERN *)AC_MALLOC(sizeof (ACSM_PATTERN));
	MEMASSERT(plist, "acsmAddPattern");
	plist->patrn = (unsigned char *)AC_MALLOC(n + 1);
	memset(plist->patrn + n, 0, 1);
	ConvertCaseEx(plist->patrn, pat, n);
	plist->casepatrn = (unsigned char *)AC_MALLOC(n + 1);
	memset(plist->casepatrn + n, 0, 1);
	memcpy(plist->casepatrn, pat, n);
	plist->n = n;
	plist->nocase = nocase;
	plist->nmatch = 0;

	/*Add the pattern into the pattern list*/
	plist->next = p->acsmPatterns;
	p->acsmPatterns = plist;

	return 0;
}

/*
*   Compile State Machine
*/
int acsmCompile(ACSM_STRUCT * acsm)
{
	int i, k;
	ACSM_PATTERN * plist;

	/* Count number of states */
	acsm->acsmMaxStates = 1; /*State 0*/
	for (plist = acsm->acsmPatterns; plist != NULL; plist = plist->next) 
	{
		acsm->acsmMaxStates += plist->n; //求出所有模式的总和
	}

	acsm->acsmStateTable = (ACSM_STATETABLE *)AC_MALLOC(sizeof (ACSM_STATETABLE)* acsm->acsmMaxStates);
	MEMASSERT(acsm->acsmStateTable, "acsmCompile");
	memset(acsm->acsmStateTable, 0, sizeof (ACSM_STATETABLE)* acsm->acsmMaxStates);

	/* Initialize state zero as a branch */
	acsm->acsmNumStates = 0;

	/* Initialize all States NextStates to FAILED */
	for (k = 0; k < acsm->acsmMaxStates; k++)
	{
		for (i = 0; i < ALPHABET_SIZE; i++)
		{
			acsm->acsmStateTable[k].NextState[i] = ACSM_FAIL_STATE;
		}
	}

	/* This is very import */
	/* Add each Pattern to the State Table */
	for (plist = acsm->acsmPatterns; plist != NULL; plist = plist->next)
	{
		AddPatternStates(acsm, plist);
	}

	/* Set all failed state transitions which from state 0 to return to the 0'th state */
	for (i = 0; i < ALPHABET_SIZE; i++)
	{
		if (acsm->acsmStateTable[0].NextState[i] == ACSM_FAIL_STATE)
		{
			acsm->acsmStateTable[0].NextState[i] = 0;
		}
	}

	/* Build the NFA  */
	Build_NFA(acsm);

	/* Convert the NFA to a DFA */
	//Convert_NFA_To_DFA (acsm);

	return 0;
}


/*64KB Memory*/
static unsigned char Tc[64 * 1024];

/*
*   Search Text or Binary Data for Pattern matches
*/
int acsmSearch(ACSM_STRUCT * acsm, unsigned char *Tx, int n, void(*PrintMatch) (ACSM_PATTERN * pattern, ACSM_PATTERN * mlist, int nline, int index))
{
	int state;
	ACSM_PATTERN * mlist;
	unsigned char *Tend;
	ACSM_STATETABLE * StateTable = acsm->acsmStateTable;
	int nfound = 0; /*Number of the found(matched) patten string*/
	unsigned char *T;
	int index;

	/* Case conversion */
	ConvertCaseEx(Tc, Tx, n);
	T = Tc;
	Tend = T + n;

	for (state = 0; T < Tend; T++)
	{
		state = StateTable[state].NextState[*T];

		/* State is a accept state? */
		if (StateTable[state].MatchList != NULL)
		{
			for (mlist = StateTable[state].MatchList; mlist != NULL;
				mlist = mlist->next)
			{
				/*Get the index  of the Match Pattern String in  the Text*/
				index = T - mlist->n + 1 - Tc;

				//mlist->nmatch++;
				nfound++;
				PrintMatch(acsm->acsmPatterns, mlist, nline, index);
			}
		}
	}

	return nfound;
}


/*
*   Free all memory
*/
void acsmFree(ACSM_STRUCT * acsm)
{
	int i;
	ACSM_PATTERN * mlist, *ilist;
	for (i = 0; i < acsm->acsmMaxStates; i++)

	{
		if (acsm->acsmStateTable[i].MatchList != NULL)

		{
			mlist = acsm->acsmStateTable[i].MatchList;
			while (mlist)
			{
				ilist = mlist;
				mlist = mlist->next;
				AC_FREE(ilist);
			}
		}
	}
	AC_FREE(acsm->acsmStateTable);
}

/*
*   Print A Match String's Information
*/
void PrintMatch(ACSM_PATTERN * pattern, ACSM_PATTERN * mlist, int nline, int index)
{
	/* Count the Each Match Pattern */
	ACSM_PATTERN *temp = pattern;
	for (; temp != NULL; temp = temp->next)
	{
		if (!strcmp((char *)temp->patrn, (char *)mlist->patrn))
		{
			temp->nmatch++;
		}

	}

	if (mlist->nocase)
		fprintf(stdout, "Match KeyWord %s at %d line %d char\n", mlist->patrn, nline, index);
	else
		fprintf(stdout, "Match KeyWord %s at %d line %d char\n", mlist->casepatrn, nline, index);

}

/*
* Print Summary Information of the AC Match
*/
void PrintSummary(ACSM_PATTERN * pattern)
{
	ACSM_PATTERN * mlist = pattern;
	printf("\n### Summary ###\n");
	for (; mlist != NULL; mlist = mlist->next)
	{
		if (mlist->nocase)
			printf("%12s : %5d\n", mlist->patrn, mlist->nmatch);
		else
			printf("%12s : %5d\n", mlist->casepatrn, mlist->nmatch);
	}
}