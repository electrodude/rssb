#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define VMDEBUG 0
#define ASMDEBUG 0
#define LIBDEBUG 0

// VM
void vm(int* mem, int start)
{
	int* pc  = &mem[0];
	int* acc = &mem[1];
	mem[2] = 0;

	*pc = start;
	*acc = 0;

	while (!(*pc == 2 && *acc == 1))
	{
		int op = mem[*pc];
#if VMDEBUG
		printf("pc = %d, op = %d, acc = %d, ", *pc, op, *acc);
#endif
		switch (op)
		{
			case 3:
			{
				mem[op] = getchar();
				break;
			}
		}


#if VMDEBUG
		printf("*op = %d, ", mem[op]);
#endif
		if (op != 4)
		{
			mem[op] = *acc = mem[op] - *acc;
		}
		else
		{
			mem[op] = *acc;
		}


		(*pc) += *acc >= 0 ? 1 : 2;
#if VMDEBUG
		printf("acc = %d, pc = %d", *acc, *pc);
#endif
		switch (op)
		{
			case 2:
			{
				mem[op] = 0;
				break;
			}
			case 4:
			{
#if VMDEBUG
				printf(", output: %c (%d)", mem[op], mem[op]);
#else
				putchar(mem[op]);
#endif
				break;
			}
		}
#if VMDEBUG
		printf("\n");

		usleep(100000);
#endif
	}
}


// symbol table

typedef union symtabentry
{
	union symtabentry* next;
	int this;
} symtabentry;

symtabentry symbols[256];

void addsymbol(char* p, int pc)
{
	symtabentry* symtab = symbols;

	while (*p != 0)
	{
		symtabentry* symtab2 = symtab[*p].next;
		if (symtab2 == NULL)
		{
			symtab2 = symtab[*p].next = calloc(256,sizeof(symtabentry));
		}
		symtab = symtab2;
		p++;
	}
	symtab[0].this = pc;
}

int getsymbol(char* p)
{
	char* s2 = p;

	symtabentry* symtab = symbols;

	//printf("getsymbol(\"%s\"): ", p);
	while (*p != 0)
	{
#if LIBDEBUG
		putchar(*p);
#endif
		symtab = symtab[*p].next;
		if (symtab == NULL)
		{
			printf("\nUnknown symbol: %s\n", s2);
			exit(1);
			return -1;
		}
		p++;
	}
#if LIBDEBUG
	putchar('\n');
#endif
	return symtab[0].this;
}

// expression
typedef struct operand operand;

typedef struct operand
{
	enum {INT, IDENT, BINOP} tp;
	union
	{
		int val;
		char* ident;
		struct
		{
			operand* operands[2];
			char op;
		} binop;
	} val;
} operand;

operand* int_new(int x)
{
	operand* this = malloc(sizeof(operand));

	this->tp = INT;
	this->val.val = x;

	return this;
}

operand* ident_new(char* p)
{
	operand* this = malloc(sizeof(operand));

	this->tp = IDENT;
	this->val.ident = p;

	return this;
}

operand* binop_new(char op, operand* lhs, operand* rhs)
{
	operand* this = malloc(sizeof(operand));

	this->tp = BINOP;
	this->val.binop.op = op;
	this->val.binop.operands[0] = lhs;
	this->val.binop.operands[1] = rhs;

	return this;
}

int operand_eval(operand* this)
{
	if (this == NULL)
	{
		return 0;
	}

#if LIBDEBUG
	printf("operand_eval: ");
#endif

	switch (this->tp)
	{
		case INT:
		{
#if LIBDEBUG
			printf("int: %d\n", this->val.val);
#endif
			return this->val.val;
		}
		case IDENT:
		{
#if LIBDEBUG
			printf("label: %s\n", this->val.ident);
#endif
			return getsymbol(this->val.ident);
		}
		case BINOP:
		{
#if LIBDEBUG
			printf("binop: %c\n", this->val.binop.op);
#endif
			int lhs = operand_eval(this->val.binop.operands[0]);
			int rhs = operand_eval(this->val.binop.operands[1]);
			switch (this->val.binop.op)
			{
				case '+':
				{
					return lhs + rhs;
				}
				case '-':
				{
					return lhs - rhs;
				}
				case '*':
				{
					return lhs * rhs;
				}
				case '/':
				{
					return lhs / rhs;
				}
				case 'n':
				{
					return -lhs;
				}
			}
			printf("Error: unknown op: %c\n", this->val.binop.op);
			exit(1);
			return 0;
		}
	}
	printf("Error: unknown value type: %d\n", this->tp);
	exit(1);
}

