import math
import sys
import collections

Key = collections.namedtuple('Key', 'count length plen slen')

data = collections.defaultdict(lambda: [[], []])
for line in sys.stdin:
        name, count, length, plen, slen, time = line.split(',')
        length = int(length)
        if length < 10:
                continue
        key = Key(count=int(count),
                  length=length,
                  plen=int(plen),
                  slen=int(slen))
        data[key]['Trie' in name].append(float(time))

points = collections.defaultdict(lambda: collections.defaultdict(list))
for key, times in data.items():
        vec_times, trie_times = times
        y = min(trie_times) / max(vec_times)
        points[key.length][key.plen].append(y)


def wr(msg, *args, **kw):
        if args or kw:
                msg = msg.format(*args, **kw)
        sys.stdout.write(msg)

def fmt_log_num(n):
        if n < 0:
                return '0.' + '0' * (-n - 1) + '1'
        elif n < 3:
                return str(10 ** n)
        elif n < 6:
                return str(10 ** (n - 3)) + ' k'
        else:
                return str(10 ** (n - 6)) + ' M'

HORIZONTAL = 8
SPREAD = 10
def cy(y):
        return round(y * 40) - 20
def cx(x):
        return x * 8 * SPREAD + 116


wr("""<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" version="1.1"
     width="35em" height="26.25em" viewBox="0 0 560 420"
     stroke-width="1" text-anchor="middle">
""")

wr('  <path fill="#ccc" d="')
width = cx(1) - cx(0)
for x in range(0, 6, 2):
        wr('M{x},0h{width}v{height}H{x}z',
           x=cx(x) - width // 2, width=width, height=cy(9) - cy(0))
wr('" />\n')

wr('  <path stroke="#ccebc5" d="')
for y in range(1, 10):
        wr('M{},{}H{}'.format(cx(0) - 40, cy(y), cx(5) + 40))
wr('" />\n')

wr('  <path stroke="#e41a1c" d="')
for length, data in sorted(points.items()):
        x_base  = cx(int(math.log10(length)) - 1)
        x_base -= HORIZONTAL // 2
        x_base -= (int(math.log10(length)) + 1) * SPREAD // 2
        for plen, ys in sorted(data.items()):
                x = (-1 if plen == 0 else int(math.log10(plen))) + 1
                x = x_base + x * SPREAD
                prev = None
                h = HORIZONTAL
                for y in ys:
                        y = cy(8 - math.log10(y))
                        if prev is None:
                                wr('\n    M{},{}h{}', x, y, h)
                                prev = y
                        elif prev == y:
                                wr('h{}', h)
                        else:
                                wr('m0,{}h{}', y - prev, h)
                        prev = y
                        h = -h
wr('" />\n')

wr("""  <g font-size="16" font-style="italic">
    <text transform="rotate(-90)"
          x="{}" y="{}">Trie ÷ Vector time</text>
    <text x="{}" y="{}">Words length (k)</text>
  </g>
""", -(cy(0) + cy(10)) // 2, 21, (cx(0) + cx(5)) // 2, cy(10) + 28)

wr("""  <g font-size="14">
    <text text-anchor="end">""")
for y in range(-1, 8):
        wr('<tspan x="{}" y="{}">{}×</tspan>',
           cx(0) - 41, cy(8 - y) + 4, fmt_log_num(y))
wr('</text>\n    <text y="{}">', cy(10))
for x in range(6):
        wr('<tspan x="{}">{}</tspan>', cx(x), fmt_log_num(x + 1))
wr('</text>\n  </g>\n</svg>')
