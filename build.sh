#!/bin/sh
# vi: set et ft=sh ts=2 sw=2 fenc=utf-8 :vi
export LC_ALL=C
export TZ=UTC

IsBuildDebug=1
IsBuildEnabled=1
IsTestsEnabled=1

PROJECT_NAME=game
OUTPUT_NAME=$PROJECT_NAME

usage() {
  cat <<EOF
  NAME
    build.sh [OPTIONS]
  
  DESCRIPTION
    Build script of $PROJECT_NAME.
  
  OPTIONS
    -d, --debug
      Build with debugging information.

    -r, --release
      Build with optimizations turned on.

    --build-directory=path
      Build executables in this folder. If directory not exists, one will be
      created.

    --disable-$PROJECT_NAME
      Do not build $PROJECT_NAME binary.

    --force-build-sdl3
      Even if you have sdl3 library installed on your system, build it from
      source.

    test
      Run tests.

    -h, --help
      Display help page.

  EXAMPLES
     $ ./build.sh
     Build only the $PROJECT_NAME

     $ ./build.sh test
     Run only the tests.
EOF
}

FORCE_BUILD_SDL3=0
for i in "$@"; do
  case $i in
    -d|--debug)
      IsBuildDebug=1
      ;;
    -r|--release)
      IsBuildDebug=0
      ;;
    --build-directory=*)
      OutputDir="${i#*=}"
      ;;
    "--disable-$PROJECT_NAME")
      IsBuildEnabled=0
      ;;
    --force-build-sdl3)
      FORCE_BUILD_SDL3=1
      ;;
    test|tests)
      IsBuildEnabled=0
      IsTestsEnabled=1
      ;;
    -h|-help|--help)
      usage
      exit 0
      ;;
    *)
      echo "argument $i not recognized"
      usage
      exit 1
      ;;
  esac
done

################################################################
# UTILITY FUNCTIONS
################################################################

# [0,1] IsCommandExists(cmd)
IsCommandExists() {
  cmd="$1"
  if command -v "$cmd" >/dev/null 2>&1; then
    echo 1
  else
    echo 0
  fi
}

################################################################
# TEXT FUNCTIONS
################################################################

# [0,1] StringContains(string, search)
StringContains() {
  string="$1"
  search="$2"

  case "$string" in
    *"$search"*)
      echo 1
      ;;
    *)
      echo 0
      ;;
  esac
}

# [0,1] StringStartsWith(string, search)
StringStartsWith() {
  string="$1"
  search="$2"

  case "$string" in
    "$search"*)
      echo 1
      ;;
    *)
      echo 0
      ;;
  esac
}

# [0,1] StringEndsWith(string, search)
StringEndsWith() {
  string="$1"
  search="$2"

  case "$string" in
    *"$search")
      echo 1
      ;;
    *)
      echo 0
      ;;
  esac
}

# [0,1] StringEquals(left, right)
StringEquals() {
  left="$1"
  right="$2"

  if [ "$left" = "$right" ]; then
    echo 1
  else
    echo 0
  fi
}

# string Basename(path)
Basename() {
  path="$1"
  echo "${path##*/}"
}

# string BasenameWithoutExtension(path)
BasenameWithoutExtension() {
  path="$1"
  basename="$(Basename "$path")"
  echo "${basename%.*}"
}

# string Dirname(path)
Dirname() {
  path="$1"
  if [ $(StringStartsWith "$path" '/') ]; then
    dirname="${path%/*}"
    if [ -z "$dirname" ]; then
      echo '/'
    else
      echo "$dirname"
    fi
  else
    echo '.'
  fi
}

################################################################
# Network FUNCTIONS
################################################################
HAS_CURL=$(IsCommandExists curl)
if [ "$HAS_CURL" -eq 1 ]; then
  CURL_VERSION=$(curl --version | head -n 1 | cut -d ' ' -f2)
fi

HAS_WGET=$(IsCommandExists wget)
if [ "$HAS_WGET" -eq 1 ]; then
  WGET_VERSION=$(wget --version | head -n 1 | cut -d ' ' -f3)
fi

# Download(string url, string output)
Download() {
  if [ "$HAS_CURL" -eq 0 ] && [ "$HAS_WGET" -eq 0 ]; then
    echo "curl or wget requried"
    exit 1
  fi
  url="$1"
  output="$2"

  if [ "$HAS_CURL" -eq 1 ]; then
    curl --location --output "$output" "$url"
  elif [ "$HAS_WGET" -eq 1 ]; then
    wget -O "$output" "$url"
  else
    echo "assertion failed at Download()"
    exit 1
  fi
}

################################################################
# HASH FUNCTIONS
################################################################
HAS_B2SUM=$(b2sum --version >/dev/null 2>&1 && echo 1 || echo 0)
HAS_SHA256SUM=$(sha256sum --version >/dev/null 2>&1 && echo 1 || echo 0)
HAS_SHA1SUM=$(sha1sum --version >/dev/null 2>&1 && echo 1 || echo 0)

