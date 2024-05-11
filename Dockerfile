FROM scratch
COPY bootsh /bin/sh
COPY root/ /root/
WORKDIR /root/
