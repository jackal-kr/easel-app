#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Builds and installs Easel's packages (0024). The files go in dist/.
#
#   tools/package.sh flatpak           the Flatpak bundle (needs org.gnome.Sdk//51 and org.gnome.Platform//51 from Flathub)
#   tools/package.sh deb               the deb
#   tools/package.sh all               both, and SHA256SUMS
#   tools/package.sh install-flatpak   installs the bundle for this user, building it if it is not there
#   tools/package.sh install-deb       installs the deb with apt, building it if it is not there; asks for your password
set -eu
cd "$(dirname "$0")/.."

version=$(sed -n "s/^  version: '\(.*\)',$/\1/p" meson.build)
manifest=build-aux/flatpak/io.github.jackal_kr.Easel.yml
bundle=dist/easel-$version.flatpak
deb=dist/easel_${version}_amd64.deb

build_flatpak () {
  mkdir -p dist build
  flatpak-builder --user --force-clean --disable-rofiles-fuse --state-dir=build/flatpak-state --repo=build/flatpak-state/repo build/flatpak "$manifest"
  flatpak build-bundle --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo build/flatpak-state/repo "$bundle" io.github.jackal_kr.Easel
  echo "built $bundle"
}

build_deb () {
  mkdir -p dist build
  dpkg-buildpackage -b -us -uc
  # The file name has no ~, which GitHub would change on upload (REQ-0024-03); the version inside keeps it.
  mv "../easel_${version/-/\~}_amd64.deb" "$deb"
  rm -f "../easel_${version/-/\~}_amd64.buildinfo" "../easel_${version/-/\~}_amd64.changes"
  echo "built $deb"
}

case "${1:-}" in
  flatpak) build_flatpak ;;
  deb) build_deb ;;
  all)
    build_flatpak
    build_deb
    (cd dist && sha256sum "$(basename "$bundle")" "$(basename "$deb")" > SHA256SUMS)
    echo "wrote dist/SHA256SUMS"
    ;;
  install-flatpak)
    [ -f "$bundle" ] || build_flatpak
    flatpak install --user -y "$bundle"
    ;;
  install-deb)
    [ -f "$deb" ] || build_deb
    sudo apt install -y "./$deb"
    ;;
  *)
    sed -n '3,9p' "$0" | sed 's/^# \{0,1\}//' >&2
    exit 2
    ;;
esac
