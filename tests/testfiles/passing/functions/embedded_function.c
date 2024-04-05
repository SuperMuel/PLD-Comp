int sum(int x, int y) { return x + y; }

int main() {
  int x = sum(1, sum(2, 3));
  return x;
}
