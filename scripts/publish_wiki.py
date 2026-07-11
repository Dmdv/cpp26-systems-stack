#!/usr/bin/env python3
"""Generate GitHub Wiki pages from docs/ and push to <repo>.wiki.git.

GitHub only creates the wiki git remote after the *first* page exists.
If clone fails with "Repository not found", open:

  https://github.com/Dmdv/cpp26-systems-stack/wiki

click "Create the first page", save a Home page (any text), then re-run:

  python3 scripts/publish_wiki.py
"""

from __future__ import annotations

import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OWNER_REPO = "Dmdv/cpp26-systems-stack"
WIKI_URL = f"https://github.com/{OWNER_REPO}.wiki.git"
WIKI_WEB = f"https://github.com/{OWNER_REPO}/wiki"

PAGES: list[tuple[str, str]] = [
    ("docs/blueprint/README.md", "Blueprint-Overview"),
    ("docs/blueprint/LOW_LATENCY_STACK.md", "Blueprint-Low-Latency-Stack"),
    ("docs/blueprint/AUDIT.md", "Blueprint-Audit"),
    ("docs/blueprint/01-hardware-os.md", "Blueprint-01-Hardware-OS"),
    ("docs/blueprint/02-network-ingress.md", "Blueprint-02-Network-Ingress"),
    ("docs/blueprint/03-memory.md", "Blueprint-03-Memory"),
    ("docs/blueprint/04-concurrency.md", "Blueprint-04-Concurrency"),
    ("docs/blueprint/05-compiler.md", "Blueprint-05-Compiler"),
    ("docs/blueprint/06-telemetry.md", "Blueprint-06-Telemetry"),
    ("docs/blueprint/07-industry-libraries.md", "Blueprint-07-Industry-Libraries"),
    ("docs/tutorials/industry-stack.md", "Tutorial-Industry-Stack"),
    ("docs/tutorials/sbe-codegen.md", "Tutorial-SBE-Codegen"),
    ("docs/tutorials/linux-numa-uring.md", "Tutorial-Linux-NUMA-io-uring"),
    ("docs/tutorials/hdrhistogram-c.md", "Tutorial-HdrHistogram-C"),
    ("docs/tutorials/kernel-bypass-lab.md", "Tutorial-Kernel-Bypass-Lab"),
    ("docs/tutorials/ninja-build.md", "Tutorial-Ninja-Build"),
    ("docs/tutorials/modern-cpp-tooling-arm-intel.md", "Tutorial-Modern-Cpp-Tooling-ARM-Intel"),
    ("docs/libraries/README.md", "Libraries-Index"),
    ("docs/libraries/asio.md", "Library-Asio"),
    ("docs/libraries/beast.md", "Library-Beast"),
    ("docs/libraries/taskflow.md", "Library-Taskflow"),
    ("docs/libraries/tbb.md", "Library-TBB"),
    ("docs/libraries/stdexec.md", "Library-stdexec"),
    ("docs/libraries/simdjson.md", "Library-simdjson"),
    ("docs/libraries/grpc-protobuf.md", "Library-gRPC-Protobuf"),
    ("docs/libraries/folly.md", "Library-Folly"),
    ("docs/libraries/hpx.md", "Library-HPX"),
    ("docs/clion.md", "CLion-Setup"),
    ("cmake/toolchains/README.md", "CMake-Toolchains"),
]

