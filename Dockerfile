# docker run -it -v `pwd`:/src -v `pwd`/out:/opt/shv  <img> /src/build-docker.sh

FROM debian:jessie

LABEL maintainer "Milan Dunghubel <dunghubel@elektroline.cz>"

ENV HOME /root
ENV DEBIAN_FRONTEND noninteractive
ENV TERM=xterm

RUN apt-get update
RUN apt-get -qqy install build-essential git libfontconfig1 curl
ADD http://claudius.elektroline.cz:8080/Public/dev/qt_5.8.tar.gz .
RUN tar -xf qt_5.8.tar.gz -C /

ENV PATH="/opt/qt/5.8/gcc_64/bin:$PATH"

RUN apt-get -qqy install libglib2.0-0
RUN apt-get -qqy install mesa-common-dev libglu1-mesa-dev
RUN apt-get -qqy install libfuse-dev libxml2-dev libvncserver-dev
#RUN apt-get -qqy install libfuse-dev libxml2-dev libvncserver-dev libboost-system1.58-dev

CMD mkdir /build
WORKDIR /build


