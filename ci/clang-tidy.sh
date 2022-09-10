build_dir=$1
find test -name "*.cpp" -or -name "*.h" \
    | xargs clang-tidy -p $build_dir --export-fixes=$build_dir/clang-fixes.yaml
