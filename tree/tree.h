#ifndef TREE_H_INCLUDED
#define TREE_H_INCLUDED

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define SEC_ON

#ifdef SEC_ON
#define TREE_VERIFY(tree)                                                                          \
  if (TreeVerify(tree) != OK) {                                                                    \
    TreeDump(tree);                                                                                \
    TreeFree(tree);                                                                                \
    printf("executed with errors! see dump file for details\n");                                   \
    exit(-1);                                                                                      \
  }
#else
#define TREE_VERIFY(tree)
#endif

//####################//

enum TreeErrs {
  OK,
  E_HASH_CORRUPTED,
  E_INIT_ERR,
  E_REALLOC_ERR,
  E_SEQ_CORRUPTED,
  E_SIZE_INVALID,
  E_LOG_DEAD,
};

enum Branches {
  Left,
  Right,
};

typedef struct _NodeData {
  int type;
  int value;
} Data;

typedef struct _Node {
  int  parent;
  int  branches[2];
  Data data;
} Node;

typedef struct _Tree {
  Node* nodes;
  int*  memmap;
  int   max;
  int   cur;
  int   root;
  int   free;

  enum TreeErrs err;

#ifdef SEC_ON
  long hash;
#endif
} Tree;

//####################//

Node* NodeAlloc();
int NodeInit(Node* node, const int parent, const int branch_l, const int branch_r, const Data data);
void NodeFree(Node* node);

Tree* TreeAlloc();
int   TreeInit(Tree* tree);
void  TreeFree(Tree* tree);
int   TreeResize(Tree* tree, const int sizeNew);
int   TreeSort(Tree* tree);
int   _TreeSort(Tree* tree, const int node, const int parent, const int branch, int* counter,
                Node* buf);

int TreeFind(Tree* tree, const int node, const Data data);

int TreeInsertNode(Tree* tree, const int parent, const int branch, Data* data);

int TreeCountSubtree(Tree* tree, const int node);
int _TreeCountSubtree(Tree* tree, const int node, int* counter);

Node* TreeCopySubtree(Tree* src, const int node);
int   _TreeCopySubtree(Tree* src, Node* dst, const int node, int* pos);

int TreeDeleteNode(Tree* tree, const int node);
int _TreeDeleteNode(Tree* tree, const int node);

int TreeChangeNode(Tree* tree, const int node, int* parentNew, int* branch_lNew, int* branch_rNew,
                   Data* dataNew);

int TreeGlueSubtree(Tree* tree, Node* subtree, int node, int branch, int nodesCount);
int _TreeGlueSubtree(Tree* tree, Node* subtree, int* index, int nodesCount, int node, int branch);

Tree* TreeRead(const char* pathname);
int   _TreeRead(Tree* tree, char** s);

int TreeGetGr(char** s, Tree* tree, const int parent, const int branch);
int TreeGetNode(char** s, Tree* tree, const int parent, const int branch);
int GetNumber(char** s);
int GetString(char** s);
int GetSpace(char** s);
int TreeWrite(Tree* tree, const char* pathname);
int _TreeWrite(Tree* tree, int node, int fd);

#ifdef SEC_ON
int  TreeVerify(Tree* tree);
void TreeDump(Tree* tree);
long TreeGetHash(Tree* tree);
#endif

#endif