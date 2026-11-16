import sys, bisect, re, collections
nm, samples, thread = sys.argv[1], sys.argv[2], sys.argv[3]
mode = sys.argv[4] if len(sys.argv) > 4 else 'func'
addrs=[]; names=[]
for line in open(nm):
    p = line.rstrip('\n').split(' ', 2)
    if len(p) < 3: continue
    addrs.append(int(p[0], 16)); names.append(p[2])
def sym(a):
    i = bisect.bisect_right(addrs, a) - 1
    return names[i] if i >= 0 else '?'
flat = collections.Counter(); inc = collections.Counter(); addrc = collections.Counter()
windows = 0; cur = None
for line in open(samples):
    m = re.match(r'== (\d+) s, thread (\d+): (\d+) samples', line)
    if m:
        cur = m.group(2)
        if cur == thread: windows += 1
        continue
    if cur != thread: continue
    t = line.split()
    if len(t) < 3 or not t[0].endswith('%'): continue
    pct = float(t[0][:-1])
    if t[1] == 'elf':
        a = int(t[2], 16); flat[sym(a)] += pct; addrc[a] += pct
    elif t[1] == 'helper':
        a, b = int(t[3], 16), int(t[4], 16)
        flat['[%s] <- %s' % (sym(a), sym(b))] += pct
        inc['helper:' + sym(a)] += pct
        inc['by:' + sym(b)] += pct
    elif t[1] in ('call', 'wait'):
        a = int(t[3], 16); b = int(t[4], 16) if len(t) > 4 else 0
        flat['%s %s <- %s' % (t[1], sym(a), sym(b) if b else '-')] += pct
    else:
        flat[' '.join(t[1:])] += pct
W = windows or 1
if mode == 'func':
    for k, v in flat.most_common(400): print('%6.2f%% %s' % (v / W, k[:150]))
elif mode == 'addr':
    for k, v in addrc.most_common(80): print('%6.2f%% 0x%x %s' % (v / W, k, sym(k)[:100]))
elif mode == 'inc':
    for k, v in inc.most_common(40): print('%6.2f%% %s' % (v / W, k[:150]))
