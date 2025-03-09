# vi: set et ft=sh ts=2 sw=2 fenc=utf-8 :vi
HAS_SDL3=0

if [ "$IsPlatformWindows" -eq 0 ]; then
  if [ $(IsCommandExists sdl3-config) -eq 1 ]; then
    HAS_SDL3=1
  elif [ $(IsCommandExists pkg-config) -eq 1 ] && pkg-config --exists sdl3; then
    HAS_SDL3=1
  fi
fi

if [ "$FORCE_BUILD_SDL3" -eq 1 ]; then
  HAS_SDL3=0
fi

if [ $HAS_SDL3 -eq 1 ]; then
  # If system already has sdl3 library installed
  if [ $(IsCommandExists sdl3-config) -eq 1 ]; then
    LIBSDL_VERSION=$(sdl3-config --version)
    INC_LIBSDL="$(sdl3-config --cflags)"
    LIB_LIBSDL="$(sdl3-config --libs)"
  elif [ $(IsCommandExists pkg-config) -eq 1 ]; then
    LIBSDL_VERSION=$(pkg-config --modversion sdl3)
    INC_LIBSDL="$(pkg-config --cflags sdl3)"
    LIB_LIBSDL="$(pkg-config --libs sdl3)"
  else
    echo Error: Detected to have sdl3 installed but could not figure out where.
    exit 1
  fi

  Log "3rdparty/SDL"
  Log "- using system"
  Log "- version: $LIBSDL_VERSION"
else
  # TODO: Extract sdl3 for Windows
  
  # If system do not have sdl3 library installed
  outputDir="$OutputDir/3rdparty"
  if [ ! -d "$outputDir" ]; then
    mkdir "$outputDir"
  fi

  # To see the available tags, execute below command:
  # $ curl -sH 'accept: application/vnd.github+json' https://api.github.com/repos/libsdl-org/SDL/releases | jq -r '.[].tag_name'
  LIBSDL_TAG=release-3.2.8
  LIBSDL_PREFIX=${LIBSDL_TAG%%-*}
  LIBSDL_VERSION=${LIBSDL_TAG##*-}

  ################################################################
  # DOWNLOAD
  ################################################################
  LIBSDL_FILEBASENAME="SDL${LIBSDL_VERSION%%.*}-$LIBSDL_VERSION"
  LIBSDL_URL="https://github.com/libsdl-org/SDL/releases/download/$LIBSDL_PREFIX-$LIBSDL_VERSION/$LIBSDL_FILEBASENAME.tar.gz"
  LIBSDL_FILE="$outputDir/$LIBSDL_FILEBASENAME.tar.gz"
  LIBSDL_HASH_B2="e89f240f9643305fb191049b9162f8adfe2f59b07464df28d08437130f8724b8ac8ab22075a6548cb6d8203ec289f3a8a917c08697047f3d39377af267d8187b"

  Log "3rdparty/SDL"
  Log "- tag:     $LIBSDL_TAG"
  Log "- version: $LIBSDL_VERSION"
  Log "- url:     $LIBSDL_URL"
  Log "- file:    $LIBSDL_FILE"
  Log "- b2sum:   $LIBSDL_HASH_B2"

  LIBSDL_HASH_OK=$(HashCheckB2 "$LIBSDL_FILE" "$LIBSDL_HASH_B2")
  if [ "$LIBSDL_HASH_OK" -eq 0 ]; then
    StartTimer
    echo Downloading SDL "$LIBSDL_VERSION" archive
    Download "$LIBSDL_URL" "$LIBSDL_FILE"
    elapsed=$(StopTimer)
    Log "  downloaded in $elapsed seconds"

    LIBSDL_HASH_OK=$(HashCheckB2 "$LIBSDL_FILE" "$LIBSDL_HASH_B2")
    if [ "$LIBSDL_HASH_OK" -eq 0 ]; then
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
    StartTimer
    cmake -G Ninja -S "$LIBSDL_DIR" -B "$LIBSDL_DIR/build" -D CMAKE_INSTALL_PREFIX="$LIBSDL_DIR-install" \
      -D CMAKE_BUILD_TYPE=Release \
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

    target=install
    if [ "$IsBuildDebug" -eq 0 ]; then
      target=install/strip
    fi
    ninja -C "$LIBSDL_DIR/build" $target -j8
    elapsed=$(StopTimer)
    Log "- built in $elapsed seconds"
  fi

  INC_LIBSDL="-I$LIBSDL_DIR-install/include"
  LIB_LIBSDL="-L$LIBSDL_DIR-install/lib64 -lSDL3"
  export PKG_CONFIG_LIBDIR="$PKG_CONFIG_LIBDIR:$LIBSDL_DIR-install/pkgconfig"
fi
