#!/usr/bin/env python3
"""
@requirement RF-RTM-001 a RF-RTM-004
Gerador de matriz de rastreabilidade requisito→arquivo.
Lê SRS para extrair lista canônica de RF/RNF.
Lê tags "@requirement RF-XXX[..RF-YYY]" em .c/.h.
Saída: docs/RTM_AUTO.md + retorna exit code != 0 se cobertura < min.
"""
import os, re, sys, argparse
from pathlib import Path

REQ_RE     = re.compile(r"\b(RF|RNF)-[A-Z0-9-]+\b")
RANGE_RE   = re.compile(r"\b((?:RF|RNF)-[A-Z]+(?:-[A-Z]+)*)-(\d{3})\s*a\s*(?:RF|RNF)?-?[A-Z]*(?:-[A-Z]+)*-?(\d{3})\b", re.IGNORECASE)
ANNOT_RE   = re.compile(r"@requirement\s+([^\n\r]+)")

def find_srs_path():
    for p in ["SRS v3.11 COMPLETA — GO CODING READY.txt",
              "../SRS v3.11 COMPLETA — GO CODING READY.txt",
              "docs/SRS v3.11.txt"]:
        if os.path.exists(p):
            return p
    return None

def parse_srs(path):
    text = Path(path).read_text(encoding='utf-8', errors='replace')
    return sorted(set(REQ_RE.findall(text) and REQ_RE.findall(text)))

def parse_srs_ids(path):
    text = Path(path).read_text(encoding='utf-8', errors='replace')
    return sorted(set(REQ_RE.findall(text)))

def expand_range(family, lo, hi):
    lo = int(lo); hi = int(hi)
    return [f"{family}-{i:03d}" for i in range(lo, hi+1)]

def parse_code_tags(root):
    found = {}
    for p in Path(root).rglob("*"):
        if p.suffix not in (".c", ".h"): continue
        if "managed_components" in p.parts: continue
        try:
            txt = p.read_text(encoding='utf-8', errors='replace')
        except:
            continue
        for line_no, line in enumerate(txt.splitlines(), 1):
            m = ANNOT_RE.search(line)
            if not m: continue
            body = m.group(1)
            for rm in RANGE_RE.finditer(body):
                fam, lo, hi = rm.group(1), rm.group(2), rm.group(3)
                for r in expand_range(fam, lo, hi):
                    found.setdefault(r, []).append(f"{p}:{line_no}")
            for r in REQ_RE.findall(body):
                found.setdefault(r, []).append(f"{p}:{line_no}")
    return found

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--srs", default=None)
    ap.add_argument("--root", default=".")
    ap.add_argument("--out",  default="docs/RTM_AUTO.md")
    ap.add_argument("--min-coverage", type=float, default=0.0)
    ap.add_argument("--strict", action="store_true")
    args = ap.parse_args()

    srs = args.srs or find_srs_path()
    if not srs:
        print("ERRO: SRS não encontrado", file=sys.stderr)
        sys.exit(2)

    srs_ids = parse_srs_ids(srs)
    code    = parse_code_tags(args.root)

    covered = [r for r in srs_ids if r in code]
    missing = [r for r in srs_ids if r not in code]
    coverage = (len(covered) / len(srs_ids) * 100.0) if srs_ids else 0.0

    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    with open(args.out, "w", encoding='utf-8') as f:
        f.write(f"# RTM AUTO — gerado por tools/rtm_gen.py\n\n")
        f.write(f"- SRS: {srs}\n")
        f.write(f"- Total requisitos: {len(srs_ids)}\n")
        f.write(f"- Cobertos: {len(covered)}\n")
        f.write(f"- Cobertura: {coverage:.2f}%\n\n")
        f.write("| ID | Arquivos | Status |\n|----|----------|--------|\n")
        for r in srs_ids:
            files = code.get(r, [])
            status = "OK" if files else "MISSING"
            f.write(f"| {r} | {'<br>'.join(files) if files else '-'} | {status} |\n")

    print(f"Cobertura: {coverage:.2f}% ({len(covered)}/{len(srs_ids)})")
    if args.strict and coverage < args.min_coverage:
        print(f"ERRO: cobertura {coverage:.2f}% < mínima {args.min_coverage}%", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
