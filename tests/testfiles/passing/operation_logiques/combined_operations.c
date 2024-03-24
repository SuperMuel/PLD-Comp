int main() {
    int a;
    a = 12; 
    int b;
    b = 5;  
    int c;
    int d;
    int e;

    c = a & (b+3);  
    d = a | b - c;  
    e = 4*(c+a) ^ (d-4/7); 

    return e; 
}
