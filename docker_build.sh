#!/bin/bash
set -euo pipefail

if [ "$#" -lt 2 ]; then
  echo "Usage: sh docker_build.sh <image-tag> <arch>"
  echo "Example: sh docker_build.sh v0.1.0 x86"
  exit 1
fi

IMAGE_TAG="$1"
ARCH="$2"

if [ "$ARCH" != "x86" ]; then
  echo "Only x86 is supported in this iteration."
  exit 1
fi

IMAGE_REPO="registry.jingan.com:32008/ja/ja-media-fs"
DOCKERFILE_PATH="./docker/x86/dockerfile"

echo "Building ${IMAGE_REPO}:${IMAGE_TAG} with ${DOCKERFILE_PATH}"
docker build -f "${DOCKERFILE_PATH}" --no-cache -t "${IMAGE_REPO}:${IMAGE_TAG}" .
echo "Pushing ${IMAGE_REPO}:${IMAGE_TAG}"
docker push "${IMAGE_REPO}:${IMAGE_TAG}"
echo "Done."
