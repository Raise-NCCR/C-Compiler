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

assert 15 'main() { a = 1; b = 2; c = 3; d = 4; e = 5; foo(a, b, c, d, e); } foo(a, b, c, d, e) { return a + b + c + d + e; }'

assert 5 'main() { a = 0; b = 5; loop(a, b);} loop(a, b) { for (c = 0; c < b; c = c + 1) { a = a + 1; } return a;}'

assert 6 'main() { a = 0; b = 5; loop(a, b);} loop(a, b) { for (c = 0; c < b; c = c + 1) { a = a + 1; } a = a + 1; return a;}'