# HashCheckB2(string path, string hash) -> bool
HashCheckB2() {
  if [ "$HAS_B2SUM" -eq 0 ]; then
    echo "b2sum is required"
    exit 1
  fi
  path="$1"
  hash="$2"
  echo "$hash $path" | b2sum --check --quiet >/dev/null 2>&1 && echo 1 || echo 0
}

# HashCheckSHA1(string path, string hash) -> bool
HashCheckSHA1() {
  if [ "$HAS_SHA1SUM" -eq 0 ]; then
    echo "sha1sum is required"
    exit 1
  fi
  path="$1"
  hash="$2"
  echo "$hash $path" | sha1sum --check --quiet >/dev/null 2>&1 && echo 1 || echo 0
}

# HashCheckSHA256(string path, string hash) -> bool
HashCheckSHA256() {
  if [ "$HAS_SHA256SUM" -eq 0 ]; then
    echo "sha256sum is required"
    exit 1
  fi
  path="$1"
  hash="$2"
  echo "$hash $path" | sha256sum --check --quiet >/dev/null 2>&1 && echo 1 || echo 0
}

################################################################
# TIME FUNCTIONS
################################################################

StartTimer() {
  startedAt=$(date +%s)
}

StopTimer() {
  echo $(( $(date +%s) - startedAt ))
}

################################################################
# LOG FUNCTIONS
################################################################

Timestamp="$(date +%Y%m%dT%H%M%S)"

Log() {
  string=$1
  output="$OutputDir/logs/build-$Timestamp.log"
  if [ ! -e "$(Dirname "$output")" ]; then
    mkdir "$(Dirname "$output")"
  fi
  echo "$string" >> "$output"
}

Debug() {
  string=$1
  output="$OutputDir/logs/build-$Timestamp.log"
  if [ ! -e "$(Dirname "$output")" ]; then
    mkdir "$(Dirname "$output")"
  fi
  echo "[DEBUG] $string" >> "$output"
}

################################################################

ProjectRoot="$(Dirname "$(realpath "$0")")"
if [ "$(pwd)" != "$ProjectRoot" ]; then
  echo "Must be call from project root!"
  echo "  $ProjectRoot"
  exit 1
fi

OutputDir="${OutputDir:-$ProjectRoot/build}"
if [ ! -e "$OutputDir" ]; then
  mkdir "$OutputDir"

  # version control ignore
  echo '*' > "$OutputDir/.gitignore"

  echo 'syntax: glob' > "$OutputDir/.hgignore"
  echo '**/*' >> "$OutputDir/.hgignore"
fi

IsPlatformLinux=$(StringEquals "$(uname)" 'Linux')
IsPlatformWindows=$(StringEquals "$(uname)" 'Windows')

cc="${CC:-clang}"
IsCompilerGCC=$(StringStartsWith "$("$cc" --version | head -n 1 -c 32)" "gcc")
IsCompilerClang=$(StringStartsWith "$("$cc" --version | head -n 1 -c 32)" "clang")
if [ "$IsCompilerGCC" -eq 0 ] && [ "$IsCompilerClang" -eq 0 ]; then
  echo "unsupported compiler $cc. continue (y/n)?"
  read -r input
  if [ "$input" != 'y' ] && [ "$input" != 'Y' ]; then
    exit 1
  fi

  echo "Assuming $cc as GCC"
  IsCompilerGCC=1
fi

cflags="$CFLAGS"
# standard
cflags="$cflags -std=c99"
# performance
if [ $(StringContains "$cflags" '-march=') -eq 0 ]; then
  cflags="$cflags -march=x86-64-v3"
fi
cflags="$cflags -funroll-loops"
cflags="$cflags -fomit-frame-pointer"
# warnings
cflags="$cflags -Wall -Werror"
cflags="$cflags -Wconversion"
if [ "$IsCompilerGCC" -eq 1 ]; then
  cflags="$cflags -Wshadow=local"
elif [ "$IsCompilerClang" -eq 1 ]; then
  cflags="$cflags -Wshadow"
fi
cflags="$cflags -Wno-unused-parameter"
cflags="$cflags -Wno-unused-result"
cflags="$cflags -Wno-missing-braces"
cflags="$cflags -Wno-unused-function"

cflags="$cflags -DCOMPILER_GCC=$IsCompilerGCC"
cflags="$cflags -DCOMPILER_CLANG=$IsCompilerClang"

cflags="$cflags -DIS_PLATFORM_LINUX=$IsPlatformLinux"
cflags="$cflags -DIS_PLATFORM_WINDOWS=$IsPlatformWindows"

cflags="$cflags -DIS_BUILD_DEBUG=$IsBuildDebug"
if [ $IsBuildDebug -eq 1 ]; then
  cflags="$cflags -g -O0"
  cflags="$cflags -Wno-unused-but-set-variable"
  cflags="$cflags -Wno-unused-variable"
else
  cflags="$cflags -O2"
fi

