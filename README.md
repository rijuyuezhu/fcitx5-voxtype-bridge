# fcitx5-voxtype-bridge

This is a fcitx5 addon that bridges [my fork of voxtype](https://github.com/rijuyuezhu/voxtype), providing a better experience when using voxtype. It now supports functionalities including voice input, voice edition.

See [this post](https://forum.nju-aia.lsamc.website/t/topic/136) and [this blog](https://blog.rijuyuezhu.top/posts/efe0c0d6) for more information.


## Build & Install

### For Arch Linux Users

Try [This PKGBUILD](https://github.com/rijuyuezhu/fcitx5-voxtype-bridge.pkg):

```bash
git clone https://github.com/rijuyuezhu/fcitx5-voxtype-bridge.pkg.git
cd fcitx5-voxtype-bridge.pkg
paru -Bi .
```

### For Other Linux Users


To install it locally:

```bash
git clone https://github.com/rijuyuezhu/fcitx5-voxtype-bridge.git
cd fcitx5-voxtype-bridge
cmake -DCMAKE_INSTALL_PREFIX=$(realpath ~/.local) -DCMAKE_INSTALL_LIBDIR=$(realpath ~/.local/lib) -B build
cmake --build build -j$(nproc) --target install
```

To install it system-wide:

```bash
git clone https://github.com/rijuyuezhu/fcitx5-voxtype-bridge.git
cd fcitx5-voxtype-bridge
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_LIBDIR=/usr/lib -B build
cmake --build build -j$(nproc) --target install
```