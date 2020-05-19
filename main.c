#include "./diff/diff.h"
#include "./utils/log.h"

int main() {
  printf("hui\n");

  //   Tree* tree = TreeRead("./input/database.txt");

  //   TreeWrite(tree, "./output/tree.txt");

  Tree* tree = DiffReadExpression("input/expression.txt");

  DiffInitPdf();
  DiffPrintToPdf("d( ");
  DiffPrintTree(tree);
  DiffPrintToPdf(")/dx = ");

  if (DiffGetDerivative(tree) == -1) {
    printf("-1\n");
    return -1;
  }

  DiffPrintTree(tree);
  DiffClosePdf();
  DiffCompilePdf();

  TreeFree(tree);

  return 0;
}