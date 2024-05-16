FROM scratch
COPY bootsh /bin/sh
COPY boot.sh /bin/boot.sh
COPY Makefile.packages /tmp/Makefile
COPY wak.c /bin/awk
ENTRYPOINT ["/bin/boot.sh"]
