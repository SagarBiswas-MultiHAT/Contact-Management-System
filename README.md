# Contact Manager (C11)

**Production-grade, CLI-first contact manager in C11** — a single small, portable binary that is safe to use interactively or from automation scripts. It stores data in SQLite, supports Argon2id password hashing (via _libsodium_ or _libargon2_), and includes robust CSV import/export, JSON output, and a keyboard-friendly interactive menu.

---

## Interface:

```
PS E:\New folder\GitHub Clone\Probe-TheRepo0fTesting> ./build-mingw/contacts.exe --menu

..:: Enter password: Sagar

                                === Contact Manager ===

                01. Add Contact
                02. List Contacts
                03. Search Contacts
                04. Edit Contact
                05. Delete Contact
                06. Export CSV
                07. Import CSV
                08. Set Password
                09. Statistics
                10. Sort Contacts
                0. Exit

==>Select:
```

---

## Quick summary

- **Single binary**: use from a terminal or in scripts.
- **Storage**: SQLite database (`contacts.db` by default) with migrations and backups.
- **Security**: Argon2id password hashing (libsodium preferred; libargon2 fallback), no plaintext passwords.
- **Modes**: interactive menu and full non-interactive CLI for automation.
- **Output**: human-friendly terminal UI plus `--json` for machine consumption.
- **Safety**: `--backup-before` snapshots and transactional imports to avoid partial writes.

> Goal: make the program understandable and scriptable — anyone reading this README should be able to build, run, and automate the app.

---

## Highlights (at a glance)

- Add, list, search, edit, and delete contacts
- Persisted sort preference: `name`, `phone`, or `due-date`
- Due-date awareness: `on-time`, `due soon`, `overdue` status and stats
- CSV import/export with strict quoting and `--dry-run`
- JSON mode for `jq`, Python, etc.
- Argon2id password management with interactive and scripted flows
- Unit and integration tests (cmocka)

---

## Table of contents

