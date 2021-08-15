#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 1 'int foo(int x, int y) { return x + y; } int main() { int x; int y; x = 0; y = 1; foo(x, y); }'

assert 3 'int main() { int x; int *y; y = &x; *y = 3; return x; }'

assert 3 'int main() { int x; int *y; int **z; y = &x; z = &y; **z = 3; return x; }'

assert 9 'int main() { int x; int *y; int **z; y = &x; z = &y; **z = 3; return x + *y + **z; }'

assert 3 'int main() { int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p + *(p + 1); }'

assert 3 'int main() { int a[2]; a[0] = 1; a[1] = 2; int *p; p = a; return *p + *(p + 1); }'