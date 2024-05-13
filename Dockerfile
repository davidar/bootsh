FROM scratch
COPY bootsh /bin/sh
COPY root/ /root/
ENTRYPOINT ["/root/boot.sh"]
