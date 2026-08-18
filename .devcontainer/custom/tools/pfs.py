# =============================================================================================
# AoTv4 -- read and write EQ PFS archives (.eqg / .s3d)             2026-08-17
#
#   import pfs; files, buf = pfs.pfs_list("zone.eqg")
#   data = pfs.read_blocks(buf, f["off"], f["size"])
#   pfs.pfs_write("out.eqg", [(name, data), ...])
#
# Written to add a tree model to freeporttheater.eqg, which has no vegetation of its own.
#
# FORMAT: uint32 dir_offset | "PFS " | uint32 version(0x20000); then per-file zlib block runs
# {uint32 deflated, uint32 inflated, data} in 8192-byte inflated chunks; then a filename blob
# (uint32 count, then uint32 len-incl-NUL + bytes, in FILE-OFFSET order) carrying the well-known
# CRC 0x61580AC9; then the directory {uint32 count, count x (crc, offset, size)} at EOF.
#
# ⚠️⚠️ THE DIRECTORY MUST BE SORTED BY CRC ASCENDING -- stock archives are, and the client is
# entitled to binary-search it. The filename blob is ordered by OFFSET, not by CRC, so the two
# orderings differ and pairing them up wrongly silently renames every file in the archive.
#
# ⚠️⚠️ THE CRC IS NOT zlib.crc32. It is poly 0x04C11DB7, NON-reflected, init 0, no final xor,
# taken over the name INCLUDING its NUL terminator. Verified against all 120 entries of a stock
# freeporttheater.eqg before this was trusted to write one.
#
# ⚠️⚠️ A MODEL'S TEXTURE LIST IS NOT CONFINED TO THE .mod STRING POOL. Reading only the pool
# (offset 8 gives its length) found THREE textures for obp_sakura_; scanning the whole file found
# FOUR -- ra_sakuratree_03.dds sits outside it. A missing texture is an untextured or invisible
# model with nothing logged, so scan the entire .mod for `*.dds` instead.
#
# 📌 Round-trip against the untouched archive before trusting any write: rewrite it, re-read it,
# and compare every file byte-for-byte. That test is what proved this writer.
# =============================================================================================
import struct, zlib

def read_blocks(buf, off, inflated_size):
    out = bytearray()
    while len(out) < inflated_size:
        dlen, ilen = struct.unpack_from('<II', buf, off); off += 8
        out += zlib.decompress(buf[off:off+dlen]); off += dlen
    return bytes(out)

def pfs_list(path):
    buf = open(path,'rb').read()
    diroff, magic, ver = struct.unpack_from('<I4sI', buf, 0)
    assert magic == b'PFS ', magic
    count = struct.unpack_from('<I', buf, diroff)[0]
    entries = []
    for i in range(count):
        crc, off, size = struct.unpack_from('<III', buf, diroff+4+i*12)
        entries.append({'crc':crc,'off':off,'size':size})
    # filename directory has this well-known CRC
    names = None
    for e in entries:
        if e['crc'] == 0x61580AC9:
            raw = read_blocks(buf, e['off'], e['size'])
            n = struct.unpack_from('<I', raw, 0)[0]
            p = 4; names = []
            for _ in range(n):
                ln = struct.unpack_from('<I', raw, p)[0]; p += 4
                names.append(raw[p:p+ln-1].decode('latin-1')); p += ln
            break
    files = [e for e in entries if e['crc'] != 0x61580AC9]
    files.sort(key=lambda e: e['off'])
    for e, nm in zip(files, names or []):
        e['name'] = nm
    return files, buf

def extract(path, wanted_prefixes):
    files, buf = pfs_list(path)
    got = {}
    for e in files:
        nm = e.get('name','')
        if any(nm.lower().startswith(w.lower()) for w in wanted_prefixes):
            got[nm] = read_blocks(buf, e['off'], e['size'])
    return got

# ---- writer ----
_T = None
def _tab():
    global _T
    if _T is None:
        _T = []
        for i in range(256):
            c = i << 24
            for _ in range(8):
                c = ((c << 1) ^ 0x04C11DB7) & 0xFFFFFFFF if c & 0x80000000 else (c << 1) & 0xFFFFFFFF
            _T.append(c)
    return _T

def eqcrc(s):
    T = _tab(); c = 0
    for b in s.encode('latin-1') + b'\x00':
        c = ((c << 8) & 0xFFFFFFFF) ^ T[((c >> 24) ^ b) & 0xFF]
    return c

BLOCK = 8192

def _deflate_blocks(data):
    out = bytearray()
    if not data:
        comp = zlib.compress(b'', 9)
        out += struct.pack('<II', len(comp), 0) + comp
        return bytes(out)
    for i in range(0, len(data), BLOCK):
        chunk = data[i:i+BLOCK]
        comp = zlib.compress(chunk, 9)
        out += struct.pack('<II', len(comp), len(chunk)) + comp
    return bytes(out)

def pfs_write(path, items):
    """items: ordered list of (name, data). Layout mirrors the stock archives:
    file blocks, then the filename directory blob, then the CRC-sorted directory."""
    body = bytearray(); body += struct.pack('<I4sI', 0, b'PFS ', 0x20000)
    placed = []
    for name, data in items:
        off = len(body)
        body += _deflate_blocks(data)
        placed.append({'crc': eqcrc(name), 'off': off, 'size': len(data)})
    # filename directory: count, then (len incl NUL, bytes) in file order
    raw = bytearray(struct.pack('<I', len(items)))
    for name, _ in items:
        nb = name.encode('latin-1') + b'\x00'
        raw += struct.pack('<I', len(nb)) + nb
    fn_off = len(body)
    body += _deflate_blocks(bytes(raw))
    placed.append({'crc': 0x61580AC9, 'off': fn_off, 'size': len(raw)})

    diroff = len(body)
    placed.sort(key=lambda e: e['crc'])
    body += struct.pack('<I', len(placed))
    for e in placed:
        body += struct.pack('<III', e['crc'], e['off'], e['size'])
    struct.pack_into('<I', body, 0, diroff)
    open(path, 'wb').write(bytes(body))
    return len(body)
