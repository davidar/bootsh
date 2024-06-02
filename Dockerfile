ARG TAG=latest

FROM alpine AS alpine-amd64

FROM alpine AS alpine-386
RUN apk add setarch
SHELL [ "setarch", "i386", "/bin/sh", "-c" ]

ARG TARGETARCH
FROM alpine-$TARGETARCH AS build-latest
RUN apk add build-base
RUN apk add ninja

FROM davidar/bootsh:latest AS build-stage0
COPY tarballs /src/tarballs
RUN boot.sh

FROM davidar/bootsh:stage0 AS build-stage1
COPY tarballs /src/tarballs
RUN boot.sh

FROM davidar/bootsh:stage1 AS build-stage2
COPY tarballs /src/tarballs
RUN boot.sh

FROM build-$TAG AS build
WORKDIR /tmp
COPY configure boot.sh configure-musl.sh wak.c install.sh bootsh/
COPY src bootsh/src
COPY lib bootsh/lib
RUN cd bootsh && CFLAGS=-Werror ./configure && ninja && DESTDIR=/dest ./install.sh

FROM scratch AS bootsh
COPY --from=build /dest /
ENTRYPOINT ["/bin/boot.sh"]
