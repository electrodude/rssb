#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>


// VM
void vm(int* mem, int start)
{
	int* pc  = &mem[0];
	int* acc = &mem[1];
	mem[2] = 0;

	*pc = start;
	*acc = 0;

	while (1)
	{
		int op = mem[*pc];
		printf("pc = %d, op = %d, acc = %d, ", *pc, op, *acc);

		switch (op)
		{
			case 3:
			{
				mem[op] = getchar();
				break;
			}
		}
		
		printf("*op = %d, ", mem[op]);
	
		if (op != 4)
		{
			mem[op] = *acc = mem[op] - *acc;
		}
		else
		{
			mem[op] = *acc;
		}


		(*pc) += *acc >= 0 ? 1 : 2;

		printf("acc = %d, pc = %d", *acc, *pc);

		switch (op)
		{
			case 2:
			{
				mem[op] = 0;
				break;
			}
			case 4:
			{
				printf(", output: %c (%d)", mem[op], mem[op]);
				putchar(mem[op]);
				break;
			}
		}

		printf("\n");

		usleep(100000);
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
		putchar(*p);
		symtab = symtab[*p].next;
		if (symtab == NULL)
		{
			printf("\nUnknown symbol: %p\n", s2);
			return -1;
		}
		p++;
	}
	putchar('\n');
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

	printf("operand_eval: ");

	switch (this->tp)
	{
		case INT:
		{
			printf("int: %d\n", this->val.val);
			return this->val.val;
		}
		case IDENT:
		{
			printf("label: %s\n", this->val.ident);
			return getsymbol(this->val.ident);
		}
		case BINOP:
		{
			printf("binop: %c\n", this->val.binop.op);
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
			return 0;
		}
	}
	printf("Error: unknown value type: %d\n", this->tp);
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
	cons* top;
} stack;

void stack_push(stack* this, void* v)
{
	cons* top = malloc(sizeof(cons));

	top->this = v;
	top->next = this->top;
	this->top = top;
}

void* stack_pop(stack* this)
{
	if (this->top == NULL)
	{
		return NULL;
	}

	void* val = this->top->this;
	cons* next = this->top->next;

	free(this->top);

	this->top = next;

	return val;
}

void* stack_peek(stack* this)
{
	if (this->top == NULL)
	{
		return NULL;
	}

	return this->top->this;
}
	

// assembler

stack vstack;

stack ostack;

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
	printf("fold: nextop = %c (0x%x)\n", nextop, nextop);

	char topop;

	while (precedence(nextop) < precedence(topop = stack_peek(&ostack)))
	{
		char op = stack_pop(&ostack);
		printf("fold: op %c (0x%x)\n", op, op);
		operand* rhs = op != 'n' ? stack_pop(&vstack) : NULL;
		operand* lhs = stack_pop(&vstack);
		stack_push(&vstack, binop_new(op, lhs, rhs));
	}

	if (nextop == 0 && stack_peek(&ostack) == '(')
	{
		printf("Error: unmatched left parentheses\n");
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

	addsymbol("ip", 0);
	addsymbol("acc", 1);
	addsymbol("zero", 2);
	addsymbol("in", 3);
	addsymbol("out", 4);

	int isinstr = 0;

	goto line;

newline:
	printf("\n");

	if (isinstr)
	{
	
		mem[pc] = stack_pop(&vstack);
		printf("%d: %d\n", pc, mem[pc]);

		pc++;
		if (pc >= memlen)
		{
			memlen *= 2;
			mem = realloc(mem, memlen*sizeof(int));
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
		case 'r' : p++; goto s;
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
	printf("label: %s\n", s);
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
	printf("rssb\n");
	isinstr = 1;
	{
		char stray = stack_peek(&ostack);
		if (stray != 0)
		{
			printf("Error: stray operator: %c\n", stray);
		}
	}
	{
		operand* stray = stack_peek(&vstack);
		if (stray != NULL)
		{
			printf("Error: stray value: %d\n", operand_eval(stray));
		}
	}

	//printf("gap: %c\n", *p);
	switch (*p)
	{
		case 0   : goto end;
		case ' ' :
		case '\t': goto expr;
	}

	goto error;

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
	printf("lparen: %c\n", *p);
	stack_push(&ostack, '(');
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
	printf("num: %s\n", s);
	stack_push(&vstack, int_new(atoi(s)));
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
	printf("ident: %s\n", s);
	stack_push(&vstack, ident_new(s));

	goto operator;

unm:
	printf("unm: %c\n", *p);
	stack_push(&ostack, 'n');
	p++;
	goto expr;

here:
	printf("here: %c\n", *p);
	stack_push(&vstack, int_new(pc));
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
			printf("operator: %c\n", *p);
			stack_push(&ostack, *p);
			p++; goto expr;
		}
		case ')' : goto rparen;
	}

	goto error;

rparen:
	printf("rparen: %c\n", *p);
	fold(')');
	char lp = stack_pop(&ostack);
	if (lp != '(')
	{
		printf("Error: unmatched right parentheses");
	}
	p++;

	goto operator;

error:
	printf("Parse error at %d\n", p-ps);

end:	; // silly compile error without the ;
	int* mem2 = malloc(pc*sizeof(int));
	for (int i=0; i<pc; i++)
	{
		mem2[i] = operand_eval(mem[i]);
		operand_kill(mem[i]);

		printf("%d: %d\n", i, mem2[i]);
	}
	for (int i=0; i<pc; i++)
	{
		printf("%d: %d\n", i, mem2[i]);
	}

	puts("done");
	return mem2;
}


// main

int main(int argc, char** argv)
{
	char s[65536];
	FILE* f = fopen(argv[1], "r");
	fread(s, 65536, 1, f);
	fclose(f);
	vm(assembler(s), 5);
}
