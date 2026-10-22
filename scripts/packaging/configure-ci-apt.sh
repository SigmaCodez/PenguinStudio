#!/usr/bin/env bash
# Configure only disposable GitHub Ubuntu runners, never a user's installation.
set -euo pipefail
if [[ ${GITHUB_ACTIONS:-} != true || $EUID != 0 ]]; then
  echo 'This helper is only for GitHub Actions Ubuntu runners (run with sudo).' >&2
  exit 2
fi
source /etc/os-release
[[ $ID == ubuntu ]] || { echo 'Expected an Ubuntu runner' >&2; exit 2; }
# Hosted runners may use mirror+file: URIs rather than direct archive URLs.
for source in /etc/apt/sources.list /etc/apt/sources.list.d/*.list /etc/apt/sources.list.d/*.sources /etc/apt/apt-mirrors*.txt; do
  [[ -f $source ]] || continue
  # Preserve release suites, components, signing keys and third-party sources.
  sed -i 's|http://azure\.archive\.ubuntu\.com/ubuntu|https://archive.ubuntu.com/ubuntu|g; s|https://azure\.archive\.ubuntu\.com/ubuntu|https://archive.ubuntu.com/ubuntu|g' "$source"
done
cat > /etc/apt/apt.conf.d/80penguin-ci <<'APT'
Acquire::Retries "3";
Acquire::http::Timeout "30";
Acquire::https::Timeout "30";
APT
