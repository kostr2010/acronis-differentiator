#include "./diff.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../tree/tree.h"
#include "../utils/assertm.h"
#include "../utils/log.h"

const int PRECISION = 1000;

Tree* DiffReadExpression(const char* pathname) {
  LOG_LVL_DIFF_INIT();

  Tree* tree = TreeAlloc();

  assertm(TreeInit(tree) == 0, "[DiffReadExpression] unable to init tree");

  int fd = 0;
  if ((fd = open(pathname, O_RDONLY)) == -1) {
    LOG_LVL_DIFF_ERROR("unable to open source file <%s>!\n", pathname);
    return NULL;
  }

  int len = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  char* buf = calloc(len, sizeof(char));

  if (read(fd, buf, len) != len) {
    LOG_LVL_DIFF_ERROR("unable to read from source file <%s>!\n", pathname);

    free(buf);
    TreeFree(tree);

    return NULL;
  }

  char* s = buf;

  // FIXME: fix this hack
  Data data_new = {9, 99};
  INSERT(tree->free, Left, data_new);

  if (GetTier4Expression(&s, tree, tree->root) == -1) {
    LOG_LVL_DIFF_ERROR("error while reading tier 4 expression at pos %ld!\n", s - buf);

    free(buf);
    TreeFree(tree);

    return NULL;
  }

  free(buf);

  return tree;
}

// ====================
// PARSING
// from here on, I'll try to use as many abstract language when manipulating tree as possible, as
// all needed interfase should be provided by DSL

int GetTier4Expression(char** s, Tree* tree, int node) {
  GetSpace(s);

  if (GetGr(s, tree, node) == -1) {
    return -1;
  }

  return 0;
}

int GetGr(char** s, Tree* tree, int node) {
  // FIXME: dirty hack
  if (GetTier3Expression(s, tree, node, Left) == -1) {
    LOG_LVL_DIFF_FAILURE(
        "expression identified as empty line (can't read even tier 3 expression)!\n");
  }

  GetSpace(s);

  if (**s != '\n' && **s != '\0' && **s != EOF) {
    LOG_LVL_DIFF_ERROR("unexpected end symbol <%c>!\n", **s);
    return -1;
  }

  return 0;
}

int GetTier3Expression(char** s, Tree* tree, int node, int branch) {
  GetSpace(s);

  if (GetSumSub(s, tree, node, branch) == -1) {
    return -1;
  }

  return 0;
}

int GetSumSub(char** s, Tree* tree, int node, int branch) {
  if (GetTier2Expression(s, tree, node, branch) == -1) {
    LOG_LVL_DIFF_FAILURE("unable to read tier 2 expression!\n");
    return -1;
  }

  int node_ = BRANCH(node, branch);

  GetSpace(s);

  while (**s == '+' || **s == '-') {
    int sign = (**s == '+') ? (Sum) : (Sub);

    (*s)++;

    Node* cpy     = TreeCopySubtree(tree, node_);
    int   n_nodes = TreeCountSubtree(tree, node_);
    DELETE(L(node_));
    DELETE(R(node_));

    Data data_new = {Operator, sign};

    CHANGE(node_, data_new);

    if (TreeGlueSubtree(tree, cpy, node_, Left, n_nodes) == -1) {
      LOG_LVL_DIFF_ERROR("unable to glue subtree!\n");
      return -1;
    }
    free(cpy);

    GetSpace(s);

    if (GetTier2Expression(s, tree, node_, Right) == -1) {
      LOG_LVL_DIFF_ERROR(
          "GetTier2Expression executed with errors after successfully reading sign!\n");
      return -1;
    }

    GetSpace(s);

    switch (sign) {
    case Sum:
      node_ = R(node_);
      break;
    case Sub:
      node_ = L(node_);
      break;
    }
  }

  return 0;
}

int GetTier2Expression(char** s, Tree* tree, int node, int branch) {
  GetSpace(s);

  if (GetMulDiv(s, tree, node, branch) == -1) {
    return -1;
  }

  return 0;
}

int GetMulDiv(char** s, Tree* tree, int node, int branch) {
  if (GetTier1Expression(s, tree, node, branch) == -1) {
    LOG_LVL_DIFF_ERROR("GetTier1Expression executed with eror!\n");
    return -1;
  }

  int node_ = BRANCH(node, branch);

  GetSpace(s);

  while (**s == '*' || **s == '/') {
    int sign = (**s == '*') ? (Mul) : (Div);

    *s += sizeof(char);

    Node* cpy     = TreeCopySubtree(tree, node_);
    int   n_nodes = TreeCountSubtree(tree, node_);
    DELETE(L(node_));
    DELETE(R(node_));

    Data data = {Operator, sign};

    CHANGE(node_, data);

    if (TreeGlueSubtree(tree, cpy, node_, Left, n_nodes) == -1) {
      LOG_LVL_DIFF_ERROR("unable to glue subtree!\n");
      return -1;
    }

    free(cpy);

    GetSpace(s);

    if (GetTier1Expression(s, tree, node_, Right) == -1) {
      LOG_LVL_DIFF_ERROR(
          "GetTier1Expression executed with errors after successfully reading sign!\n");
      return -1;
    }

    GetSpace(s);

    switch (sign) {
    case Mul:
      node_ = R(node_);
      break;
    case Div:
      node_ = L(node_);
      break;
    }
  }

  return 0;
}

int GetTier1Expression(char** s, Tree* tree, int node, int branch) {
  GetSpace(s);

  if (GetPow(s, tree, node, branch) == -1) {
    return -1;
  }

  return 0;
}

