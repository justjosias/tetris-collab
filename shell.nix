{ pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    nativeBuildInputs = [
      pkgs.SDL2
      pkgs.SDL2_ttf
      pkgs.SDL2_mixer
      pkgs.pkg-config
    ];
}

