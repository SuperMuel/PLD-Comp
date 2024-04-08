int main() {
    int x;
    x= 0;
    x=!x;
    int y;
    y= !x +1;
    int z;
    z = !y + !x;
    int r;
    r = !z;
    int s;
    s = !r;
    return s+r;
}