int GetPow(char** s, Tree* tree, int node, int branch) {
  if (GetTier0Expression(s, tree, node, branch) == -1) {
    LOG_LVL_DIFF_ERROR("GetTier0Expression executed with error!\n");
    return -1;
  }

  int node_ = BRANCH(node, branch);

  GetSpace(s);

  while (**s == '^') {
    *s += sizeof(char);

    Node* cpy     = TreeCopySubtree(tree, node_);
    int   n_nodes = TreeCountSubtree(tree, node_);
    DELETE(L(node_));
    DELETE(R(node_));

    Data data = {Operator, Pow};

    CHANGE(node_, data);

    if (TreeGlueSubtree(tree, cpy, node_, Left, n_nodes) == -1) {
      LOG_LVL_DIFF_ERROR("unable to glue subtree!\n");
      return -1;
    }

    free(cpy);

    GetSpace(s);

    if (GetTier0Expression(s, tree, node_, Right) == -1) {
      LOG_LVL_DIFF_ERROR(
          "GetTier0Expression executed with errors after successfully reading sign!\n");
      return -1;
    }

    GetSpace(s);

    node_ = R(node_);
  }

  return 0;
}

int GetTier0Expression(char** s, Tree* tree, int node, int branch) {
  GetSpace(s);

  if (GetVar(s, tree, node, branch) != -1) {
    // printf("  [GetTier0Expression] variable was read!\n");
    return 0;
  }

  if (GetNum(s, tree, node, branch) != -1) {
    // printf("  [GetTier0Expression] number was read!\n");
    return 0;
  }

  if (GetPar(s, tree, node, branch) != -1) {
    // printf("  [GetTier0Expression] parenthesis was read!\n");
    return 0;
  }

  if (GetFunc(s, tree, node, branch) != -1) {
    // printf("  [GetTier0Expression] function was read!\n");
    return 0;
  }

  LOG_LVL_DIFF_ERROR("no valid tier 0 expression was read!\n");
  return -1;
}

int GetVar(char** s, Tree* tree, int node, int branch) {
  if (**s == 'x') {
    *s += sizeof(char);
    Data data_new = {Variable, 'x'};

    INSERT(node, branch, data_new);

    return 0;
  }
  // TODO: add multiple variables

  return -1;
}

int GetNum(char** s, Tree* tree, int node, int branch) {
  int val = 0;

  if (strncmp(*s, "pi", 2) == 0) {
    val = (int)(3.14 * PRECISION);
    *s += 2 * sizeof(char);
  } else if (strncmp(*s, "e", 1) == 0) {
    val = (int)(2.73 * PRECISION);
    *s += sizeof(char);
  } else if (isdigit(**s) == 0 && **s != '-' && **s != '+') {
    return -1;
  } else {
    val = (int)(atof(*s) * PRECISION);
  }

  // skipping what we just have read
  if (**s == '-' || **s == '+')
    **s += sizeof(char);

  while (isdigit(**s) != 0)
    *s += 1;

  if (**s == '.') {
    *s += 1;
    while (isdigit(**s) != 0)
      *s += 1;
  }

  Data data_new = {Number, val};
  INSERT(node, branch, data_new);

  return 0;
}

int GetPar(char** s, Tree* tree, int node, int branch) {
  if (**s == '(') {
    *s += sizeof(char);
    GetSpace(s);

    if (GetTier3Expression(s, tree, node, branch) == -1) {
      LOG_LVL_DIFF_ERROR("invalid expression in parenthsis!\n");
      return -1;
    }

    GetSpace(s);

    if (**s == ')') {
      *s += sizeof(char);

      return 0;
    } else {
      LOG_LVL_DIFF_ERROR("no closing parenthesis detected!\n");
      return -1;
    }
  }

  return -1;
}

int GetFunc(char** s, Tree* tree, int node, int branch) {
  Data data_new = {Function, -1};

  if (strncmp(*s, "sin", 3) == 0) {
    data_new.value = Sin;

    *s += 3 * sizeof(char);
  } else if (strncmp(*s, "cos", 3) == 0) {
    data_new.value = Cos;

    *s += 3 * sizeof(char);
  } else if (strncmp(*s, "tan", 3) == 0) {
    data_new.value = Tan;

    *s += 3 * sizeof(char);
  } else if (strncmp(*s, "ctan", 4) == 0) {
    data_new.value = Ctan;

    *s += 4 * sizeof(char);
  } else if (strncmp(*s, "asin", 4) == 0) {
    data_new.value = Asin;

    *s += 4 * sizeof(char);
  } else if (strncmp(*s, "acos", 4) == 0) {
    data_new.value = Acos;

    *s += 4 * sizeof(char);
  } else if (strncmp(*s, "atan", 4) == 0) {
    data_new.value = Atan;

    *s += 4 * sizeof(char);
  } else if (strncmp(*s, "actan", 5) == 0) {
    data_new.value = Actan;

    *s += 5 * sizeof(char);
  } else if (strncmp(*s, "ln", 2) == 0) {
    data_new.value = Ln;

    *s += 2 * sizeof(char);
  } else if (strncmp(*s, "log", 3) == 0) {
    data_new.value = Log;

    *s += 3 * sizeof(char);
  } else if (strncmp(*s, "sqrt", 4) == 0) {
    data_new.value = Sqrt;

    *s += 4 * sizeof(char);
  } else {
    LOG_LVL_DIFF_ERROR("no viable function was read!\n");
    return -1;
  }

  INSERT(node, branch, data_new);

  int node_ = BRANCH(node, branch);

  if (GetTier0Expression(s, tree, node_, Left) == -1) {
    LOG_LVL_DIFF_ERROR("unable to interpret given argument!\n");
    return -1;
  }

  return 0;
}

//=====================
// DERIVATIVES

int DiffGetDerivative(Tree* tree) {
  if (DiffSimplify(tree) == -1) {
    LOG_LVL_DIFF_FAILURE("unable to simplify!\n");
    return -1;
  }

  if (D(L(ROOT)) == -1) {
    LOG_LVL_DIFF_ERROR("unable to differentiate given tree!\n");
    return -1;
  }

  if (DiffSimplify(tree) == -1) {
    LOG_LVL_DIFF_FAILURE("unable to simplify!\n");
    return -1;
  }

  return 0;
}

