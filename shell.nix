{ pkgs ? import <nixpkgs> {}}:
let
in
pkgs.mkShell {
  packages = with pkgs; [
    stdenv
    cmake
    ut
    asio
    pkg-config
  ];
}
