clangformat=${CLANGFORMAT:-clang-format} # override for specific version

find include test -name "*.cpp" -or -name "*.h" \
    | xargs ${clangformat} -n -Werror