EXTRA_MAP = {
    "blueprint/README.md": "Blueprint-Overview",
    "blueprint/AUDIT.md": "Blueprint-Audit",
    "blueprint/LOW_LATENCY_STACK.md": "Blueprint-Low-Latency-Stack",
    "blueprint/01-hardware-os.md": "Blueprint-01-Hardware-OS",
    "blueprint/02-network-ingress.md": "Blueprint-02-Network-Ingress",
    "blueprint/03-memory.md": "Blueprint-03-Memory",
    "blueprint/04-concurrency.md": "Blueprint-04-Concurrency",
    "blueprint/05-compiler.md": "Blueprint-05-Compiler",
    "blueprint/06-telemetry.md": "Blueprint-06-Telemetry",
    "blueprint/07-industry-libraries.md": "Blueprint-07-Industry-Libraries",
    "../blueprint/AUDIT.md": "Blueprint-Audit",
    "../blueprint/05-compiler.md": "Blueprint-05-Compiler",
    "../blueprint/07-industry-libraries.md": "Blueprint-07-Industry-Libraries",
    "../blueprint/README.md": "Blueprint-Overview",
    "../clion.md": "CLion-Setup",
    "clion.md": "CLion-Setup",
    "industry-stack.md": "Tutorial-Industry-Stack",
    "sbe-codegen.md": "Tutorial-SBE-Codegen",
    "linux-numa-uring.md": "Tutorial-Linux-NUMA-io-uring",
    "hdrhistogram-c.md": "Tutorial-HdrHistogram-C",
    "kernel-bypass-lab.md": "Tutorial-Kernel-Bypass-Lab",
    "ninja-build.md": "Tutorial-Ninja-Build",
    "modern-cpp-tooling-arm-intel.md": "Tutorial-Modern-Cpp-Tooling-ARM-Intel",
    "asio.md": "Library-Asio",
    "beast.md": "Library-Beast",
    "taskflow.md": "Library-Taskflow",
    "tbb.md": "Library-TBB",
    "stdexec.md": "Library-stdexec",
    "simdjson.md": "Library-simdjson",
    "grpc-protobuf.md": "Library-gRPC-Protobuf",
    "folly.md": "Library-Folly",
    "hpx.md": "Library-HPX",
    "../../README.md": "Home",
    "../README.md": "Home",
    "cmake/toolchains/README.md": "CMake-Toolchains",
}

LINK_RE = re.compile(r"\[([^\]]+)\]\(([^)]+)\)")


def build_path_map() -> dict[str, str]:
    m: dict[str, str] = dict(EXTRA_MAP)
    for rel, name in PAGES:
        m[rel] = name
        m[Path(rel).name] = name
    return m


def rewrite_links(text: str, source_rel: str, path_map: dict[str, str]) -> str:
    def repl(match: re.Match[str]) -> str:
        label, url = match.group(1), match.group(2)
        if url.startswith(("http://", "https://", "mailto:", "#")):
            return match.group(0)
        path, anchor = url, ""
        if "#" in url:
            path, frag = url.split("#", 1)
            anchor = "#" + frag
        path = path.split("?")[0]
        wiki = path_map.get(path) or path_map.get(Path(path).name)
        if wiki is None:
            try:
                resolved = (Path(source_rel).parent / path).as_posix()
                while "/../" in resolved:
                    resolved = re.sub(r"[^/]+/\.\./", "", resolved)
                wiki = path_map.get(resolved)
            except Exception:
                wiki = None
        if wiki is None:
            clean = path
            while clean.startswith("../"):
                clean = clean[3:]
            if clean and not clean.startswith("http"):
                return (
                    f"[{label}](https://github.com/{OWNER_REPO}/blob/main/{clean}{anchor})"
                )
            return match.group(0)
        if wiki.startswith("http"):
            return f"[{label}]({wiki}{anchor})"
        return f"[{label}]({wiki}{anchor})"

    return LINK_RE.sub(repl, text)


