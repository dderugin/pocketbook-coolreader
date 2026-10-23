FROM ubuntu:24.04@sha256:68ee22004a9109b44973313b02ff7b1de250f1de0f5ecb628d73f579aaff33ac
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential ca-certificates cmake curl e2fsprogs file gettext git libgmp10 \
    libmpc3 libmpfr6 libzstd1 openssl p7zip-full pkg-config python3 \
    qemu-user rsync squashfs-tools util-linux unzip zip zlib1g \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
