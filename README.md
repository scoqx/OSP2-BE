# OSP2-BE: Extended OSP2 version

Visit website for handy console commands search and check out Ready to play Quake 3 Compilations
## https://q3a.space/osp2/

### Deploy downloads (GitHub Releases)

CI publishes build/download assets to public repo **[scoqx/osp2-be](https://github.com/scoqx/osp2-be)** release tag `latest`:

- https://github.com/scoqx/osp2-be/releases/latest/download/zz-osp-pak8be.pk3
- https://github.com/scoqx/osp2-be/releases/latest/download/osp2-be-linux.tar.gz
- https://github.com/scoqx/osp2-be/releases/latest/download/version.txt
- https://github.com/scoqx/osp2-be/releases/latest/download/version_changelog.txt
- https://github.com/scoqx/osp2-be/releases/latest/download/changelog_be.txt

1. Create a fine-grained PAT on account **scoqx** with **Contents: Read and write** on repository `scoqx/osp2-be`.
2. Add repository secret **`OSP2_BE_RELEASE_TOKEN`** on OSP2-PBE (the value of that PAT).
3. Makefile CI job `create-release` and workflow_dispatch **Deploy version files** use this secret.
4. Version text files are also mirrored onto `scoqx/osp2-be` **main** (for the website UI; release assets lack browser CORS).

### Deploy cfg to q3a.space (CI)

Jobs `deploy-to-q3a-space` / `deploy-cfg-to-q3a-space` **rsync** only `OSP2.cfg` and `OSP2English.cfg` into `/opt/ruthless-stats/web/osp2/assets/` (commands UI). Downloads are not copied to Moscow.

1. GitHub **Environment** `q3a-deploy` with **Required reviewers**.
2. Secrets: `Q3A_DEPLOY_SSH_KEY` (required); optional `Q3A_DEPLOY_HOST` (default `5.188.224.180`), `Q3A_DEPLOY_USER` (default `root`).
3. On the server, put the matching **public** key in `authorized_keys` with `restrict` + `command=` (e.g. `rrsync -wo /opt/ruthless-stats/web/osp2/assets`) so the key cannot open a full shell.

## 🔧 Contributions

OSP2-BE incorporates code from several notable Quake mods and projects:
- **OSP2 by snems**: [Github Link](https://github.com/snems/OSP2)
- **baseq3a by Cyrax**: [GitHub Link](https://github.com/ec-/baseq3a)
- **XQ3E Engine**: [GitHub Link](https://github.com/xq3e/engine)
- **Unlagged Mod by Nail Toronto**
- **Ratmod**: [GitHub Link](https://github.com/rdntcntrl/ratoa_gamecode)
- **Devotion**: [GitHub Link](https://github.com/alcachofass/devotion)
- **Quake 3 1.16n extensions (X-Mod)**: [GitHub Link](https://github.com/navgg/Quake-3-1.16n-extensions)
- **Nemesis Client 1.16n**: [GitHub Link](https://github.com/dledmonds/quake3-nemesis-1.16n)

Some Assets taken from:
- **CPMA**: https://playmorepromode.com/


## ✨ BE-Features



## 📞 Contacts

Join the OSP2 community on Telegram for updates, discussions, and support:
- [Telegram Channel](https://t.me/q3osp2)
- [Telegram Chat](https://t.me/q3_osp2)

## 🛠 New Commands

See [docs/commands.md](docs/commands.md) for a comprehensive list of new commands and how to use them.

Example configuration file: `OSP2English.cfg` located inside the mod directory.

## 📥 Installation

To install OSP2-BE:

1. **Install Quake 3** and the **OSP mod v1.03** or **OSP 1.03 resouces** (from website).
2. Copy the mod files (pack or library) into the `osp` folder within your Quake 3 directory.
