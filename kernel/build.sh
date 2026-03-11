#!/bin/sh
# kernel/build.sh — download, verify, extract, and build a Linux kernel
set -eu

dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(dirname "$dir")
cache=$dir/cache
gnupg=$cache/gnupg
tree=$dir/linux
config=$dir/base.config
image_name=refract/build
default_flags='-fno-safety -fno-zeroinit -fno-unwind-tables -fno-asynchronous-unwind-tables'

die() { printf 'build.sh: %s\n' "$*" >&2; exit 1; }

if [ -z "${REFRACT_CONTAINER:-}" ] && [ "$(uname -s)" != Linux ]; then
	command -v docker >/dev/null 2>&1 || die "docker required (https://docs.docker.com/get-docker/)"

	# start Docker Desktop on macOS if not running
	if [ "$(uname -s)" = Darwin ] && ! docker info >/dev/null 2>&1; then
		printf 'build.sh: starting Docker Desktop\n'
		open -a Docker
		timeout=60
		while ! docker info >/dev/null 2>&1; do
			timeout=$((timeout - 1))
			[ "$timeout" -gt 0 ] || die "Docker Desktop failed to start"
			sleep 1
		done
	fi

	# build image if missing
	if ! docker image inspect "$image_name" >/dev/null 2>&1; then
		printf 'build.sh: building docker image (one-time)\n'
		docker build -t "$image_name" "$dir"
	fi

	exec docker run --rm \
		-v "$root:/work" \
		-w /work \
		-e REFRACT_CONTAINER=1 \
		-e "ARCH=${ARCH:-aarch64}" \
		-e "KCFLAGS=${KCFLAGS:-$default_flags}" \
		-e "JOBS=${JOBS:-}" \
		"$image_name" sh kernel/build.sh "${1:-}"
fi

