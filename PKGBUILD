# Maintainer: Jesse McClure AKA "Trilby" <jmcclure [at] cns [dot] umass [dot] edu>
_gitname="blueplate"
pkgname="${_gitname}-git"
pkgver=0.2.65c0b40
pkgrel=1
pkgdesc="System tray client indicators (mail, workspaces, ...)"
url="https://github.com/TrilbyWhite/Blueplate"
arch=('x86_64' 'i686')
license=('GPL3')
depends=('libx11')
makedepends=('git')
source=("${_gitname}::git://github.com/TrilbyWhite/Blueplate.git")
sha256sums=('SKIP')

pkgver() {
	cd "${_gitname}";
	echo "0.$(git rev-list --count HEAD).$(git describe --always )"
}

build() {
	cd "${_gitname}"
	make
}

package() {
	cd "${_gitname}"
	make PREFIX=/usr DESTDIR="${pkgdir}" install
}
