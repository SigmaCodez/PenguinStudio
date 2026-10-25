#!/usr/bin/env bash
# Configure only disposable GitHub Ubuntu runners, never a user's installation.
set -euo pipefail
if [[ ${GITHUB_ACTIONS:-} != true || $EUID != 0 ]]; then
  echo 'This helper is only for GitHub Actions Ubuntu runners (run with sudo).' >&2
  exit 2
fi
source /etc/os-release
[[ $ID == ubuntu ]] || { echo 'Expected an Ubuntu runner' >&2; exit 2; }
# Apply after the runner's 99enable-amd64v3 configuration.
# Use direct archive URLs and baseline architecture indices. Snapcraft bundles
# its own APT, which must resolve the same indices as the Ubuntu runner.
# Hosted runners may use mirror+file: URIs and amd64v3 variant indices.
for source in /etc/apt/sources.list /etc/apt/sources.list.d/*.list /etc/apt/sources.list.d/*.sources /etc/apt/apt-mirrors*.txt; do
  [[ -f $source ]] || continue
  # Preserve release suites, components, signing keys and third-party sources.
  sed -i 's|mirror+file:/etc/apt/apt-mirrors-security.txt|https://security.ubuntu.com/ubuntu|g; s|mirror+file:/etc/apt/apt-mirrors.txt|https://archive.ubuntu.com/ubuntu|g; s|http://azure\.archive\.ubuntu\.com/ubuntu|https://archive.ubuntu.com/ubuntu|g; s|https://azure\.archive\.ubuntu\.com/ubuntu|https://archive.ubuntu.com/ubuntu|g' "$source"
done
cat > /etc/apt/apt.conf.d/99zz-penguin-ci <<'APT'
#clear APT::Architecture-Variants;
APT::Architecture-Variants "";
Acquire::Retries "3";
Acquire::http::Timeout "30";
Acquire::https::Timeout "30";
APT
