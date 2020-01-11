SYSTEM_HEADER_PROJECTS="libc kernel"
PROJECTS="libc kernel"

export MAKE=${MAKE:-make}
export HOST=${HOST:-$(./default-host.sh)}

export AR=${HOST}-ar
export AS=${HOST}-as
export CC=${HOST}-gcc

export PATH="$HOME/opt/cross/bin:$PATH"

export PREFIX=/usr
export EXEC_PREFIX=$PREFIX
export BOOTDIR=/boot
export LIBDIR=$EXEC_PREFIX/lib
export INCLUDEDIR=$PREFIX/include

export BUILD_CONFIG=${BUILD_CONFIG-$(./default-build-config.sh)}
if [ "$BUILD_CONFIG" = "DEBUG" ]; then
  echo "DEBUG build"
  export CFLAGS='-Og -g -ggdb -Werror -D_DEBUG'
else
  echo "RELEASE build"
  export CFLAGS='-O2 -Werror'
fi
export CPPFLAGS=''

# Configure the cross-compiler to use the desired system root.
export SYSROOT="$(pwd)/sysroot"
export CC="$CC --sysroot=$SYSROOT"

# Work around that the -elf gcc targets doesn't have a system include directory
# because it was configured with --without-headers rather than --with-sysroot.
if echo "$HOST" | grep -Eq -- '-elf($|-)'; then
  export CC="$CC -isystem=$INCLUDEDIR"
fi

