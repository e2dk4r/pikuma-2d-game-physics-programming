# vi: set et ft=sh ts=2 sw=2 fenc=utf-8 :vi
################################################################
# TEST FUNCTIONS
################################################################

# void RunTest(testExecutable, failMessage)
RunTest() {
  executable="$1"
  failMessage="$2"

  "$executable"
  statusCode=$?
  if [ $statusCode -ne 0 ]; then
    echo "$failMessage code $statusCode"
    exit $statusCode
  fi
}

################################################################

pwd="$ProjectRoot/test"
outputDir="$OutputDir/test"
if [ ! -e "$outputDir" ]; then
  mkdir "$outputDir"
fi

### memory_test
inc="-I$ProjectRoot/include"
src="$pwd/memory_test.c"
output="$outputDir/$(BasenameWithoutExtension "$src")"
lib="$LIB_M"
"$cc" $cflags $ldflags $inc -o "$output" $src $lib
RunTest "$output" "TEST memory failed."

### text_test
inc="-I$ProjectRoot/include"
src="$pwd/text_test.c"
output="$outputDir/$(BasenameWithoutExtension "$src")"
lib="$LIB_M"
"$cc" $cflags $ldflags $inc -o "$output" $src $lib
RunTest "$output" "TEST text failed."

### string_builder
inc="-I$ProjectRoot/include"
src="$pwd/string_builder_test.c"
output="$outputDir/$(BasenameWithoutExtension "$src")"
lib="$LIB_M"
"$cc" $cflags $ldflags $inc -o "$output" $src $lib
RunTest "$output" "TEST string_builder failed."

### math_test
inc="-I$ProjectRoot/include"
src="$pwd/math_test.c"
output="$outputDir/$(BasenameWithoutExtension "$src")"
lib="$LIB_M"
"$cc" $cflags $ldflags $inc -o "$output" $src $lib
RunTest "$output" "TEST math failed."

### teju_test
inc="-I$ProjectRoot/include"
src="$pwd/teju_test.c"
output="$outputDir/$(BasenameWithoutExtension "$src")"
lib="$LIB_M"
"$cc" $cflags $ldflags $inc -o "$output" $src $lib
RunTest "$output" "TEST teju failed."
