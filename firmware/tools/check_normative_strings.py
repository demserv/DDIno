#!/usr/bin/env python3
# check_normative_strings.py
#
# Audit helper: scans firmware source for forbidden strings that indicate
# non-compliant or placeholder code. Exits with code 1 if any are found.
#
# Usage:
#   python tools/check_normative_strings.py [--root <path>]
#
# Default root: parent of tools/ directory

import os
import sys
import argparse

FORBIDDEN = [
    "PZEM-004T v3.0",
    "placeholder",
    "nao integrada",
    "Aguardando integracao",
    "aguardando integracao",
    "TODO",
    "FIXME",
]

EXTENSIONS = (".c", ".h")
EXCLUDE_DIRS = {"managed_components", "build", ".git", "__pycache__"}


def scan_file(filepath):
    found = []
    try:
        with open(filepath, "r", encoding="utf-8", errors="replace") as f:
            for lineno, line in enumerate(f, start=1):
                for token in FORBIDDEN:
                    if token in line:
                        found.append((lineno, token, line.rstrip()))
    except Exception as e:
        print(f"  ERROR reading {filepath}: {e}", file=sys.stderr)
    return found


def main():
    parser = argparse.ArgumentParser(
        description="Check firmware for forbidden normative strings"
    )
    parser.add_argument("--root", default=None, help="Root directory to scan")
    args = parser.parse_args()

    if args.root:
        root = os.path.abspath(args.root)
    else:
        root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

    print(f"Scanning {root} for forbidden strings...")
    total_issues = 0

    for dirpath, dirnames, filenames in os.walk(root):
        # Exclude directories
        dirnames[:] = [
            d
            for d in dirnames
            if d not in EXCLUDE_DIRS and not d.startswith(".")
        ]
        for fname in filenames:
            if not fname.endswith(EXTENSIONS):
                continue
            fpath = os.path.join(dirpath, fname)
            issues = scan_file(fpath)
            for lineno, token, line in issues:
                rel = os.path.relpath(fpath, root)
                print(f"  {rel}:{lineno}: found '{token}' -> {line}")
                total_issues += 1

    if total_issues:
        print(f"\nFAIL: {total_issues} forbidden string(s) found.")
        sys.exit(1)
    else:
        print("PASS: no forbidden strings found.")
        sys.exit(0)


if __name__ == "__main__":
    main()
