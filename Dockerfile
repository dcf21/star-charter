# Use Python 3.10 running on Debian Bullseye
FROM python:3.10-bullseye

# Install required libraries
RUN apt-get update
RUN apt-get install -y wget gzip git libgsl-dev libcairo2-dev ; apt-get clean

# Install ephemeris software
WORKDIR /
RUN git clone https://github.com/dcf21/ephemeris-compute-de430.git
WORKDIR /ephemeris-compute-de430
RUN /ephemeris-compute-de430/setup.sh

# Copy code into container
WORKDIR /
ADD . star-charter

# Fetch data
WORKDIR /star-charter
RUN /star-charter/setup.sh 2>&1 | tee installation.log

# Make demo charts
WORKDIR /star-charter/examples

