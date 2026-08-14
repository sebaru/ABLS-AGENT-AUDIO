# abls-agent-audio

Standalone audio runtime for Abls-Habitat.

## Current implementation status

- Runtime based on `abls-agent-libs`
- Keeps SRC audio business logic:
  - receives `AUDIO_ZONE/<zone>` messages
  - plays TTS via `gtts-cli`
  - plays mp3 via `mpg123`
  - sets sink volume with `wpctl`
  - emits periodic jingle when no message is received for a while

Expected API config fields:

- `language` (default: `fr`)
- `volume` (0-100)
- `audio_zones` array with objects containing `audio_zone_name`

## Build

```sh
./install_deps.sh
./build.sh
```

## Packaging RPM

```sh
./build_rpm.sh
```

Produces runtime RPM package in `build/`.

## Packaging DEB

```sh
./build_apt.sh --dist bookworm
./build_apt.sh --dist trixie
```

Default target suite is detected from host OS codename (`/etc/os-release`), with `bookworm` fallback.

Useful options:

- `--version-suffix <s>`: override Debian version suffix (example `~trixie`)
- `--no-dist-suffix`: disable automatic `~<suite>` suffix

Produces runtime DEB package and copies normalized artifacts to:

- `build/deb/<suite>/<arch>/`

`build_apt.sh` builds only the native host architecture.

Package signatures are centralized in ABLS-PKGS (both DEB repository metadata and RPM package/repository signatures).

## Release bump + publication

```sh
./bump.sh 1.2.3
```

The release flow:

- tags `v1.2.3` from `trunk`
- merges `trunk` into `main`
- builds RPM + DEB packages
- copies RPM to `../ABLS-PKGS/public/rpms/<arch>/`
- copies DEB to `../ABLS-PKGS/deb-packages/<suite>/<arch>/`
