# SIDBlaster

*SIDBlaster 0.7.0 - C64 SID Music File Processor*

Developed by Robert Troughton (Raistlin of Genesis Project)

## Overview

SIDBlaster is a versatile tool for processing C64 SID music files. It provides several key functions:

- **Player**: Convert SID files to executable PRG files with various player routines
- **Relocate**: Move SID music to different memory addresses while preserving functionality
- **Disassemble**: Convert SID files to human-readable assembly language
- **Trace**: Analyze SID register access patterns for debugging and verification

## Basic Usage

```
SIDBlaster [command] [options] inputfile [outputfile]
```

## Commands

SIDBlaster supports the following main commands:

### `-player[=<type>]` 
Links a SID file with a player routine to create an executable PRG file.

```
SIDBlaster -player music.sid music.prg
```

Options:
- Use `-player` for the default player (SimpleRaster)
- Use `-player=<type>` to specify a different player (e.g., `-player=SimpleBitmap` or `-player=RaistlinBars`)
- `-playeraddr=<address>`: Player load address (default: $4000)

### `-relocate=<address>`
Relocates a SID file to a different memory address. By default, performs verification to ensure the relocated file behaves identically to the original.

```
SIDBlaster -relocate=$2000 music.sid relocated.sid
```

The address parameter is required and specifies the target memory location (e.g., $2000).

Options:
- `-noverify`: Skip verification after relocation (faster, but less safe)

### `-disassemble`
Disassembles a SID file to assembly code.

```
SIDBlaster -disassemble music.sid music.asm
```

### `-trace[=<file>]`
Traces SID register writes during emulation.

```
SIDBlaster -trace music.sid
```

Options:
- Use `-trace` to output to default file (trace.bin in binary format)
- Use `-trace=<file>` to specify output file
  - Files with .txt or .log extension use text format
  - Files with other extensions use binary format
- `-frames=<num>`: Number of frames to emulate (default: 30000)

## General Options

These options can be used with any command:

- `-verbose`: Enable verbose logging
- `-force`: Force overwrite of output file
- `-log=<file>`: Log file path (default: SIDBlaster.log)
- `-kickass=<path>`: Path to KickAss.jar assembler
- `-exomizer=<path>`: Path to Exomizer compression tool
- `-nocompress`: Disable compression for PRG output

## SID Metadata Options

- `-title=<text>`: Override SID title
- `-author=<text>`: Override SID author
- `-copyright=<text>`: Override SID copyright
- `-sidloadaddr=<address>`: Override SID load address
- `-sidinitaddr=<address>`: Override SID init address
- `-sidplayaddr=<address>`: Override SID play address

## Configuration System

SIDBlaster features a comprehensive configuration system that allows you to customize default settings. When you first run SIDBlaster, it automatically creates a configuration file (`SIDBlaster.cfg`) that contains all available settings with their default values.

### Configuration File Location

SIDBlaster looks for the configuration file in the following locations, in order:
1. The current working directory
2. The executable's directory

### Editing the Configuration File

The configuration file is a plain text file that you can edit with any text editor. Each setting is defined in a `key=value` format, with sections organized by comments.

Example configuration entries:
```
# Path to KickAss jar file (include 'java -jar' prefix if needed)
kickassPath=java -jar KickAss.jar -silentMode

# Number of frames to emulate for analysis and tracing
emulationFrames=30000
```

### Common Configuration Settings

Here are some common settings you might want to customize:

#### Tool Paths
- `kickassPath`: Path to KickAss assembler (e.g., `java -jar C:\Tools\KickAss.jar -silentMode`)
- `exomizerPath`: Path to Exomizer compression tool
- `pucrunchPath`: Path to Pucrunch compression tool (alternative to Exomizer)
- `compressorType`: Preferred compression tool (`exomizer` or `pucrunch`)

#### Player Settings
- `playerName`: Default player routine to use (e.g., `SimpleRaster`, `SimpleBitmap`)
- `playerAddress`: Default memory address for player code (e.g., `$4000`)
- `playerDirectory`: Directory containing player code files

#### Emulation Settings
- `emulationFrames`: Number of frames to emulate (default: `30000`, about 10 minutes of C64 time)
- `cyclesPerLine`: CPU cycles per scan line (PAL: `63.0`, NTSC: `65.0`)
- `linesPerFrame`: Scan lines per frame (PAL: `312.0`, NTSC: `263.0`)

#### Logging Settings
- `logFile`: Default log file path
- `logLevel`: Logging detail level (1=Error, 2=Warning, 3=Info, 4=Debug)

#### Development Settings
- `debugComments`: Include debug comments in generated assembly (`true` or `false`)
- `keepTempFiles`: Preserve temporary files after processing (`true` or `false`)

### When to Use Configuration vs. Command Line

- **Configuration File**: Use for persistent changes that you want to apply to all operations (e.g., paths to external tools, emulation performance settings, default player preferences)
- **Command Line Options**: Use for operation-specific settings that vary by task (e.g., specific relocate addresses, trace file names)

### Configuration Updates

The configuration file is automatically updated with new settings when SIDBlaster adds features. Your custom settings will be preserved during updates.

## Player Types

SIDBlaster includes several player routines:

- **SimpleRaster**: Basic raster-based player (default)
- **SimpleBitmap**: Player with bitmap display capabilities

Additional player types may be available in the SIDPlayers directory.

## Examples

### Convert SID to PRG with default player:

```
SIDBlaster -player music.sid music.prg
```

### Convert SID with specific player:

```
SIDBlaster -player=SimpleBitmap music.sid player.prg
```

### Relocate SID to address $2000:

```
SIDBlaster -relocate=$2000 music.sid relocated.sid
```

### Relocate SID without verification:

```
SIDBlaster -relocate=$2000 -noverify music.sid relocated.sid
```

### Disassemble SID to assembly:

```
SIDBlaster -disassemble music.sid music.asm
```

### Trace SID register writes to default output:

```
SIDBlaster -trace music.sid
```

### Trace SID register writes to custom file:

```
SIDBlaster -trace=music.log music.sid
```

### Trace with specific number of frames:

```
SIDBlaster -trace -frames=1000 music.sid
```

## Supported File Formats

- **Input**: SID files (.sid)
- **Output**: 
  - PRG files (.prg) - Executable Commodore 64 programs
  - SID files (.sid) - Commodore 64 music format
  - ASM files (.asm) - Assembly language source code

## Requirements

- Java Runtime Environment (for KickAss assembler)
- KickAss Assembler (for assembling code)
- Exomizer (for compression, optional)

## Technical Details

SIDBlaster includes a complete 6510 CPU emulator to analyze SID files and ensure accurate relocation and disassembly. It tracks memory access patterns to identify code, data, and jump targets, producing high-quality disassembly output with meaningful labels.

The relocation verification process traces SID register writes from both the original and relocated files to ensure they behave identically, guaranteeing that the relocation preserves all musical features.

## Acknowledgements

- Zagon for Exomizer
- Mads Nielsen for KickAss assembler