"""Splits the merged Liquid Layers NG plugin into the CBBE 3BA core and an optional UBE plugin.

  core  'Liquid Layers NG.esp'        masters Skyrim, Update, SexLab   - everything except the UBE armor addons
  ube   'Liquid Layers NG - UBE.esp'  masters ... UBE_AllRace, core    - the 16 UBE armor addons + the 16 armors
                                                                         overridden with the UBE addon listed first

Works on the bytes of the merged plugin (masters Skyrim, Update, SexLab, UBE_AllRace): every FormID field these record
types carry is remapped (core: own 04 -> 03; UBE plugin: its own new addons 04 -> 05, the rest point into the core,
which is master 04 there). Any FormID field not listed here makes the script stop, so a new field cannot slip through.

Usage: python -I split_ube.py <merged.esp> <out dir>
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from esplist import subrecords  # noqa: E402

CORE = 'Liquid Layers NG.esp'
UBE = 'Liquid Layers NG - UBE.esp'
MASTERS_CORE = ['Skyrim.esm', 'Update.esm', 'SexLab.esm']
MASTERS_UBE = MASTERS_CORE + ['UBE_AllRace.esp', CORE]

# subrecords that are one FormID / an array of FormIDs, per record type
FID = {
    'ARMO': {'YNAM', 'ZNAM', 'RNAM', 'KWDA', 'MODL', 'ETYP', 'BIDS', 'BAMT', 'TNAM', 'EITM'},
    'ARMA': {'RNAM', 'MODL', 'SNDD', 'ONAM', 'NAM0', 'NAM1', 'NAM2', 'NAM3'},
    'SNDR': {'GNAM', 'SNAM', 'ONAM'},
    'TXST': set(),
    'QUST': set(),
}
ALT_TEXTURES = {'MO2S', 'MO3S', 'MO4S', 'MO5S', 'MODS'}  # count, then (len, name, TXST FormID, index)
SAFE = {
    'EDID', 'OBND', 'FULL', 'DESC', 'BOD2', 'DATA', 'DNAM', 'KSIZ', 'MOD2', 'MOD3', 'MOD4', 'MOD5', 'MO2T', 'MO3T',
    'MO4T', 'MO5T', 'TX00', 'TX01', 'TX02', 'TX03', 'TX04', 'TX05', 'TX06', 'TX07', 'CNAM', 'BNAM', 'ANAM', 'NEXT',
    'VMAD',
}


def parse(b):
    """Top groups in file order: [(label, [(sig, fid, header bytes, body bytes)])]."""
    hdr_size = struct.unpack_from('<I', b, 4)[0]
    tes4 = b[:24 + hdr_size]
    groups, i = [], 24 + hdr_size
    while i < len(b):
        assert b[i:i + 4] == b'GRUP'
        gsize = struct.unpack_from('<I', b, i + 4)[0]
        label = b[i + 8:i + 12].decode()
        recs, j = [], i + 24
        while j < i + gsize:
            sig = b[j:j + 4].decode()
            size, flags, fid = struct.unpack_from('<III', b, j + 4)
            assert sig != 'GRUP' and not flags & 0x40000, (sig, hex(fid))
            recs.append((sig, fid, b[j:j + 24], b[j + 24:j + 24 + size]))
            j += 24 + size
        groups.append((label, b[i:i + 24], recs))
        i += gsize
    return tes4, groups


def remap_body(sig, body, fmap):
    out = bytearray()
    blob = b'\0' * 24 + body
    for _, s, d in subrecords(blob, 0, len(body)):
        if s in FID[sig]:
            assert len(d) % 4 == 0, (sig, s)
            d = b''.join(struct.pack('<I', fmap(x)) for x in struct.unpack(f'<{len(d) // 4}I', d))
        elif s in ALT_TEXTURES:
            n = struct.unpack_from('<I', d)[0]
            nd, k = bytearray(d[:4]), 4
            for _ in range(n):
                ln = struct.unpack_from('<I', d, k)[0]
                name = d[k:k + 4 + ln]
                fid, idx = struct.unpack_from('<II', d, k + 4 + ln)
                nd += name + struct.pack('<II', fmap(fid), idx)
                k += 4 + ln + 8
            assert k == len(d)
            d = bytes(nd)
        elif s not in SAFE:
            raise SystemExit(f'{sig}: subrecord {s} is not classified - add it to FID or SAFE')
        out += s.encode() + struct.pack('<H', len(d)) + d
    return bytes(out)


def record(sig, fid, header, body):
    return header[:4] + struct.pack('<I', len(body)) + header[8:12] + struct.pack('<I', fid) + header[16:] + body


def write(path, tes4, masters, groups):
    hdr_body = tes4[24:]
    fields = [(s, d) for _, s, d in subrecords(tes4, 0, len(hdr_body))]
    hedr = next(d for s, d in fields if s == 'HEDR')
    count = sum(len(r) for _, _, r in groups) + len(groups)
    next_id = max([fid & 0xFFFFFF for _, _, recs in groups for _, fid, _, _ in recs if fid >> 24 == len(masters)] +
                  [0x7FF]) + 1
    body = b'HEDR' + struct.pack('<H', 12) + hedr[:4] + struct.pack('<II', count, next_id)
    for s, d in fields:
        if s not in ('HEDR', 'MAST', 'DATA'):
            body += s.encode() + struct.pack('<H', len(d)) + d
    for m in masters:
        name = m.encode() + b'\0'
        body += b'MAST' + struct.pack('<H', len(name)) + name + b'DATA' + struct.pack('<H', 8) + b'\0' * 8
    out = tes4[:4] + struct.pack('<I', len(body)) + tes4[8:24] + body
    for _, ghdr, recs in groups:
        data = b''.join(record(*r) for r in recs)
        out += ghdr[:4] + struct.pack('<I', 24 + len(data)) + ghdr[8:] + data
    with open(path, 'wb') as f:
        f.write(out)


def main():
    src, out_dir = sys.argv[1], sys.argv[2]
    b = open(src, 'rb').read()
    tes4, groups = parse(b)
    masters = [d.rstrip(b'\0').decode() for _, s, d in subrecords(tes4, 0, len(tes4) - 24) if s == 'MAST']
    assert masters == MASTERS_CORE + ['UBE_AllRace.esp'], masters
    edid = {}
    for _, _, recs in groups:
        for sig, fid, h, body in recs:
            for _, s, d in subrecords(b'\0' * 24 + body, 0, len(body)):
                if s == 'EDID':
                    edid[fid] = d.rstrip(b'\0').decode()
    ube_addons = {fid for _, _, recs in groups for sig, fid, _, _ in recs if sig == 'ARMA' and edid[fid].endswith('_UBE')}
    assert len(ube_addons) == 16, len(ube_addons)

    def fmap_core(x):
        if x >> 24 == 3 or x in ube_addons:
            raise ValueError(f'core keeps a UBE link {x:08X}')
        return (3 << 24) | (x & 0xFFFFFF) if x >> 24 == 4 else x

    def fmap_ube(x):
        return (5 << 24) | (x & 0xFFFFFF) if x in ube_addons else x  # 0-3 unchanged, own 04 = the core

    def strip_ube(body):  # the core armors lose their UBE addon
        out, blob = b'', b'\0' * 24 + body
        for _, s, d in subrecords(blob, 0, len(body)):
            if s == 'MODL' and struct.unpack('<I', d)[0] in ube_addons:
                continue
            out += s.encode() + struct.pack('<H', len(d)) + d
        return out

    core, ube = [], []
    for label, ghdr, recs in groups:
        c, u = [], []
        for sig, fid, h, body in recs:
            if fid in ube_addons:
                u.append((sig, fmap_ube(fid), h, remap_body(sig, body, fmap_ube)))
                continue
            if sig == 'ARMO':
                u.append((sig, fid, h, remap_body(sig, body, fmap_ube)))
                body = strip_ube(body)
            c.append((sig, fmap_core(fid), h, remap_body(sig, body, fmap_core)))
        if c:
            core.append((label, ghdr, c))
        if u:
            ube.append((label, ghdr, u))
    order = {'ARMO': 0, 'ARMA': 1}
    ube.sort(key=lambda g: order[g[0]])
    os.makedirs(out_dir, exist_ok=True)
    write(os.path.join(out_dir, CORE), tes4, MASTERS_CORE, core)
    write(os.path.join(out_dir, UBE), tes4, MASTERS_UBE, ube)
    print(f'{CORE}: {sum(len(r) for _, _, r in core)} records; {UBE}: {sum(len(r) for _, _, r in ube)} records')


if __name__ == '__main__':
    main()