# ---------------------------------------------------------------------------
# Everything below runs inside the container (or natively on Linux)
MAKE=${MAKE:-make}

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

	# skip re-init if tree matches version and inputs haven't changed
	init_stamp="$tree/.init-stamp"
	need_init=true
	if [ -f "$init_stamp" ] && [ -d "$tree/.git" ]; then
		old_v=$(cat "$init_stamp" 2>/dev/null || echo "")
		if [ "$old_v" = "$v" ]; then
			# check if patches, drop, or src changed since last init
			changed=false
			for f in "$dir/drop" "$dir/patches"/*.sh "$dir/src"/* "$dir/func"/*; do
				[ -e "$f" ] || continue
				if [ "$f" -nt "$init_stamp" ]; then
					changed=true
					break
				fi
			done
			if [ "$changed" = false ]; then
				need_init=false
				printf 'build.sh: tree already initialized for %s\n' "$v"
			fi
		fi
	fi
	[ "$need_init" = true ] || { exit 0; }

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
	git -C "$tree" add -Af
	git -C "$tree" -c user.name=kernel-bootstrap -c user.email=kernel-bootstrap@local commit -q -m "linux $v"
	# drop: remove files and their Makefile obj references
	if [ -f "$dir/drop" ]; then
		dropped=0
		while read -r line; do
			case "$line" in \#*|"") continue ;; esac
			f=${line%% *}
			[ -f "$tree/$f" ] || { printf 'build.sh: drop: not found %s\n' "$f" >&2; continue; }
			obj=$(basename "$f" .c).o
			makefile="$tree/$(dirname "$f")/Makefile"
			rm "$tree/$f"
			if [ -f "$makefile" ]; then
				sed -i "/[^[:alnum:]_]$obj/d" "$makefile"
			fi
			dropped=$((dropped + 1))
		done < "$dir/drop"
		if [ "$dropped" -gt 0 ]; then
			git -C "$tree" add -Af
			git -C "$tree" -c user.name=kernel-bootstrap -c user.email=kernel-bootstrap@local \
				commit -q -m "refract: drop $dropped file(s)"
			printf 'build.sh: dropped %s file(s)\n' "$dropped"
		fi
	fi
	# overlay: replace kernel files with prism-enhanced versions from src/
	if [ -d "$dir/src" ]; then
		(cd "$dir/src" && find . -type f) | while read -r f; do
			f=${f#./}
			mkdir -p "$tree/$(dirname "$f")"
			cp "$dir/src/$f" "$tree/$f"
		done
		changed=$(git -C "$tree" diff --name-only | wc -l)
		new=$(git -C "$tree" ls-files --others --exclude-standard | wc -l)
		total=$((changed + new))
		if [ "$total" -gt 0 ]; then
			git -C "$tree" add -Af
			git -C "$tree" -c user.name=kernel-bootstrap -c user.email=kernel-bootstrap@local \
				commit -q -m "refract: overlay $total file(s) from src/"
			printf 'build.sh: overlaid %s file(s) from src/\n' "$total"
		fi
	fi
	# func: per-function patches from func/
	# directory layout: func/<kernel-path>.c/<function_name>.c
	# each file contains the complete replacement function (signature + body)
	if [ -d "$dir/func" ]; then
		patched=0
		for fpath in $(cd "$dir/func" && find . -type f -name '*.c' | sort); do
			fpath=${fpath#./}
			fname=$(basename "$fpath" .c)
			target=$(dirname "$fpath")
			[ -f "$tree/$target" ] || { die "func: target not found: $target"; }
			replacement="$dir/func/$fpath"
			awk -v fname="$fname" -v rfile="$replacement" '
			function braces(s,    n, i, c) {
				n = 0
				for (i = 1; i <= length(s); i++) {
					c = substr(s, i, 1)
					if (c == "{") n++; else if (c == "}") n--
				}
				return n
			}
			BEGIN { state = 0; depth = 0; prev = ""; found = 0; sig = 0 }
			# state 2: inside function body, counting braces
			state == 2 {
				depth += braces($0)
				if (depth <= 0) {
					state = 0; depth = 0
					while ((getline ln < rfile) > 0) print ln
					close(rfile); found = 1
				}
				next
			}
			# state 1: found name, looking for opening {
			state == 1 {
				sig++
				if (sig > 20) {
					printf "%s", accum; accum = ""
					state = 0; sig = 0; print; next
				}
				if (index($0, "{") > 0) {
					depth = braces($0)
					if (depth <= 0) {
						state = 0; depth = 0
						while ((getline ln < rfile) > 0) print ln
						close(rfile); found = 1
					} else { state = 2 }
					next
				}
				if ($0 ~ /;[[:space:]]*$/) {
					printf "%s", accum; print
					accum = ""; state = 0; sig = 0; next
				}
				next
			}
			# state 0: searching for function
			state == 0 {
				d = braces($0)
				if (depth == 0 && match($0, "(^|[^_a-zA-Z0-9])" fname "\\(")) {
					state = 1; sig = 0; accum = ""
					if (prev != "" && prev !~ /;[[:space:]]*$/ && prev !~ /^[[:space:]]*$/ && prev !~ /^#/ && prev !~ /^[[:space:]]*\//) {
						accum = prev "\n"
					} else if (prev != "") { print prev }
					prev = ""
					if (index($0, "{") > 0) {
						depth = braces($0)
						if (depth <= 0) {
							state = 0; depth = 0
							while ((getline ln < rfile) > 0) print ln
							close(rfile); found = 1
						} else { state = 2 }
					}
					next
				}
				if (prev != "") print prev
				prev = $0; depth += d; next
			}
			END {
				if (prev != "" && state == 0) print prev
				if (!found) { print "func: not found: " fname > "/dev/stderr"; exit 1 }
			}' "$tree/$target" > "$tree/$target.func.tmp" \
				&& mv "$tree/$target.func.tmp" "$tree/$target" \
				|| { rm -f "$tree/$target.func.tmp"; die "func: failed to patch $fname in $target"; }
			patched=$((patched + 1))
		done
		if [ "$patched" -gt 0 ]; then
			git -C "$tree" add -Af
			git -C "$tree" -c user.name=kernel-bootstrap -c user.email=kernel-bootstrap@local \
				commit -q -m "refract: patch $patched function(s)"
			printf 'build.sh: patched %s function(s)\n' "$patched"
		fi
	fi
	printf 'build.sh: ready at %s\n' "$tree"
	printf '%s\n' "$v" > "$init_stamp"
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
	"$MAKE" -C "$tree" CC="$__cc" ARCH="$arch" allnoconfig KCONFIG_ALLCONFIG="$config"
	"$MAKE" -C "$tree" CC="$__cc" ARCH="$arch" KCFLAGS="$__kcflags" -j"$jobs" "$target"
	cp "$tree/$image" "$dir/kernel"
	size=$(wc -c < "$dir/kernel")
	printf '\nkernel ready: %s/kernel (%.2f MB)\n\n' "$dir" "$(echo "$size / 1048576" | bc -l)"
	"$0" stats
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

clean)
	rm -rf "$tree"
	rm -f "$dir/kernel"
	printf 'build.sh: cleaned extracted tree and kernel image\n'
	;;

stats)
	[ -d "$tree/.git" ] || die "run init first"
	baseline=$cache/baseline

	# find the vanilla commit (first commit = plain linux)
	vanilla=$(git -C "$tree" rev-list --max-parents=0 HEAD)
	[ -n "$vanilla" ] || die "cannot find vanilla commit"

	printf '\n── Prism conversion stats ──\n\n'

	# --- per-file LOC diff ---
	total_orig=0
	total_new=0
	total_removed=0

	# dropped files (deleted entirely)
	if [ -f "$dir/drop" ]; then
		while read -r line; do
			case "$line" in \#*|"") continue ;; esac
			f=${line%% *}
			orig=$(git -C "$tree" show "$vanilla:$f" 2>/dev/null | wc -l)
			[ "$orig" -gt 0 ] || continue
			total_orig=$((total_orig + orig))
			total_removed=$((total_removed + orig))
			printf '  %-45s  %6d → %6d  (%+d)\n' "$f [dropped]" "$orig" 0 "-$orig"
		done < "$dir/drop"
	fi

	# overlaid files (replaced with prism versions from src/)
	if [ -d "$dir/src" ]; then
		(cd "$dir/src" && find . -type f) | while read -r f; do
			f=${f#./}
			orig=$(git -C "$tree" show "$vanilla:$f" 2>/dev/null | wc -l)
			now=$(wc -l < "$dir/src/$f")
			diff=$((now - orig))
			total_orig=$((total_orig + orig))
			total_new=$((total_new + now))
			sign=; [ "$diff" -ge 0 ] && sign=+
			printf '  %-45s  %6d → %6d  (%s%d)\n' "$f" "$orig" "$now" "$sign" "$diff"
		done
	fi

	# re-sum outside subshell (the pipe above runs in a subshell)
	sum_orig=0; sum_new=0; sum_removed=0
	if [ -f "$dir/drop" ]; then
		while read -r line; do
			case "$line" in \#*|"") continue ;; esac
			f=${line%% *}
			n=$(git -C "$tree" show "$vanilla:$f" 2>/dev/null | wc -l)
			sum_orig=$((sum_orig + n))
			sum_removed=$((sum_removed + n))
		done < "$dir/drop"
	fi
	if [ -d "$dir/src" ]; then
		for f in $(cd "$dir/src" && find . -type f | sed 's|^\./||'); do
			n=$(git -C "$tree" show "$vanilla:$f" 2>/dev/null | wc -l)
			sum_orig=$((sum_orig + n))
			sum_new=$((sum_new + $(wc -l < "$dir/src/$f")))
		done
	fi
	sum_total=$((sum_new + 0))
	sum_delta=$((sum_total - sum_orig))
	sign=; [ "$sum_delta" -ge 0 ] && sign=+
	printf '\n  %-45s  %6d → %6d  (%s%d)\n' "TOTAL (touched files)" "$sum_orig" "$sum_total" "$sign" "$sum_delta"
	if [ "$sum_orig" -gt 0 ]; then
		pct=$((sum_delta * 100 / sum_orig))
		printf '  LOC change: %s%d%%\n' "$sign" "$pct"
	fi

	# --- object file sizes ---
	if [ -f "$tree/.config" ]; then
		printf '\n  ── Object sizes ──\n\n'
		_arch=${ARCH:-$(uname -m)}
		case "$_arch" in
			x86_64|amd64)  _arch=x86_64 ;;
			aarch64|arm64) _arch=arm64 ;;
			riscv64|riscv) _arch=riscv ;;
			i?86|x86)      _arch=x86 ;;
		esac
		_cc=${CC:-prism}
		_kcflags=${KCFLAGS:-$default_flags}
		_fmt() { echo "$1" | awk '{ if ($1==0) printf "      0"; else if ($1<1024) printf "%5d B",$1; else if ($1<1048576) printf "%5.1f KB",$1/1024; else printf "%5.2f MB",$1/1048576 }'; }

		obj_sum_v=0; obj_sum_p=0

		# — overlaid files: prism .o already built —
		if [ -d "$dir/src" ]; then
			for f in $(cd "$dir/src" && find . -type f -name '*.c' | sed 's|^\./||'); do
				obj=$tree/${f%.c}.o
				if [ -f "$obj" ]; then p_sz=$(wc -c < "$obj"); else p_sz=0; fi
				# restore vanilla, recompile
				git -C "$tree" show "$vanilla:$f" > "$tree/$f" 2>/dev/null || continue
				rm -f "$obj"
				"$MAKE" -C "$tree" CC="$_cc" ARCH="$_arch" KCFLAGS="$_kcflags" "${f%.c}.o" >/dev/null 2>&1 || true
				if [ -f "$obj" ]; then v_sz=$(wc -c < "$obj"); else v_sz=0; fi
				d=$((p_sz - v_sz))
				sign=; [ "$d" -ge 0 ] && sign=+
				printf '  %-40s  %s → %s  (%s%d B)\n' "$f" "$(_fmt "$v_sz")" "$(_fmt "$p_sz")" "$sign" "$d"
				obj_sum_v=$((obj_sum_v + v_sz))
				obj_sum_p=$((obj_sum_p + p_sz))
			done
		fi

		# — dropped files: prism .o = 0, compile vanilla on the side —
		if [ -f "$dir/drop" ]; then
			while read -r line; do
				case "$line" in \#*|"") continue ;; esac
				f=${line%% *}
				case "$f" in *.c) ;; *) continue ;; esac
				obj=$tree/${f%.c}.o
				# restore both the .c and its Makefile reference so make can find it
				git -C "$tree" show "$vanilla:$f" > "$tree/$f" 2>/dev/null || continue
				makefile=$tree/$(dirname "$f")/Makefile
				git -C "$tree" show "$vanilla:$(dirname "$f")/Makefile" > "$makefile" 2>/dev/null || true
				rm -f "$obj"
				"$MAKE" -C "$tree" CC="$_cc" ARCH="$_arch" KCFLAGS="$_kcflags" "${f%.c}.o" >/dev/null 2>&1 || true
				if [ -f "$obj" ]; then v_sz=$(wc -c < "$obj"); else v_sz=0; fi
				printf '  %-40s  %s →       0  (-%d B)\n' "$f [dropped]" "$(_fmt "$v_sz")" "$v_sz"
				obj_sum_v=$((obj_sum_v + v_sz))
			done < "$dir/drop"
		fi

		# restore tree to prism state
		git -C "$tree" checkout HEAD -- . 2>/dev/null || true

		# total
		obj_d=$((obj_sum_p - obj_sum_v))
		sign=; [ "$obj_d" -ge 0 ] && sign=+
		printf '\n  %-40s  %s → %s  (%s%d B' "TOTAL (.o)" "$(_fmt "$obj_sum_v")" "$(_fmt "$obj_sum_p")" "$sign" "$obj_d"
		if [ "$obj_sum_v" -gt 0 ]; then
			obj_pct=$((obj_d * 100 / obj_sum_v))
			printf ', %s%d%%' "$sign" "$obj_pct"
		fi
		printf ')\n'
	else
		printf '\n  (no .config — run build first for object size comparison)\n'
	fi

	# --- compiled size ---
	printf '\n'
	if [ -f "$dir/kernel" ]; then
		cur=$(wc -c < "$dir/kernel")
		printf '  kernel image: %d bytes (%.2f MB)\n' "$cur" "$(echo "$cur / 1048576" | bc -l)"
		if [ -f "$baseline" ]; then
			base=$(cat "$baseline")
			sdiff=$((cur - base))
			sign=; [ "$sdiff" -ge 0 ] && sign=+
			printf '  vs baseline:  %s%d bytes' "$sign" "$sdiff"
			if [ "$base" -gt 0 ]; then
				spct=$((sdiff * 100 / base))
				printf ' (%s%d%%)' "$sign" "$spct"
			fi
			printf '\n'
		else
			printf '  (no baseline — run "build.sh baseline" with a vanilla build to compare)\n'
		fi
	else
		printf '  (no kernel image — run build first)\n'
	fi
	printf '\n'
	;;

baseline)
	[ -f "$dir/kernel" ] || die "build first, then run baseline"
	size=$(wc -c < "$dir/kernel")
	printf '%d\n' "$size" > "$cache/baseline"
	printf 'build.sh: baseline saved: %d bytes (%.2f MB)\n' "$size" "$(echo "$size / 1048576" | bc -l)"
	;;

-h|--help|help)
	printf 'usage: %s [setup|init|build|run|clean|stats|baseline]\n' "$(basename "$0")"
	;;

*)
	die "usage: $(basename "$0") [setup|init|build|run|clean|stats|baseline]"
	;;
esac