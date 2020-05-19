#include "./tree.h"

#include "../utils/log.h"

const char* TREE_ERR_DESC[] = {
    "tree is ok\n",
    "hash is corrupted!\n",
    "initialization error!\n",
    "reallocation error!\n",
    "logical sequence corrupted!\n",
    "invalide tree size!\n",
    "logfile not created or unexpectedly closed!\n",
};

const int TREE_INIT_SZ = 10;
const int DELTA        = 20;

Node* NodeAlloc() {
  Node* node = calloc(1, sizeof(Node));

  return node;
}

int NodeInit(Node* node, const int parent, const int branch_l, const int branch_r,
             const Data data) {
  if (node == NULL) {
    LOG_LVL_TREE_ERROR("NULL node given");
    return -1;
  } else {
    node->parent          = parent;
    node->branches[Left]  = branch_l;
    node->branches[Right] = branch_r;

    node->data.type  = data.type;
    node->data.value = data.value;

    return 0;
  }
}

void NodeFree(Node* node) {
  if (node != NULL)
    free(node);

  return;
}

Tree* TreeAlloc() {
  Tree* tree   = calloc(1, sizeof(Tree));
  tree->nodes  = calloc(TREE_INIT_SZ, sizeof(Node));
  tree->memmap = calloc(TREE_INIT_SZ, sizeof(int));

  return tree;
}

int TreeInit(Tree* tree) {
  LOG_LVL_TREE_INIT();

  if (tree == NULL) {
    LOG_LVL_TREE_ERROR("[TreeInit] nullptr was given!\n");
    return -1;
  } else if (tree->nodes == NULL) {
    LOG_LVL_TREE_ERROR("[TreeInit] tree->nodes is nullptr!\n");
    tree->err = E_INIT_ERR;
    return -1;
  } else if (tree->memmap == NULL) {
    tree->err = E_INIT_ERR;
    LOG_LVL_TREE_ERROR("[TreeInit] tree->memmap is nullptr!\n");
    return -1;
  } else {
    // just in case
    memset(tree->nodes, '\0', TREE_INIT_SZ * sizeof(Node));

    // setting up memory map
    for (int i = 1; i < TREE_INIT_SZ - 1; i++)
      tree->memmap[i] = i + 1;
    tree->memmap[TREE_INIT_SZ - 1] = 0;

    tree->max  = TREE_INIT_SZ;
    tree->cur  = 0;
    tree->root = 0;
    tree->free = 1;

    tree->err = OK;

#ifdef SEC_ON
    tree->hash = TreeGetHash(tree);
#endif

    TREE_VERIFY(tree);
  }

  return 0;
}

void TreeFree(Tree* tree) {
  if (tree == NULL)
    return;

  if (tree->memmap != NULL)
    free(tree->memmap);

  if (tree->nodes != NULL)
    free(tree->nodes);

  free(tree);

  return;
}

int TreeResize(Tree* tree, const int size_new) {
  TREE_VERIFY(tree);

  if (size_new < DELTA || size_new < tree->cur) {
    LOG_LVL_TREE_FAILURE("invalid new size %d -> %d\n", tree->max, size_new);
    return -1;

  } else if (size_new == tree->max) {
    LOG_LVL_TREE_FAILURE("same size %d -> %d\n", tree->max, size_new);
    return -1;

  } else if (size_new < tree->max) {
    LOG_LVL_TREE_ROUTINE("shrink %d -> %d\n", tree->max, size_new);

    tree->max    = size_new;
    tree->nodes  = realloc(tree->nodes, size_new * sizeof(Node));
    tree->memmap = realloc(tree->memmap, size_new * sizeof(Node));

  } else if (size_new > tree->max) {
    LOG_LVL_TREE_ROUTINE("extend %d -> %d\n", tree->max, size_new);

    tree->nodes = realloc(tree->nodes, size_new * sizeof(Node));
    memset(tree->nodes + tree->max * sizeof(Node), '\0', (size_new - tree->max) * sizeof(Node));

    tree->memmap = realloc(tree->memmap, size_new * sizeof(int));
    for (int i = tree->max - 1; i < size_new - 1; i++)
      tree->memmap[i] = i + 1;
    tree->memmap[size_new - 1] = 0;

    if (tree->free == 0)
      tree->free = tree->max;

    tree->max = size_new;
  }

#ifdef SEC_ON
  tree->hash = TreeGetHash(tree);
#endif

  TREE_VERIFY(tree);

  return 0;
}