int DiffNode(Tree* tree, const int node) {
  switch (TYPE(node)) {
  case Variable:
    if (DiffVariable(tree, node) == -1) {
      return -1;
    }
    break;
  case Number:
    if (DiffNumber(tree, node) == -1) {
      return -1;
    }
    break;
  case Function:
    if (DiffFunction(tree, node) == -1) {
      return -1;
    }
    break;
  case Operator:
    if (DiffOperator(tree, node) == -1) {
      return -1;
    }
    break;
  default:
    LOG_LVL_DIFF_ERROR("invalid node type given! %d\n", TYPE(node));
    return -1;
    break;
  }

  return 0;
}

int DiffVariable(Tree* tree, const int node) {
  Data data_new = {Number, 1 * PRECISION};

  CHANGE(node, data_new);

  return 0;
}

int DiffNumber(Tree* tree, const int node) {
  Data data_new = {Number, 0};

  CHANGE(node, data_new);

  return 0;
}

int DiffFunction(Tree* tree, const int node) {
  // copying function's and argument's subtrees, clearing node's subtree;
  Node* cpy_args = TreeCopySubtree(tree, L(node));
  int   n_args   = TreeCountSubtree(tree, L(node));

  Node* cpy_func = TreeCopySubtree(tree, node);
  int   n_func   = TreeCountSubtree(tree, node);

  TreeDeleteNode(tree, L(node));
  TreeDeleteNode(tree, R(node));

  // replacing function's node with multiplication of func and it's argument, freeing temporary
  // buffers
  Data data_new = {Operator, Mul};
  CHANGE(node, data_new);

  TreeGlueSubtree(tree, cpy_func, node, Left, n_func);
  TreeGlueSubtree(tree, cpy_args, node, Right, n_args);

  free(cpy_args);
  free(cpy_func);

  // getting func's derivative. these functions only change function's node, f.ex: sin(nx) ->
  // cos(nx); cos(nx) -> -1 * sin(nx)
  switch (VALUE(L(node))) {
  case Sin:
    if (DiffSin(tree, L(node)) == -1) {
      return -1;
    }
    break;
  case Cos:
    if (DiffCos(tree, L(node)) == -1) {
      return -1;
    }
    break;
  case Tan:
    if (DiffTan(tree, L(node)) == -1) {
      return -1;
    }
    break;
  case Ctan:
    if (DiffCtan(tree, L(node)) == -1) {
      return -1;
    }
    break;
  case Asin:
    if (DiffAsin(tree, L(node)) == -1) {
      return -1;
    }
    break;
  case Acos:
    if (DiffAcos(tree, L(node)) == -1) {
      return -1;
    }
    break;
  case Atan:
    if (DiffAtan(tree, L(node)) == -1) {
      return -1;
    }
    break;
  case Actan:
    if (DiffActan(tree, L(node)) == -1) {
      return -1;
    }
    break;
  case Log:
    if (DiffLog(tree, L(node)) == -1) {
      return -1;
    }
    break;
  case Ln:
    if (DiffLn(tree, L(node)) == -1) {
      return -1;
    }
    break;
  case Sqrt:
    if (DiffSqrt(tree, L(node)) == -1) {
      return -1;
    }
    break;
  default:
    LOG_LVL_DIFF_ERROR("invalid function code!\n");
    return -1;
    break;
  }

  // getting argument's derivative
  if (DiffNode(tree, R(node)) == -1) {
    LOG_LVL_DIFF_ERROR("couldn't get argument's derivatie!\n");
    return -1;
  }

  return 0;
}

int DiffSin(Tree* tree, const int node) {
  Data data_new = {Function, Cos};
  if (CHANGE(node, data_new) == -1)
    return -1;

  return 0;
}

int DiffCos(Tree* tree, const int node) {
  Node* cpy_args = TreeCopySubtree(tree, L(node));
  int   n_args   = TreeCountSubtree(tree, L(node));

  TreeDeleteNode(tree, L(node));
  TreeDeleteNode(tree, R(node));

  Data data_new = {Operator, Mul};
  CHANGE(node, data_new);

  data_new.type  = Number;
  data_new.value = -1 * PRECISION;
  INSERT(node, Right, data_new);

  data_new.type  = Function;
  data_new.value = Sin;
  INSERT(node, Left, data_new);

  TreeGlueSubtree(tree, cpy_args, L(node), Left, n_args);

  free(cpy_args);

  return 0;
}

int DiffTan(Tree* tree, const int node) {
  Node* cpy_args = TreeCopySubtree(tree, L(node));
  int   n_args   = TreeCountSubtree(tree, L(node));

  Data data_new = {Operator, Div};
  CHANGE(node, data_new);
  DELETE(R(node));
  DELETE(L(node));

  data_new.type  = Number;
  data_new.value = 1 * PRECISION;
  INSERT(node, Left, data_new);

  data_new.type  = Operator;
  data_new.value = Pow;
  INSERT(node, Right, data_new);

  data_new.type  = Number;
  data_new.value = 2 * PRECISION;
  INSERT(R(node), Right, data_new);

  data_new.type  = Function;
  data_new.value = Cos;
  INSERT(R(node), Left, data_new);

  TreeGlueSubtree(tree, cpy_args, L(R(node)), Left, n_args);

  free(cpy_args);

  return 0;
}

int DiffCtan(Tree* tree, const int node) {
  Node* cpy_args = TreeCopySubtree(tree, L(node));
  int   n_args   = TreeCountSubtree(tree, L(node));

  Data data_new = {Operator, Div};
  CHANGE(node, data_new);
  DELETE(R(node));
  DELETE(L(node));

  data_new.type  = Number;
  data_new.value = -1 * PRECISION;
  INSERT(node, Left, data_new);

  data_new.type  = Operator;
  data_new.value = Pow;
  INSERT(node, Right, data_new);

  data_new.type  = Number;
  data_new.value = 2 * PRECISION;
  INSERT(R(node), Right, data_new);

  data_new.type  = Function;
  data_new.value = Sin;
  INSERT(R(node), Left, data_new);

  TreeGlueSubtree(tree, cpy_args, L(R(node)), Left, n_args);

  free(cpy_args);

  return 0;
}

