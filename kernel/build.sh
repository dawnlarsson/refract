#!/bin/sh
# kernel/build.sh — download, verify, extract, and build a Linux kernel
set -eu

dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cache=$dir/cache
gnupg=$cache/gnupg
tree=$dir/linux
config=$dir/base.config
default_flags='-fno-safety -fno-zeroinit -fno-unwind-tables -fno-asynchronous-unwind-tables'

die() { printf 'build.sh: %s\n' "$*" >&2; exit 1; }

case "${1:-}" in
"") # no argument means do everything
	"$0" setup
	"$0" init
	"$0" build
	;;
setup)
	mkdir -p "$cache"
	v=$(curl -fsSL https://www.kernel.org/releases.json |
		tr -d '\n' |
		sed -n 's/.*"latest_stable"[[:space:]]*:[[:space:]]*{[[:space:]]*"version"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p')
	[ -n "$v" ] || die "failed to parse version from kernel.org"
	stamp=$cache/latest.version
	[ -f "$stamp" ] && [ "$(cat "$stamp")" = "$v" ] && { printf 'build.sh: already cached %s\n' "$v"; exit 0; }

	base=https://cdn.kernel.org/pub/linux/kernel/v${v%%.*}.x
	tarball=$cache/linux-$v.tar.xz
	sig=$cache/linux-$v.tar.sign
	[ -f "$tarball" ] || curl -fL -o "$tarball" "$base/linux-$v.tar.xz"
	[ -f "$sig" ]     || curl -fL -o "$sig"     "$base/linux-$v.tar.sign"

	mkdir -p "$gnupg" && chmod 700 "$gnupg"
	GNUPGHOME=$gnupg gpg --batch -q --list-keys gregkh@kernel.org >/dev/null 2>&1 ||
	GNUPGHOME=$gnupg gpg --batch -q --auto-key-locate clear,wkd,nodefault --locate-keys \
		torvalds@kernel.org gregkh@kernel.org sashal@kernel.org benh@kernel.org >/dev/null 2>&1 ||
		die "failed to import signing keys"

	xz -cd "$tarball" | GNUPGHOME=$gnupg gpg --batch --verify "$sig" - >/dev/null 2>&1 ||
		die "signature verification failed"
	printf '%s\n' "$v" > "$stamp"
	printf 'build.sh: cached and verified %s\n' "$v"
	;;

init)
	stamp=$cache/latest.version
	[ -f "$stamp" ] || die "run setup first"
	v=$(cat "$stamp")
	tarball=$cache/linux-$v.tar.xz
	[ -f "$tarball" ] || die "run setup first"

	rm -rf "$tree"
	tar -C "$dir" -xf "$tarball"
	mv "$dir/linux-$v" "$tree"
	if [ -d "$dir/patches" ]; then
		for p in "$dir/patches"/*.sh; do
			[ -f "$p" ] || continue
			printf 'build.sh: applying %s\n' "$(basename "$p")"
			(cd "$tree" && sh "$p")
		done
	fi
	printf '%s\n' "$v" > "$tree/.kernel-version"
	git -C "$tree" init -q
	git -C "$tree" add -A
	git -C "$tree" -c user.name=kernel-bootstrap -c user.email=kernel-bootstrap@local commit -q -m "linux $v"
	printf 'build.sh: ready at %s\n' "$tree"
	;;

build)
	[ -d "$tree/.git" ] || die "run init first"
	[ -f "$config" ]    || die "missing $config"
	arch=${ARCH:-$(uname -m)}
	jobs=${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)}
	__cc=${CC:-prism}
	__kcflags=${KCFLAGS:-$default_flags}
	case "$arch" in
		x86_64|amd64)  arch=x86_64; target=bzImage; image=arch/x86/boot/bzImage ;;
		aarch64|arm64) arch=arm64;   target=Image;   image=arch/arm64/boot/Image ;;
		riscv64|riscv) arch=riscv;   target=Image;   image=arch/riscv/boot/Image ;;
		i?86|x86)      arch=x86;     target=bzImage; image=arch/x86/boot/bzImage ;;
		*)             target=vmlinux; image=vmlinux ;;
	esac
	git -C "$tree" reset --hard HEAD >/dev/null
	git -C "$tree" clean -fdx >/dev/null
	make -C "$tree" CC="$__cc" ARCH="$arch" allnoconfig KCONFIG_ALLCONFIG="$config"
	make -C "$tree" CC="$__cc" ARCH="$arch" KCFLAGS="$__kcflags" -j"$jobs" "$target"
	cp "$tree/$image" "$dir/kernel"
	size=$(wc -c < "$dir/kernel")
	printf '\nkernel ready: %s/kernel (%.2f MB)\n\n' "$dir" "$(echo "$size / 1048576" | bc -l)"
	;;

run)
	[ -d "$tree/.git" ] || die "run init first"
	if [ ! -f "$dir/kernel" ] || [ "$config" -nt "$dir/kernel" ] || \
	   [ -n "$(git -C "$tree" diff --name-only HEAD 2>/dev/null)" ]; then
		"$0" build
	fi
	printf 'build.sh: starting qemu\n'
	qemu-system-x86_64 \
		-kernel "$dir/kernel" \
		-append "console=tty0 console=ttyS0 earlyprintk=serial" \
		-serial mon:stdio \
		-m 256 \
		-no-reboot
	;;

-h|--help|help)
	printf 'usage: %s [setup|init|build|run]\n' "$(basename "$0")"
	;;

*)
	die "usage: $(basename "$0") [setup|init|build|run]"
	;;
esac