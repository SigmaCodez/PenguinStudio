#!/usr/bin/env bash
# Check the target release and dependencies before invoking the installer.
set -euo pipefail
check_only=false
if [[ ${1:-} == --check ]]; then
  check_only=true
  shift
fi
if [[ $# != 1 || ! -f $1 ]]; then
  echo 'Usage: install-deb.sh [--check] path/to/penguin-studio.deb' >&2
  exit 2
fi
package=$(realpath -- "$1")
if [[ $(dpkg-deb --field "$package" Package) != penguin-studio ]]; then
  echo 'This installer accepts only Penguin Studio packages.' >&2
  exit 2
fi
version=$(dpkg-deb --field "$package" Version)
if [[ ! $version =~ -[0-9]+\+(ubuntu|debian)([0-9]+(\.[0-9]+)?)$ ]]; then
  echo 'This package does not identify its target distribution release.' >&2
  echo 'The original 0.1.0/0.2.0 DEBs target Ubuntu 24.04 only. Download a release-labelled DEB or use Flatpak.' >&2
  exit 2
fi
package_distro=${BASH_REMATCH[1]}
package_release=${BASH_REMATCH[2]}
source /etc/os-release
if [[ $ID != "$package_distro" || $VERSION_ID != "$package_release" ]]; then
  printf 'Package targets %s %s; this system is %s %s.\n' "$package_distro" "$package_release" "$ID" "$VERSION_ID" >&2
  echo 'Use the DEB built for this release or the distribution-independent Flatpak.' >&2
  exit 2
fi
if [[ $(dpkg-deb --field "$package" Architecture) != "$(dpkg --print-architecture)" ]]; then
  echo 'The package architecture does not match this system.' >&2
  exit 2
fi
# Do not remove a native OBS installation or bypass missing dependencies.
apt-get --simulate --no-remove install "$package"
if "$check_only"; then
  echo 'Distribution, architecture and dependency checks passed. Nothing was installed.'
elif [[ $EUID == 0 ]]; then
  apt-get --no-remove install "$package"
else
  sudo apt-get --no-remove install "$package"
fi
