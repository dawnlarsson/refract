# config.sh — Kconfig patches to make base.config =n entries take effect
# many x86 Kconfig options are invisible or force-selected, so allnoconfig
# + KCONFIG_ALLCONFIG silently ignores our =n settings without these fixes

# --- init/Kconfig: EXPERT forces DEBUG_KERNEL=y (visibility gate) ---
sed -i '/^menuconfig EXPERT$/,/^config /{/^[[:space:]]*select DEBUG_KERNEL$/d}' init/Kconfig

# --- arch/x86/Kconfig: X86_64 force-selects DMA compat and vDSO bloat ---
sed -i '/^config X86_64$/,/^config /{
  /^\tselect SWIOTLB$/d
  /^\tselect ZONE_DMA32$/d
}' arch/x86/Kconfig

# --- arch/x86/Kconfig: X86 force-selects sigframe and unwinder bloat ---
sed -i '/^config X86$/,/^config /{
  /^\tselect DYNAMIC_SIGFRAME$/d
  /^\tselect HAVE_UNWIND_USER_FP/d
  /^\tselect VDSO_GETRANDOM/d
}' arch/x86/Kconfig

# --- arch/x86/entry/vdso/Makefile: vgetrandom unconditionally linked ---
sed -i 's/vobjs-y := vdso-note.o vclock_gettime.o vgetcpu.o vgetrandom.o vgetrandom-chacha.o/vobjs-y := vdso-note.o vclock_gettime.o vgetcpu.o\nvobjs-$(CONFIG_VDSO_GETRANDOM) += vgetrandom.o vgetrandom-chacha.o/' arch/x86/entry/vdso/Makefile

# --- arch/x86/Kconfig: HPET_TIMER is def_bool X86_64 (2004-era timer) ---
sed -i '/^config HPET_TIMER$/,/^config /{
  s/^\tdef_bool X86_64$/\tbool "HPET Timer Support"/
  /^\tprompt "HPET Timer Support" if X86_32$/d
}' arch/x86/Kconfig

# --- arch/x86/Kconfig: X86_MPPARSE default y (1990s pre-ACPI SMP) ---
sed -i '/^config X86_MPPARSE$/,/^config /{/^\tdefault y$/d}' arch/x86/Kconfig

# --- arch/x86/Kconfig: MICROCODE is def_bool y (no-op in VMs) ---
sed -i '/^config MICROCODE$/,/^config /{
  s/^\tdef_bool y$/\tbool "CPU microcode loading support"/
}' arch/x86/Kconfig
