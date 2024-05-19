ARG TAG=latest

FROM alpine AS build-latest
RUN apk add build-base bash

FROM davidar/bootsh:latest AS build-stage0
RUN --mount=type=cache,target=/src/tarballs boot.sh bash

FROM davidar/bootsh:stage0 AS build-stage1
RUN --mount=type=cache,target=/src/tarballs boot.sh bash

FROM davidar/bootsh:stage1 AS build-stage2
RUN --mount=type=cache,target=/src/tarballs boot.sh bash

FROM build-$TAG AS build
WORKDIR /tmp
COPY configure Makefile bootsh/
COPY src bootsh/src
COPY lib bootsh/lib
RUN cd bootsh && CFLAGS=-Werror ./configure && make clean && make -j$(nproc)

FROM scratch AS bootsh
COPY --from=build /tmp/bootsh/bootsh /bin/sh
COPY boot.sh /bin/boot.sh
COPY Makefile.packages /tmp/Makefile
COPY wak.c /bin/awk
ENTRYPOINT ["/bin/boot.sh"]
