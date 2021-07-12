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

assert 42 '42;'
assert 21 '5+20-4;'
assert 41 ' 12 + 34 - 5 ;'
assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'
assert 10 '-10+20;'
assert 10 '- -10;'
assert 10 '- - +10;'

assert 0 '0==1;'
assert 1 '42==42;'
assert 1 '0!=1;'
assert 0 '42!=42;'

assert 1 '0<1;'
assert 0 '1<1;'
assert 0 '2<1;'
assert 1 '0<=1;'
assert 1 '1<=1;'
assert 0 '2<=1;'

assert 1 '1>0;'
assert 0 '1>1;'
assert 0 '1>2;'
assert 1 '1>=0;'
assert 1 '1>=1;'
assert 0 '1>=2;'

assert 14 'a=3;b=5*6-8;a+b/2;'
assert 14 'ab = 3; bc = 5 * 6 - 8; ab + bc / 2;'
assert 14 'a1 = 3; a2 = 5 * 6 - 8; a1 + a2 / 2;'

assert 5 'return 5;'
assert 14 'a1 = 3; a2 = 5 * 6 - 8; return a1 + a2 / 2;'

assert 3 'a1 = 3; a2 = 5; if (a1 < a2) return a1;'
assert 3 'a1 = 3; a2 = 5; if (a1 < a2) return a1; else return a2;'
assert 5 'a1 = 3; a2 = 5; if (a1 > a2) return a1; else return a2;'

assert 5 'a1 = 0; while (a1 != 5) a1 = a1 + 1; return a1;'

assert 5 'a2 = 0; for (a1 = 0; a1 < 5; a1 = a1 + 1) a2 = a2 + 1; return a2;'

assert 6 'a2 = 0; for (a1 = 0; a1 < 4; a1 = a1 + 1) {a2 = a1+ a2; a1 = a1 - 1; a1 = a1 + 1;} return a2;'

assert 15 '{ a = 1; b = 2; c = 3; d = 4; e = 5; foo(a, b, c, d, e); } foo(a, b, c, d, e) { return a + b + c + d + e; }'


echo OK
