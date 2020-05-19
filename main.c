#include "./tree/tree.h"
#include "./utils/log.h"

int main() {
  printf("hui\n");

  Tree* tree = TreeRead("./input/database.txt");

  TreeWrite(tree, "./output/tree.txt");

  Tree* tree2 = TreeRead("./output/tree.txt");

  TreeWrite(tree2, "./output/tree2.txt");

  return 0;
}