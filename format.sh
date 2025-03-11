if command -v clang-format &> /dev/null; then
  find "src" -type f -name "*.cpp" | xargs -n1 "clang-format" "-i"
  find "src" -type f -name "*.hpp" | xargs -n1 "clang-format" "-i"
else
  echo "clang-format is not installed. Please install it to format the code."
fi
