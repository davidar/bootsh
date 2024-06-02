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
RUN --mount=type=cache,target=/src/tarballs boot.sh

FROM davidar/bootsh:stage0 AS build-stage1
RUN --mount=type=cache,target=/src/tarballs boot.sh

FROM davidar/bootsh:stage1 AS build-stage2
RUN --mount=type=cache,target=/src/tarballs boot.sh

FROM build-$TAG AS build
WORKDIR /tmp
COPY configure bootsh/
COPY src bootsh/src
COPY lib bootsh/lib
RUN cd bootsh && CFLAGS=-Werror ./configure && ninja

FROM scratch AS bootsh
COPY wak.c /bin/awk
COPY Makefile.packages /tmp/Makefile
COPY boot.sh /bin/boot.sh
COPY --from=build /tmp/bootsh/build/bootsh /bin/sh
ENTRYPOINT ["/bin/boot.sh"]