if [ "$IsPlatformLinux" -eq 1 ]; then
  # needed by c libraries
  cflags="$cflags -D_GNU_SOURCE=1"
  cflags="$cflags -D_XOPEN_SOURCE=700"
fi

ldflags="${LDFLAGS}"
ldflags="$ldflags -Wl,--as-needed"
ldflags="${ldflags# }"

Log "Started at $(date '+%Y-%m-%d %H:%M:%S')"
Log "================================================================"
Log "root:      $ProjectRoot"
Log "build:     $OutputDir"
Log "os:        $(uname)"
Log "compiler:  $cc"

Log "cflags: $cflags"
if [ -n "$CFLAGS" ]; then
  Log "from your env: $CFLAGS"
fi

Log "ldflags: $ldflags"
if [ -n "$LDFLAGS" ]; then
  Log "from your env: $LDFLAGS"
fi
Log "================================================================"

# TODO: Fix tests in windows
if [ $IsPlatformWindows -eq 1 ]; then
  IsTestsEnabled=0
fi

if [ $IsBuildEnabled -eq 1 ]; then
  if [ "$IsPlatformWindows" -eq 1 ]; then
    ################################################################
    # WINDOWS BUILD
    ################################################################

    LIBSDL_DIR="build/3rdparty/SDL3-3.2.4"
    INC_LIBSDL="-I$LIBSDL_DIR-install/include"
    LIB_LIBSDL="-L$LIBSDL_DIR-install/lib/x64 -lSDL3"
    if [ ! -e build/SDL3.dll ]; then
      cp "$LIBSDL_DIR-install/lib/x64/SDL3.dll" build/SDL3.dll
    fi

    if [ $IsBuildDebug -eq 1 ]; then
      src="src/game.c"
      output="build/$OUTPUT_NAME.dll"
      inc="-Iinclude $INC_LIBSDL"
      lib="$LIB_LIBSDL"

      # HACK: Windows locks files that are belonging to process
      mv "$output" "$output.old"

      StartTimer
      if "$cc" --shared $cflags $ldflags $inc -o "$output" $src $lib; then
        echo "$OUTPUT_NAME.dll compiled in $(StopTimer) seconds."
      fi
    fi

    src="src/main.c"
    output="build/$OUTPUT_NAME.exe"
    inc="-Iinclude $INC_LIBSDL"
    lib="$LIB_LIBSDL"
    StartTimer
    if "$cc"  $cflags $ldflags $inc -o "$output" $src $lib; then
      echo "$OUTPUT_NAME.exe compiled in $(StopTimer) seconds."
    fi

  elif [ "$IsPlatformLinux" -eq 1 ]; then
    ################################################################
    # LINUX BUILD
    #      .--.
    #     |o_o |
    #     |:_/ |
    #    //   \ \
    #   (|     | )
    #  /'\_   _/`\
    #  \___)=(___/
    ################################################################
    #INC_LIBSDL3=
    #LIB_LIBSDL3=
    . "$ProjectRoot/3rdparty/SDL3/build.sh"

    LIB_M='-lm'

    ### <PROJECT_NAME>
    # ShaderInc=
    #. $ProjectRoot/shader/build.sh

    if [ $IsBuildDebug -eq 1 ]; then
      src="$ProjectRoot/src/game.c"
      output="$OutputDir/$OUTPUT_NAME.so"
      inc="-I$ProjectRoot/include $INC_LIBSDL"
      lib="$LIB_LIBSDL $LIB_M"
      StartTimer
      if "$cc" --shared $cflags $ldflags $inc -o "$output" $src $lib; then
        echo "$OUTPUT_NAME.so compiled in $(StopTimer) seconds."
      fi
    fi

    src="$ProjectRoot/src/main.c"
    output="$OutputDir/$OUTPUT_NAME"
    inc="-I$ProjectRoot/include $INC_LIBSDL"
    lib="$LIB_LIBSDL $LIB_M"
    StartTimer
    if "$cc" $cflags $ldflags $inc -o "$output" $src $lib; then
      echo "$OUTPUT_NAME compiled in $(StopTimer) seconds."
    fi
  else
    echo "Do not know how to compile on this OS"
    echo "  OS: $(uname)"
    exit 1
  fi
fi

if [ $IsTestsEnabled -eq 1 ]; then
  . "$ProjectRoot/test/build.sh"
fi

Log "================================================================"
Log "Finished at $(date '+%Y-%m-%d %H:%M:%S')"

if [ ! -e tags ] && [ $IsBuildDebug -eq 1 ]; then
  src=""
  if [ "$HAS_SDL3" -eq 0 ]; then
    for file in "$OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/*.h"; do
      src="$src $file"
    done
  else
    for file in /usr/include/SDL3/*.h; do
      src="$src $file"
    done
  fi

  for file in vk_platform vulkan_core vulkan_wayland; do
    src="$src /usr/include/vulkan/$file.h"
  done

  ctags --fields=+iaS --extras=+q --c-kinds=+pf $src
  echo tags are generated
fi
