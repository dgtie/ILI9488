#include <iostream>

using namespace std;

enum { EQU=14, ADD=15, SUB=16, MUL=17, DIV=18, NOP=19 };

struct { double v;
  double getValue(void) { return v; }
  void setValue(double d) { v = d; cout << "value set: " << v << endl; }
} display;

struct Calc {
  bool feed(int k){
    static const struct { int rank; bool(*op)(double&,double); } table[] = {
      { 0, 0 }, { 0, &add }, { 0, &sub }, { 1, &mul }, { 1, &div }
    };
    if (k < 14) return true;
    if ((k -= 14) > 4) return true;
    if (op2) {
      if (!(*table[op1].op)(v1, display.getValue())) return false;
      display.setValue(v1);
      op1 = op2; op2 = 0; v1 = v2;
    }
    if (op1) {
      if (table[k].rank > table[op1].rank) { op2 = op1; v2 = v1; }
      else {
        if (!(*table[op1].op)(v1, display.getValue())) return false;
        display.setValue(v1);
      }
    }
    op1 = k;
    v1 = display.getValue();
    return true;
  }
private:
  int op1, op2; double v1, v2;
  static bool add(double &v1, double v2) { v1 = v1 + v2; return true; }
  static bool sub(double &v1, double v2) { v1 = v1 - v2; return true; }
  static bool mul(double &v1, double v2) { v1 = v1 * v2; return true; }
  static bool div(double &v1, double v2) {
    if (v2 == 0) return false;
    v1 = v1 / v2; return true;
  }
} calc;

struct { int op; double v; } test[] = {
  { NOP, 8 },
  { ADD, 4 },
  { EQU, 0 }
};

int main(int argc, char* argv[]) {
  int i = 0;
  while (1) {
    calc.feed(test[i].op);
    if (test[i].op == EQU) break;
    display.setValue(test[i++].v);
  }
  cout << display.getValue() << endl;
  return 0;
}
