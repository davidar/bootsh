#!/bin/sh

set -e

# Grab default values for $CFLAGS and such.
# set -o pipefail
. scripts/portability.sh

# Shell functions called by the build

# Build a tool that runs on the host
hostcomp()
{
  if [ ! -f "$UNSTRIPPED"/$1 ] || [ "$UNSTRIPPED"/$1 -ot scripts/$1.c ]
  then
    $HOSTCC $CFLAGS $LDFLAGS scripts/$1.c -o "$UNSTRIPPED"/$1 || exit 1
  fi
}

# --as-needed removes libraries we don't use any symbols out of, but the
# compiler has no way to ignore a library that doesn't exist, so detect
# and skip nonexistent libraries for it (probing in parallel).
LIBRARIES=$(
  [ -z "$V" ] && X=/dev/null || X=/dev/stderr
  for i in util crypt m resolv selinux smack attr crypto z log iconv tls ssl
  do
    echo "int main(int argc,char*argv[]){return 0;}" | \
    ${CROSS_COMPILE}${CC} $CFLAGS $LDFLAGS -xc - -l$i >>$X 2>&1 \
      -o /dev/null &&
      echo -l$i &
  done | sort | xargs
)

[ -z "$VERSION" ] && [ -d ".git" ] && [ -n "$(which git 2>/dev/null)" ] &&
  VERSION="$(git describe --tags --abbrev=12 2>/dev/null)"

# Set/record build environment information
compflags()
{
  # The #d lines tag dependencies that force full rebuild if changed
  echo '#!/bin/sh'
  echo
  echo "VERSION='$VERSION'"
  echo "LIBRARIES='$LIBRARIES'"
  echo "BUILD='${CROSS_COMPILE}${CC} $CFLAGS -I$PWD $OPTIMIZE" \
       "'\"\${VERSION:+-DTOYBOX_VERSION=\\\"$VERSION\\\"}\" #d"
  echo "LINK='$LDOPTIMIZE $LDFLAGS '\"\$LIBRARIES\" #d"
  echo "#d Config was $KCONFIG_CONFIG"
  echo "#d PATH was '$PATH'"
}

mkdir -p "$UNSTRIPPED"  "$(dirname $OUTNAME)"

# Extract a list of toys/*/*.c files to compile from the data in $KCONFIG_CONFIG
# (First command names, then filenames with relevant {NEW,OLD}TOY() macro.)

