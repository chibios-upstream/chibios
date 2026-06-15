# this will always pull the latest version of the base image, which is updated regularly by Microsoft
FROM mcr.microsoft.com/devcontainers/base:ubuntu

ENV LANG=C.UTF-8 \
    LC_ALL=C.UTF-8 \
    LANGUAGE=C.UTF-8 \
    PYTHONUTF8=1

RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    gdb-multiarch \
    openocd \
    build-essential \
    make \
    gawk \
    git \
    python3 \
    python-is-python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

RUN ln -sf /usr/bin/gdb-multiarch /usr/local/bin/arm-none-eabi-gdb
