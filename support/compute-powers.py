#!/usr/bin/env python
# Compute 10 ** exp with exp in the range [min_exponent, max_exponent] and print
# normalized (with most-significant bit equal to 1) significands in hexadecimal.

from __future__ import print_function

min_exponent = -348
max_exponent = 340
step = 8
significand_size = 64
for exp in range(min_exponent, max_exponent + 1, step):
    n = 10 ** exp if exp >= 0 else 2 ** 2000 / 10 ** -exp
    k = significand_size + 1
    # Convert to binary and round.
    n = (int('{:0<{}b}'.format(n, k)[:k], 2) + 1) / 2
    print('{:0<#16x}ull'.format(n))
