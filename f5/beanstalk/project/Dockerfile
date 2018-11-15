FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update -q && apt-get install --no-install-recommends -yq build-essentials alien wget unzip clinfo \
    && apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*
# ================================================================





# ================================================================

# Node install dependencies
RUN apt-get update && \
    apt-get install -y curl gnupg2 uuid

# Install node
RUN curl -sL https://deb.nodesource.com/setup_8.x | bash
RUN apt-get install --yes nodejs

# Create work directory
RUN mkdir /work/
WORKDIR /work/

# Npm
RUN npm install express request request-promise-native aws-sdk

# The core stuff
COPY docker-entrypoint.sh /work/
COPY server.js /work/

EXPOSE 80

ENTRYPOINT /work/docker-entrypoint.sh