#!/bin/sh

set -e

IMAGE=davidar/bootsh

for tag in latest stage0 stage1 stage2; do
  docker build . -t $IMAGE:$tag --build-arg TAG=$tag --platform linux/amd64,linux/i386
done

if [ "$1" = "--kernel" ]; then
  docker build . --target output -o build/ --build-arg TAG=latest --platform linux/i386
fi

if [ "$1" = "--test" ]; then
  for tag in latest stage2; do
    for arch in amd64 i386; do
      docker run --rm --platform linux/$arch \
        -v $PWD/test-cc:/tmp/test-cc \
        -v $PWD/lib/toybox:/tmp/lib/toybox \
        -v $PWD/tarballs:/src/tarballs \
        $IMAGE:$tag sh -c "
            set -e

            cd /tmp
            export PATH=/local/bin:/bin

            [ -f /src/tarballs/make-4.4.1.tar.gz ] || wget http://ftp.gnu.org/gnu/make/make-4.4.1.tar.gz -O /src/tarballs/make-4.4.1.tar.gz
            tar -xf /src/tarballs/make-4.4.1.tar.gz
            cd make-4.4.1
            ./configure --disable-dependency-tracking
            ./build.sh
            ./make
            ./make install
            cd ..

            [ -f /src/tarballs/bash-5.2.21.tar.gz ] || wget http://ftp.gnu.org/gnu/bash/bash-5.2.21.tar.gz -O /src/tarballs/bash-5.2.21.tar.gz
            tar -xf /src/tarballs/bash-5.2.21.tar.gz
            cd bash-5.2.21
            ./configure --prefix=/ --without-bash-malloc
            make
            make install
            cd ..

            cd test-cc
            make ARCH=\$(uname -m | sed 's/i.86/i386/')
            make clean
            cd ..

            cd lib/toybox
            TEST_HOST=1 USER=root scripts/test.sh
            rm -rf generated/testdir
            cd ..

            echo All tests passed!
        "
    done
  done
fi

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