[ -n "$V" ] && printf "\nWhich C files to build...\n"
TOYFILES="$($SED -n 's/^CONFIG_\([^=]*\)=.*/\1/p' "$KCONFIG_CONFIG" | xargs | tr ' ' '|')"
TOYFILES="$(egrep -l "^USE_($TOYFILES)[(]...TOY[(]" toys/*/*.c | xargs)"

# Write build variables (and set them locally), then append build invocation.
compflags > "$GENDIR"/build.sh && . "$GENDIR/build.sh" &&
  {
    printf 'FILES="\n%s\n"' "$(echo "$TOYFILES" | fold -s)" &&
    echo &&
    printf "\$BUILD lib/*.c \$FILES \$LINK -o $OUTNAME\n"
  } >> "$GENDIR"/build.sh &&
  chmod +x "$GENDIR"/build.sh || exit 1

scripts/genconfig.sh

# The multiplexer is the first element in the array
echo "USE_TOYBOX(NEWTOY(toybox, 0, TOYFLAG_STAYROOT|TOYFLAG_NOHELP))" \
  > "$GENDIR"/newtoys.h
# Sort rest by name for binary search (copy name to front, sort, remove copy)
$SED -n 's/^\(USE_[^(]*(.*TOY(\)\([^,]*\)\(,.*\)/\2 \1\2\3/p' toys/*/*.c \
  | sort -s -k 1,1 | $SED 's/[^ ]* //'  >> "$GENDIR"/newtoys.h
[ $? -ne 0 ] && exit 1

# Rebuild config.h from .config
$SED -En $KCONFIG_CONFIG > "$GENDIR"/config.h \
  -e 's/^# CONFIG_(.*) is not set.*/#define CFG_\1 0\n#define USE_\1(...)/p;t' \
  -e 's/^CONFIG_(.*)=y.*/#define CFG_\1 1\n#define USE_\1(...) __VA_ARGS__/p;t'\
  -e 's/^CONFIG_(.*)=/#define CFG_\1 /p' || exit 1

# Process config.h and newtoys.h to generate FLAG_x macros. Note we must
# always #define the relevant macro, even when it's disabled, because we
# allow multiple NEWTOY() in the same C file. (When disabled the FLAG is 0,
# so flags&0 becomes a constant 0 allowing dead code elimination.)

hostcomp mkflags

# Parse files through C preprocessor twice, once to get flags for current
# .config and once to get flags for allyesconfig
for I in A B
do
  (
  # define macros and select header files with option string data

  echo "#define NEWTOY(aa,bb,cc) aa $I bb"
  echo '#define OLDTOY(...)'
  if [ "$I" = A ]
  then
    cat "$GENDIR"/config.h
  else
    $SED '/USE_.*([^)]*)$/s/$/ __VA_ARGS__/' "$GENDIR"/config.h
  fi
  echo '#include "lib/toyflags.h"'
  cat "$GENDIR"/newtoys.h

  # Run result through preprocessor, glue together " " gaps leftover from USE
  # macros, delete comment lines, print any line with a quoted optstring,
  # turn any non-quoted opstring (NULL or 0) into " " (because fscanf can't
  # handle "" with nothing in it, and mkflags uses that).

  ) | ${CROSS_COMPILE}${CC} -E - | \
  $SED -n -e 's/" *"//g;/^#/d;t clear;:clear;s/"/"/p;t;s/\( [AB] \).*/\1 " "/p'

# Sort resulting line pairs and glue them together into triplets of
#   command "flags" "allflags"
# to feed into mkflags C program that outputs actual flag macros
# If no pair (because command's disabled in config), use " " for flags
# so allflags can define the appropriate zero macros.

done | sort -s | $SED -n -e 's/ A / /;t pair;h;s/\([^ ]*\).*/\1 " "/;x' \
  -e 'b single;:pair;h;n;:single;s/[^ ]* B //;H;g;s/\n/ /;p' | \
  tee "$GENDIR"/flags.raw | "$UNSTRIPPED"/mkflags > "$GENDIR"/flags.h || exit 1

# Extract global structure definitions and flag definitions from toys/*/*.c

{
  STRUX="$($SED -ne 's/^#define[ \t]*FOR_\([^ \t]*\).*/\1/;T s1;h;:s1' \
  -e '/^GLOBALS(/,/^)/{s/^GLOBALS(//;T s2;g;s/.*/struct &_data {/;:s2;s/^)/};\n/;p}' \
  $TOYFILES)"
  echo "$STRUX" &&
  echo "extern union global_union {" &&
  echo "$STRUX" | $SED -n 's/^struct \(.*\)_data .*/\1/;T;s/.*/\tstruct &_data &;/p' &&
  echo "} this;"
} > "$GENDIR"/globals.h || exit 1

hostcomp mktags
$SED -n '/TAGGED_ARRAY(/,/^)/{s/.*TAGGED_ARRAY[(]\([^,]*\),/\1/;p}' \
  toys/*/*.c lib/*.c | "$UNSTRIPPED"/mktags > "$GENDIR"/tags.h

# Create help.h, and zhelp.h if zcat enabled
hostcomp config2help
"$UNSTRIPPED"/config2help Config.in $KCONFIG_CONFIG > "$GENDIR"/help.h||exit 1

if grep -qx 'CONFIG_TOYBOX_ZHELP=y' "$KCONFIG_CONFIG"
then
  $HOSTCC -I . scripts/install.c -o "$UNSTRIPPED"/instlist || exit 1
  { echo "#define ZHELP_LEN $("$UNSTRIPPED"/instlist --help | wc -c)" &&
    "$UNSTRIPPED"/instlist --help | gzip -9 | od -Anone -vtx1 | \
    sed 's/ /,0x/g;1s/^,/static char zhelp_data[] = {\n /;$s/.*/&};/'
  } > "$GENDIR"/zhelp.h || exit 1
else
  rm -f "$GENDIR"/zhelp.h
fi

[ -z "$DIDNEWER" ] || echo }
[ -n "$NOBUILD" ] && exit 0

# echo "Compile $OUTNAME"
DOTPROG=.

# This is a parallel version of: do_loudly $BUILD lib/*.c $TOYFILES $LINK

# Build all if oldest generated/obj file isn't newer than all header files.
[ -d "$GENDIR"/obj/ ] && X="$(ls -1t "$GENDIR"/obj/* 2>/dev/null | tail -n 1)"
if [ ! -e "$X" ] || [ -n "$(find toys -name "*.h" -newer "$X")" ]
then
  rm -rf "$GENDIR"/obj && mkdir -p "$GENDIR"/obj || exit 1
else
  # always redo toy_list[] and help_data[]
  rm -f "$GENDIR"/obj/main.o || exit 1
fi

# build each generated/obj/*.o file in parallel

cat > build.ninja <<EOF
rule cc_toybox
  command = $BUILD -c \$in -o \$out
  description = Building \$in

rule ar_toybox
  command = ar rcs \$out \$in
  description = Building \$out
EOF

LNKFILES=
for i in lib/*.c $TOYFILES
do
  X=$(echo "$i" | sed 's|lib/|lib_|')
  X=${X##*/}
  OUT="\$builddir/toybox/${X%%.c}.o"
  LNKFILES="$LNKFILES $OUT"
  echo build $OUT: cc_toybox $PWD/$i >> build.ninja
done

echo build \$builddir/libtoybox.a: ar_toybox $LNKFILES >> build.ninja