def home_md() -> str:
    return f"""# C++26 Systems Stack Wiki

Documentation mirror of [{OWNER_REPO}](https://github.com/{OWNER_REPO}).
Generated from the `docs/` tree in the main repository.

## Start here

| Page | Description |
|------|-------------|
| [Blueprint-Overview](Blueprint-Overview) | Low-latency blueprint index |
| [Blueprint-Audit](Blueprint-Audit) | SHIPPED / OPTIONAL / GAP checklist |
| [Tutorial-Industry-Stack](Tutorial-Industry-Stack) | Industry libraries by layer |
| [Libraries-Index](Libraries-Index) | Library mesh guides |
| [CLion-Setup](CLion-Setup) | IDE / IntelliSense |
| [Tutorial-Ninja-Build](Tutorial-Ninja-Build) | Ninja build system |

## Blueprint

| Page | Layer |
|------|--------|
| [Blueprint-01-Hardware-OS](Blueprint-01-Hardware-OS) | §1 Hardware & OS |
| [Blueprint-02-Network-Ingress](Blueprint-02-Network-Ingress) | §2 Network ingress |
| [Blueprint-03-Memory](Blueprint-03-Memory) | §3 Memory |
| [Blueprint-04-Concurrency](Blueprint-04-Concurrency) | §4 Concurrency |
| [Blueprint-05-Compiler](Blueprint-05-Compiler) | §5 Compiler |
| [Blueprint-06-Telemetry](Blueprint-06-Telemetry) | §6 Telemetry |
| [Blueprint-07-Industry-Libraries](Blueprint-07-Industry-Libraries) | Industry library map |
| [Blueprint-Low-Latency-Stack](Blueprint-Low-Latency-Stack) | Narrative |
| [Blueprint-Audit](Blueprint-Audit) | Full audit table |

## Tutorials

| Page | Topic |
|------|--------|
| [Tutorial-Industry-Stack](Tutorial-Industry-Stack) | Industry stack |
| [Tutorial-SBE-Codegen](Tutorial-SBE-Codegen) | Real Logic SBE |
| [Tutorial-Linux-NUMA-io-uring](Tutorial-Linux-NUMA-io-uring) | NUMA + io_uring |
| [Tutorial-HdrHistogram-C](Tutorial-HdrHistogram-C) | HDR histograms |
| [Tutorial-Kernel-Bypass-Lab](Tutorial-Kernel-Bypass-Lab) | DPDK / Onload lab |
| [Tutorial-Ninja-Build](Tutorial-Ninja-Build) | Ninja |
| [Tutorial-Modern-Cpp-Tooling-ARM-Intel](Tutorial-Modern-Cpp-Tooling-ARM-Intel) | ARM & Intel tooling |

## Library guides

| Page | Component |
|------|-----------|
| [Library-Asio](Library-Asio) | Asio |
| [Library-Beast](Library-Beast) | Boost.Beast |
| [Library-Taskflow](Library-Taskflow) | Taskflow |
| [Library-TBB](Library-TBB) | oneTBB |
| [Library-stdexec](Library-stdexec) | stdexec |
| [Library-simdjson](Library-simdjson) | simdjson |
| [Library-gRPC-Protobuf](Library-gRPC-Protobuf) | gRPC + protobuf |
| [Library-Folly](Library-Folly) | Folly |
| [Library-HPX](Library-HPX) | HPX |

## Tooling

| Page | Topic |
|------|--------|
| [CLion-Setup](CLion-Setup) | CLion presets |
| [CMake-Toolchains](CMake-Toolchains) | Cross-compile examples |

## Source of truth

Prefer PRs against `docs/` in the main repo. Refresh this wiki with:

```bash
python3 scripts/publish_wiki.py
```
"""


def sidebar_md() -> str:
    return """## C++26 Systems Stack

**[Home](Home)**

### Blueprint
* [Overview](Blueprint-Overview)
* [Audit](Blueprint-Audit)
* [Narrative](Blueprint-Low-Latency-Stack)
* [01 Hardware & OS](Blueprint-01-Hardware-OS)
* [02 Network ingress](Blueprint-02-Network-Ingress)
* [03 Memory](Blueprint-03-Memory)
* [04 Concurrency](Blueprint-04-Concurrency)
* [05 Compiler](Blueprint-05-Compiler)
* [06 Telemetry](Blueprint-06-Telemetry)
* [07 Industry libs](Blueprint-07-Industry-Libraries)

### Tutorials
* [Industry stack](Tutorial-Industry-Stack)
* [SBE codegen](Tutorial-SBE-Codegen)
* [NUMA + io_uring](Tutorial-Linux-NUMA-io-uring)
* [HdrHistogram_c](Tutorial-HdrHistogram-C)
* [Kernel bypass lab](Tutorial-Kernel-Bypass-Lab)
* [Ninja build](Tutorial-Ninja-Build)
* [ARM / Intel tooling](Tutorial-Modern-Cpp-Tooling-ARM-Intel)

### Libraries
* [Index](Libraries-Index)
* [Asio](Library-Asio)
* [Beast](Library-Beast)
* [Taskflow](Library-Taskflow)
* [TBB](Library-TBB)
* [stdexec](Library-stdexec)
* [simdjson](Library-simdjson)
* [gRPC / protobuf](Library-gRPC-Protobuf)
* [Folly](Library-Folly)
* [HPX](Library-HPX)

### Tooling
* [CLion](CLion-Setup)
* [CMake toolchains](CMake-Toolchains)

[Code repository](https://github.com/Dmdv/cpp26-systems-stack)
"""


