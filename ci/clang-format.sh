find include test -name "*.cpp" -or -name "*.h" \
    | xargs clang-format -n -Werror
