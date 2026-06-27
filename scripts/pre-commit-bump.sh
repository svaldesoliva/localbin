#!/bin/bash
# Pre-commit hook to auto-bump the patch version in version.h
# It only bumps if version.h is NOT already staged (meaning we didn't manually bump it)

VERSION_FILE="include/localbin/app/version.h"

# If version.h is already staged, assume the user bumped it manually
if git diff --cached --name-only | grep -q "^$VERSION_FILE$"; then
    exit 0
fi

if [ ! -f "$VERSION_FILE" ]; then
    echo "Warning: $VERSION_FILE not found."
    exit 0
fi

# Extract current version parts
MAJOR=$(grep "LOCALBIN_VERSION_MAJOR" "$VERSION_FILE" | awk '{print $3}')
MINOR=$(grep "LOCALBIN_VERSION_MINOR" "$VERSION_FILE" | awk '{print $3}')
PATCH=$(grep "LOCALBIN_VERSION_PATCH" "$VERSION_FILE" | awk '{print $3}')

if [ -z "$MAJOR" ] || [ -z "$MINOR" ] || [ -z "$PATCH" ]; then
    echo "Warning: Could not parse version from $VERSION_FILE"
    exit 0
fi

NEW_PATCH=$((PATCH + 1))
NEW_VERSION="$MAJOR.$MINOR.$NEW_PATCH"

echo "Auto-bumping version: $MAJOR.$MINOR.$PATCH -> $NEW_VERSION"

# Update the file
sed -i.bak "s/#define LOCALBIN_VERSION_PATCH .*/#define LOCALBIN_VERSION_PATCH $NEW_PATCH/" "$VERSION_FILE"
sed -i.bak "s/#define LOCALBIN_VERSION \".*\"/#define LOCALBIN_VERSION \"$NEW_VERSION\"/" "$VERSION_FILE"
rm -f "${VERSION_FILE}.bak"

# Stage the version bump
git add "$VERSION_FILE"

exit 0
