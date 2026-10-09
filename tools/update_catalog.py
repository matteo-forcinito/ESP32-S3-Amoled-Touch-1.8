"""Add a release to a product's catalog (<product>.json in the public releases repo).

Used by .github/workflows/release.yml; the same file is in every firmware repo.

    python tools/update_catalog.py CATALOG.json --version 1.3.0-dev.2 --url URL \
        --size 2560000 --sha256 HEX --notes "text" [--date 2026-10-10]

Catalog format (read by the watches, see services/fw_update.h):

    {
      "schema": 2,
      "version": "1.2.0", "url": "...",          <- latest stable, for older firmwares
      "channels": {"stable": {"version", "url"}, "dev": {"version", "url"}},
      "releases": [{"version", "channel", "date", "notes", "url", "size", "sha256"}, ...]
    }

Versions: X.Y.Z = stable channel, X.Y.Z-something = dev channel. Order as in
fw_version.c: 1.2.9 < 1.3.0-dev.2 < 1.3.0-dev.10 < 1.3.0.
"""

import argparse
import datetime
import functools
import json
import os
import re

KEEP = 50
VERSION = re.compile(r"^(\d+)\.(\d+)\.(\d+)(?:-([0-9A-Za-z.]+))?$")


def compare_identifier(a, b):
    if a.isdigit() and b.isdigit():
        return (int(a) > int(b)) - (int(a) < int(b))
    if a.isdigit() != b.isdigit():
        return -1 if a.isdigit() else 1
    return (a > b) - (a < b)


def compare(a, b):
    ma, mb = VERSION.match(a), VERSION.match(b)
    if not ma or not mb:
        return (a > b) - (a < b)
    for i in range(1, 4):
        x, y = int(ma.group(i)), int(mb.group(i))
        if x != y:
            return 1 if x > y else -1
    pa, pb = ma.group(4), mb.group(4)
    if pa is None or pb is None:
        return (pa is None) - (pb is None)
    for x, y in zip(pa.split("."), pb.split(".")):
        c = compare_identifier(x, y)
        if c:
            return c
    return (len(pa.split(".")) > len(pb.split("."))) - (len(pa.split(".")) < len(pb.split(".")))


def channel_of(version):
    match = VERSION.match(version)
    return "dev" if match and match.group(4) else "stable"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("catalog")
    parser.add_argument("--version", required=True)
    parser.add_argument("--url", required=True)
    parser.add_argument("--size", type=int, default=0)
    parser.add_argument("--sha256", default="")
    parser.add_argument("--notes", default="")
    parser.add_argument("--date", default=datetime.date.today().isoformat())
    args = parser.parse_args()

    if not VERSION.match(args.version):
        raise SystemExit("not a version: " + args.version)

    catalog = {}
    if os.path.exists(args.catalog):
        with open(args.catalog, encoding="utf-8") as f:
            catalog = json.load(f)

    releases = [r for r in catalog.get("releases", []) if r.get("version") != args.version]
    releases.append({
        "version": args.version,
        "channel": channel_of(args.version),
        "date": args.date,
        "notes": args.notes[:79],
        "url": args.url,
        "size": args.size,
        "sha256": args.sha256,
    })
    releases.sort(key=functools.cmp_to_key(lambda a, b: compare(b["version"], a["version"])))
    releases = releases[:KEEP]

    channels = {}
    for release in releases:   # newest first: the first of each channel wins
        channels.setdefault(release["channel"], {"version": release["version"], "url": release["url"]})

    out = {"schema": 2}
    if "stable" in channels:
        out.update(channels["stable"])   # what firmwares without channels read
    out["channels"] = channels
    out["releases"] = releases

    with open(args.catalog, "w", encoding="utf-8") as f:
        json.dump(out, f, indent=2)
        f.write("\n")

    print("catalog:", args.catalog, "->", args.version, channel_of(args.version),
          "| stable", channels.get("stable", {}).get("version"), "| dev", channels.get("dev", {}).get("version"))


if __name__ == "__main__":
    main()