int TreeSort(Tree* tree) {
  TREE_VERIFY(tree);

  int counter = 0;

  Node* nodes_new = calloc(tree->max, sizeof(Node));

  if (_TreeSort(tree, tree->root, 0, 0, &counter, nodes_new) == -1)
    return -1;

  for (int i = 1; i < tree->max - 1; i++)
    tree->memmap[i] = i + 1;
  tree->memmap[tree->max - 1] = 0;

  free(tree->nodes);
  tree->nodes = nodes_new;
  tree->free  = tree->cur + 1;

  LOG_LVL_TREE_ROUTINE("tree sorted");

#ifdef SEC_ON
  tree->hash = TreeGetHash(tree);
#endif

  TREE_VERIFY(tree);

  return 0;
}

int _TreeSort(Tree* tree, const int node, const int parent, const int branches, int* counter,
              Node* buf) {
  if (node == 0)
    return 0;

  (*counter) += 1;
  if (*counter > tree->max) {
    LOG_LVL_TREE_ERROR("index out of range!\n");
    return -1;
  }

  int this     = *counter;
  int branch_l = (tree->nodes[node]).branches[Left];
  int branch_r = (tree->nodes[node]).branches[Right];

  buf[this]        = tree->nodes[node];
  buf[this].parent = parent;
  if (parent != 0)
    buf[parent].branches[branches] = this;

  if (branch_l != 0)
    if (_TreeSort(tree, branch_l, this, Left, counter, buf) == -1)
      return -1;

  if (branch_r != 0)
    if (_TreeSort(tree, branch_r, this, Right, counter, buf) == -1)
      return -1;

  return *counter;
}

int TreeFind(Tree* tree, const int node, const Data data) {
  TREE_VERIFY(tree);

  if (tree->cur == 0) {
    LOG_LVL_TREE_FAILURE("[TreeFind] tree is empty!\n");
    return -1;
  }

  if (node == 0)
    return -1;

  int res = -1;
  if (tree->nodes[node].data.value == data.value && tree->nodes[node].data.type == data.type)
    res = node;

  if (res != -1)
    return res;

  res = TreeFind(tree, (tree->nodes[node]).branches[Left], data);
  if (res != -1)
    return res;

  res = TreeFind(tree, (tree->nodes[node]).branches[Right], data);
  if (res != -1)
    return res;

  return -1;
}

int TreeInsertNode(Tree* tree, const int parent, const int branches, Data* data) {
  TREE_VERIFY(tree);

  int addr_to_insert = tree->free;

  int res = 0;

  if (tree->cur == 0) {
    tree->cur++;

    (tree->nodes[addr_to_insert]).data.type  = data->type;
    (tree->nodes[addr_to_insert]).data.value = data->value;

    if (res == -1)
      return -1;

    (tree->nodes[addr_to_insert]).parent          = 0;
    (tree->nodes[addr_to_insert]).branches[Left]  = 0;
    (tree->nodes[addr_to_insert]).branches[Right] = 0;
    tree->root                                    = addr_to_insert;
    tree->free                                    = tree->memmap[addr_to_insert];

    LOG_LVL_TREE_ROUTINE("root added with type %d and value %d\n", data->type, data->value);
  } else if ((tree->nodes[parent]).branches[branches] != 0) {
    LOG_LVL_TREE_FAILURE("branches %d of node %d is already occupied!\n", branches, parent);
    return -1;
  } else if ((tree->nodes[parent]).parent == 0 && parent != tree->root) {
    LOG_LVL_TREE_FAILURE("trying to insert in unlinked chunk!\n");
    return -1;
  } else {
    if (tree->cur >= tree->max - 2) {
      if (TreeResize(tree, tree->max * 2) == -1)
        return -1;
    }

    (tree->nodes[parent]).branches[branches] = tree->free;
    (tree->nodes[addr_to_insert]).parent     = parent;

    (tree->nodes[addr_to_insert]).data.type  = data->type;
    (tree->nodes[addr_to_insert]).data.value = data->value;

    (tree->nodes[addr_to_insert]).branches[Left]  = 0;
    (tree->nodes[addr_to_insert]).branches[Right] = 0;

    tree->free = tree->memmap[addr_to_insert];
    tree->cur++;

    LOG_LVL_TREE_ROUTINE(
        "inserted node, parent=%d type=%d and value=%d\n", parent, data->type, data->value);
  }

#ifdef SEC_ON
  tree->hash = TreeGetHash(tree);
#endif

  TREE_VERIFY(tree);

  return addr_to_insert;
}

int TreeCountSubtree(Tree* tree, const int node) {
  TREE_VERIFY(tree);

  int counter = 0;

  if (_TreeCountSubtree(tree, node, &counter) == -1)
    return -1;

  TREE_VERIFY(tree);

  return counter;
}

