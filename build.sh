#!/bin/sh
# vi: set et ft=sh ts=2 sw=2 fenc=utf-8 :vi
export LC_ALL=C
export TZ=UTC

IsBuildDebug=1
IsBuildEnabled=1
IsTestsEnabled=1

PROJECT_NAME=boilerplate-sdl3
OUTPUT_NAME=$PROJECT_NAME

usage() {
  cat <<EOF
  NAME
    build.sh [OPTIONS]
  
  DESCRIPTION
    Build script of $PROJECT_NAME.
  
  OPTIONS
    --debug
      Build with debugging information.

    --build-directory=path
      Build executables in this folder. If directory not exists, one will be
      created.

    --disable-$PROJECT_NAME
      Do not build $PROJECT_NAME binary.

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

for i in "$@"; do
  case $i in
    --debug)
      IsBuildDebug=1
      ;;
    --build-directory=*)
      OutputDir="${i#*=}"
      ;;
    --disable-handmadehero)
      IsBuildEnabled=0
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

# string Basename(path)
Basename() {
  path="$1"
  echo "${path##*/}"
}

# string BasenameWithoutExtension(path)
BasenameWithoutExtension() {
  path="$1"
  basename="$(Basename "$path")"
  echo ${basename%.*}
}

# string Dirname(path)
Dirname() {
  path="$1"
  if [ $(StringStartsWith "$path" '/') ]; then
    dirname="${path%/*}"
    if [ -z "$dirname" ]; then
      echo '/'
    else
      echo $dirname
    fi
  else
    echo '.'
  fi
}

################################################################
# Network FUNCTIONS
################################################################
HAS_CURL=$(curl --version >/dev/null 2>&1 && echo 1 || echo 0)
HAS_WGET=$(wget --version >/dev/null 2>&1 && echo 1 || echo 0)

# Download(string url, string output)
Download() {
  if [ $HAS_CURL -eq 0 ] && [ $HASH_WGET -eq 0 ]; then
    echo "curl or wget requried"
    exit 1
  fi
  url="$1"
  output="$2"

  if [ $HAS_CURL -eq 1 ]; then
    curl --location --output "$output" "$url"
  elif [ $HAS_WGET -eq 1 ]; then
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
  if [ $HAS_B2SUM -eq 0 ]; then
    echo "b2sum is required"
    exit 1
  fi
  path="$1"
  hash="$2"
  echo "$hash $path" | b2sum --check --quiet >/dev/null 2>&1 && echo 1 || echo 0
}

# HashCheckSHA1(string path, string hash) -> bool
HashCheckSHA1() {
  if [ $HAS_SHA1SUM -eq 0 ]; then
    echo "sha1sum is required"
    exit 1
  fi
  path="$1"
  hash="$2"
  echo "$hash $path" | sha1sum --check --quiet >/dev/null 2>&1 && echo 1 || echo 0
}

