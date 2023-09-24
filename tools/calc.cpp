#include <iostream>

using namespace std;

bool _add(double &v1, double v2) { v1 = v1 + v2; return true; }
bool _sub(double &v1, double v2) { v1 = v1 - v2; return true; }
bool _mul(double &v1, double v2) { v1 = v1 * v2; return true; }
bool __div(double &v1, double v2) {
  if (v2 == 0) return false;
  v1 = v1 / v2; return true;
}
struct OP { int rank; bool (*op)(double&, double); char id;
} add = { 0, &_add, 'a' }, sub = { 0, &_sub, 's' },
  mul = { 1, &_mul, 'm' }, _div = { 1, &__div, 'd' },
  nop = { 0, 0, 'n' }, equ = { 0, 0, 'e' };

struct { double v;
  double getValue(void) { return v; }
  void setValue(double d) { v = d; cout << "value set: " << v << endl; }
} display;

struct { OP *op; double v; } test[] = {
  { &nop, 8 },
  { &mul, 4 },
  { &mul, 6 },
  { &equ, 0 }
};

struct { OP *op1, *op2; double v1, v2;
  bool feed(OP *op) {
    if (op == &nop) return true;
    if (op2) {
      if (!(*op1->op)(v1, display.getValue())) return false;
      display.setValue(v1);
      op1 = op2; op2 = 0;
      v1 = v2;
    }
    if (op1) {
      if (op->rank > op1->rank) { op2 = op1; v2 = v1; }
      else {
        if (!(*op1->op)(v1, display.getValue())) return false;
        display.setValue(v1);
      }
    }
    op1 = op == &equ ? 0 : op;
    v1 = display.getValue();
    return true;
  }
} calc;

int main(int argc, char* argv[]) {
  int i = 0;
  while (1) {
    calc.feed(test[i].op);
    if (test[i].op == &equ) break;
    display.setValue(test[i++].v);
  }
  cout << display.getValue() << endl;
  return 0;
}
