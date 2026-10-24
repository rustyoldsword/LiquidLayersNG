# List records of a plugin: signature, FormID, form version, EditorID, flags.
import struct, sys

def records(b):
    out = []
    def walk(s, e):
        i = s
        while i < e:
            sig = bytes(b[i:i+4]); size = struct.unpack_from('<I', b, i+4)[0]
            if sig == b'GRUP':
                walk(i+24, i+size); i += size; continue
            flags, fid = struct.unpack_from('<II', b, i+8)
            ver = struct.unpack_from('<H', b, i+20)[0]
            out.append((i, sig.decode(), fid, ver, flags, size))
            i += 24 + size
    walk(0, len(b))
    return out

def subrecords(b, off, size):
    i, e = off+24, off+24+size
    while i < e:
        sig = bytes(b[i:i+4]); n = struct.unpack_from('<H', b, i+4)[0]
        yield i, sig.decode(), b[i+6:i+6+n]
        i += 6+n

if __name__ == '__main__':
    b = open(sys.argv[1], 'rb').read()
    for off, sig, fid, ver, flags, size in records(b):
        edid = ''
        if sig != 'TES4' and not flags & 0x40000:
            for _, s, d in subrecords(b, off, size):
                if s == 'EDID': edid = d.rstrip(b'\0').decode('cp1252'); break
        print(f'{sig} {fid:08X} v{ver} fl={flags:X} {edid}')
