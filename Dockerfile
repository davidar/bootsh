FROM scratch
COPY bootsh /bin/sh
COPY root/ /root/
CMD ["/root/boot.sh"]
