# Sprites

## Scope

Level objects and sprites are separate concepts:

- An **object placement** stores an object ID, subtype, position, and flags.
- A **sprite mapping** describes the VDP pieces in one frame.
- **Art** provides the 8x8 patterns used by those pieces.
- A **DPLC** selects art ranges and loads them into an object's VRAM space.
- Runtime object code selects frames, art, palettes, and animation.

The six placement bytes do not contain enough information to reproduce an object's runtime appearance. Start with read-only Sonic 2 object and ring overlays using static previews. Add editing and other games after parsing and rendering are reliable.

## Formats

All multibyte values are big-endian.

### Sonic 2 object placements

Each entry is six bytes:

1. 16-bit X position;
2. 16-bit Y and flags: 12-bit Y, unused bit, X flip, Y flip, remember-state;
3. 8-bit object ID;
4. 8-bit subtype.

Entries are sorted by X and end with `FFFF 0000 0000`. Preserve all flags and subtype bits so data can round-trip without loss.

Rings are stored separately in four-byte groups: X followed by `DNNNYYYY YYYYYYYY`. `D` is the direction, `NNN + 1` is the count, and Y is 12 bits. Rings are spaced 0x18 pixels apart. The list ends with `FFFF`.

### Sonic 2 sprite mappings

A frame starts with a 16-bit piece count. Each piece has four words:

1. signed 8-bit Y and an 8-bit size;
2. one-player VDP attributes;
3. two-player VDP attributes;
4. signed 16-bit X.

The size byte encodes tile width in bits 3:2 and height in bits 1:0. Both range from one to four tiles. VDP attributes contain priority, palette, Y/X flip, and an 11-bit pattern index. Keep both attribute words; initially render the one-player value.

A mapping set usually starts with one word offset per frame. The owning object definition must supply bounds because offsets do not identify the table length.

Sonic 3 and Sonic & Knuckles pieces are six bytes and have only one attribute word. Use separate format readers instead of renderer conditionals.

### DPLCs

A Sonic 2 DPLC frame starts with a request count. Each request word stores `tile count - 1` in the high nibble and the source tile offset in the low 12 bits. It copies 1-16 consecutive 32-byte patterns into the frame-local sequence.

Static objects do not need DPLCs. Add DPLC support before character and other dynamically loaded previews. Sonic 3 has two layouts: player DPLCs match Sonic 2, while most other objects store `request count - 1` and reverse the fields.

### Nemesis compression

Nemesis encodes nibble runs using run-length and prefix coding. A stream has:

1. a 16-bit pattern count, with the high bit enabling XOR mode;
2. a variable code table ending in `FF`;
3. an MSB-first bitstream.

Codes are prefix-free and at most eight bits. Prefix `111111` introduces a seven-bit inline run: three bits for `repeat count - 1` and four for the pixel. Decode exactly `pattern count * 32` bytes. In XOR mode, XOR each four-byte row with the previous output row.

Only a decoder is needed. Placement editing does not require recompressing art.

## Implementation plan

### 1. Locate level data

- Support the canonical ROM revision handled by `Sonic2Rom` first.
- Identify it using header data and a known table signature; reject other layouts clearly.
- Keep revision-specific addresses in `src/games/Sonic2Rom.cpp`.
- Resolve bounded object and ring regions through pointer tables where possible.
- Determine writable capacity from region boundaries, not terminators.
- Test Emerald Hill Acts 1/2 and Chemical Plant Act 1. Known Act 1 offsets are `0xE684A`/`0xE4344` for Emerald Hill and `0xEA9D2`/`0xE5E2C` for Chemical Plant (objects/rings).

### 2. Parse placements

- Add non-Qt models such as `ObjectPlacement`, `RingGroup`, and `LevelObjects`.
- Use bounded readers with errors for truncation, missing terminators, invalid bits, and unsorted X positions.
- Add serializers and exact round-trip tests now, even if the first UI is read-only.
- Load placements through `Sonic2Rom::loadLevel` and report malformed layers.

### 3. Decode Nemesis art

- Follow the `KosinskiReader` API and error handling.
- Separate code-table parsing from bitstream decoding.
- Support normal and XOR modes and output standard Mega Drive pattern data.
- Reject conflicting codes, invalid lengths, truncation, and output overruns.
- Test dictionary codes, inline runs, byte boundaries, XOR rows, invalid input, and exact output sizes. Optionally validate a ROM asset by count and digest.
- Fuzz malformed streams after deterministic tests are stable.

### 4. Parse mappings and DPLCs

