def parse_line(line):
    p = line.find(')', 10)
    g = line[10:p]
    o = line[p + 8]
    return (g, o)

on = []
for line in open("/tmp/list"):
    r = parse_line(line)
    if r[1] == '1':
        on.append(int(r[0]))

for i in range(4096):
    print('1, ' if i in on else '0, ', end = None if (i + 1) % 32 == 0 else '')
