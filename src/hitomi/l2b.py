off = []
for line in open("/tmp/list"):
    off.append(int(line[5:-2]))

for i in range(4096):
    print('0, ' if i in off else '1, ', end = None if (i + 1) % 32 == 0 else '')
