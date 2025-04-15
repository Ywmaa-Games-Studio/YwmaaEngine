# This nix file was made for Nix OS because I use it as my main, but it can be a guide for the required dependecies.
let
  unstable = import (fetchTarball https://nixos.org/channels/nixos-unstable/nixexprs.tar.xz) { };
in
{ pkgs ? (import <nixpkgs> {}) }:
with pkgs;
  mkShell {
  packages = [ #These are only specific to NixOS because it doesn't provide it at runtime
    #X11
    #xorg.libX11
    #xorg.libxcb
    #libxkbcommon

    #Wayland
    #wayland #libwayland-client
    #wayland-scanner
    #libdecor

    #Vulkan
    vulkan-loader
    vulkan-validation-layers
    vulkan-tools        # vulkaninfo

    renderdoc           # Graphics debugger
    #tracy               # Graphics profiler
    vulkan-tools-lunarg # vkconfig
    gdb # Debugger for vs code

    clang-tools
  ];

  buildInputs = with pkgs; [ # Required packages for build
    unstable.zig

    # For Android, only required when building for Android
    jdk17
    unstable.sdkmanager
  ];
  
  #These are only specific to NixOS because it doesn't provide it at runtime
  LD_LIBRARY_PATH = "${pkgs.wayland}/lib:${pkgs.xorg.libX11}/lib:${pkgs.xorg.libxcb}/lib:${vulkan-loader}/lib:${vulkan-validation-layers}/lib";
}

