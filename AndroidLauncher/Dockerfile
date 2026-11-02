FROM ubuntu:22.04
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends openjdk-17-jdk-headless curl unzip ca-certificates && rm -rf /var/lib/apt/lists/*
RUN curl -fL https://dl.google.com/android/repository/commandlinetools-linux-13114758_latest.zip -o /tmp/sdk.zip && \
    echo '7ec965280a073311c339e571cd5de778b9975026cfcbe79f2b1cdcb1e15317ee  /tmp/sdk.zip' | sha256sum -c - && \
    mkdir -p /opt/android-sdk/cmdline-tools && unzip -q /tmp/sdk.zip -d /opt/android-sdk/cmdline-tools && \
    mv /opt/android-sdk/cmdline-tools/cmdline-tools /opt/android-sdk/cmdline-tools/19.0 && rm /tmp/sdk.zip
RUN curl -fL https://services.gradle.org/distributions/gradle-8.9-bin.zip -o /tmp/gradle.zip && \
    echo 'd725d707bfabd4dfdc958c624003b3c80accc03f7037b5122c4b1d0ef15cecab  /tmp/gradle.zip' | sha256sum -c - && \
    unzip -q /tmp/gradle.zip -d /opt && rm /tmp/gradle.zip
ENV ANDROID_HOME=/opt/android-sdk
RUN yes | /opt/android-sdk/cmdline-tools/19.0/bin/sdkmanager --sdk_root=/opt/android-sdk --licenses >/dev/null && \
    /opt/android-sdk/cmdline-tools/19.0/bin/sdkmanager --sdk_root=/opt/android-sdk 'platforms;android-35' 'build-tools;34.0.0'
WORKDIR /android
COPY . .
RUN mkdir -p build/test-classes && javac -encoding UTF-8 -d build/test-classes app/src/main/java/ru/citylive/launcher/ServerInfo.java tests/ServerInfoTest.java && \
    java -ea -cp build/test-classes ru.citylive.launcher.ServerInfoTest && \
    /opt/gradle-8.9/bin/gradle --no-daemon :app:assembleDebug :app:lintDebug
CMD ["sh", "-c", "mkdir -p /out && cp app/build/outputs/apk/debug/app-debug.apk /out/CityLiveLauncher-0.1.0-debug.apk"]
