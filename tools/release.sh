#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Builds the packages of the version in meson.build and publishes them as the GitHub release v<version>.
#
#   tools/release.sh
#
# It refuses, before building anything: a branch other than main; changes not committed or commits not pushed (the release
# is built from what GitHub has); no notes in docs/release-notes-<version>.md; and a tag v<version> that is on another
# commit (a release must match its tag: bump the version in meson.build). Then tools/package.sh all; the tag is made and
# pushed if it is not there; the release is created (a pre-release when the version has alpha, beta or rc in it) with those
# notes, or, when it exists already for this same commit, its files are replaced: the bundle, the deb and SHA256SUMS.
set -eu
cd "$(dirname "$0")/.."

version=$(sed -n "s/^  version: '\(.*\)',$/\1/p" meson.build)
tag="v$version"
notes="docs/release-notes-$version.md"
files=("dist/easel-$version.flatpak" "dist/easel_${version}_amd64.deb" "dist/SHA256SUMS")

[ -n "$version" ] || { echo "no version in meson.build" >&2; exit 1; }
if [ -n "$(git status --porcelain)" ]; then
  echo "the tree has changes not committed: commit them first" >&2
  exit 1
fi
if [ "$(git rev-parse --abbrev-ref HEAD)" != main ]; then
  echo "not on main" >&2
  exit 1
fi
git fetch -q origin
if [ "$(git rev-parse HEAD)" != "$(git rev-parse origin/main)" ]; then
  echo "HEAD is not what origin/main has: push first" >&2
  exit 1
fi
if [ ! -f "$notes" ]; then
  echo "no release notes: write $notes first" >&2
  exit 1
fi
if git rev-parse -q --verify "refs/tags/$tag" > /dev/null && [ "$(git rev-parse "$tag^{commit}")" != "$(git rev-parse HEAD)" ]; then
  echo "$tag is on another commit: bump the version in meson.build for a new release" >&2
  exit 1
fi

echo "== building $tag"
tools/package.sh all

if ! git rev-parse -q --verify "refs/tags/$tag" > /dev/null; then
  git tag -a "$tag" -m "Easel $version"
fi
git push origin "$tag"

if gh release view "$tag" > /dev/null 2>&1; then
  echo "== $tag exists: replacing its files"
  gh release upload "$tag" "${files[@]}" --clobber
else
  args=(--title "Easel $tag")
  case "$version" in *alpha* | *beta* | *rc*) args+=(--prerelease) ;; esac
  args+=(--notes-file "$notes")
  echo "== creating $tag"
  gh release create "$tag" "${files[@]}" "${args[@]}"
fi
gh release view "$tag" --json url --jq .url
