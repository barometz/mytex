build_dir=$1 # to find compile_commands.json and write clang-fixes.yaml
clangtidy=${CLANGTIDY:-clang-tidy} # override for specific version

find test -name "*.cpp" -or -name "*.h" \
    | xargs $clangtidy -p $build_dir --export-fixes=$build_dir/clang-fixes.yaml
