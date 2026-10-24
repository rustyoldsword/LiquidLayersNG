# Bring the LE-era (form 39) TullAnimatedCreampie records in Liquid Layers NG.esp up to form 44:
#   BODT (slots, flags, pad, armor type) -> BOD2 (slots, armor type)
#   drop MOxT texture-hash subrecords written in the LE layout (optional data, the engine rebuilds it)
#   set the record form version to 44
# Only records whose EditorID starts with "TullAnimated" are touched. Re-runnable.
# The plugin holds only flat top-level groups (ARMO/ARMA/TXST/SNDR/QUST without children), so group
# sizes are recomputed by walking each top-level group's records.
import struct, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import esplist as E

HASHES = {b'MODT', b'MO2T', b'MO3T', b'MO4T', b'MO5T'}


def rebuild(b, off, size):
    body = bytearray()
    for _, sig, data in E.subrecords(b, off, size):
        s = sig.encode()
        if s in HASHES:
            continue
        if s == b'BODT':
            slots, armor = struct.unpack_from('<I', data, 0)[0], struct.unpack_from('<I', data, 8)[0]
            s, data = b'BOD2', struct.pack('<II', slots, armor)
        body += s + struct.pack('<H', len(data)) + bytes(data)
    return bytes(body)


def main(path):
    b = bytearray(open(path, 'rb').read())
    targets = []
    for off, sig, fid, ver, flags, size in E.records(b):
        if sig not in ('ARMA', 'ARMO') or flags & 0x40000:
            continue
        edid = next((d.rstrip(b'\0') for _, s, d in E.subrecords(b, off, size) if s == 'EDID'), b'')
        if edid.startswith(b'TullAnimated'):
            targets.append((off, size, edid.decode()))
    for off, size, edid in reversed(targets):  # back to front: earlier offsets stay valid
        body = rebuild(b, off, size)
        b[off+24:off+24+size] = body
        struct.pack_into('<I', b, off+4, len(body))
        struct.pack_into('<H', b, off+20, 44)
        print(f'{edid}: {size} -> {len(body)} bytes')
    i = 24 + struct.unpack_from('<I', b, 4)[0]
    while i < len(b):
        assert bytes(b[i:i+4]) == b'GRUP'
        j = i + 24
        while j < len(b) and bytes(b[j:j+4]) != b'GRUP':
            j += 24 + struct.unpack_from('<I', b, j+4)[0]
        struct.pack_into('<I', b, i+4, j - i)
        i = j
    open(path, 'wb').write(b)


if __name__ == '__main__':
    main(sys.argv[1])
