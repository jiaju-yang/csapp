import click
import re
from collections import namedtuple

set_bits = 4
E_way = 1
block_bits = 6


class MemOp:
    def __init__(self, op_type, address, word_size):
        self.op_type = op_type
        self.address = address
        self.word_size = word_size
        remain, self.block_offset = divmod(address, 1 << block_bits)
        self.tag, self.set_offset = divmod(remain, 1 << set_bits)


AccessEntry = namedtuple(
    'AccessEntry', 'op_type address word_size result eviction')


class Cache:
    def __init__(self):
        sets = []
        for _ in range(1 << set_bits):
            sets.append(Set(Line() for _ in range(E_way)))
        self.sets = tuple(sets)
        self.time = 0
        self.records = []

    def update(self, mem_op: MemOp):
        self.time += 1
        if mem_op.op_type in {'L', 'S'}:
            self._access(mem_op)
        elif mem_op.op_type == 'M':
            self._access(mem_op)
            self._access(mem_op)
        else:
            raise Exception(
                'Not supported cache operation: {}!'.format(mem_op.op_type))

    def _access(self, mem_op):
        set = self.sets[mem_op.set_offset]
        for line in set.lines:
            if line.tag == mem_op.tag and line.valid:
                line.time = self.time
                self.records.append(AccessEntry(
                    mem_op.op_type, mem_op.address, mem_op.word_size, 'hit', ''))
                return
        victim = self._lru_line(set)
        self.records.append(AccessEntry(mem_op.op_type, mem_op.address,
                                        mem_op.word_size, 'miss', 'eviction' if victim.valid else ''))
        victim.valid = True
        victim.tag = mem_op.tag
        victim.time = self.time

    def _lru_line(self, set):
        result = set.lines[0]
        for line in set.lines:
            if line.time < result.time:
                result = line
        return result

    def stat(self):
        hits = 0
        misses = 0
        evictions = 0
        for entry in self.records:
            if entry.result == 'miss':
                misses += 1
            elif entry.result == 'hit':
                hits += 1
            if entry.eviction:
                evictions += 1
        return hits, misses, evictions


class Set:
    def __init__(self, lines):
        self.lines = tuple(lines)


class Line:
    def __init__(self, valid=False, tag=None, block=None, time=0):
        self.valid = valid
        self.tag = tag
        self.block = block
        self.time = time


def file_lines_2_MemOps(lines):
    result = []
    for line in lines:
        if line[0] == ' ':
            op_type, address, word_size = re.split(' +|,', line.strip())
            result.append(MemOp(op_type, int(address, 16), int(word_size)))
    return result


def get_file_lines(file_path):
    with open(file_path, mode='r') as f:
        return f.readlines()


@click.command()
# @click.option('-v', type=bool, help='Optional verbose flag that displays trace info')
@click.option('-s', '--s', type=int, help='Number of set index bits (S = 2^s is the number of sets)')
@click.option('-E', '--E', type=int, help='Associativity (number of lines per set)')
@click.option('-b', '--b', type=int, help='Number of block bits (B = 2^b is the block size)')
@click.option('-t', '--trace_file', type=str, help='Name of the valgrind trace to replay')
def main(s, e, b, trace_file):
    run(s, e, b, trace_file)


def run(s, e, b, trace_file):
    global set_bits, E_way, block_bits
    set_bits, E_way, block_bits = s, e, b
    lines = get_file_lines(trace_file)
    operations = file_lines_2_MemOps(lines)
    cache = Cache()
    for op in operations:
        cache.update(op)
    hits, misses, evictions = cache.stat()
    click.echo('hits:{} misses:{} evictions:{}'.format(
        hits, misses, evictions))


@click.command()
def test():
    run(1, 1, 1, 'traces/yi2.trace')
    run(4, 2, 4, 'traces/yi.trace')
    run(2, 1, 4, 'traces/dave.trace')
    run(2, 1, 3, 'traces/trans.trace')
    run(2, 2, 3, 'traces/trans.trace')
    run(2, 4, 3, 'traces/trans.trace')
    run(5, 1, 5, 'traces/trans.trace')
    run(5, 1, 5, 'traces/long.trace')


if __name__ == '__main__':
    test()
