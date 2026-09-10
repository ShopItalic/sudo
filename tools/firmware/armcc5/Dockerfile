# Linux host dependencies for the supplier's Arm Compiler 5.06u7.
# The compiler and its license are NOT included or downloaded here.
# Arm's CMSIS_5 Docker setup uses Ubuntu focal and lib32stdc++6:
# https://github.com/ARM-software/CMSIS_5/blob/develop/docker/dockerfile
FROM ubuntu:20.04@sha256:8feb4d8ca5354def3d8fce243717141ce31e2c428701f6682bd2fafe15388214

RUN test "$(dpkg --print-architecture)" = amd64 \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends -y \
       ca-certificates=20240203~20.04.1 \
       file=1:5.38-4 \
       lib32stdc++6=10.5.0-1ubuntu1~20.04 \
       lib32z1=1:1.2.11.dfsg-2ubuntu1.5 \
       make=4.2.1-1.2 \
       python3=3.8.2-0ubuntu2 \
    && dpkg-query -W -f='${binary:Package}=${Version}\n' > /opt/armcc5-host-packages.txt \
    && useradd --create-home --uid 1000 ringbuild

COPY probe_host.py /usr/local/bin/probe-armcc5-host.py
WORKDIR /workspace
USER ringbuild
CMD ["python3", "/usr/local/bin/probe-armcc5-host.py"]