// FIXME:
int DiffAsin(Tree* tree, const int node) {
  LOG_LVL_DIFF_FAILURE("still in active development! come back later!\n");

  return -1;
}

// FIXME:
int DiffAcos(Tree* tree, const int node) {
  LOG_LVL_DIFF_FAILURE("still in active development! come back later!\n");

  return -1;
}

// FIXME:
int DiffAtan(Tree* tree, const int node) {
  LOG_LVL_DIFF_FAILURE("still in active development! come back later!\n");

  return -1;
}

// FIXME:
int DiffActan(Tree* tree, const int node) {
  LOG_LVL_DIFF_FAILURE("still in active development! come back later!\n");

  return -1;
}

int DiffLog(Tree* tree, const int node) {
  Node* cpy_args = TreeCopySubtree(tree, L(node));
  int   n_args   = TreeCountSubtree(tree, L(node));

  Data data_new = {Operator, Div};
  CHANGE(node, data_new);
  DELETE(R(node));
  DELETE(L(node));

  data_new.type  = Number;
  data_new.value = 1 * PRECISION;
  INSERT(node, Left, data_new);

  data_new.type  = Operator;
  data_new.value = Mul;
  INSERT(node, Right, data_new);

  TreeGlueSubtree(tree, cpy_args, R(node), Left, n_args);

  data_new.type  = Function;
  data_new.value = Ln;
  INSERT(R(node), Right, data_new);

  data_new.type  = Number;
  data_new.value = 10 * PRECISION;
  INSERT(R(R(node)), Left, data_new);

  free(cpy_args);

  return 0;
}

int DiffLn(Tree* tree, const int node) {
  Node* cpy_args = TreeCopySubtree(tree, L(node));
  int   n_args   = TreeCountSubtree(tree, L(node));

  Data data_new = {Operator, Div};
  CHANGE(node, data_new);
  DELETE(R(node));
  DELETE(L(node));

  data_new.type  = Number;
  data_new.value = 1 * PRECISION;
  INSERT(node, Left, data_new);

  TreeGlueSubtree(tree, cpy_args, node, Right, n_args);

  free(cpy_args);

  return 0;
}

int DiffSqrt(Tree* tree, const int node) {
  Node* cpy_func = TreeCopySubtree(tree, node);
  int   n_func   = TreeCountSubtree(tree, node);

  Data data_new = {Operator, Div};
  CHANGE(node, data_new);
  DELETE(R(node));
  DELETE(L(node));

  data_new.type  = Number;
  data_new.value = 1 * PRECISION;
  INSERT(node, Left, data_new);

  data_new.type  = Operator;
  data_new.value = Mul;
  INSERT(node, Right, data_new);

  data_new.type  = Number;
  data_new.value = 2 * PRECISION;
  INSERT(R(node), Left, data_new);

  TreeGlueSubtree(tree, cpy_func, R(node), Right, n_func);

  free(cpy_func);

  return 0;
}

int DiffOperator(Tree* tree, const int node) {
  switch (VALUE(node)) {
  case Mul:
    if (DiffMul(tree, node) == -1) {
      return -1;
    }
    break;
  case Div:
    if (DiffDiv(tree, node) == -1) {
      return -1;
    }
    break;
  case Sum:
    if (DiffSumSub(tree, node) == -1) {
      return -1;
    }
    break;
  case Sub:
    if (DiffSumSub(tree, node) == -1) {
      return -1;
    }
    break;
  case Pow:
    if (DiffPow(tree, node) == -1) {
      return -1;
    }
    break;
  default:
    LOG_LVL_DIFF_ERROR("invalid operator code!\n");
    return -1;
    break;
  }

  return 0;
}

int DiffMul(Tree* tree, const int node) {
  Node* cpy_left  = TreeCopySubtree(tree, L(node));
  int   n_left    = TreeCountSubtree(tree, L(node));
  Node* cpy_right = TreeCopySubtree(tree, R(node));
  int   n_right   = TreeCountSubtree(tree, R(node));

  Data data_new = {Operator, Sum};
  CHANGE(node, data_new);
  DELETE(L(node));
  DELETE(R(node));

  data_new.type  = Operator;
  data_new.value = Mul;
  INSERT(node, Left, data_new);

  data_new.type  = Operator;
  data_new.value = Mul;
  INSERT(node, Right, data_new);

  TreeGlueSubtree(tree, cpy_left, L(node), Left, n_left);
  TreeGlueSubtree(tree, cpy_right, L(node), Right, n_right);
  TreeGlueSubtree(tree, cpy_left, R(node), Left, n_left);
  TreeGlueSubtree(tree, cpy_right, R(node), Right, n_right);

  if (D(L(L(node))) == -1 || D(R(R(node))) == -1) {
    LOG_LVL_DIFF_ERROR("error while getting secondary expression derivative");
    return -1;
  }

  free(cpy_left);
  free(cpy_right);

  return 0;
}

int DiffDiv(Tree* tree, const int node) {
  Node* cpy_left  = TreeCopySubtree(tree, L(node));
  int   n_left    = TreeCountSubtree(tree, L(node));
  Node* cpy_right = TreeCopySubtree(tree, R(node));
  int   n_right   = TreeCountSubtree(tree, R(node));

  DELETE(L(node));
  DELETE(R(node));

  Data data_new = {Operator, Mul};
  INSERT(node, Right, data_new);

  data_new.type  = Operator;
  data_new.value = Sub;
  INSERT(node, Left, data_new);

  TreeGlueSubtree(tree, cpy_right, R(node), Right, n_right);
  TreeGlueSubtree(tree, cpy_right, R(node), Left, n_right);

  data_new.type  = Operator;
  data_new.value = Mul;
  INSERT(L(node), Left, data_new);

  data_new.type  = Operator;
  data_new.value = Mul;
  INSERT(L(node), Right, data_new);

  TreeGlueSubtree(tree, cpy_left, L(L(node)), Left, n_left);
  TreeGlueSubtree(tree, cpy_right, L(L(node)), Right, n_right);

  TreeGlueSubtree(tree, cpy_left, R(L(node)), Left, n_left);
  TreeGlueSubtree(tree, cpy_right, R(L(node)), Right, n_right);

  if (D(L(L(L(node)))) == -1 || D(R(R(L(node)))) == -1) {
    LOG_LVL_DIFF_ERROR("error while getting secondary expression derivative");
    return -1;
  }

  free(cpy_left);
  free(cpy_right);

  return 0;
}