1. [Requirements](#requirements)
2. [Building](#building)
3. [Quick start & examples](#quick-start--examples)
4. [Commands / CLI reference](#commands--cli-reference)
5. [Interactive mode features](#interactive-mode-features)
6. [Data rules & behavior guarantees](#data-rules--behavior-guarantees)
7. [Testing](#testing)
8. [Security notes](#security-notes)
9. [Troubleshooting](#troubleshooting)
10. [Project layout & design decisions](#project-layout--design-decisions)
11. [Contributing & CI suggestions](#contributing--ci-suggestions)
12. [License](#license)

---

## Requirements

- C11-capable C compiler (GCC, Clang, MSVC)
- CMake 3.15+
- SQLite3 dev headers and library
- Either **libsodium** _or_ **libargon2** (for Argon2id hashing)
- **cmocka** for tests (optional at runtime)

Platform install examples (package names vary by distro):

- **Ubuntu / Debian**

```bash
sudo apt update
sudo apt install build-essential cmake sqlite3 libsqlite3-dev libsodium-dev cmocka libcmocka-dev
```

- **macOS (Homebrew)**

```bash
brew install cmake sqlite libsodium cmocka
```

- **Windows (in MSYS2 MINGW64)**

```bash
# UCRT64 (recommended — prefers libsodium)
pacman -S mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-sqlite3 mingw-w64-ucrt-x86_64-libsodium mingw-w64-ucrt-x86_64-cmocka

# If you need libargon2 (not always available in UCRT64), use the MINGW64 packages
# which include libargon2, or build libargon2 yourself and make it visible to pkg-config.
# MINGW64 alternative (includes libargon2):
pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-sqlite3 mingw-w64-x86_64-libargon2 mingw-w64-x86_64-pkg-config mingw-w64-x86_64-cmocka
```

> If you use MSVC, prefer `vcpkg` to install dependencies and pass the vcpkg toolchain to CMake.

---

## Building

### MinGW / MSYS2 (matches CI/test environment)

```powershell
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_C_COMPILER="C:/msys64/ucrt64/bin/gcc.exe" -DCMAKE_CXX_COMPILER="C:/msys64/ucrt64/bin/g++.exe"
cmake --build build-mingw
```

---

## Quick start & examples

Build, then run the interactive menu:

```bash
# after building
./build-mingw/contacts.exe --menu
```

Add a contact non-interactively (script-friendly):

```bash
./contacts --add --name "Sagar" --phone "01828361088" --email "sagar@example.com" --due 19.99 --due-date 2026-02-01 --address "xyz.earth"
```

List contacts in JSON for piping to `jq`:

```bash
./contacts --list --json | jq
```

Import CSV using a dry-run first:

```bash
./contacts --backup-before --import new_leads.csv --dry-run
# once happy
./contacts --import new_leads.csv
```

Non-interactive password rotation:

```bash
./contacts --set-password --current-password "oldpass" --new-password "supersecure" --yes
```

Automated overdue filter (example):

```bash
./contacts --list --sort due-date --json | jq '.[] | select(.status == "overdue")'
```

---

## Commands — quick CLI reference

All commands support `--db PATH` to override the default database file and `--backup-before` for destructive operations.

| Command           |                                                                      Purpose | Usage examples                                                                                     |           |                          |
| ----------------- | ---------------------------------------------------------------------------: | -------------------------------------------------------------------------------------------------- | --------- | ------------------------ |
| `--menu`          |                                               Interactive keyboard-driven UI | `./contacts --menu`                                                                                |           |                          |
| `--add`           | Add contact (requires `--name`, `--phone`, `--email`, `--due`, `--due-date`) | `./contacts --add --name "Bob" --phone "1" --email b@example.com --due 10.5 --due-date 2026-03-01` |           |                          |
| `--list`          |                                 Show contacts; combine `--sort` and `--json` | `./contacts --list --sort due-date --json`                                                         |           |                          |
| `--search <term>` |                                   Case-insensitive match on name/email/phone | `./contacts --search Alice`                                                                        |           |                          |
| `--edit <id>`     |                                        Update provided fields for numeric ID | `./contacts --edit 12 --phone "555-0099"`                                                          |           |                          |
| `--delete <id>`   |                               Delete by ID; use `--yes` to skip confirmation | `./contacts --delete 8 --yes --backup-before`                                                      |           |                          |
| `--export <file>` |                                     Export CSV (or `--json` for JSON export) | `./contacts --export all.csv`                                                                      |           |                          |
| `--import <file>` |                                 Import CSV; use `--dry-run` to validate only | `./contacts --import leads.csv --dry-run`                                                          |           |                          |
| `--sort <key>`    |                                              Persist default sort key: `name | phone                                                                                              | due-date` | `./contacts --sort name` |
| `--stats`         |                     Print totals and letter distribution; `--json` supported | `./contacts --stats --json`                                                                        |           |                          |
| `--set-password`  | Set/rotate Argon2id password; supports `--current-password`/`--new-password` | `./contacts --set-password --current-password old --new-password new --yes`                        |           |                          |

Notes:

- Editing and deletion require **numeric IDs** to avoid ambiguity.
- Dates are **ISO 8601** (`YYYY-MM-DD`) and validated.

---

## Interactive mode features

When you run `--menu`:

- A compact table shows the current date and each contact’s due status badge (`on-time`, `due soon`, `overdue`).
- Add/Edit flow validates fields step-by-step (non-empty name/phone, ISO date check, numeric `--due` parsed as `double`).
- CSV import has auto-detect for quoting style and a `--dry-run` preview. Import runs inside a transaction and will roll back on error.

---

## Data rules & guarantees

- **Due dates**: strictly `YYYY-MM-DD` (ISO 8601). Invalid dates are rejected.
- **Due amounts**: stored as `double`; omitting `--due` defaults to `0.0`.
- **Identity**: contacts are identified by an immutable numeric ID. Name/phone duplicates are allowed; editing/deleting by name is intentionally unsupported.
- **Atomicity**: imports and other multi-row operations use transactions so partial writes don’t occur.
- **Backups**: `--backup-before` creates `contacts.db.bak` (timestamped if necessary) prior to destructive actions.

---

## Testing

Build tests and run the suite (requires `cmocka`). If you develop with MSYS2 / MinGW, you can run tests from the `build-mingw` tree so the tests use the same GNU toolchain used for local builds:

```bash
# Configure MinGW build with tests enabled
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_C_COMPILER="C:/msys64/ucrt64/bin/gcc.exe" -DCMAKE_CXX_COMPILER="C:/msys64/ucrt64/bin/g++.exe" -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release

# Build and run the tests
cmake --build build-mingw
ctest --test-dir build-mingw --output-on-failure
```

Notes:

- Unit tests use `cmocka` and include database integration tests; ensure `cmocka` is installed in your MinGW/MSYS2 environment (for example `pacman -S mingw-w64-ucrt-x86_64-cmocka`).
- If a test fails, run the failing test binary directly from `build-mingw` to see stdout/stderr quickly.
- For cross-toolchain coverage, run the MSVC-based `build` tests in CI or locally when using the Visual Studio toolchain.

---

## Security notes (short & practical)

- Passwords are _never_ stored in plaintext — only Argon2id hashes are saved.
- The program prefers **libsodium** (it offers a stable API and wide platform support). If libsodium is missing but `libargon2` is present, the build will use that instead.
- Interactive password changes prompt for confirmation. Non-interactive rotation is possible via `--current-password` and `--new-password` plus `--yes` for scripting.
- Sensitive operations recommend using `--backup-before` to maintain recovery points.

Further recommendations are in `docs/SECURITY.md` (threat model, recommended deployment configs, and OWASP-like guidance).

---

## Troubleshooting (common issues)

- **CMake cannot find SQLite**: install the platform dev package, or pass `-DSQLITE3_INCLUDE_DIR=/path -DSQLITE3_LIBRARY=/path`.

- **MSVC picking up MinGW headers**: clean `INCLUDE` and `LIB` environment variables, or use the MinGW generator for MinGW builds.

- **`build` directory deletion on Windows**:

  PowerShell:

  ```powershell
  Remove-Item -LiteralPath 'H:\GitHub Clone\Contact-Management-System-C\build' -Recurse -Force
  ```

  Command Prompt:

  ```cmd
  rmdir /S /Q "H:\GitHub Clone\Contact-Management-System-C\build"
  ```

  If files are locked: close editors and terminals, use Sysinternals `handle.exe` to find the locking process, then close or kill it. If all else fails, reboot.

- **Missing Argon2 runtime at link time**: install `libsodium` or `libargon2` and reconfigure CMake.

---

## Project Structure Tree

```
.
├── CMakeLists.txt          # Build configuration and targets
├── LICENSE                # Project license (MIT)
├── README.md              # This file
├── CHANGELOG.md           # Release notes / change history
├── .gitignore             # Files to ignore in Git
├── Makefile               # Optional convenience Makefile (MSYS2 / Unix)
├── run_tests.sh           # POSIX shell script to build & run tests
├── include/               # Public headers (embedding API)
│   ├── auth.h
│   ├── contacts.h
│   ├── csv.h
│   ├── db.h
│   └── util.h
├── src/                   # CLI, DB, and business logic implementation
│   ├── main.c
│   ├── contacts.c
│   ├── db.c
│   ├── auth.c
│   ├── csv.c
│   └── util.c
├── tests/                 # `cmocka` unit and integration tests
│   ├── CMakeLists.txt
│   ├── test_util.c
│   ├── test_auth.c
│   ├── test_csv.c
│   └── test_integration.c
├── docs/                  # Security notes and architecture docs
│   └── SECURITY.md
├── samples/               # Example CSVs for manual testing/import
│   └── contacts.csv
├── output/                # Build artifacts, example outputs (not tracked ideally)
│   ├── contacts.csv
│   ├── CustomerDetailsManagementSystem.exe
│   ├── main.exe
│   └── password.txt
└── contacts.db            # Local SQLite DB (example / dev snapshot)
```

Design notes:

- Keep the binary small and dependency minimal; prefer platform-native SQLite and a single crypto provider (libsodium) when available.
- Favor explicitness: require numeric IDs for destructive ops and ISO dates for due-date inputs so parsing and automation are predictable.
- Always prefer transactional operations for multi-row changes.

---

## CI / Packaging recommendations

- Add a CI workflow that builds for at least Linux (GCC), macOS (Clang), and Windows (MSVC). If possible, include a MinGW job to match the local developer environment.
- Run `cmake --build` and `ctest` in CI; add a code style/lint step (`clang-format` or `cppcheck`) and static analysis (`clang --analyze`).
- Publish artifacts (single binary + `LICENSE` + `docs`) and a small installer for Windows if you expect non-technical users.

---

## Contributing

Contributions are welcome. Short guidelines:

- Fork, branch, and open a PR with a clear description.
- Keep PRs focused and testable.
- Add unit tests for new logic and ensure `ctest` passes locally.

See `CONTRIBUTING.md` for a suggested template and checklist.

---

## FAQ

**Q: Can I edit or delete by name?**
A: No — edits and deletes require numeric IDs to avoid accidental data loss from ambiguous names.

**Q: Which Argon2 implementation should I use?**
A: Prefer `libsodium` for cross-platform stability. `libargon2` is acceptable as a fallback.

**Q: Can I run the import without touching my production DB?**
A: Yes — use `--dry-run` for validation-only, and `--backup-before` for safe production imports.

---

## License

MIT — see `LICENSE`.

---
