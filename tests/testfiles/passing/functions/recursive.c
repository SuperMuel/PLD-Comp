int get_n(int n) {
  if (n == 0) {
    return 0;
  }
  return 1 + get_n(n - 1);
}

int main() { return get_n(12); }
