# no-debug.sh — ensure zero debug code in the kernel
# EXPERT forces DEBUG_KERNEL=y (it's a select in init/Kconfig)
# DEBUG_KERNEL alone adds no code — it's a Kconfig visibility gate
# but remove the select so nothing can accidentally depend on it
sed -i '/^menuconfig EXPERT$/,/^config /{/^[[:space:]]*select DEBUG_KERNEL$/d}' init/Kconfig
