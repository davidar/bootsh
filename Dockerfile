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
FROM davidar/bootsh:stage0 AS build-stage1
FROM davidar/bootsh:stage1 AS build-stage2

FROM build-$TAG AS build
COPY tarballs /src/tarballs
WORKDIR /tmp
COPY configure configure-musl.sh init.sh install.sh wak.c bootsh/
COPY src bootsh/src
COPY lib bootsh/lib
RUN cd bootsh && CFLAGS=-Werror ./configure && ninja && DESTDIR=/dest ./install.sh

FROM scratch AS bootsh
COPY --from=build /dest /
ENTRYPOINT [ "/sbin/init" ]
CMD [ "/bin/sh" ]
SHELL [ "/sbin/init", "/bin/sh", "-c" ]