int _TreeCountSubtree(Tree* tree, const int node, int* counter) {
  if (*counter > tree->cur) {
    LOG_LVL_TREE_ERROR("counter out of range!\n");
    tree->err = E_SEQ_CORRUPTED;
    return -1;
  }

  (*counter)++;

  int branch_l = (tree->nodes[node]).branches[Left];
  int branch_r = (tree->nodes[node]).branches[Right];

  if (branch_l != 0)
    if (_TreeCountSubtree(tree, branch_l, counter) == -1)
      return -1;

  if (branch_r != 0)
    if (_TreeCountSubtree(tree, branch_r, counter) == -1)
      return -1;

  return 0;
}

Node* TreeCopySubtree(Tree* src, const int node) {
  TREE_VERIFY(src);

  int subtreeSz = TreeCountSubtree(src, node);
  if (subtreeSz == -1) {
    LOG_LVL_TREE_ERROR("can't count elements to copy subtree!\n");
    return NULL;
  }

  Node* dst = calloc(subtreeSz, sizeof(Node));
  int   pos = 0;

  if (_TreeCopySubtree(src, dst, node, &pos) == -1)
    return NULL;

  TREE_VERIFY(src);

  return dst;
}

int _TreeCopySubtree(Tree* src, Node* dst, const int node, int* pos) {
  int branch_l = (src->nodes[node]).branches[Left];
  int branch_r = (src->nodes[node]).branches[Right];

  dst[*pos] = src->nodes[node];
  (*pos)++;

  if (branch_l != 0)
    if (_TreeCopySubtree(src, dst, branch_l, pos) == -1)
      return -1;

  if (branch_r != 0)
    if (_TreeCopySubtree(src, dst, branch_r, pos) == -1)
      return -1;

  return 0;
}

int TreeDeleteNode(Tree* tree, const int node) {
  TREE_VERIFY(tree);

  if (node == 0) {
    LOG_LVL_TREE_FAILURE("deleting empty node!\n");
    return 0;
  } else if (tree->cur == 0) {
    LOG_LVL_TREE_FAILURE("deleting node from empty tree!\n");
    return 0;
  } else if (_TreeDeleteNode(tree, node) == -1) {
    LOG_LVL_TREE_ERROR("can't delete node and it's subtree!\n");
    return -1;
  }

  LOG_LVL_TREE_ROUTINE("deleted subtree for node %d\n", node);

#ifdef SEC_ON
  tree->hash = TreeGetHash(tree);
#endif

  TREE_VERIFY(tree);

  return 0;
}

int _TreeDeleteNode(Tree* tree, const int node) {
  if (tree->cur == 0) {
    LOG_LVL_TREE_ERROR("tree is already empty!\n");
    return -1;
  } else if (tree->cur < 0) {
    LOG_LVL_TREE_ERROR("tree->cur went negative while deleting!\n");
    return -1;
  } else {
    int parent   = (tree->nodes[node]).parent;
    int branch_l = (tree->nodes[node]).branches[Left];
    int branch_r = (tree->nodes[node]).branches[Right];

    if (branch_l != 0)
      if (_TreeDeleteNode(tree, branch_l) == -1)
        return -1;

    if (branch_r != 0)
      if (_TreeDeleteNode(tree, branch_r) == -1)
        return -1;

    if ((tree->nodes[node]).parent != 0 || node == tree->root)
      tree->cur--;

    if ((tree->nodes[parent]).branches[Left] == node)
      (tree->nodes[parent]).branches[Left] = 0;
    else if ((tree->nodes[parent]).branches[Right] == node)
      (tree->nodes[parent]).branches[Right] = 0;

    (tree->nodes[node]).parent = 0;

    tree->memmap[node] = tree->free;
    tree->free         = node;
  }

  return 0;
}

int TreeChangeNode(Tree* tree, const int node, int* parent_new, int* branch_l_new,
                   int* branch_r_new, Data* data_new) {
  TREE_VERIFY(tree);

  int branch_l = (tree->nodes[node]).branches[Left];
  int branch_r = (tree->nodes[node]).branches[Right];

  if (parent_new != NULL)
    (tree->nodes[node]).parent = *parent_new;

  if (branch_l_new != NULL) {
    (tree->nodes[branch_l]).parent = 0;
    branch_l                       = *branch_l_new;
  }

  if (branch_r_new != NULL) {
    (tree->nodes[branch_r]).parent = 0;
    branch_r                       = *branch_r_new;
  }

  if (data_new != NULL) {
    (tree->nodes[node]).data.value = data_new->value;
    (tree->nodes[node]).data.type  = data_new->type;
  } else {
    LOG_LVL_TREE_ERROR("NULL value data_new given");
    return -1;
  }

  LOG_LVL_TREE_ROUTINE(
      "changed node %d's value to type=%d value=%d\n", node, data_new->type, data_new->value);

#ifdef SEC_ON
  tree->hash = TreeGetHash(tree);
#endif

  TREE_VERIFY(tree);

  return 0;
}

