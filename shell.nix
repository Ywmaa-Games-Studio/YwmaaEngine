# This nix file was made for Nix OS because I use it as my main, but it can be a guide for the required dependecies.
{ pkgs ? (import <nixpkgs> {}) }:
with pkgs;
  mkShell {
  packages = [
    glfw
    freetype
    vulkan-headers
    vulkan-loader
    vulkan-validation-layers
    vulkan-tools        # vulkaninfo
    shaderc             # GLSL to SPIRV compiler - glslc
    #renderdoc           # Graphics debugger
    #tracy               # Graphics profiler
    vulkan-tools-lunarg # vkconfig
    gdb # Debugger for vs code
  ];

  buildInputs = with pkgs; [
    glfw
    freetype
    clang
    clang-tools

    libxkbcommon.dev
    xorg.libX11.dev
    xorg.libxcb.dev
  ];

  LD_LIBRARY_PATH="${glfw}/lib:${freetype}/lib:${vulkan-loader}/lib:${vulkan-validation-layers}/lib";
  VULKAN_SDK = "${vulkan-headers}";
  VK_LAYER_PATH = "${vulkan-validation-layers}/share/vulkan/explicit_layer.d";
}

