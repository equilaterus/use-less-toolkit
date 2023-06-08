# Use-Less-Toolkit for Linux

## Quick start

### Fedora

1. Download custom fonts into **fonts/** directory:
   * [fa-brands-400](https://github.com/FortAwesome/Font-Awesome/blob/6.x/webfonts/fa-brands-400.ttf)
   * [fa-regular-400](https://github.com/FortAwesome/Font-Awesome/blob/6.x/webfonts/fa-regular-400.ttf)
   * [fa-solid-900](https://github.com/FortAwesome/Font-Awesome/blob/6.x/webfonts/fa-solid-900.ttf)

2. Install these packages:
```bash
sudo dnf install clang
sudo dnf install freetype-devel
sudo dnf install glfw-devel
```

3. Install **clangd** extension for VSCode (or VSCodium).

4. Build with **Ctrl + Shift + B** and debug with **F5**.