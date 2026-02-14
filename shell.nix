{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = [
    # Toolchain
    pkgs.clang
    pkgs.cmake
    pkgs.ninja
    pkgs.pkg-config

    # OpenGL / Mesa
    pkgs.mesa
    pkgs.libGL
    pkgs.libglvnd

    # Windowing + loading
    pkgs.glfw      # Default GLFW (X11)
    pkgs.glew
    pkgs.glm

    # Shell
    pkgs.fish
  ];

  shellHook = ''
    exec fish
  '';
}
