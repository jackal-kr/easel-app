# Contributing

Easel is spec-driven: each feature has a specification in `docs/specs/` and tests that check it, and the code follows from them. [How we work](docs/how-we-work.md) explains the loop, and [CLAUDE.md](CLAUDE.md) holds the rules the project is built under.

**Issues are welcome**: bug reports and ideas, using the templates. Please say which version you use and how you installed it.

**Code changes are discussed in an issue first.** A change that does not fit a spec usually needs the spec changed before the code, so a pull request that arrives without a discussion is likely to be asked to start there.

To build and test: `meson setup build`, `ninja -C build`, `meson test -C build --print-errorlogs` (the README has the details). The tests that need a display are run through `tools/isolated.sh`.

The code is under the GPL-3.0-or-later; by contributing you agree to that.
