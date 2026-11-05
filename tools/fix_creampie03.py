"""AnimatedCreampie03.nif (TullAnimatedCreampie): make the drop vanish at once instead of flying away.

The mesh is rigid, attached to NPC Pelvis, and its translation keys slide the drop along the pelvis Z axis:
12 -> -1 over 0..4 s (the drip), then -1 -> -105 over 4..5 s, which hides it under the floor of a STANDING actor.
That last segment is linear, so the drop visibly flies 104 units along the pelvis axis - sideways or up when the actor
lies down (looping at half speed: a 2-second flight every 24 s). The key at 5.0 s moves to 4.01 s, so the hiding
jump takes one frame; every other key is unchanged.

Idempotent. Usage: python -I fix_creampie03.py <AnimatedCreampie03.nif>
"""
import struct
import sys

OLD = struct.pack('<4f', 5.0, -1.0, 0.0, -105.0)
NEW = struct.pack('<4f', 4.01, -1.0, 0.0, -105.0)


def main(path):
    data = open(path, 'rb').read()
    if data.count(NEW) == 1 and data.count(OLD) == 0:
        print('already fixed')
        return
    if data.count(OLD) != 1:
        raise SystemExit(f'expected exactly one 5.0 s / -105 key, found {data.count(OLD)}')
    open(path, 'wb').write(data.replace(OLD, NEW))
    print('fixed: hiding key 5.0 s -> 4.01 s')


if __name__ == '__main__':
    main(sys.argv[1])
