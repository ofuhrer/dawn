FROM ubuntu:latest

# Setup base environment
RUN apt-get update && apt-get install -y \
    build-essential \
    bash \
    make \
    cmake \
    autoconf \
    automake \
    libtool \
    pkg-config \
    curl \
    tar \
    git \
    clang \
    libboost-dev \
    python \
    unzip

# Download and install Google protobuf 3.4
RUN git clone https://github.com/google/protobuf.git && \
    cd protobuf && \
    git checkout v3.4.0 && \
    git submodule update --init --recursive && \
    ./autogen.sh && \
    ./configure --prefix=/usr && \
    make && \
    make install && \
    ldconfig && \
    make clean && \
    cd .. && \
    rm -r protobuf

# Download yoda
RUN git clone https://github.com/MeteoSwiss-APN/yoda.git

# Download and install dawn
RUN git clone https://github.com/MeteoSwiss-APN/dawn.git && \
    cd dawn && \
    mkdir build && \
    cd build && \
    cmake .. -DYODA_ROOT=/yoda

CMD ["/bin/bash"]

