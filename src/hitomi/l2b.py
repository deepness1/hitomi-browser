on = []
for line in open("/tmp/list"):
    n = int(line[4:-2])
    on.append(n)

for i in range(4096):
    print('1, ' if i in on else '0, ', end = None if (i + 1) % 32 == 0 else '')
