#!/usr/bin/env sh
# Emit standard release-note sections for GitHub Releases (prepend to artifact SHA block).
# Usage: ./.ci/release_notes_template.sh [TAG]
set -eu
TAG="${1:-}"

cat <<EOF
## Upgrade notes

- **EEPROM:** If this release bumps \`kEepromLayoutInit\` in \`firmware/core/balansun_board.h\`, the first boot **erases** persisted config. Back up settings or note Wi‑Fi/source before flashing.
- **Home Assistant:** Reload MQTT discovery after upgrade; default prefix is \`balansun\`.
- **Locale:** \`localStorage.solar.locale\` for EN/FR UI.

## Build

**Production (field):** flash the \`*-wroom32-*\` binaries only (\`pio run -e wroom32\`, not \`hil\`).

**ESP32-S3 (alternate hardware):** \`*-esp32s3-*\` binaries target ESP32-S3 DevKit (\`pio run -e esp32s3\`). **Do not** flash S3 images on ESP32-WROOM-32 boards.

Flashing checklist: [/en/getting-started/](/en/getting-started/).

## Bench (optional)

\`\`\`bash
export BALANSUN_HIL_URL=http://<esp-ip>
pytest firmware/test/hil/ -q
\`\`\`

EOF

echo ""
echo "## Changes"
echo ""
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  if [ -n "$TAG" ]; then
    PREV_TAG="$(git describe --tags --abbrev=0 "${TAG}^" 2>/dev/null || true)"
  else
    PREV_TAG="$(git describe --tags --abbrev=0 2>/dev/null || true)"
  fi
  if [ -n "$PREV_TAG" ]; then
    git log --pretty='- %s' "${PREV_TAG}..HEAD" || true
  else
    git log --pretty='- %s' -30 HEAD || true
  fi
else
  echo "- (git history unavailable)"
fi

if [ -n "$TAG" ]; then
  echo ""
  echo "Tag: \`$TAG\`"
  case "$TAG" in
    *-*)
      echo ""
      echo "Published as a **GitHub nightly prerelease** (tag contains \`-nightly.\`)."
      ;;
  esac
fi
