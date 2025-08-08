#!/usr/bin/python3

with open('light.bin', 'rb') as f:
    font = f.read()

c = 0

for i in font:
    if c % 8 == 0:
        print('-'*40)
        print(f'{int(c / 8): x}')
        print('-'*40)

    row = []
    for b in range(7, -1, -1):
        if (i & (2**b)) > 0:
            row.append('*')
        else:
            row.append(' ')

    print(''.join(row))
    c += 1

