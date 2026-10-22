#!/bin/bash
# validate_uber_glsl.sh — extract embedded GLSL from hfUberGLSL.cpp and
# hfOpenGLPostprocess.cpp and syntax-check them with glslangValidator when present.
# Run from the engine root (/home/kali/hellforge-engine).

set -e

GLSLANG=""
for p in /tmp/opencode/glslang/usr/bin/glslangValidator /usr/bin/glslangValidator; do
    if [ -x "$p" ]; then GLSLANG="$p"; break; fi
done

if [ -z "$GLSLANG" ]; then
    echo "validate_uber_glsl: glslangValidator not found, skipping"; exit 0
fi

TMPDIR=$(mktemp -d /tmp/uber_glsl_XXXXXX)
trap "rm -rf $TMPDIR" EXIT

PASS=0; FAIL=0

extract_and_check() {
    local shader="$1" label="$2" stage="$3"
    local args=(-S "$stage")
    # GLSL 420 core and below need no target env; push to modern so the
    # validator's helper is exercised identically.
    if grep -q '#version' "$shader"; then :; fi
    echo "  check $label"
    if "$GLSLANG" "${args[@]}" "$shader" > /dev/null 2>&1; then
        echo "    OK"
        PASS=$((PASS+1))
    else
        echo "    FAIL"
        "$GLSLANG" "${args[@]}" "$shader" 2>&1 | head -10
        FAIL=$((FAIL+1))
    fi
}

cd /home/kali/hellforge-engine

# Extract every embedded GLSL block into temp files with a stage hint
python3 - <<EOF
import re, os
blocks = []
for path, tag in [ ("neo/renderer/hfUberGLSL.cpp","uber"),
                   ("neo/renderer/hfOpenGLPostprocess.cpp","post") ]:
    text = open(path).read()
    for i, b in enumerate(re.findall(r'R"GLSL\((.*?)\)GLSL"', text, re.DOTALL)):
        src = b.strip()
        ext = "comp" if "#version 430" in src else ("vert" if "attribute" in src and "fragColor" not in src and "in_" not in src else ("vert" if "#version 140" in src else ("frag" if "#version 140" in src else "vert")))
        # classify: vert if it declares in_Position, frag if it declares gl_FragColor or fragColor or vofi_ prefix as input, comp if compute
        if "#version 430" in src:
            ext = "comp"
        elif "in_Position" in src:
            ext = "vert"
        else:
            ext = "frag" if ("gl_FragColor" in src or "fragColor" in src or "vofi_" in src) else "frag"
        # fix stage: use the block index to disambiguate (0,1 = vert; 2 = frag) for uber
        name = f"{tag}_{i}.{ext}"
        open(os.path.join("$TMPDIR", name), "w").write(src)
        print(name)
EOF

for f in "$TMPDIR"/*; do
    case "$f" in
        *.vert) stage=vert ;;
        *.frag) stage=frag ;;
        *.comp) stage=comp ;;
    esac
    extract_and_check "$f" "$(basename "$f")" "$stage"
done

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="
[ "$FAIL" -eq 0 ]