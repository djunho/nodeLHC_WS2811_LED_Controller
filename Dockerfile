FROM alpine AS fetch

RUN apk add --update openssl
RUN wget -q https://dl.espressif.com/dl/xtensa-lx106-elf-gcc8_4_0-esp-2020r3-linux-amd64.tar.gz
RUN tar xzf /xtensa-lx106-elf-gcc*.tar.gz

FROM python

CMD /bin/bash
COPY --from=fetch /xtensa-lx106-elf /opt/xtensa-lx106-elf
ENV PATH /opt/xtensa-lx106-elf/bin:$PATH

RUN apt update -y && apt install -y \
        cmake \
        gcc \
        git \
        wget \
        make \
        libncurses-dev \
        flex \
        bison \
        bear \
        gperf

RUN pip install pyserial
COPY libs/ESP8266_RTOS_SDK/requirements.txt /tmp/requirements.txt
RUN python -m pip install --user -r /tmp/requirements.txt

# To avoid "fatal: detected dubious ownership in repository at 'repo'"
RUN git config --system --add safe.directory '*' # For all users and all repositories

CMD [ "make" ]
