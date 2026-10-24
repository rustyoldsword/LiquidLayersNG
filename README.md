# Liquid Layers NG

Source code of Liquid Layers NG, a Skyrim SE/AE (1.6.1170) SKSE plugin that puts the animated creampie of
TullAnimatedCreampie and the liquid layers of [Predator] Liquid Pack v2 on SexLab P+ climaxes - no SL Triggers, no
Papyrus timers.

The released mod (plugin, light ESP, meshes, textures, BodySlide projects, installer) is distributed separately; this
repository holds only the code that builds it.

## How it works

- `LLNG_Listener` (Papyrus, a start-game-enabled quest) listens for SexLab P+'s per-actor `SexlabOrgasmSeparate` mod
  event - a Papyrus-only event SKSE plugins cannot see - asks P+ 2.19 who received the climax and how
  (`GetPartnersByInteractionType`, falling back to the scene tags without collision data) and calls the native
  `LiquidLayersNG.Climax(giver, receiver, kind)`.
- `Layers` decides the pieces (vaginal / anal: animated creampie + floor puddle + Liquid Bottom; oral: mouth + Liquid
  Face; facial: Liquid Face), equips them silently with `ActorEquipManager`, skips any piece whose slot already holds
  an item of the actor's, and keeps per-actor timers: animation in real seconds, liquid in game hours.
- Who is in a scene comes from P+'s thread events (`AnimationStart` / `StageStart` / `AnimationEnd`, SKSE mod
  callbacks). Nothing comes off during a scene or in the second after it: taking a piece off while SexLab redresses
  the actor and rebuilds its 3D leaves the geometry on the skeleton. The floor puddle comes off when the scene ends.
- `Ticker` drives the timers four times a second from a thread (paused time does not count; `PlayerCharacter::Update`
  does not run during a P+ scene, so a frame hook would stop there).
- Washing: deep water or swimming, Bathing in Skyrim (`BiS_WashActorFinish`, through the listener), optionally rain;
  Bottom/Torso come off under body clothing. State is kept in the SKSE co-save (`LLNG` / `WORN`).
- `Menu` - the SKSE Menu Framework page; `Settings` - `LiquidLayersNG.ini` (see `data/`).

## Building

### Plugin

Requires [xmake](https://xmake.io) 3.x and Visual Studio 2022, plus two libraries placed in `plugin/lib/` (not
included):

- [CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR/tree/ng) (alandtse's `ng` branch) at
  `plugin/lib/CommonLibSSE-NG`
- [SKSE-Menu-Framework-3-API](https://github.com/QTR-Modding/SKSE-Menu-Framework-3-API) at
  `plugin/lib/SKSE-Menu-Framework-3-API`

```
cd plugin
xmake f -m releasedbg --skyrim_vr=n
xmake build LiquidLayersNG
```

### Papyrus

Compile `papyrus/src` with the game's Papyrus Compiler, importing `papyrus/src`, `papyrus/stubs` (minimal SexLab P+
stubs - the real P+ sources do not parse with the stock compiler) and the SKSE script sources.

### Plugin file

`Liquid Layers NG.esp` merges TullAnimatedCreampie, [Predator] Liquid Pack v2 and their UBE patches into one light
plugin. `tools/fix_form39.py` brings the LE-era (form 39) records up to form 44 (BODT to BOD2, LE texture hashes
dropped); `tools/esplist.py` lists a plugin's records.

## Credits

- Tull - TullAnimatedCreampie
- PredatorRJ - [Predator] Liquid Pack v2
- The authors of the UBE patches for both mods
- CommonLibSSE-NG, SKSE Menu Framework

## License

The code in this repository is MIT (see `LICENSE`). The assets of the original mods are not part of it.