# HashCheckSHA256(string path, string hash) -> bool
HashCheckSHA256() {
  if [ $HAS_SHA256SUM -eq 0 ]; then
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
  echo $(( $(date +%s) - $startedAt ))
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

ProjectRoot="$(Dirname $(realpath "$0"))"
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
  echo '**/*' > "$OutputDir/.hgignore"
fi

IsOSLinux=$(StringEndsWith "$(uname)" 'Linux')

cc="${CC:-clang}"
IsCompilerGCC=$(StringStartsWith "$("$cc" --version | head -n 1 -c 32)" "gcc")
IsCompilerClang=$(StringStartsWith "$("$cc" --version | head -n 1 -c 32)" "clang")
if [ $IsCompilerGCC -eq 0 ] && [ $IsCompilerClang -eq 0 ]; then
  echo "unsupported compiler $cc. continue (y/n)?"
  read input
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
cflags="$cflags -O3"
if [ $(StringContains "$cflags" '-march=') -eq 0 ]; then
  cflags="$cflags -march=x86-64-v3"
fi
cflags="$cflags -funroll-loops"
cflags="$cflags -fomit-frame-pointer"
# warnings
cflags="$cflags -Wall -Werror"
cflags="$cflags -Wconversion"
cflags="$cflags -Wno-unused-parameter"
cflags="$cflags -Wno-unused-result"
cflags="$cflags -Wno-missing-braces"
cflags="$cflags -Wno-unused-function"

cflags="$cflags -DCOMPILER_GCC=$IsCompilerGCC"
cflags="$cflags -DCOMPILER_CLANG=$IsCompilerClang"

cflags="$cflags -DIS_BUILD_DEBUG=$IsBuildDebug"
if [ $IsBuildDebug -eq 1 ]; then
  cflags="$cflags -g -O0"
  cflags="$cflags -Wno-unused-but-set-variable"
  cflags="$cflags -Wno-unused-variable"
fi

if [ $IsOSLinux -eq 1 ]; then
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
if [ ! -z "$CFLAGS" ]; then
  Log "from your env: $CFLAGS"
fi

Log "ldflags: $ldflags"
if [ ! -z "$LDFLAGS" ]; then
  Log "from your env: $LDFLAGS"
fi
Log "================================================================"

LIB_M='-lm'

if [ $IsBuildEnabled -eq 1 ]; then
  if [ $IsOSLinux -eq 0 ]; then
    echo "Do not know how to compile on this OS"
    echo "  OS: $(uname)"
    exit 1
  elif [ $IsOSLinux -eq 1 ]; then
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
    . $ProjectRoot/3rdparty/SDL3/build.sh

    ### <PROJECT_NAME>
    # ShaderInc=
    #. $ProjectRoot/shader/build.sh

    src=""
    src="$src $ProjectRoot/src/main.c"
    src="${src# }"

    output="$OutputDir/$OUTPUT_NAME"
    inc="-I$ProjectRoot/include $INC_LIBSDL"
    lib="$LIB_LIBSDL $LIB_M"
    StartTimer
    "$cc" $cflags $ldflags $inc -o "$output" $src $lib
    [ $? -eq 0 ] && echo "$OUTPUT_NAME compiled in $(StopTimer) seconds."
  fi
fi

if [ $IsTestsEnabled -eq 1 ]; then
  . "$ProjectRoot/test/build.sh"
fi

Log "================================================================"
Log "Finished at $(date '+%Y-%m-%d %H:%M:%S')"

if [ ! -e tags ] && [ $IsBuildDebug -ne 0 ]; then
  src=""
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_assert.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_atomic.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_audio.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_begin_code.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_bits.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_blendmode.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_camera.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_clipboard.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_close_code.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_copying.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_cpuinfo.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_dialog.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_egl.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_endian.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_error.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_events.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_filesystem.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_gamepad.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_gpu.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_guid.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_haptic.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_hidapi.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_hints.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_init.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_intrin.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_iostream.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_joystick.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_keyboard.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_keycode.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_loadso.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_locale.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_log.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_main.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_main_impl.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_messagebox.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_metal.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_misc.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_mouse.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_mutex.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_oldnames.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_opengles2_gl2ext.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_opengles2_gl2.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_opengles2_gl2platform.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_opengles2.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_opengles2_khrplatform.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_opengles.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_opengl_glext.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_opengl.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_pen.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_pixels.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_platform_defines.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_platform.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_power.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_process.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_properties.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_rect.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_render.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_revision.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_scancode.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_sensor.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_stdinc.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_storage.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_surface.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_system.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_test_assert.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_test_common.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_test_compare.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_test_crc32.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_test_font.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_test_fuzzer.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_test.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_test_harness.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_test_log.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_test_md5.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_test_memory.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_thread.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_time.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_timer.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_touch.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_version.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_video.h"
  src="$src $OutputDir/3rdparty/SDL3-$LIBSDL_VERSION-install/include/SDL3/SDL_vulkan.h"

  src="$src /usr/include/vulkan/vk_platform.h"
  src="$src /usr/include/vulkan/vulkan_core.h"
  src="$src /usr/include/vulkan/vulkan_wayland"

  ctags --fields=+iaS --extras=+q --c-kinds=+pf $src
fi
