#include <iostream>

struct Base {
  virtual void *func(int) { return nullptr; }
};

struct Derived : Base {
  void *func(const int) override { return nullptr; }
};

int main() { return 0; }