# Projects

## Design sketch

The `Game` interface is the abstraction used by the rest of the application. It exposes compatibility checks, level names, level loading and save capabilities without specifying where the game data comes from. Although the current `Sonic2Rom` and `Sonic3Rom` implementations own a `Rom`, resolve hard-coded pointer tables and construct `Level` objects that read directly from that ROM, the interface itself should be adaptable to games backed by disassemblies.

A disassembly spreads a game's data across many files. Projects are intended to make those individual components easier to identify, locate and manage as a unit. A project records the game and disassembly layout it targets, its root directory and the files that provide components such as palettes, patterns, blocks, chunks and maps.

## Proposed responsibilities

`Game` remains the application-facing abstraction. Its existing interface answers questions such as:

- Is the source compatible with Sonic 2, Sonic 3 or another game?
- Which levels are available?
- How is a level loaded?
- Can levels be relocated or saved?
- How is an edited `Level` saved?

Concrete game implementations remain responsible for game-specific details such as pointer tables, compression formats, map dimensions, palette composition and level naming. Those implementations may be backed by a ROM or by the components in a disassembly project.

`Project` manages a disassembly and its components. It answers questions such as:

- Which game and supported disassembly layout does this project use?
- Where is the project root?
- Which file or files provide each component?
- Does a component exist, and is it writable?
- How should a component be opened or replaced?

Projects should use explicit mappings for supported disassemblies rather than trying to discover components through loose filename searches. Sonic 2 and Sonic 3 disassemblies need not organise equivalent components in the same way.

## Components

A component is a logical piece of game data, such as a level map, palette, pattern set, block set or chunk set. Game code should refer to components by stable logical identifiers, while the project maps those identifiers to paths relative to its root. For example, `sonic2/ehz1/map` might map to `levels/EHZ/Act 1/Layout.kos` in a supported Sonic 2 disassembly.

Some logical data may be assembled from several files, and some files may be shared by several levels. The project should preserve those relationships rather than presenting every component as an unrelated byte stream. This is the main reason for a project abstraction: it gives the editor a coherent view of the individual parts of a disassembly and a place to store the metadata needed to manage them.

The existing codecs operate on `QIODevice`, so a component API can expose streams without coupling game code to ordinary files. Stream ownership should be explicit, probably by returning `std::unique_ptr<QIODevice>`.

## Migration path

1. Introduce a project model containing a root directory, a game identifier, a supported disassembly layout and explicit component mappings.
2. Add a factory path that constructs an appropriate `Game` from a project. Keep `GameFactory::build(std::shared_ptr<Rom>&)` as the existing ROM entry point.
3. Adapt the Sonic game and level-loading code so that it can read palettes, patterns, blocks, chunks and maps from project components as well as ROM addresses. The existing Kosinski readers can continue to consume `QIODevice` streams.
4. Move component save logic behind the same source-specific boundary. ROM-backed games continue to enforce available-space limits and update checksums, while project-backed games replace the relevant disassembly files.
5. Add explicit mappings for known layouts such as `sonicretro/s2disasm`.

## Open design points

- Decide whether ROM-backed games and project-backed games should be separate concrete `Game` implementations or share source-independent Sonic 2 and Sonic 3 logic.
- Define how components composed from multiple files, and files shared by multiple levels, are represented.
- Decide whether project metadata belongs in a checked-in manifest, an editor-specific file or a mapper selected for a known disassembly layout.
- Saving assembly source may require preserving labels and include structure, not just replacing binary include files.
