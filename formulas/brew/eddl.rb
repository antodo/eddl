# Homebrew Formula for EDDL
# Usage: brew install eddl

class Eddl < Formula
  desc "European Distributed Deep Learning Library (EDDL)"
  homepage "https://github.com/deephealthproject/eddl"
  url "https://github.com/deephealthproject/eddl/archive/v0.5.4a.tar.gz"
  sha256 "50acb25976f14e6caf5535b82fc760dd1a0f7e0b81db83906b62eddd8eba0db7"

  depends_on "cmake" => :build
  depends_on "eigen" => :build
  depends_on "zlib" => :build
  depends_on "protobuf" => :build
  depends_on "graphviz" => :build
  depends_on "wget" => :build

  def install
    mkdir "build" do
      system "cmake", "..", "-DBUILD_SUPERBUILD=OFF -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF", *std_cmake_args
      system "make", "install", "PREFIX=#{prefix}"
    end
  end

  test do
    (testpath/"CMakeLists.txt").write <<~EOS
      cmake_minimum_required(VERSION 3.9.2)
      project(test)

      set(CMAKE_CXX_STANDARD 11)

      add_executable(test example.cpp)

      find_package(EDDL REQUIRED)
      target_link_libraries(test PUBLIC EDDL::eddl)
    EOS

    (testpath/"test.cpp").write <<~EOS
      #include <iostream>
      #include <eddl/tensor/tensor.h>
      int main(){
        Tensor *t1 = Tensor::ones({5, 5});
        std::cout << t1->sum() << std::endl;
      }
    EOS

    system "cmake", "."
    system "make"
    assert_equal "25", shell_output("./test").strip
  end
end