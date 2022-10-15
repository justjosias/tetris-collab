{ pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    nativeBuildInputs = [
      pkgs.SDL2
      pkgs.SDL2_ttf
      pkgs.SDL2_mixer
      pkgs.SDL2_image
      pkgs.pkg-config
    ];
}

