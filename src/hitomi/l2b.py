on = []
total = 0
for line in open("/tmp/list"):
    o1 = line.find('=') + 2
    o2 = line.find(';', o1)
    if line[o1:o2] == '1':
        p1 = line.find(' ') + 1
        p2 = line.find(' ', p1) - 1
        on.append(int(line[p1:p2]))
    total += 1

for i in range(total):
    print('1, ' if i in on else '0, ', end = None if (i + 1) % 32 == 0 else '')
