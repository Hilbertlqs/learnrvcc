#!/bin/bash

assert()
{
    expected="$1"
    input="$2"

    ./rvcc "${input}" > tmp.s || exit
    ${RISCV}/bin/riscv64-unknown-linux-gnu-gcc -static -o tmp tmp.s

    ${RISCV}/bin/qemu-riscv64 -L ${RISCV}/sysroot ./tmp

    actual="$?"

    if [ "${actual}" = "${expected}" ]; then
        echo "${input} => ${actual}"
    else
        echo "${input} => ${expected} expected, but got ${actual}"
        exit 1
    fi
}

# assert expected input
assert 0 0
assert 42 42

assert 34 '12-34+56'

assert 41 ' 12 + 34 - 5 '

echo "OK"
