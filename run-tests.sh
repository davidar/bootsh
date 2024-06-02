#!/bin/bash

set -ex

IMAGE=davidar/bootsh

for tag in latest stage0 stage1 stage2; do
  docker build . -t $IMAGE:$tag --build-arg TAG=$tag --platform linux/amd64,linux/i386
done

for tag in latest stage2; do
  for arch in amd64 i386; do
    docker run --rm --platform linux/$arch \
        -v $PWD/test-cc:/tmp/test-cc \
        -v $PWD/lib/toybox:/tmp/lib/toybox \
        -v $PWD/tarballs:/src/tarballs \
        $IMAGE:$tag test-host
  done
done

for tag in latest stage0 stage1 stage2; do
  for arch in amd64 i386; do
    ID=$(docker create --platform linux/$arch $IMAGE:$tag)
    docker cp $ID:/bin/sh bootsh.$arch.$tag
    docker rm $ID
  done
done

ls -lh bootsh.*

diff -q bootsh.amd64.stage1 bootsh.amd64.stage2
diff -q bootsh.i386.stage1 bootsh.i386.stage2
