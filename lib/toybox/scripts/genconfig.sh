#!/bin/sh

set -e

# This has to be a separate file from scripts/make.sh so it can be called
# before menuconfig. (It's called again from scripts/make.sh just to be sure.)

. scripts/portability.sh

mkdir -p "$GENDIR"

probecc()
{
  ${CROSS_COMPILE}${CC} $CFLAGS $LDFLAGS -xc -o /dev/null - "$@"
}

# Probe for a single config symbol with a "compiles or not" test.
# Symbol name is first argument, flags second, feed C file to stdin
probesymbol()
{
  symbol="$1"
  shift
  probecc "$@" 2>/dev/null && DEFAULT=y || DEFAULT=n
  rm a.out 2>/dev/null
  printf "config $symbol\n\tbool\n\tdefault $DEFAULT\n\n" || exit 1
}

probeconfig()
{
  # Some commands are android-specific
  probesymbol TOYBOX_ON_ANDROID -c << EOF
    #ifndef __ANDROID__
    #error nope
    #endif
EOF

  # nommu support
  probesymbol TOYBOX_FORK << EOF
    #include <unistd.h>
    int main(int argc, char *argv[]) { return fork(); }
EOF
  printf '\tdepends on !TOYBOX_FORCE_NOMMU\n'
}

genconfig()
{
  # Reverse sort puts posix first, examples last.
  for j in $(ls toys/*/README | sort -s -r)
  do
    DIR="$(dirname "$j")"

    [ $(ls "$DIR" | wc -l) -lt 2 ] && continue

    echo "menu \"$(head -n 1 $j)\""
    echo

    # extract config stanzas from each source file, in alphabetical order
    for i in $(ls -1 $DIR/*.c)
    do
      # Grab the config block for Config.in
      echo "# $i"
      $SED -n '/^\*\//q;/^config [A-Z]/,$p' $i || return 1
      echo
    done

    echo endmenu
  done
}

probeconfig > "$GENDIR"/Config.probed || rm "$GENDIR"/Config.probed
genconfig > "$GENDIR"/Config.in || rm "$GENDIR"/Config.in

# Find names of commands that can be built standalone in these C files
toys()
{
  grep 'TOY(.*)' "$@" | grep -v TOYFLAG_NOFORK | grep -v "0))" | \
    $SED -En 's/([^:]*):.*(OLD|NEW)TOY\( *([a-zA-Z][^,]*) *,.*/\1:\3/p'
}

WORKING= PENDING= EXAMPLE=
toys toys/*/*.c | (
while IFS=":" read FILE NAME
do
  printf "test_$NAME:\n\tscripts/test.sh $NAME\n\n"
  [ "$NAME" = help ] && continue
  [ "$NAME" = install ] && continue
  [ "$NAME" = sh ] && FILE="toys/*/*.c"
  printf "$NAME: $FILE *.[ch] lib/*.[ch]\n\tscripts/single.sh $NAME\n\n"
  case "$FILE" in
    *example*) EXAMPLE="$EXAMPLE $NAME" ;;
    *pending*) PENDING="$PENDING $NAME" ;;
    *) WORKING="$WORKING $NAME" ;;
  esac
done &&
printf "clean::\n\t@rm -f $WORKING $PENDING\n" &&
printf "list:\n\t@echo $(echo $WORKING | tr ' ' '\n' | sort | xargs)\n" &&
printf "list_example:\n\t@echo $(echo $EXAMPLE | tr ' ' '\n' | sort | xargs)\n" &&
printf "list_pending:\n\t@echo $(echo $PENDING | tr ' ' '\n' | sort | xargs)\n" &&
printf ".PHONY: $WORKING $PENDING\n" | $SED 's/ \([^ ]\)/ test_\1/g'
) > .singlemake