int DiffSumSub(Tree* tree, const int node) {
  if (D(L(node)) == -1 || D(R(node)) == -1) {
    LOG_LVL_DIFF_ERROR("error while getting secondary expression derivative");
    return -1;
  }

  return 0;
}

int DiffPow(Tree* tree, const int node) {
  Node* cpy_pow   = TreeCopySubtree(tree, node);
  int   n_pow     = TreeCountSubtree(tree, node);
  Node* cpy_left  = TreeCopySubtree(tree, L(node));
  int   n_left    = TreeCountSubtree(tree, L(node));
  Node* cpy_right = TreeCopySubtree(tree, R(node));
  int   n_right   = TreeCountSubtree(tree, R(node));

  Data data_new = {Operator, Mul};
  CHANGE(node, data_new);
  DELETE(L(node));
  DELETE(R(node));

  data_new.type  = Operator;
  data_new.value = Mul;
  INSERT(node, Right, data_new);

  TreeGlueSubtree(tree, cpy_pow, node, Left, n_pow);
  TreeGlueSubtree(tree, cpy_right, R(node), Right, n_right);

  data_new.type  = Function;
  data_new.value = Ln;
  INSERT(R(node), Left, data_new);

  TreeGlueSubtree(tree, cpy_left, R(L(node)), Left, n_left);

  if (D(R(node)) == -1)
    return -1;

  free(cpy_left);
  free(cpy_right);
  free(cpy_pow);

  return 0;
}

//=====================
// SIMPLIFICATION

int DiffSimplify(Tree* tree) {
  int flagModified = 1;

  while (flagModified == 1) {
    flagModified = 0;

    if (_DiffSimplify(tree, L(ROOT), &flagModified) == -1) {
      LOG_LVL_DIFF_ERROR("error while simplifying!\n");
      return -1;
    }
  }

  return 0;
}

int _DiffSimplify(Tree* tree, const int node, int* flag) {
  if (node != 0) {
    if (_DiffSimplify(tree, L(node), flag) == -1 || _DiffSimplify(tree, R(node), flag) == -1)
      return -1;

    switch (TYPE(node)) {
    case Operator:
      if (SimplifyOperator(tree, node, flag) == -1) {
        return -1;
      }
      break;
    case Function:
      if (SimplifyFunction(tree, node, flag) == -1) {
        return -1;
      }
      break;
    default:
      return 0;
    }
  }

  return 0;
}

int SimplifyOperator(Tree* tree, const int node, int* flag) {
  switch (VALUE(node)) {
  case Mul:
    if (SimplifyMul(tree, node, flag) == -1) {
      return -1;
    }
    break;
  case Div:
    if (SimplifyDiv(tree, node, flag) == -1) {
      return -1;
    }
    break;
  case Sum:
    if (SimplifySum(tree, node, flag) == -1) {
      return -1;
    }
    break;
  case Sub:
    if (SimplifySub(tree, node, flag) == -1) {
      return -1;
    }
    break;
  }

  return 0;
}

int SimplifyMul(Tree* tree, const int node, int* flag) {
  LOG_LVL_DIFF_ROUTINE("simplifying node %d: [mul]([%d, %d], [%d, %d]), parent=%d\n",
                       node,
                       TYPE(L(node)),
                       VALUE(L(node)),
                       TYPE(R(node)),
                       VALUE(R(node)),
                       PARENT(node));

  int parent        = PARENT(node);
  int branch_parent = -1;
  if (parent != 0) {
    if (L(parent) == node)
      branch_parent = Left;
    else
      branch_parent = Right;
  }

  int l_is_zero = TYPE(L(node)) == Number && VALUE(L(node)) == 0;
  int r_is_zero = TYPE(R(node)) == Number && VALUE(R(node)) == 0;
  int l_is_one  = TYPE(L(node)) == Number && VALUE(L(node)) == 1 * PRECISION;
  int r_is_one  = TYPE(R(node)) == Number && VALUE(R(node)) == 1 * PRECISION;
  int r_is_num  = TYPE(R(node)) == Number;
  int l_is_num  = TYPE(L(node)) == Number;

  if (l_is_zero || r_is_zero) {
    Data data_new = {Number, 0};
    CHANGE(node, data_new);

    DELETE(L(node));
    DELETE(R(node));

    LOG_LVL_DIFF_ROUTINE("zeroing the node\n");

    *flag = 1;
  } else if (l_is_one) {
    Node* cpy_right = TreeCopySubtree(tree, R(node));
    int   n_right   = TreeCountSubtree(tree, R(node));

    DELETE(node);

    TreeGlueSubtree(tree, cpy_right, parent, branch_parent, n_right);

    free(cpy_right);

    LOG_LVL_DIFF_ROUTINE("L == 1\n");

    *flag = 1;
  } else if (r_is_one) {
    Node* cpy_left = TreeCopySubtree(tree, L(node));
    int   n_left   = TreeCountSubtree(tree, L(node));

    DELETE(node);

    TreeGlueSubtree(tree, cpy_left, parent, branch_parent, n_left);

    free(cpy_left);

    LOG_LVL_DIFF_ROUTINE("R == 1\n");

    *flag = 1;
  } else if (r_is_num && l_is_num) {
    Data data_new = {Number, (VALUE(L(node)) * VALUE(R(node))) / PRECISION};
    CHANGE(node, data_new);

    DELETE(L(node));
    DELETE(R(node));

    LOG_LVL_DIFF_ROUTINE("R == num, L == num\n");

    *flag = 1;
  }

  return 0;
}

