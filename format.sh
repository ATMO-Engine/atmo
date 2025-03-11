if command -v clang-format &> /dev/null; then
  find . -type f -name "*.cpp" | grep -e "src" | xargs -n1 "clang-format" "-i"
  find . -type f -name "*.hpp" | grep -e "src" | xargs -n1 "clang-format" "-i"
else
  echo "clang-format is not installed. Please install it to format the code."
fi
