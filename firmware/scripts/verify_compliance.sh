#!/usr/bin/env bash
# @requirement TC-QA-TRACE-001 Verificação automática de @requirement tags
set -e
CODE_DIR="${1:-components main}"
CATALOG="${2:-docs/catalogo_srs.txt}"
total=$(wc -l < "$CATALOG")
compliant=0
missing_file="build/missing_ids.txt"
: > "$missing_file"
while IFS= read -r id; do
    if grep -qrE "@requirement[s]?[[:space:]:-]*([^\"]*[, ])?$id([, ]|\$|\*)" $CODE_DIR 2>/dev/null; then
        compliant=$((compliant+1))
    else
        echo "$id" >> "$missing_file"
    fi
done < "$CATALOG"
pct=$(awk "BEGIN{printf \"%.1f\", 100*$compliant/$total}")
echo "Compliance: $compliant / $total ($pct %)"
echo "IDs sem tag em: $missing_file"
awk "BEGIN{exit !($pct >= 95.0)}" || { echo "FAIL: compliance < 95%"; exit 1; }