def write_pages(dest: Path) -> None:
    path_map = build_path_map()
    for rel, name in PAGES:
        src = ROOT / rel
        if not src.exists():
            print(f"skip missing {rel}", file=sys.stderr)
            continue
        body = rewrite_links(src.read_text(encoding="utf-8"), rel, path_map)
        header = (
            f"> Source: [`{rel}`](https://github.com/{OWNER_REPO}/blob/main/{rel}) "
            f"in the main repository.\n\n"
        )
        (dest / f"{name}.md").write_text(header + body, encoding="utf-8")
    (dest / "Home.md").write_text(home_md(), encoding="utf-8")
    (dest / "_Sidebar.md").write_text(sidebar_md(), encoding="utf-8")
    (dest / "_Footer.md").write_text(
        "---\n*Wiki mirror of repository docs. Edit `docs/` in the main repo, then "
        "`python3 scripts/publish_wiki.py`.*\n",
        encoding="utf-8",
    )


def run(cmd: list[str], cwd: Path | None = None, check: bool = True) -> subprocess.CompletedProcess:
    print("+", " ".join(cmd))
    return subprocess.run(cmd, cwd=cwd, check=check, text=True, capture_output=True)


def write_preview() -> Path:
    preview = ROOT / "build" / "wiki-preview"
    preview.mkdir(parents=True, exist_ok=True)
    for p in preview.glob("*.md"):
        p.unlink()
    write_pages(preview)
    return preview


def main() -> int:
    if "--preview-only" in sys.argv:
        preview = write_preview()
        print(f"Wrote local preview: {preview} ({len(list(preview.glob('*.md')))} pages)")
        return 0

    with tempfile.TemporaryDirectory(prefix="cpp26-wiki-") as tmp:
        wiki = Path(tmp) / "wiki"
        # Try clone existing wiki
        clone = run(["git", "clone", WIKI_URL, str(wiki)], check=False)
        if clone.returncode != 0:
            print(clone.stderr, file=sys.stderr)
            print(
                f"""
Wiki git remote is not created yet (GitHub only creates it after the first page).

One-time bootstrap:
  1. Open {WIKI_WEB}
  2. Click "Create the first page"
  3. Title: Home  — paste any short text — Save
  4. Re-run:  python3 scripts/publish_wiki.py
""",
                file=sys.stderr,
            )
            preview = write_preview()
            print(f"Wrote local preview (not published): {preview}")
            return 2

        # Replace content
        for p in wiki.glob("*.md"):
            if p.name != ".git":
                p.unlink()
        write_pages(wiki)

        run(["git", "add", "-A"], cwd=wiki)
        status = run(["git", "status", "--porcelain"], cwd=wiki)
        if not status.stdout.strip():
            print("Wiki already up to date.")
            return 0
        run(
            [
                "git",
                "-c",
                "user.name=Dmdv",
                "-c",
                "user.email=dima.dyachkov@gmail.com",
                "commit",
                "-m",
                "Sync wiki from docs/ (blueprint, tutorials, libraries)",
            ],
            cwd=wiki,
        )
        # Prefer master (classic wiki default); try main if needed
        push = run(["git", "push", "origin", "HEAD:master"], cwd=wiki, check=False)
        if push.returncode != 0:
            push = run(["git", "push", "origin", "HEAD:main"], cwd=wiki, check=False)
        if push.returncode != 0:
            print(push.stderr, file=sys.stderr)
            return 1
        print(f"Published: {WIKI_WEB}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
