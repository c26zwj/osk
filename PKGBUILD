pkgname=orbit-osk
pkgver=0.1.0
pkgrel=23
pkgdesc='Touch-friendly on-screen keyboard for KDE Plasma Wayland'
arch=('x86_64')
license=('MIT')
depends=('qt6-base' 'layer-shell-qt' 'wayland')
makedepends=('cmake' 'layer-shell-qt' 'wayland')
build() {
  cmake -S "$startdir" -B "$srcdir/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
  cmake --build "$srcdir/build"
}

package() {
  DESTDIR="$pkgdir" cmake --install "$srcdir/build"
}
