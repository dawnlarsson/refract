# Refract
the Linux Kernel with the [Prism C transpiler](https://github.com/dawnlarsson/prism)
Aims to modernize parts of the Linux kernel with Prism's features, reducing bug surface, and removing any pre-2009 hardware support for a more modern & lean kernel

this is also partly a testing ground for a new Linux Distro based on Prism

## Build
```sh
sh kernel/build.sh          # does everything: setup, init, build
sh kernel/build.sh run      # boot in QEMU
```

`setup` downloads and GPG-verifies the latest stable kernel. `init` extracts it, applies patches, drops dead files, and overlays Prism-enhanced sources. `build` compiles with `CC=prism`.

## Prism
[Prism](https://github.com/dawnlarsson/prism) is a C transpiler that adds `defer`, `orelse`, and automatic zero-initialization to C. It compiles as a single file, has zero dependencies, and works as a drop-in `CC=prism` overlay — all GCC/Clang flags pass through automatically.

This repo tracks the latest `main` branch. To install:
```sh
git clone https://github.com/dawnlarsson/prism
cd prism
cc prism.c -flto -s -O3 -o /tmp/prism && /tmp/prism install && rm /tmp/prism
```

The kernel is built with `-fno-safety -fno-zeroinit` — only `defer` and `orelse` are active. Safety checks and zero-init conflict with kernel internals (inline asm, intentional goto-over-declaration patterns, explicit initialization control).

### Structure
- `kernel/base.config` — minimal Kconfig fragment (allnoconfig base)
- `kernel/patches/*.sh` — source-level patches (sed scripts run inside the tree)
- `kernel/drop` — manifest of files to remove and their Makefile references
- `kernel/src/` — Prism-enhanced kernel files, mirroring the tree structure

Each `init` produces three git commits: upstream → drops → overlays, so `git diff` always shows exactly what changed.

## Copyright
anything in `kernel/` directory is GPL-2.0 or other licenses with its multiple respective authors from the modified linux source code.

Prism itself is Apache 2.0, and this repo as well (that's not in `kernel/`)