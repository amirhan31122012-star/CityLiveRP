FROM ubuntu:22.04
RUN dpkg --add-architecture i386 && apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    g++-multilib cmake make python3 python3-ply ca-certificates qemu-user \
    libssl-dev:i386 zlib1g-dev:i386 && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-m32 \
    -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_SHARED_LINKER_FLAGS=-m32 \
    -DCMAKE_CROSSCOMPILING_EMULATOR=/usr/bin/qemu-i386 && \
    cmake --build build -j 3 && ctest --test-dir build --output-on-failure && \
    cmake -S ThirdParty/pawn-compiler/source/compiler -B build-pawn -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build-pawn --target pawncc -j 3 && \
    build-pawn/pawncc config/bootstrap.pwn -obuild/citylive.amx
RUN cp /usr/share/doc/libssl3/copyright ThirdParty/OpenSSL-COPYRIGHT.txt && \
    cp /usr/share/doc/zlib1g/copyright ThirdParty/zlib-COPYRIGHT.txt && \
    cp /usr/share/doc/gcc-11-base/copyright ThirdParty/GCC-runtime-COPYRIGHT.txt
CMD ["sh", "-c", "python3 scripts/package.py --plugin build/citylive.so --bootstrap build/citylive.amx && mkdir -p /out && cp dist/*.zip /out/"]
