#!/bin/bash
# macOS対応ビルドスクリプト
# 環境: Apple Clang with C++17 support

echo "=== GDSII Analysis - macOS Build Script ==="
echo

# コンパイラ設定
CXX="clang++"
CXXFLAGS="-std=c++17 -fPIC -Wno-deprecated-declarations"

# ステップのリスト
steps=(
  "step1"
  "step2"
  "step3"
  "step5"
  "step6"
)

# ビルド
echo "Building with $CXX..."
for step in "${steps[@]}"; do
  echo "  Compiling $step.cpp..."
  $CXX $CXXFLAGS -o $step $step.cpp
  if [ $? -ne 0 ]; then
    echo "  ERROR: Failed to compile $step.cpp"
    exit 1
  fi
done

echo
echo "Build completed successfully!"
echo
echo "Running Step 6 (complete system)..."
echo

./step6_fixed

echo
echo "Build complete! You can also run individual steps:"
for step in "${steps[@]}"; do
  echo "  ./$step"
done