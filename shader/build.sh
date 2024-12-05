pwd="$ProjectRoot/shader"
outputDir="$OutputDir/shader"
if [ ! -e "$outputDir" ]; then
  mkdir "$outputDir"
fi

export ShaderInc="-I$outputDir"

# ShaderCompile(src, output)
ShaderCompile() {
  src="$1"
  output="$2"
  glslc -O "$src" -o "$output"
  glslang --quiet -Os -V -x -o "$output.h" "$src"
}

ShaderCompile "$pwd/shader.vert" "$outputDir/vert.spv"
ShaderCompile "$pwd/shader.frag" "$outputDir/frag.spv"
