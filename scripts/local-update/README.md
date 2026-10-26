# Restricted local DEB updates

This is a narrow, administrator-reviewed convenience helper, not a general
passwordless package manager. No password is read, stored or typed by the agent.

## Exact initial authorization

- User `ivan` may invoke one root-owned helper **without arguments**.
- The helper reads only `/home/ivan/.cache/penguin-studio-updates/candidate.deb`,
  takes a private root-owned copy, then validates that immutable copy.
- The public Penguin Studio 0.7.1 Ubuntu 26.04 DEB is the pinned baseline.
  Only its GPU worker executable may change. Every other payload file, file
  type, permission and symlink target must match the baseline.
- Package name, architecture, dependencies and other control metadata are pinned.
  Only a bounded `.localN` Debian revision and installed-size metadata may vary.
  The package must contain exactly the original paths. Root ownership is required.
- Added/changed maintainer scripts, configuration hooks, triggers, dependencies,
  file paths, special files, hardlinks, extended attributes and special permission
  bits are rejected. Existing installed maintainer scripts also block an update.
- A single `dpkg --install` runs only after validation. No shell, apt dependency
  resolver, arbitrary command argument or package removal is available through
  this authorization. The pinned existing `ldconfig` trigger may run normally.
- Updates are serialized, cannot downgrade, and refuse while `obs` is running.
  A different installed public version requires a new administrator policy review.

The authorized GPU worker code runs as the desktop user when Penguin Studio uses
it. This permission intentionally allows that application component to be
replaced. It does not establish that candidate code is correct or trustworthy;
local source review and functional tests remain necessary. A compromised desktop
account could replace this approved component, just as it can modify its own user
plugins. Do not expand this to arbitrary files or maintainer scripts.

## Setup and limits

Prepare the policy from the independently checksum-verified public DEB with
`prepare-policy.py`. The resulting JSON, `installer.py`, and a sudoers entry are
reviewed and installed root-owned. The helper runs using `/usr/bin/python3 -I`.
The sudoers entry uses an explicit empty argument list; validate it with
`visudo -cf` before enabling it. Setup requires one normal Ubuntu administrator
confirmation. Only after explicit approval is setup performed.

Future changes to other components, new package dependencies, public releases or
an AMD runtime update are **not covered**. They continue to require administrator
approval unless a new narrowly reviewed policy is installed. Do not silently
broaden the policy to make an installation pass.

Remove `/etc/sudoers.d/penguin-local-update` as administrator to revoke access.
The root helper is `/usr/local/sbin/penguin-local-update`; its policy is
`/etc/penguin-local-update.json` and its temporary state directory is
`/var/lib/penguin-local-update`.

Run the non-privileged rejection tests with:

```sh
python3 -m unittest discover -s scripts/local-update -v
```

Tests build harmless fixture DEBs and validate them; they never install packages.
