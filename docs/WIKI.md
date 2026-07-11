# GitHub Wiki

Public wiki: **https://github.com/Dmdv/cpp26-systems-stack/wiki**

The wiki is a **read-only mirror** of the documentation under `docs/` (blueprint
chapters, tutorials, library guides).

**Original docs are never removed or moved.**  
`docs/blueprint/`, `docs/tutorials/`, `docs/libraries/`, and `docs/clion.md` stay
in the main repository as the source of truth. Publishing only *copies* content
into the separate `*.wiki.git` remote (or into `build/wiki-preview/` locally).

## Publish / refresh

```bash
# From repo root (requires push access)
python3 scripts/publish_wiki.py
```

### One-time bootstrap (empty wiki)

GitHub does not create the `*.wiki.git` remote until the first page exists.

1. Open https://github.com/Dmdv/cpp26-systems-stack/wiki  
2. Click **Create the first page**  
3. Title: `Home` — any short body is fine — **Save page**  
4. Run `python3 scripts/publish_wiki.py`  

After that, every re-run regenerates all pages, `_Sidebar`, and `_Footer` from `docs/`.

## What gets published

| Wiki section | Repository sources |
|--------------|-------------------|
| Blueprint | `docs/blueprint/*` |
| Tutorials | `docs/tutorials/*` |
| Libraries | `docs/libraries/*` |
| CLion / toolchains | `docs/clion.md`, `cmake/toolchains/README.md` |

Local dry-run preview (when wiki remote is missing): `build/wiki-preview/`.
