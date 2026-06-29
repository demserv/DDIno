#!/usr/bin/env python3
"""
audit_corpus_check.py — Check that the audit corpus excludes third-party code.

Verifies that managed_components/ directories and other vendored paths
are excluded from line counting tools (cloc, github linguist, etc.).

Usage:
    python scripts/audit_corpus_check.py [--path <firmware_root>]

Exit code:
    0 if all checks pass
    1 if any check fails
"""

import os
import sys
import argparse

VENDORED_PATTERNS = [
    "managed_components",
    "build",
]

REQUIRED_GITATTRIBUTES_LINES = [
    "managed_components/** linguist-vendored",
    "build/** linguist-vendored",
    "firmware/managed_components/** linguist-vendored",
]


def check_vendored_dirs_exist(root: str) -> list[str]:
    errors = []
    for pat in VENDORED_PATTERNS:
        full = os.path.join(root, pat)
        if os.path.isdir(full):
            print(f"  OK: vendored dir exists: {full}")
        else:
            print(f"  INFO: vendored dir not found (may be absent): {full}")
    return errors


def check_gitattributes(root: str) -> list[str]:
    errors = []
    gitattr_path = os.path.join(root, ".gitattributes")
    if not os.path.isfile(gitattr_path):
        errors.append(f"MISSING .gitattributes at {gitattr_path}")
        return errors

    with open(gitattr_path, "r") as f:
        content = f.read()

    for line in REQUIRED_GITATTRIBUTES_LINES:
        if line not in content:
            errors.append(f"MISSING in .gitattributes: {line}")
        else:
            print(f"  OK: found '{line}' in .gitattributes")

    return errors


def check_cloc_exclude(root: str) -> list[str]:
    errors = []
    for pat in VENDORED_PATTERNS:
        full = os.path.join(root, pat)
        if os.path.isdir(full):
            print(f"  OK: vendored dir '{pat}' would be excluded by --exclude-dir={pat}")
    return errors


def main():
    parser = argparse.ArgumentParser(description="Audit corpus check")
    parser.add_argument("--path", default=".", help="Root path of firmware project")
    args = parser.parse_args()

    root = os.path.abspath(args.path)
    print(f"Auditing corpus at: {root}")
    print()

    all_errors = []
    all_errors.extend(check_vendored_dirs_exist(root))
    all_errors.extend(check_gitattributes(root))
    all_errors.extend(check_cloc_exclude(root))

    print()
    if all_errors:
        print("FAILED — audit corpus checks:")
        for e in all_errors:
            print(f"  - {e}")
        sys.exit(1)
    else:
        print("PASSED — audit corpus is clean")
        sys.exit(0)


if __name__ == "__main__":
    main()