void operand_kill(operand* this)
{
	if (this == NULL)
	{
		return;
	}

	switch (this->tp)
	{
		case INT:
		{
			break;
		}
		case IDENT:
		{
			free(this->val.ident);
			break;
		}
		case BINOP:
		{
			operand_kill(this->val.binop.operands[0]);
			operand_kill(this->val.binop.operands[1]);
			break;
		}
		default:
		{
			printf("Error: unknown value type: %d\n", this->tp);
			//exit(1); // who cares about memory leaks? :)
			break;
		}
	}

	free(this);
}


// stack

typedef struct cons
{
	struct cons* next;
	void* this;
} cons;

typedef struct stack
{
	void** base;
	int top;
	int len;
} stack;

stack* stack_new()
{
	stack* this = malloc(sizeof(stack));

	this->len = 16;
	this->base = malloc(this->len*sizeof(void*));
	this->top = 0;

	return this;
}

void stack_push(stack* this, void* v)
{
	this->top++;

	if (this->top >= this->len)
	{
		this->len *= 2;
		this->base = realloc(this->base, this->len*sizeof(void*));
#if LIBDEBUG
		printf("resize stack to %d\n", this->len);
#endif
	}

	this->base[this->top] = v;
}

void* stack_pop(stack* this)
{
	if (this->top <= 0)
	{
		return NULL;
	}

	return this->base[this->top--];
}

void* stack_peek(stack* this)
{
	if (this->top <= 0)
	{
		return NULL;
	}

	return this->base[this->top];
}


// assembler

stack* vstack;

stack* ostack;

char* tok2str(char* start, char* end)
{
	char* s = malloc(end - start + 1);

	strncpy(s, start, end - start);

	s[end-start] = 0;

	return s;
}

int precedence(char op)
{
	switch (op)
	{
		case 'n' : return 6;
		case '/' : return 5;
		case '*' : return 4;
		case '-' : return 3;
		case '+' : return 2;
		case '(' : return INT_MIN;
		case ')' : return INT_MIN;
		default  : return INT_MIN;
	}
}

void fold(char nextop)
{
#if ASMDEBUG
	printf("fold: nextop = %c (0x%x)\n", nextop, nextop);
#endif

	char topop;

	while (precedence(nextop) < precedence(topop = stack_peek(ostack)))
	{
		char op = stack_pop(ostack);
#if ASMDEBUG
		printf("fold: op %c (0x%x)\n", op, op);
#endif
		operand* rhs = op != 'n' ? stack_pop(vstack) : NULL;
		operand* lhs = stack_pop(vstack);
		stack_push(vstack, binop_new(op, lhs, rhs));
	}

	if (nextop == 0 && stack_peek(ostack) == '(')
	{
		printf("Error: unmatched left parentheses\n");
		exit(1);
	}
}

int* assembler(char* p)
{
	char* ps = p;

	char* ts = p;

	//char* s;

	int pc = 5;

	int memlen = 64;
	operand** mem = malloc(memlen*sizeof(operand*));
	mem[0] = int_new(0);
	mem[1] = int_new(0);
	mem[2] = int_new(0);
	mem[3] = int_new(0);
	mem[4] = int_new(0);

	addsymbol("ip", 0);
	addsymbol("acc", 1);
	addsymbol("zero", 2);
	addsymbol("in", 3);
	addsymbol("out", 4);

	int isinstr = 0;

	goto line;

newline:
#if ASMDEBUG
	printf("\n");
#endif

	if (isinstr)
	{
		mem[pc] = stack_pop(vstack);

		pc++;
		if (pc >= memlen)
		{
			memlen *= 2;
			mem = realloc(mem, memlen*sizeof(operand*));
		}
	}

	isinstr = 0;

	goto line;

line:
	//printf("line\n", *p);
	//printf("line: %c\n", *p);
	switch (*p)
	{
		case 0   : goto end;
		case ';' : p++; goto comment;
		case 'r' : ts = p; p++; goto s;
		case ' ' :
		case '\t':
		case '\n':
		case '\r': p++; goto line;
	}

	goto label;

comment:
	//printf("comment: %c\n", *p);
	switch (*p)
	{
		case 0   : goto end;
		case '\n':
		case '\r': p++; goto newline;
	}

	p++; goto comment;

label:
	ts = p;
label_l:
	//printf("label: %c\n", *p);
	switch (*p)
	{
		case 0   : goto error;
	}
	if ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '_') { p++; goto label_l; }

	char* s = tok2str(ts, p);
	addsymbol(s, pc);
#if ASMDEBUG
	printf("label: %s\n", s);