int SimplifyDiv(Tree* tree, const int node, int* flag) {
  LOG_LVL_DIFF_ROUTINE("simplifying node %d: [div]([%d, %d], [%d, %d]), parent=%d\n",
                       node,
                       TYPE(L(node)),
                       VALUE(L(node)),
                       TYPE(R(node)),
                       VALUE(R(node)),
                       PARENT(node));

  int l_is_zero = TYPE(L(node)) == Number && VALUE(L(node)) == 0;
  int r_is_one  = TYPE(R(node)) == Number && VALUE(R(node)) == 1 * PRECISION;
  int r_is_num  = TYPE(R(node)) == Number;
  int l_is_num  = TYPE(L(node)) == Number;

  if (l_is_zero) {
    Data data_new = {Number, 0};
    CHANGE(node, data_new);

    DELETE(L(node));
    DELETE(R(node));

    LOG_LVL_DIFF_ROUTINE("L == 0, zeroing node\n");

    *flag = 1;
  } else if (r_is_one) {
    int parent        = PARENT(node);
    int branch_parent = -1;

    if (tree->nodes[node].parent != 0) {
      if (L(parent) == node)
        branch_parent = Left;
      else
        branch_parent = Right;
    }

    Node* cpy_left = TreeCopySubtree(tree, L(node));
    int   n_left   = TreeCountSubtree(tree, L(node));

    DELETE(node);

    TreeGlueSubtree(tree, cpy_left, parent, branch_parent, n_left);

    free(cpy_left);

    LOG_LVL_DIFF_ROUTINE("R == 1\n");

    *flag = 1;
  } else if (r_is_num && l_is_num) {
    Data data_new = {Number, (int)(PRECISION * (float)VALUE(L(node)) / (float)VALUE(R(node)))};
    CHANGE(node, data_new);

    DELETE(L(node));
    DELETE(R(node));

    *flag = 1;
  }

  return 0;
}

int SimplifySum(Tree* tree, const int node, int* flag) {
  LOG_LVL_DIFF_ROUTINE("simplifying node %d: [sum]([%d, %d], [%d, %d]), parent=%d\n",
                       node,
                       TYPE(L(node)),
                       VALUE(L(node)),
                       TYPE(R(node)),
                       VALUE(R(node)),
                       PARENT(node));

  int parent        = PARENT(node);
  int branch_parent = -1;

  if (tree->nodes[node].parent != 0) {
    if (L(parent) == node)
      branch_parent = Left;
    else
      branch_parent = Right;
  }

  int l_is_zero = TYPE(L(node)) == Number && VALUE(L(node)) == 0;
  int r_is_zero = TYPE(R(node)) == Number && VALUE(R(node)) == 0;
  int r_is_num  = TYPE(R(node)) == Number;
  int l_is_num  = TYPE(L(node)) == Number;

  if (l_is_zero) {
    Node* cpy_right = TreeCopySubtree(tree, R(node));
    int   n_right   = TreeCountSubtree(tree, R(node));

    DELETE(node);

    TreeGlueSubtree(tree, cpy_right, parent, branch_parent, n_right);

    free(cpy_right);

    *flag = 1;
  } else if (r_is_zero) {
    Node* cpy_left = TreeCopySubtree(tree, L(node));
    int   n_left   = TreeCountSubtree(tree, L(node));

    DELETE(node);

    TreeGlueSubtree(tree, cpy_left, parent, branch_parent, n_left);

    free(cpy_left);

    *flag = 1;
  } else if (r_is_num && l_is_num) {
    Data data_new = {Number, VALUE(L(node)) + VALUE(R(node))};
    CHANGE(node, data_new);

    DELETE(L(node));
    DELETE(R(node));

    *flag = 1;
  }

  return 0;
}

int SimplifySub(Tree* tree, const int node, int* flag) {
  LOG_LVL_DIFF_ROUTINE("simplifying node %d: [sub]([%d, %d], [%d, %d]), parent=%d\n",
                       node,
                       TYPE(L(node)),
                       VALUE(L(node)),
                       TYPE(R(node)),
                       VALUE(R(node)),
                       PARENT(node));

  int parent        = PARENT(node);
  int branch_parent = -1;
  if (tree->nodes[node].parent != 0) {
    if (L(parent) == node)
      branch_parent = Left;
    else
      branch_parent = Right;
  }

  int l_is_zero = TYPE(L(node)) == Number && VALUE(L(node)) == 0;
  int r_is_zero = TYPE(R(node)) == Number && VALUE(R(node)) == 0;
  int r_is_num  = TYPE(R(node)) == Number;
  int l_is_num  = TYPE(L(node)) == Number;

  if (r_is_zero) {
    Node* cpy_right = TreeCopySubtree(tree, L(node));
    int   n_right   = TreeCountSubtree(tree, L(node));

    DELETE(node);

    TreeGlueSubtree(tree, cpy_right, parent, branch_parent, n_right);

    free(cpy_right);

    *flag = 1;
  } else if (l_is_zero) {
    Data data_new = {Operator, Mul};
    CHANGE(node, data_new);

    data_new.type  = Number;
    data_new.value = -1 * PRECISION;
    CHANGE(L(node), data_new);

    *flag = 1;
  } else if (r_is_num && l_is_num) {
    Data data_new = {Number, VALUE(L(node)) - VALUE(R(node))};
    CHANGE(node, data_new);

    DELETE(L(node));
    DELETE(R(node));

    *flag = 1;
  }

  return 0;
}

int SimplifyFunction(Tree* tree, const int node, int* flag) {
  switch (VALUE(node)) {
  case Sin:
    SimplifySin(tree, node, flag);
    break;
  case Cos:
    SimplifyCos(tree, node, flag);
    break;
  case Tan:
    SimplifyTan(tree, node, flag);
    break;
  }

  return 0;
}

