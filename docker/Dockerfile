FROM ubuntu:25.04

RUN apt update && apt upgrade -y && \
    apt install -y ca-certificates build-essential cmake vim postgresql  \
    libssl-dev libpq-dev uuid-dev libspdlog-dev gdb libcurl4-openssl-dev \
    nlohmann-json3-dev file dpkg-dev fakeroot mc npm iproute2 net-tools \
    tmux libgtest-dev libgmock-dev libboost-dev && \
    npm install -g wscat