#endif
	free(s);

	goto line;

s:
	//printf("s: %c\n", *p);
	switch (*p)
	{
		case 0   : goto end;
		case 's' : p++; goto s2;
	}

	goto label;

s2:
	//printf("s2: %c\n", *p);
	switch(*p)
	{
		case 0   : goto end;
		case 's' : p++; goto b;
	}

	goto label;

b:
	//printf("b: %c\n", *p);
	switch (*p)
	{
		case 0   : goto end;
		case 'b' : p++; goto gap;
	}

	goto label;

gap:
	//printf("gap: %c\n", *p);
	switch (*p)
	{
		case 0   : goto end;
		case ' ' :
		case '\t': goto expr_entry;
	}

	goto label_l;

expr_entry:
#if ASMDEBUG
	printf("rssb\n");
#endif
	isinstr = 1;
	{
		char stray = stack_peek(ostack);
		if (stray != 0)
		{
			printf("Error: stray operator: %c\n", stray);
			exit(1);
		}
	}
	{
		operand* stray = stack_peek(vstack);
		if (stray != NULL)
		{
			printf("Error: stray value: %d\n", operand_eval(stray));
			exit(1);
		}
	}

expr:
	//printf("expr: %c\n", *p);
	switch (*p)
	{
		case 0   : goto end;
		case ' ' :
		case '\t': p++; goto expr;
		case '$' : goto here;
		case '-' : goto unm;
		case '(' : goto lparen;
	}

	if (*p >= '0' && *p <= '9') goto num;
	if ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || *p == '_') goto ident;

	goto error;

lparen:
#if ASMDEBUG
	printf("lparen: %c\n", *p);
#endif
	stack_push(ostack, '(');
	p++;

	goto expr;

num:
	ts = p;
num_l:
	//printf("num: %c\n", *p);
	switch (*p)
	{
		case 0   : goto error;
	}

	if (*p >= '0' && *p <= '9') { p++; goto num_l; }

	s = tok2str(ts, p);
#if ASMDEBUG
	printf("num: %s\n", s);
#endif
	stack_push(vstack, int_new(atoi(s)));
	free(s);

	goto operator;

ident:
	ts = p;
ident_l:
	//printf("ident: %c\n", *p);
	switch (*p)
	{
		case 0   : goto error;
	}
	if ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '_') { p++; goto ident_l; }

	s = tok2str(ts, p);
#if ASMDEBUG
	printf("ident: %s\n", s);
#endif
	stack_push(vstack, ident_new(s));

	goto operator;

unm:
#if ASMDEBUG
	printf("unm: %c\n", *p);
#endif
	stack_push(ostack, 'n');
	p++;
	goto expr;

here:
#if ASMDEBUG
	printf("here: %c\n", *p);
#endif
	stack_push(vstack, int_new(pc));
	p++;
	goto operator;

operator:
	//printf("operator: %c\n", *p);
	switch (*p)
	{
		case 0   : goto error;
		case ' ' :
		case '\t': p++; goto operator;
		case '\n':
		case '\r': fold(0); p++; goto newline;
		case ';' : fold(0); p++; goto comment;
		case '+' :
		case '-' :
		case '*' :
		case '/' :
		{
			fold(*p);
#if ASMDEBUG
			printf("operator: %c\n", *p);
#endif
			stack_push(ostack, *p);
			p++; goto expr;
		}
		case ')' : goto rparen;
	}

	goto error;

rparen:
#if ASMDEBUG
	printf("rparen: %c\n", *p);
#endif
	fold(')');
	char lp = stack_pop(ostack);
	if (lp != '(')
	{
		printf("Error: unmatched right parentheses");
		exit(1);
	}
	p++;

	goto operator;

error:
	printf("Parse error at %d\n", p-ps);
	exit(1);

end:	; // silly compile error without the ;
	int* mem2 = malloc(pc*sizeof(int));
	for (int i=0; i<pc; i++)
	{
		mem2[i] = operand_eval(mem[i]);
		operand_kill(mem[i]);

#if ASMDEBUG
		printf("%d: %d\n", i, mem2[i]);
#endif
	}
	for (int i=0; i<pc; i++)
	{
		printf("%d: %d\n", i, mem2[i]);
	}

#if ASMDEBUG
	puts("done");
#endif
	return mem2;
}


// main

int main(int argc, char** argv)
{
	vstack = stack_new();
	ostack = stack_new();

	char s[65536];
	FILE* f = fopen(argv[1], "r");
	fread(s, 65536, 1, f);
	fclose(f);
	vm(assembler(s), 5);
}
