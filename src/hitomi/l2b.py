off = []
for line in open("/tmp/list"):
    n = int(line[4:-2])
    off.append(n)

for i in range(4096):
    print('0, ' if i in off else '1, ', end = None if (i + 1) % 32 == 0 else '')