// TODO: do pi/2 simplification
int SimplifySin(Tree* tree, const int node, int* flag) {
  LOG_LVL_DIFF_ROUTINE("simplifying node %d: [sin]([%d, %d], [%d, %d]), parent=%d\n",
                       node,
                       TYPE(L(node)),
                       VALUE(L(node)),
                       TYPE(R(node)),
                       VALUE(R(node)),
                       PARENT(node));
  int l_is_n_pi = TYPE(L(node)) == Number && VALUE(L(node)) % (int)(3.14 * PRECISION) == 0;

  if (l_is_n_pi) {
    Data data_new = {Number, 0};
    CHANGE(node, data_new);

    DELETE(L(node));

    *flag = 1;
  }

  return 0;
}

// TODO: do pi simplification
int SimplifyCos(Tree* tree, const int node, int* flag) {
  LOG_LVL_DIFF_ROUTINE("simplifying node %d: [cos]([%d, %d], [%d, %d]), parent=%d\n",
                       node,
                       TYPE(L(node)),
                       VALUE(L(node)),
                       TYPE(R(node)),
                       VALUE(R(node)),
                       PARENT(node));

  int l_is_n_pi      = TYPE(L(node)) == Number && VALUE(L(node)) % (int)(3.14 * PRECISION) == 0;
  int l_is_n_pi_half = TYPE(L(node)) == Number && VALUE(L(node)) % (int)(3.14 / 2 * PRECISION) == 0;

  if (!l_is_n_pi && l_is_n_pi_half) {
    Data data_new = {Number, 0};
    CHANGE(node, data_new);

    DELETE(L(node));

    *flag = 1;
  }

  return 0;
}

int SimplifyTan(Tree* tree, const int node, int* flag) {
  LOG_LVL_DIFF_ROUTINE("simplifying node %d: [tan]([%d, %d], [%d, %d]), parent=%d\n",
                       node,
                       TYPE(L(node)),
                       VALUE(L(node)),
                       TYPE(R(node)),
                       VALUE(R(node)),
                       PARENT(node));

  int l_is_n_pi = TYPE(L(node)) == Number && VALUE(L(node)) % (int)(3.14 * PRECISION) == 0;

  if (l_is_n_pi) {
    Data data_new = {Number, 0};
    CHANGE(node, data_new);

    DELETE(L(node));

    *flag = 1;
  }

  return 0;
}

//=====================
// PDF

void DiffInitPdf() {
  int fd = open("./output/report.tex", O_CREAT | O_TRUNC | O_WRONLY, 0666);

  dprintf(fd,
          "\\documentclass[a4paper]{article}\n"
          "\\usepackage[T2A]{fontenc}\n"
          "\\usepackage[utf8]{inputenc}\n"
          "\\usepackage[english,russian]{babel}\n"
          "\\usepackage{wrapfig}\n"
          "\\usepackage{amsmath,amsfonts,amssymb,amsthm,mathtools}\n"
          "\\usepackage[pdftex]{graphicx}\n"
          "\\graphicspath{{pictures/}}\n"
          "\\DeclareGraphicsExtensions{.pdf,.png,.jpg}\n"
          "\\usepackage[left=3cm,right=3cm,\n"
          "  top=3.5cm,bottom=2cm,bindingoffset=0cm]{geometry}\n"
          "\\usepackage{wrapfig}\n"
          "\\usepackage{float}\n"
          "\\usepackage{graphicx}\n"
          "\\begin{document}\n"
          "\\newpage\n"
          "\n Let's stick it to Steven Wolfram!\n\n");

  close(fd);
}

void DiffClosePdf() {
  int fd = open("./output/report.tex", O_APPEND | O_WRONLY);

  dprintf(fd, "\n\\end{document}\n");

  close(fd);
}

void DiffPrintToPdf(const char* msg) {
  int fd = open("./output/report.tex", O_APPEND | O_WRONLY);

  dprintf(fd, "%s", msg);

  close(fd);
}

void DiffCompilePdf() {
  system("pdflatex ./output/report.tex");
}

int DiffPrintTree(Tree* tree) {
  int fd = open("./output/report.tex", O_APPEND | O_WRONLY);

  TREE_VERIFY(tree);

  dprintf(fd, "$\n");

  if (_DiffPrintTree(fd, tree, L(ROOT)) == -1) {
    LOG_LVL_DIFF_ERROR("error while writing tree to the output!\n");
    return -1;
  }

  dprintf(fd, "$\n");

  close(fd);

  return 0;
}

int _DiffPrintTree(const int fd, Tree* tree, const int node) {
  if (node != 0) {
    switch (TYPE(node)) {
    case Variable:
      if (PrintVariable(fd, tree, node) == -1) {
        return -1;
      }
      break;
    case Number:
      if (PrintNumber(fd, tree, node) == -1) {
        return -1;
      }
      break;
    case Operator:
      if (PrintOperator(fd, tree, node) == -1) {
        return -1;
      }
      break;
    case Function:
      if (PrintFunction(fd, tree, node) == -1) {
        return -1;
      }
      break;
    default:
      LOG_LVL_DIFF_ERROR("invalid node type!\n");
      return -1;
      break;
    }
  }

  return 0;
}

int PrintVariable(const int fd, Tree* tree, const int node) {
  dprintf(fd, "x");

  return 0;
}

int PrintNumber(const int fd, Tree* tree, const int node) {
  if (VALUE(node) == 3.14 * PRECISION)
    dprintf(fd, "\\pi");
  else if (VALUE(node) == 2.73 * PRECISION)
    dprintf(fd, "e");
  else if (VALUE(node) >= 0)
    dprintf(fd, "%0.2f", (double)(VALUE(node)) / PRECISION);
  else
    dprintf(fd, "(%0.2f)", (double)(VALUE(node)) / PRECISION);

  return 0;
}