int TreeGlueSubtree(Tree* tree, Node* subtree, int node, int branch, int nodesCount) {
  TREE_VERIFY(tree);

  if (tree->nodes[node].branches[branch] != 0) {
    LOG_LVL_TREE_ERROR("branch is not empty!\n");
    return -1;
  } else if (tree->nodes[node].parent == 0 && node != tree->root) {
    LOG_LVL_TREE_ERROR("invalid node to glue to!\n");
    return -1;
  }

  int index = 0;

  if (_TreeGlueSubtree(tree, subtree, &index, nodesCount, node, branch)) {
    TreeDeleteNode(tree, tree->nodes[node].branches[branch]);
    return -1;
  }

  LOG_LVL_TREE_ROUTINE("glued subtree to node %d\n", node);

#ifdef SEC_ON
  tree->hash = TreeGetHash(tree);
#endif

  TREE_VERIFY(tree);

  return 0;
}

int _TreeGlueSubtree(Tree* tree, Node* subtree, int* index, int nodesCount, int node, int branch) {
  if (TreeInsertNode(tree, node, branch, &(subtree[*index].data)) == -1) {
    LOG_LVL_TREE_ERROR("can't insert %d node's %d branch!\n", node, branch);
    return -1;
  }

  int this = tree->nodes[node].branches[branch];

  int addr_local = *index;

  if (*index < nodesCount && subtree[addr_local].branches[Left] != 0) {
    (*index)++;

    if (_TreeGlueSubtree(tree, subtree, index, nodesCount, this, Left) == -1)
      return -1;
  }

  if (*index < nodesCount && subtree[addr_local].branches[Right] != 0) {
    (*index)++;

    if (_TreeGlueSubtree(tree, subtree, index, nodesCount, this, Right) == -1)
      return -1;
  }

  return 0;
}

Tree* TreeRead(const char* pathname) {
  Tree* tree = TreeAlloc();

  if (TreeInit(tree) == -1) {
    LOG_LVL_TREE_ERROR("initialzation error!\n");
    return NULL;
  }

  int fd = open(pathname, O_RDONLY);
  if (fd == -1) {
    LOG_LVL_TREE_ERROR("can't open source file!\n");
    return NULL;
  }

  int len = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);
  char* s = calloc(len, sizeof(char));

  if (read(fd, s, len) != len) {
    LOG_LVL_TREE_ERROR("can't read file to buffer!\n");
    return NULL;
  }

  close(fd);

  if (_TreeRead(tree, &s) == -1) {
    LOG_LVL_TREE_ERROR("error while rading!\n");
    return NULL;
  }

#ifdef SEC_ON
  tree->hash = TreeGetHash(tree);
#endif

  TREE_VERIFY(tree);

  return tree;
}

int _TreeRead(Tree* tree, char** s) {
  int res = TreeGetGr(s, tree, 0, 0);

  if (res == -1)
    return -1;

  return 0;
}

int TreeGetGr(char** s, Tree* tree, const int parent, const int branch) {
  int val = 0;

  GetSpace(s);

  val = TreeGetNode(s, tree, parent, branch);

  GetSpace(s);

  if (**s != '\0' && **s != '\n' && **s != EOF) {
    LOG_LVL_TREE_ERROR("syntax error! no end of str detected, found <%c> instead!\n", **s);
    return -1;
  }

  return val;
}

int TreeGetNode(char** s, Tree* tree, const int parent, const int branch) {
  Data val  = {};
  int  node = 0;

  GetSpace(s);

  switch (**s) {
  case '#':
    (*s)++;
    return 0;
  case '[':
    (*s)++;
    break;
  default:
    return -1;
  }

  GetSpace(s);

  val.type = GetNumber(s);

  GetSpace(s);

  if (**s != ',')
    return -1;
  else
    (*s)++;

  GetSpace(s);

  val.value = GetNumber(s);

  GetSpace(s);

  if (**s != ']')
    return -1;
  else
    (*s)++;

  //   if (val.type == -1 || val.type == -1) {
  //     // no child
  //     if (**s != '#') {
  //       return -1;
  //     } else {
  //       (*s)++;
  //       return 0;
  //     }
  //   } else {
  node = TreeInsertNode(tree, parent, branch, &val);
  if (node == -1) {
    LOG_LVL_TREE_ERROR("can't insert node\n");
    return -1;
  }
  //   }

  GetSpace(s);

  while (**s == '(') {
    (*s)++;

    GetSpace(s);

    if (TreeGetNode(s, tree, node, Left) == -1)
      return -1;

    GetSpace(s);

    if (**s != ',')
      return -1;
    else
      (*s)++;

    GetSpace(s);

    if (TreeGetNode(s, tree, node, Right) == -1)
      return -1;

    GetSpace(s);

    if (**s != ')')
      return -1;
    else
      (*s)++;
  }

  GetSpace(s);

  return 0;
}

