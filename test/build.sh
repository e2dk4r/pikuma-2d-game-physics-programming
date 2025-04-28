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

### physics_test
inc="-I$ProjectRoot/include -I$ProjectRoot/src"
src="$pwd/physics_test.c"
output="$outputDir/$(BasenameWithoutExtension "$src")"
lib="$LIB_M"
"$cc" $cflags $ldflags $inc -o "$output" $src $lib
RunTest "$output" "TEST physics failed."

if [ $failedTestCount -ne 0 ]; then
  echo $failedTestCount tests failed.
  exit 1
fi
