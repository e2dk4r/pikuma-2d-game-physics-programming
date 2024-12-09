# vi: set et ft=sh ts=2 sw=2 fenc=utf-8 :vi
outputDir="$OutputDir/3rdparty"
if [ ! -d "$outputDir" ]; then
  mkdir "$outputDir"
fi

# To see the available tags, execute below command:
# $ curl -sH 'accept: application/vnd.github+json' https://api.github.com/repos/libsdl-org/SDL/releases | jq -r '.[].tag_name'
LIBSDL_TAG=preview-3.1.6
LIBSDL_PREFIX=${LIBSDL_TAG%%-*}
LIBSDL_VERSION=${LIBSDL_TAG##*-}

################################################################
# DOWNLOAD
################################################################
LIBSDL_FILEBASENAME="SDL${LIBSDL_VERSION%%.*}-$LIBSDL_VERSION"
LIBSDL_URL="https://github.com/libsdl-org/SDL/releases/download/$LIBSDL_PREFIX-$LIBSDL_VERSION/$LIBSDL_FILEBASENAME.tar.gz"
LIBSDL_FILE="$outputDir/$LIBSDL_FILEBASENAME.tar.gz"
LIBSDL_HASH_B2="6fa3182ab933bad2f3f939acddf82cddc9cc059debcc8c04e546639932f5cc3152d4a6a656ad24848c90b7827a49e19b518d98d14af50b1b914f9f2e91d8441b"
Log "3rdparty/SDL"
Log "- tag:     $LIBSDL_TAG"
Log "- version: $LIBSDL_VERSION"
Log "- url:     $LIBSDL_URL"
Log "- file:    $LIBSDL_FILE"
Log "- b2sum:   $LIBSDL_HASH"

LIBSDL_HASH_OK=$(HashCheckB2 "$LIBSDL_FILE" "$LIBSDL_HASH_B2")
if [ $LIBSDL_HASH_OK -eq 0 ]; then
  StartTimer
  echo Downloading SDL $LIBSDL_VERSION archive
  Download "$LIBSDL_URL" "$LIBSDL_FILE"
  elapsed=$(StopTimer)
  Log "  downloaded in $elapsed seconds"

  LIBSDL_HASH_OK=$(HashCheckB2 "$LIBSDL_FILE" "$LIBSDL_HASH_B2")
  if [ $LIBSDL_HASH_OK -eq 0 ]; then
    echo "3rdparty/SDL hash check failed"
    Log "  hash check failed"
    exit 1
  fi
  
#else
  # sdl source archive already downloaded
fi

################################################################
# EXTRACT
################################################################
LIBSDL_DIR="$outputDir/$LIBSDL_FILEBASENAME"
if [ ! -e "$LIBSDL_DIR" ]; then
  tar --cd "$outputDir" --extract --file "$LIBSDL_FILE"
fi

################################################################
# BUILD
################################################################
if [ ! -e "$LIBSDL_DIR-install/lib64/libSDL${LIBSDL_VERSION%%.*}.so" ]; then
  cmake -G Ninja -S "$LIBSDL_DIR" -B "$LIBSDL_DIR/build" -D CMAKE_INSTALL_PREFIX="$LIBSDL_DIR-install" -D CMAKE_BUILD_TYPE=Release \
    -D SDL_DUMMYAUDIO=OFF \
    -D SDL_DUMMYVIDEO=OFF \
    -D SDL_IBUS=OFF \
    -D SDL_PIPEWIRE=ON \
    -D SDL_PULSEAUDIO=OFF \
    -D SDL_RPATH=OFF \
    -D SDL_X11=OFF \
    -D SDL_WAYLAND=ON \
    -D SDL_KMSDRM=OFF \
    -D SDL_OFFSCREEN=OFF \
    -D SDL_TEST_LIBRARY=OFF \
    -D SDL_SHARED=ON \
    -D SDL_STATIC=OFF

  ninja -C "$LIBSDL_DIR/build" install -j8
  elapsed=$(StopTimer)
  Log "- built in $elapsed seconds"
fi

if [ ! -e "$OutputDir/libSDL${LIBSDL_VERSION%%.*}.so" ]; then
  cp "$LIBSDL_DIR-install/lib64/libSDL${LIBSDL_VERSION%%.*}.so.0" "$OutputDir/libSDL${LIBSDL_VERSION%%.*}.so.0" 
fi

export INC_LIBSDL="-I$LIBSDL_DIR-install/include"
export LIB_LIBSDL="-L$LIBSDL_DIR-install/lib64 -lSDL3"
export PKG_CONFIG_LIBDIR="$PKG_CONFIG_LIBDIR:$LIBSDL_DIR-install/pkgconfig"