int GetSpace(char** s) {
  while (isblank(**s) != 0) {
    (*s)++;
  }

  return 0;
}

int GetNumber(char** s) {
  int val = 0;

  if (isdigit(**s) == 0) {
    LOG_LVL_TREE_FAILURE("no number was read!\n");
    return -1;
  }

  while (isdigit(**s)) {
    val = val * 10 + (**s - '0');
    (*s)++;
  }

  return val;
}

int TreeWrite(Tree* tree, const char* pathname) {
  TREE_VERIFY(tree);

  int fdout = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (fdout == -1) {
    LOG_LVL_TREE_ERROR("can't open file for writing!\n");
    return -1;
  }

  if (tree->root == 0) {
    LOG_LVL_TREE_FAILURE("empty tree given!\n");
    dprintf(fdout, "#\n");
    return 0;
  } else if (_TreeWrite(tree, tree->root, fdout) == -1) {
    LOG_LVL_TREE_ERROR("failed to write tree\n");
    return -1;
  }

  dprintf(fdout, "\n");

  close(fdout);

  return 0;
}

int _TreeWrite(Tree* tree, int node, int fd) {
  dprintf(fd, "[%d, %d]", tree->nodes[node].data.type, tree->nodes[node].data.value);

  dprintf(fd, "(");

  if (tree->nodes[node].branches[Left] != 0)
    _TreeWrite(tree, tree->nodes[node].branches[Left], fd);
  else
    dprintf(fd, "#");

  dprintf(fd, ", ");

  if (tree->nodes[node].branches[Right] != 0)
    _TreeWrite(tree, tree->nodes[node].branches[Right], fd);
  else
    dprintf(fd, "#");

  dprintf(fd, ")");

  return 0;
}

#ifdef SEC_ON
int TreeVerify(Tree* tree) {
  if (tree == NULL) {
    LOG_LVL_TREE_ERROR("tree is nullptr!\n");
    return -1;
  }

  if (tree->hash != TreeGetHash(tree)) {
    LOG_LVL_TREE_ERROR("hash corruption detected");
    tree->err = E_HASH_CORRUPTED;
    return tree->err;
  }

  if (tree->cur > tree->max || tree->cur < 0 || tree->max < 0) {
    LOG_LVL_TREE_ERROR("invalid size detected");
    tree->err = E_SIZE_INVALID;
    return tree->err;
  }

  return tree->err;
}

void TreeDump(Tree* tree) {
  int fd_dump = open("/home/dominator/shitcodes/c-c++/acronis/differentiator/dump/tree.dump",
                     O_RDWR | O_TRUNC | O_CREAT,
                     S_IRUSR | S_IWUSR);

  if (tree == NULL) {
    dprintf(fd_dump, "ERROR: NULL pointer to the structure!\n");
  } else {
    char* timestamp = GetTimestamp();

    dprintf(fd_dump,
            "%s\n"
            "  hash: %ld\n"
            "  tree capacity: %d\n"
            "  tree size: %d\n"
            "  first free element's physical address: %d\n"
            "  root address: %d\n"
            "  error: %d %s\n",
            timestamp,
            tree->hash,
            tree->max,
            tree->cur,
            tree->free,
            tree->root,
            tree->err,
            TREE_ERR_DESC[tree->err]);
    free(timestamp);

    if (tree->nodes != NULL) {
      for (int i = 0; i < tree->max; i++)
        dprintf(fd_dump,
                "  [%3d] (type: %d value: %3d [%3d, %3d] <%3d>)\n",
                i,
                tree->nodes[i].data.type,
                tree->nodes[i].data.value,
                tree->nodes[i].branches[Left],
                tree->nodes[i].branches[Right],
                tree->nodes[i].parent);
    }
  }

  close(fd_dump);
}

long TreeGetHash(Tree* tree) {
  if (tree == NULL) {
    printf("[TreeGetHash] nullptr given! returning -1\n");
    return -1;
  } else {
    return 100;
  }

  return -1;
}
#endif
