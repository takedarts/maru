#!/bin/bash
set -euo pipefail

show_usage() {
    cat >&2 <<'EOF'
Usage:
  ./build.sh <image name> <dockerfile> [<branch>]

Arguments:
  <image name>   Name (and optional tag) of the Docker image to build.
  <dockerfile>   Path to the Dockerfile to use for the build.
  <branch>       Optional Git branch name passed to docker build as
                 --build-arg BRANCH=<branch>.
                 If omitted, no branch build-arg is passed.
EOF
}

if [[ $# -lt 2 || $# -gt 3 ]]; then
    show_usage
    exit 1
fi

IMAGE_NAME="$1"
DOCKERFILE="$2"

if [[ ! -f "$DOCKERFILE" ]]; then
    echo "Error: Dockerfile not found: $DOCKERFILE" >&2
    exit 1
fi

BUILD_ARGS=(
    --no-cache-filter builder
    -t "$IMAGE_NAME"
    -f "$DOCKERFILE"
)

if [[ $# -eq 3 ]]; then
    BRANCH="$3"
    BUILD_ARGS+=(--build-arg "BRANCH=$BRANCH")
fi

docker build "${BUILD_ARGS[@]}" .
