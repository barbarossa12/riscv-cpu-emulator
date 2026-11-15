#!/bin/sh
# Checks that the compiled device tree (dts/riscv-emu.dts) agrees with the emulator's headers, so the two cannot drift apart.
#
#   usage: check-dts.sh <riscv-emu.dtb> <source-dir>
#
# Header values are read from the #define lines of src/*.h; things fixed by the RISC-V privileged architecture or by
# Linux (interrupt numbers, the PLIC context order, the ISA extension names) are written in this script.
set -u

if [ $# -ne 2 ]; then
	echo "usage: $0 <riscv-emu.dtb> <source-dir>" >&2
	exit 2
fi
dtb=$1
src=$2
failures=0

# The value of "#define NAME ..." in a header, as a decimal number. The expression may have a u suffix and a comment.
define() {
	expr=$(sed -n "s|^#define[[:space:]][[:space:]]*$2[[:space:]][[:space:]]*\(.*\)\$|\1|p" "$src/src/$1" | head -n 1)
	expr=$(printf '%s' "$expr" | sed 's|//.*||; s|/\*.*||; s/\([0-9a-fA-FxX]\)[uU]\{1,\}/\1/g')
	if [ -z "$expr" ]; then
		echo "FAIL: no #define $2 in src/$1" >&2
		exit 2
	fi
	echo $(( $expr ))
}

hex() { printf '%x' "$1"; }

# fdtget as lower-case text, one line, for a node and property
get() { fdtget -t "$1" "$dtb" "$2" "$3" 2>/dev/null; }

check() {  # description, actual, expected (an empty expected value is a broken tree: both sides would be missing)
	if [ -n "$3" ] && [ "$2" = "$3" ]; then
		echo "ok:   $1"
	else
		echo "FAIL: $1: device tree has '$2', expected '$3'"
		failures=$((failures + 1))
	fi
}

check_reg() {  # description, node, base, size  (two address cells, two size cells)
	check "$1 reg" "$(get x "$2" reg)" "0 $(hex "$3") 0 $(hex "$4")"
}

ram_base=$(define machine.h MACHINE_RAM_BASE)
ram_size=$(define machine.h MACHINE_RAM_SIZE)
uart_irq=$(define machine.h MACHINE_UART_IRQ)

check_reg "memory" /memory@80000000 "$ram_base" "$ram_size"
check "timebase-frequency" "$(get u /cpus timebase-frequency)" "$(define clint.h CLINT_TIMEBASE_HZ)"

check_reg "clint" /soc/clint@2000000 "$(define clint.h CLINT_BASE)" "$(define clint.h CLINT_SIZE)"
check_reg "plic" /soc/plic@c000000 "$(define plic.h PLIC_BASE)" "$(define plic.h PLIC_SIZE)"
check "plic riscv,ndev" "$(get u /soc/plic@c000000 riscv,ndev)" "$(define plic.h PLIC_NSOURCES)"
check_reg "serial" /soc/serial@10000000 "$(define uart.h UART_BASE)" "$(define uart.h UART_SIZE)"
check "serial interrupts" "$(get u /soc/serial@10000000 interrupts)" "$uart_irq"
check "serial reg-shift" "$(get u /soc/serial@10000000 reg-shift)" "0"
check "serial reg-io-width" "$(get u /soc/serial@10000000 reg-io-width)" "1"

# Interrupt numbers of the privileged architecture: the CLINT drives the hart's machine software (3) and timer (7)
# interrupts. The order of the PLIC's list is its context number (Linux uses the position): 0 is M-mode (machine external,
# 11), 1 is S-mode (supervisor external, 9), as src/plic.h defines the contexts.
intc=$(get u /cpus/cpu@0/interrupt-controller phandle)
check "clint interrupts-extended" "$(get u /soc/clint@2000000 interrupts-extended)" "$intc 3 $intc 7"
check "plic interrupts-extended (context 0 = M, 1 = S)" "$(get u /soc/plic@c000000 interrupts-extended)" "$intc 11 $intc 9"
check "serial interrupt-parent is the plic" "$(get u /soc/serial@10000000 interrupt-parent)" "$(get u /soc/plic@c000000 phandle)"

# The finisher, and the two nodes through which OpenSBI's syscon drivers write to it (the value written is what src/finisher.h decodes)
check_reg "finisher" /soc/test@100000 "$(define finisher.h FINISHER_BASE)" "$(define finisher.h FINISHER_SIZE)"
check "finisher compatible" "$(get s /soc/test@100000 compatible)" "sifive,test1 sifive,test0 syscon"
test_phandle=$(get u /soc/test@100000 phandle)
check "poweroff compatible" "$(get s /poweroff compatible)" "syscon-poweroff"
check "poweroff regmap" "$(get u /poweroff regmap)" "$test_phandle"
check "poweroff offset" "$(get u /poweroff offset)" "0"
check "poweroff value" "$(get x /poweroff value)" "$(hex "$(define finisher.h FINISHER_PASS)")"
check "reboot compatible" "$(get s /reboot compatible)" "syscon-reboot"
check "reboot regmap" "$(get u /reboot regmap)" "$test_phandle"
check "reboot offset" "$(get u /reboot offset)" "0"
check "reboot value" "$(get x /reboot value)" "$(hex "$(define finisher.h FINISHER_RESET)")"

check "serial compatible" "$(get s /soc/serial@10000000 compatible)" "ns16550a"
check "plic compatible" "$(get s /soc/plic@c000000 compatible)" "sifive,plic-1.0.0 riscv,plic0"
check "clint compatible" "$(get s /soc/clint@2000000 compatible)" "sifive,clint0 riscv,clint0"
check "cpu interrupt controller compatible" "$(get s /cpus/cpu@0/interrupt-controller compatible)" "riscv,cpu-intc"

check "cpu isa-base" "$(get s /cpus/cpu@0 riscv,isa-base)" "rv32i"
check "cpu mmu-type" "$(get s /cpus/cpu@0 mmu-type)" "riscv,sv32"
check "cpu isa-extensions" "$(get s /cpus/cpu@0 riscv,isa-extensions)" "i m a c zicntr zicsr zifencei"
# Nothing the emulator does not implement may be advertised (the list above already pins it; this names the usual suspects)
for ext in svade svadu sstc f d v h; do
	case " $(get s /cpus/cpu@0 riscv,isa-extensions) " in
		*" $ext "*) echo "FAIL: the cpu advertises $ext"; failures=$((failures + 1)) ;;
	esac
done

if [ "$failures" -ne 0 ]; then
	echo "$failures check(s) failed"
	exit 1
fi
echo "all device tree checks passed"
