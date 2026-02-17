# Research

## Notes and references for harder tasks

- Figure out how to read level names and locations from ROM files
  - Sonic Retro's disassembly and ROM hacking guides are a practical reference for where zone pointers and title cards live in the classic games: https://info.sonicretro.org/Disassemblies
  - Sonic 2 disassembly repo (zone order, level headers, pointer tables): https://github.com/sonicretro/s2disasm
  - Sonic 3 & Knuckles disassembly repo: https://github.com/sonicretro/skdisasm

- Support for loading levels from decompilations / disassemblies
  - Clownacy's guide to split disassemblies helps map assets and pointer macros to files: https://clownacy.wordpress.com/2021/04/19/the-simplest-way-to-set-up-a-split-sonic-1-disassembly/
  - ASM68K syntax notes (needed when parsing/consuming labels and `dc.*` data): http://www.retrodev.com/asm68k/

- Sprite preview support (Nemesis de-/compression)
  - Existing C implementation and docs for the Nemesis format: https://info.sonicretro.org/Nemesis_Compression
  - Sonic Retro Compression guide (context for Kosinski/Nemesis/Enigma formats): https://info.sonicretro.org/Compression

- Sonic Mania support
  - RSDKv5 decompilation docs and format notes are a good starting point for scene/tile/chunk formats: https://github.com/RSDKModding/RSDKv5-Decompilation

- Audio extraction
  - SMPS format overview and tooling references (for Mega Drive titles): https://info.sonicretro.org/SMPS
