#!/bin/bash

ROOT=$(realpath "$(dirname "$0")")
WORKER=1

error() {
  echo "$@" 1>&2
}

fail() {
  error "$@"
  exit 1
}

find_clang() {
    if [ -f "/usr/local/opt/llvm/bin/clang" ]; then
        echo "/usr/local/opt/llvm/bin/clang"
    elif hash which 2>/dev/null; then
        which clang
    fi
}

find_linker() {
    if [ -f "/usr/local/opt/llvm/bin/clang" ]; then
        echo "/usr/local/opt/llvm/bin/clang"
    elif hash which 2>/dev/null; then
        which clang
    fi
}

find_libtool() {
    if hash which 2>/dev/null; then
        which libtool
    fi
}

compile_c++() {
    OBJFOLDER="$ROOT/.tmp/obj/$(dirname "$1")"
    FILENAME=$(basename "$1")

    if [ ! -f "$OBJFOLDER" ]; then
        mkdir -p "$OBJFOLDER"
    fi

    includes=("${!2}")
    includeArg=""
    if [ ${#includes} -ne 0 ]; then
        for i in "${includes[@]}"; do
            includeArg+="-I $i "
        done
    fi

    if [ -f "$ROOT/$1" ]; then
        if ! eval "$CXX -x c++ $CXXFLAGS -c $ROOT/$1 $includeArg -o $OBJFOLDER/${FILENAME%%.*}.o"; then
            fail "$ROOT/$1 compilation error"
        fi
    else
        fail "$ROOT/$1 doesn't exist"
    fi
}

libtool_c++() {
    srcs=("${!1}")
    objects=""
    for f in "${srcs[@]}"; do
        filename_obj="$ROOT/.tmp/obj/${f%%.*}.o"
        if [ ! -f "$filename_obj" ]; then
            fail "$filename_obj doesn't exist"
        else
            objects+="$filename_obj "
        fi
    done

    eval "$LIBTOOL -static -o $ROOT/$2.a $objects"
}

if [ -z "${CXX+x}" ]; then
    CXX=$(find_clang)
fi

if [ -z "${LD+x}" ]; then
    LD=$(find_linker)
fi

if [ -z "${LIBTOOL+x}" ]; then
    LIBTOOL=$(find_libtool)
fi

if [ -z "${CXXFLAGS+x}" ]; then
    CXXFLAGS="-Og -g -fno-omit-frame-pointer -std=c++1z -march=native "
    #CXXFLAGS="-Ofast -flto=thin -std=c++1z -march=native "
    CXXFLAGS+="-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-c++11-compat -Wno-c++11-compat-pedantic -Wno-c++14-compat -Wno-c++14-compat-pedantic -Wno-padded"
fi
if [ -z "${LDFLAGS+x}" ]; then
    LDFLAGS="-l c++ -L/usr/local/opt/llvm/lib -Wl,-rpath,/usr/local/opt/llvm/lib"
    #LDFLAGS="-flto=thin -l c++ -L/usr/local/opt/llvm/lib -Wl,-rpath,/usr/local/opt/llvm/lib"
fi

build_future() {
    FILES=(
        "future/debug.cpp"
        "future/future.cpp"
        "future/memory.cpp"
        "future/system_error.cpp"
        "future/thread.cpp"
    )

    N=$WORKER
    for f in "${FILES[@]}"; do
        ((i=i%N)); ((i++==0)) && wait
        compile_c++ "$f" &
    done
    wait

    libtool_c++ FILES[@] "libfuture"
}

build() {
    build_future
}

clean_future() {
    if [ -f "$ROOT/libfuture.a" ]; then
        rm -f "$ROOT/libfuture.a"
    fi
}

clean() {
    if [ -f "$ROOT/.hammer/obj" ]; then
        rm -rf "$ROOT/.hammer/obj"
    fi
    clean_future
}

case "$1" in
    build)
        build
        ;;

    clean)
        clean
        ;;
    *)
        echo $"Usage: $0 {build|clean}"
        exit 1;
esac
