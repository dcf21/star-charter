# Use Python 3.12 running on Ubuntu 24.04
FROM ubuntu:24.04

# Install required libraries
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get install -y apt-utils dialog file git vim python3 python3-dev \
                       build-essential make gcc wget gzip libgsl-dev \
                       pkg-config libcairo2-dev python3-numpy imagemagick \
                       ; apt-get clean

# Copy code into container
WORKDIR /
ADD . star-charter

# Fetch data
WORKDIR /star-charter
RUN /star-charter/prettymake clean
RUN /star-charter/setup.sh 2>&1 | tee installation.log

# Make demo charts
WORKDIR /star-charter/examples
