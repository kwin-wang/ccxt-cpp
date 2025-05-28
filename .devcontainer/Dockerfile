FROM mcr.microsoft.com/devcontainers/cpp:ubuntu-22.04

RUN apt-get update -y && \
    apt-get install -y --no-install-recommends \
        libssl-dev \
        libcurl4-openssl-dev \
        nlohmann-json3-dev \
        libboost-system-dev \
        libboost-filesystem-dev \
        libboost-context-dev \
        libgtest-dev \
        googletest && \
    rm -rf /var/lib/apt/lists/*