int PrintOperator(const int fd, Tree* tree, const int node) {
  switch (VALUE(node)) {
  case Sum:
    if (PrintSum(fd, tree, node) == -1) {
      return -1;
    }
    break;
  case Sub:
    if (PrintSub(fd, tree, node) == -1) {
      return -1;
    }
    break;
  case Mul:
    if (PrintMul(fd, tree, node) == -1) {
      return -1;
    }
    break;
  case Div:
    if (PrintDiv(fd, tree, node) == -1) {
      return -1;
    }
    break;
  case Pow:
    if (PrintPow(fd, tree, node) == -1) {
      return -1;
    }
    break;
  }

  return 0;
}

int PrintSum(const int fd, Tree* tree, const int node) {
  if (_DiffPrintTree(fd, tree, L(node)) == -1)
    return -1;

  dprintf(fd, " + ");

  if (_DiffPrintTree(fd, tree, R(node)) == -1)
    return -1;

  return 0;
}

int PrintSub(const int fd, Tree* tree, const int node) {
  if (_DiffPrintTree(fd, tree, L(node)) == -1)
    return -1;

  dprintf(fd, " - ");

  if (TYPE(R(node)) == Operator && (VALUE(R(node)) == Sub || VALUE(R(node)) == Sum)) {
    dprintf(fd, " \\left( ");

    if (_DiffPrintTree(fd, tree, R(node)) == -1)
      return -1;

    dprintf(fd, " \\right) ");
  } else {
    if (_DiffPrintTree(fd, tree, R(node)) == -1)
      return -1;
  }

  return 0;
}

int PrintMul(const int fd, Tree* tree, const int node) {
  if (TYPE(L(node)) == Operator &&
      (VALUE(L(node)) == Sub || VALUE(L(node)) == Sum || VALUE(L(node)) == Div)) {
    dprintf(fd, " \\left( ");

    if (_DiffPrintTree(fd, tree, L(node)) == -1)
      return -1;

    dprintf(fd, " \\right) ");
  } else {
    if (_DiffPrintTree(fd, tree, L(node)) == -1)
      return -1;
  }

  dprintf(fd, " \\cdot ");

  if (VALUE(R(node)) == Operator && (VALUE(R(node)) == Sub || VALUE(R(node)) == Sum)) {
    dprintf(fd, " \\left( ");

    if (_DiffPrintTree(fd, tree, R(node)) == -1)
      return -1;

    dprintf(fd, " \\right) ");
  } else {
    if (_DiffPrintTree(fd, tree, R(node)) == -1)
      return -1;
  }

  return 0;
}

int PrintDiv(const int fd, Tree* tree, const int node) {
  dprintf(fd, "\\dfrac{ ");

  if (_DiffPrintTree(fd, tree, L(node)) == -1)
    return -1;

  dprintf(fd, " }{ ");

  if (_DiffPrintTree(fd, tree, R(node)) == -1)
    return -1;

  dprintf(fd, " } ");

  return 0;
}

int PrintPow(const int fd, Tree* tree, const int node) {
  dprintf(fd, " \\left( ");

  if (_DiffPrintTree(fd, tree, L(node)) == -1)
    return -1;

  dprintf(fd, " \\right)^{");

  if (_DiffPrintTree(fd, tree, R(node)) == -1)
    return -1;

  dprintf(fd, " } ");

  return 0;
}

int PrintFunction(const int fd, Tree* tree, const int node) {
  switch (VALUE(node)) {
  case Sin:
    dprintf(fd, "\\sin{ \\left(");
    if (_DiffPrintTree(fd, tree, L(node)) == -1) {
      return -1;
    }
    dprintf(fd, "\\right) }");
    break;
  case Cos:
    dprintf(fd, "\\cos{ \\left(");
    if (_DiffPrintTree(fd, tree, L(node)) == -1) {
      return -1;
    }
    dprintf(fd, "\\right) }");
    break;
  case Tan:
    dprintf(fd, "\\tan{ \\left(");
    if (_DiffPrintTree(fd, tree, L(node)) == -1) {
      return -1;
    }
    dprintf(fd, "\\right) }");
    break;
  case Ctan:
    dprintf(fd, "\\ctan{ \\left(");
    if (_DiffPrintTree(fd, tree, L(node)) == -1) {
      return -1;
    }
    dprintf(fd, "\\right) }");
    break;
  case Asin:
    dprintf(fd, "\\arcsin{ \\left(");
    if (_DiffPrintTree(fd, tree, L(node)) == -1) {
      return -1;
    }
    dprintf(fd, "\\right) }");
    break;
  case Acos:
    dprintf(fd, "\\arccos{ \\left(");
    if (_DiffPrintTree(fd, tree, L(node)) == -1) {
      return -1;
    }
    dprintf(fd, "\\right) }");
    break;
  case Atan:
    dprintf(fd, "\\arctan{ \\left(");
    if (_DiffPrintTree(fd, tree, L(node)) == -1) {
      return -1;
    }
    dprintf(fd, "\\right) }");
    break;
  case Actan:
    dprintf(fd, "\\arcctan{ \\left(");
    if (_DiffPrintTree(fd, tree, L(node)) == -1) {
      return -1;
    }
    dprintf(fd, "\\right) }");
    break;
  case Log:
    dprintf(fd, "\\log{ \\left(");
    if (_DiffPrintTree(fd, tree, L(node)) == -1) {
      return -1;
    }
    dprintf(fd, "\\right) }");
    break;
  case Ln:
    dprintf(fd, "\\ln{ \\left(");
    if (_DiffPrintTree(fd, tree, L(node)) == -1) {
      return -1;
    }
    dprintf(fd, "\\right) }");
    break;
  case Sqrt:
    dprintf(fd, "\\sqrt{ \\left(");
    if (_DiffPrintTree(fd, tree, L(node)) == -1) {
      return -1;
    }
    dprintf(fd, "\\right) }");
    break;
  }

  return 0;
}
