# Old build sysroot keeps the plugin compatible with shared SA-MP hosting.
# Ubuntu 18.04 is used only as a build ABI baseline, not as a VPS recommendation.
FROM ubuntu:18.04@sha256:152dc042452c496007f07ca9127571cb9c29697f42acbfad72324b2bb2e43c98
RUN dpkg --add-architecture i386 && apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    g++-8-multilib make python3 python3-ply ca-certificates curl perl \
    libtext-template-perl qemu-user zlib1g-dev:i386 && rm -rf /var/lib/apt/lists/*
RUN curl -fL https://github.com/Kitware/CMake/releases/download/v3.31.6/cmake-3.31.6-linux-x86_64.tar.gz -o /tmp/cmake.tar.gz && \
    echo '5a1133ff103c71eb5120e2cc3de922733e7d8a26a98ae716397e8676adb367bf  /tmp/cmake.tar.gz' | sha256sum -c - && \
    mkdir -p /opt/cmake && tar -xzf /tmp/cmake.tar.gz -C /opt/cmake --strip-components=1 && rm /tmp/cmake.tar.gz
RUN curl -fL https://github.com/openssl/openssl/releases/download/openssl-3.5.9/openssl-3.5.9.tar.gz -o /tmp/openssl.tar.gz && \
    echo '603f5602e2eef00d77fbd429d34dcd5822bb301757a1bc9cdb24c670f1eb859a  /tmp/openssl.tar.gz' | sha256sum -c - && \
    mkdir -p /tmp/openssl && tar -xzf /tmp/openssl.tar.gz -C /tmp/openssl --strip-components=1 && \
    cd /tmp/openssl && CC=gcc-8 CXX=g++-8 perl Configure linux-x86 no-shared no-module no-tests no-pinshared \
      --prefix=/opt/city-openssl --libdir=lib -fPIC && \
    make -j 3 build_libs && make install_dev
ENV PATH=/opt/cmake/bin:$PATH
WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc-8 -DCMAKE_CXX_COMPILER=g++-8 \
    -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_SHARED_LINKER_FLAGS=-m32 \
    -DCITY_OPENSSL_ROOT=/opt/city-openssl -DCMAKE_CROSSCOMPILING_EMULATOR=/usr/bin/qemu-i386 && \
    cmake --build build --target citylive city_rules city_events city_plugin_load city_database -j 3 && ctest --test-dir build --output-on-failure && \
    cmake -S ThirdParty/pawn-compiler/source/compiler -B build-pawn -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build-pawn --target pawncc -j 3 && \
    build-pawn/pawncc config/bootstrap.pwn -obuild/citylive.amx
RUN cp /tmp/openssl/LICENSE.txt ThirdParty/OpenSSL-COPYRIGHT.txt && \
    cp /tmp/openssl/AUTHORS.md ThirdParty/OpenSSL-NOTICE.txt && \
    cp /usr/share/doc/zlib1g/copyright ThirdParty/zlib-COPYRIGHT.txt && \
    cp /usr/share/doc/gcc-8-base/copyright ThirdParty/GCC-runtime-COPYRIGHT.txt
CMD ["sh", "-c", "python3 scripts/package.py --plugin build/citylive.so --bootstrap build/citylive.amx && mkdir -p /out && cp dist/*.zip /out/"]
