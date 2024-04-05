int sum(int x, int y, int z, int a, int b, int c, int m, int n) {
  return x + y + z + a + b + c + m - n;
}

int main() {
  int x = sum(1, 2, 3, 4, 5, 6, 7, 8);
  return x;
}
