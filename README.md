# localbin

A minimal local binary manager for macOS. Installs executables into `~/.localbin`, tracks them with SHA256 checksums and JSON metadata, and handles upgrades with optional pre/post hooks.

## Install

```bash
curl -fsSL https://raw.githubusercontent.com/svaldesoliva/localbin/main/install.sh | bash
```

Clones, compiles, installs to `~/.localbin`, and patches your shell config (`zsh`, `bash`, `fish`, `csh`, or `~/.profile`).

### Manual

```bash
git clone https://github.com/svaldesoliva/localbin.git && cd localbin
make && make install
```

## Usage

```bash
# install
localbin install ./mytool
localbin install ./mytool --version 1.2.3 --as tool --alias t
localbin install https://example.com/binary --version 2.0

# hooks (run before/after update)
localbin install ./mytool \
  --pre-update-hook ~/.localbin/hooks/pre.sh \
  --post-update-hook ~/.localbin/hooks/post.sh

# manage
localbin list [--sort name|date|size] [--json]
localbin info  <name>
localbin search <term>
localbin update <name> <file>
localbin remove <name>

# integrity
localbin verify <name>
localbin verify --all

# system
localbin doctor
localbin setup
localbin self-update [--manual]
```

## Build

```
make          build
make install  install to ~/.localbin
make debug    build with -g and sanitizers off
make test     run help + doctor
make analyze  clang static analysis
make format   clang-format all sources
make clean    remove build artifacts
```

## How it works

Each installed binary gets a `~/.localbin/.metadata/<name>.json` file recording the source path, version, SHA256, install/update timestamps, size, permissions, optional alias, and hook scripts. `verify` and `doctor` re-hash the live binary and compare.

Backups are written to `~/.localbin/.backups/` on every `update`.

## License

MIT
