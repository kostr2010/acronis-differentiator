#include "./diff/diff.h"
#include "./utils/log.h"

int main() {
  Tree* tree = DiffReadExpression("input/expression.txt");

  DiffInitPdf();

  DiffPrintToPdf("d( ");
  DiffPrintTree(tree);
  DiffPrintToPdf(")/dx = ");

  DiffGetDerivative(tree);

  DiffPrintTree(tree);
  DiffClosePdf();
  DiffCompilePdf();

  TreeFree(tree);

  return 0;
}