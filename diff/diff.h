#ifndef DIFF_H_INCLUDED
#define DIFF_H_INCLUDED

#include "../tree/tree.h"

// ====================
// DSL
// being singletone, tree isn't needed as the parameter

#define BRANCH(node, branch) tree->nodes[node].branches[branch]
#define R(node) tree->nodes[node].branches[Right]
#define L(node) tree->nodes[node].branches[Left]
#define PARENT(node) tree->nodes[node].parent

#define ROOT tree->root

#define TYPE(node) tree->nodes[node].data.type
#define VALUE(node) tree->nodes[node].data.value

#define DELETE(node) TreeDeleteNode(tree, node)

#define CHANGE(node, data_new) TreeChangeNode(tree, node, NULL, NULL, NULL, &data_new)

#define INSERT(node, branch, data_new) TreeInsertNode(tree, node, branch, data_new)

#define D(node) DiffNode(tree, node)

// ====================
// DIFFERENTIATOR

enum Types {
  Variable,
  Number,
  Operator,
  Function,
};

enum Operators {
  Mul,
  Div,
  Sum,
  Sub,
  Pow,
};

enum Functions {
  Sin,
  Cos,
  Tan,
  Ctan,
  Asin,
  Acos,
  Atan,
  Actan,
  Ln,
  Log,
  Sqrt,
};

// ====================
// PARSING

Tree* DiffReadExpression(const char* pathname);
// general grammar of the expression
int GetTier4Expression(char** s, Tree* tree, int node);
int GetGr(char** s, Tree* tree, int node);
// sum, substraction, etc
int GetTier3Expression(char** s, Tree* tree, int node, int branch);
int GetSumSub(char** s, Tree* tree, int node, int branch);
// multiplication, division, etc
int GetTier2Expression(char** s, Tree* tree, int node, int branch);
int GetMulDiv(char** s, Tree* tree, int node, int branch);
// power, etc
int GetTier1Expression(char** s, Tree* tree, int node, int branch);
int GetPow(char** s, Tree* tree, int node, int branch);
// unitary / elementary expressions
int GetTier0Expression(char** s, Tree* tree, int node, int branch);
int GetVar(char** s, Tree* tree, int node, int branch);
int GetNum(char** s, Tree* tree, int node, int branch);
int GetPar(char** s, Tree* tree, int node, int branch);
int GetFunc(char** s, Tree* tree, int node, int branch);

// ====================
// DERIVATIVES

int DiffGetDerivative(Tree* tree);
int DiffNode(Tree* tree, const int node);
int DiffVariable(Tree* tree, const int node);
int DiffNumber(Tree* tree, const int node);
int DiffFunction(Tree* tree, const int node);
int DiffSin(Tree* tree, const int node);
int DiffCos(Tree* tree, const int node);
int DiffTan(Tree* tree, const int node);
int DiffCtan(Tree* tree, const int node);
int DiffAsin(Tree* tree, const int node);
int DiffAcos(Tree* tree, const int node);
int DiffAtan(Tree* tree, const int node);
int DiffActan(Tree* tree, const int node);
int DiffLog(Tree* tree, const int node);
int DiffLn(Tree* tree, const int node);
int DiffSqrt(Tree* tree, const int node);
int DiffOperator(Tree* tree, const int node);
int DiffMul(Tree* tree, const int node);
int DiffDiv(Tree* tree, const int node);
int DiffSumSub(Tree* tree, const int node);
int DiffPow(Tree* tree, const int node);

// ====================
// SIMPLIFICATION

int DiffSimplify(Tree* tree);
int _DiffSimplify(Tree* tree, const int node, int* flag);
int SimplifyOperator(Tree* tree, const int node, int* flag);
int SimplifyMul(Tree* tree, const int node, int* flag);
int SimplifyDiv(Tree* tree, const int node, int* flag);
int SimplifySum(Tree* tree, const int node, int* flag);
int SimplifySub(Tree* tree, const int node, int* flag);
int SimplifyFunction(Tree* tree, const int node, int* flag);
int SimplifySin(Tree* tree, const int node, int* flag);
int SimplifyCos(Tree* tree, const int node, int* flag);
int SimplifyTan(Tree* tree, const int node, int* flag);

// ====================
// PDF

void DiffInitPdf();
void DiffClosePdf();
void DiffPrintToPdf(const char* msg);
void DiffCompilePdf();
int  DiffPrintTree(Tree* tree);
int  _DiffPrintTree(int fd, Tree* tree, int node);
int  PrintVariable(const int fd, Tree* tree, const int node);
int  PrintNumber(const int fd, Tree* tree, const int node);
int  PrintOperator(const int fd, Tree* tree, const int node);
int  PrintSum(const int fd, Tree* tree, const int node);
int  PrintSub(const int fd, Tree* tree, const int node);
int  PrintMul(const int fd, Tree* tree, const int node);
int  PrintDiv(const int fd, Tree* tree, const int node);
int  PrintPow(const int fd, Tree* tree, const int node);
int  PrintFunction(const int fd, Tree* tree, const int node);

#endif
