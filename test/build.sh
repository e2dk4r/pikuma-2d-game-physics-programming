# vi: set et ft=sh ts=2 sw=2 fenc=utf-8 :vi
################################################################
# TEST FUNCTIONS
################################################################

failedTestCount=0

# void RunTest(testExecutable, failMessage)
RunTest() {
  executable="$1"
  failMessage="$2"

  "$executable"
  statusCode=$?
  if [ $statusCode -ne 0 ]; then
    echo "$failMessage code $statusCode"
    failedTestCount=$(( $failedTestCount + 1 ))
  fi
}

################################################################

pwd="$ProjectRoot/test"
outputDir="$OutputDir/test"
if [ ! -e "$outputDir" ]; then
  mkdir "$outputDir"
fi

LIB_M='-lm'

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

### linked_list_test
inc="-I$ProjectRoot/include"
src="$pwd/linked_list_test.c"
output="$outputDir/$(BasenameWithoutExtension "$src")"
"$cc" $cflags $ldflags $inc -o "$output" $src
RunTest "$output" "TEST linked list failed."

### hash_table_list_test
inc="-I$ProjectRoot/include"
src="$pwd/hash_table_list_test.c"
output="$outputDir/$(BasenameWithoutExtension "$src")"
"$cc" $cflags $ldflags $inc -o "$output" $src
RunTest "$output" "TEST hash table list failed."

### doubly_linked_list
inc="-I$ProjectRoot/include"
src="$pwd/doubly_linked_list_test.c"
output="$outputDir/$(BasenameWithoutExtension "$src")"
"$cc" $cflags $ldflags $inc -o "$output" $src
RunTest "$output" "TEST doubly linked list failed."

if [ $failedTestCount -ne 0 ]; then
  echo $failedTestCount tests failed.
  exit 1
fi
