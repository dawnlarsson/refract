FROM alpine:3.21

RUN apk add --no-cache \
    bash bc bison build-base cpio curl elfutils-dev flex git gnupg \
    linux-headers lld llvm openssl-dev perl python3 xz

RUN git clone --depth 1 https://github.com/dawnlarsson/prism /tmp/prism && \
    cc -O3 -s -o /usr/local/bin/prism /tmp/prism/prism.c && \
    rm -rf /tmp/prism