- Add neutral `SpritePiece`, `SpriteFrame`, and `DplcRequest` models.
- Use game-specific readers for Sonic 2 and Sonic 3 layouts.
- Bound frame and piece counts and validate all referenced pattern ranges.
- Materialize DPLC tiles separately from source art and mapping attributes.
- Test all sizes, signed coordinates, flips, palettes, two-player attributes, and DPLC range limits.

### 5. Define objects

Add a per-game catalog keyed by object ID, with optional subtype and zone variants. Entries describe the display name, art and compression, mappings, preview frame, optional DPLC, VRAM adjustment, palette, subtype rules, and a fallback marker.

Start with Sonic 2 springs (`0x41`), spikes (`0x36`), monitors (`0x26`), and rings. Known art locations include vertical springs at `0x78E84`, monitors at `0x79550`, and spikes at `0x7995C`. Validate absolute addresses against ROM signatures. Keep catalog data out of `MapEditor` switch statements and show labelled markers for unsupported objects.

### 6. Render sprites

- Create a non-widget `SpriteRenderer` that produces a transparent `QImage` and origin from patterns, a frame, palettes, and object flips.
- Apply piece flips before object flips. Treat palette index zero as transparent.
- Preserve priority, initially using two scene Z bands.
- Cache decoded art and rendered preview variants.
- Test orientation, bounds, overlap, transparency, and palettes with synthetic patterns.

### 7. Show read-only overlays

- Add separate object and ring layers to `MapEditor` with visibility controls.
- Place items in pixel coordinates and show ID, subtype, and position.
- Use labelled vector markers for unsupported or invisible objects.
- Expand ring groups only for display; keep groups in the model.
- Check scrolling, zooming, and objects crossing chunk boundaries.

### 8. Edit and save placements

- Add selection, drag, insert/delete, flips, and subtype edits through undoable commands.
- Keep objects sorted by X and make ring grouping visible in the UI.
- Validate values, order, and serialized size before saving.
- Refuse growth beyond the known allocation until relocation is designed.
- Serialize and validate all changes before writing, then update the checksum.
- Test no-op and changed round trips using temporary ROM-like buffers.

### 9. Extend support

- Grow the Sonic 2 catalog by zone and usage frequency.
- Add standalone Sonic 3 only after its address tables are understood. Do not reuse Sonic 3 & Knuckles addresses.
- Keep Sonic 3 read-only while `Sonic3Rom::canSave()` is false. Reuse Kosinski decoding where appropriate, but implement its mapping, ring, and DPLC formats explicitly.
- Map disassembly labels and files into the same model interfaces.

## First-release target

- Bounded Sonic 2 object and ring parsing.
- Safe normal and XOR Nemesis decoding.
- Correct static mappings, DPLCs, palettes, and flips.
- ROM-based previews for springs, spikes, and monitor subtypes.
- Labelled fallbacks for all other objects.
- Independent object and ring overlay controls that stay aligned when scrolling and zooming.
- No writes until allocation limits and transactional saving are implemented.

## Sources

- [Sonic Retro: Sonic 2 level editing](https://info.sonicretro.org/SCHG:Sonic_the_Hedgehog_2_(16-bit)/Level_Editing)
- [Sonic Retro: Sonic 2 object editing](https://info.sonicretro.org/SCHG:Sonic_the_Hedgehog_2_(16-bit)/Object_Editing)
- [Sonic Retro: Sonic 2 object pointers](https://info.sonicretro.org/SCHG:Sonic_the_Hedgehog_2_(16-bit)/Object_Editing/Pointers)
- [Sonic Retro: Sonic 2 art editing](https://info.sonicretro.org/SCHG:Sonic_the_Hedgehog_2_(16-bit)/Art_Editing)
- [Sonic Retro: Nemesis art locations](https://info.sonicretro.org/SCHG:Sonic_the_Hedgehog_2_(16-bit)/Art_Editing/Nemesis_Compressed)
- [Sonic Retro: Sonic 2 level-specific data](https://info.sonicretro.org/SCHG:Sonic_the_Hedgehog_2_(16-bit)/Level_Specific)
- [Sonic Retro: sprite mappings](https://info.sonicretro.org/Sprite_mappings)
- [Sega Retro: Nemesis compression](https://segaretro.org/Nemesis_compression)
- [Sonic Retro: Sonic 3 & Knuckles object editing](https://info.sonicretro.org/SCHG:Sonic_the_Hedgehog_3_%26_Knuckles/Object_Editing)
- [Sonic Retro: Sonic 3 & Knuckles level editing](https://info.sonicretro.org/SCHG:Sonic_the_Hedgehog_3_%26_Knuckles/Level_Editing)
