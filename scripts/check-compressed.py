#!/usr/bin/env python3
"""Check src/compressed.c against the RISC-V assembler over the whole 16-bit space.

For every halfword whose bits [1:0] are not 11 (the only ones that can be compressed instructions):

  1. Legality: the halfwords expand_compressed() accepts must be the ones objdump disassembles as a
     c.* instruction, apart from the differences listed in ref_legal() and KNOWN_DIFFERENCES.
  2. Meaning: for each accepted halfword, rewrite objdump's c.* text as the base-ISA instruction it
     stands for (c.addi x10,-16 -> addi x10,x10,-16), assemble that, and compare the 32-bit word
     with the expander's output.

Needs riscv64-unknown-elf-as, -objdump and -objcopy, and a C compiler (cc), on the PATH.
Run it from anywhere:  scripts/check-compressed.py        Exit status 0 = no mismatch.
"""
import collections
import os
import re
import struct
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MARCH = "-march=rv32imac_zicsr_zifencei"
ABI = "-mabi=ilp32"
OBJDUMP = ["riscv64-unknown-elf-objdump", "-d", "-M", "numeric,no-aliases"]

# Halfwords where objdump is more lenient than the manual. 0x6101 is C.ADDI16SP with a zero immediate,
# which the manual reserves (zca.adoc, c-addi16sp_rsv); objdump accepts it.
KNOWN_DIFFERENCES = {0x6101}

FLOAT_FORMS = {"c.fld", "c.fsd", "c.flw", "c.fsw", "c.fldsp", "c.fsdsp", "c.flwsp", "c.fswsp"}
RV64_ONLY = {"c.ld", "c.sd", "c.ldsp", "c.sdsp", "c.addiw", "c.addw", "c.subw"}


def run(cmd, **kw):
    return subprocess.run(cmd, check=True, capture_output=True, text=True, **kw)


def build_driver(tmp):
    """Compile a program that prints 'halfword legal word' for all 65536 halfwords."""
    src = os.path.join(tmp, "dump.c")
    with open(src, "w") as f:
        f.write('#include <stdio.h>\n#include "compressed.h"\n'
                'int main(void) { for (uint32_t h = 0; h < 65536; h++) { uint32_t w = 0;\n'
                '  bool ok = expand_compressed((uint16_t)h, &w);\n'
                '  printf("%04x %d %08x\\n", h, ok, ok ? w : 0); } return 0; }\n')
    exe = os.path.join(tmp, "dump")
    run(["cc", "-O1", "-I", os.path.join(ROOT, "src"), src, os.path.join(ROOT, "src", "compressed.c"), "-o", exe])
    expansions = {}
    for line in run([exe]).stdout.splitlines():
        h, ok, w = line.split()
        expansions[int(h, 16)] = (ok == "1", int(w, 16))
    return expansions


def disassemble(tmp, halves):
    asm = os.path.join(tmp, "halves.s")
    obj = os.path.join(tmp, "halves.o")
    with open(asm, "w") as f:
        f.write("".join(".insn 2, 0x%04x\n" % h for h in halves))
    run(["riscv64-unknown-elf-as", MARCH, ABI, asm, "-o", obj])
    text = {}
    for line in run(OBJDUMP + [obj]).stdout.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\s+([0-9a-f]{4})\s+(.*)$", line)
        if m:
            text[int(m.group(1), 16) // 2] = m.group(3).strip()
    if len(text) != len(halves):
        sys.exit("objdump listed %d of %d halfwords" % (len(text), len(halves)))
    return text


def split(t):
    parts = t.split(None, 1)
    ops = parts[1].strip() if len(parts) > 1 else ""
    return parts[0], ops, [x.strip() for x in ops.split(",")] if ops else []


def ref_legal(t):
    """Is the objdump text a legal RV32 compressed instruction without F or D?"""
    mn, _, o = split(t)
    if not mn.startswith("c.") or mn == "c.unimp" or mn in FLOAT_FORMS or mn in RV64_ONLY:
        return False
    if mn in ("c.slli", "c.srli", "c.srai") and int(o[-1], 0) >= 32:
        return False        # RV32 needs shamt[5] = 0; objdump does not enforce it
    return True


def base_text(t, addr):
    """The base-ISA instruction a compressed instruction (objdump text at byte address addr) stands for."""
    mn, ops, o = split(t)

    def off(x):             # objdump prints a branch target as an absolute address
        return ". + (%d)" % (int(x.split()[0], 16) - addr)

    if mn == "c.addi4spn": return "addi %s,%s,%s" % (o[0], o[1], o[2])
    if mn in ("c.lw", "c.sw", "c.lwsp", "c.swsp"): return mn[2:].replace("sp", "") + " " + ops
    if mn == "c.nop": return "addi x0,x0,0"
    if mn == "c.addi": return "addi %s,%s,%s" % (o[0], o[0], o[1])
    if mn == "c.jal": return "jal x1,%s" % off(o[0])
    if mn == "c.li": return "addi %s,x0,%s" % (o[0], o[1])
    if mn == "c.addi16sp": return "addi x2,x2,%s" % o[1]
    if mn == "c.lui": return "lui %s,%s" % (o[0], o[1])
    if mn in ("c.srli", "c.srai", "c.andi", "c.slli"): return "%s %s,%s,%s" % (mn[2:], o[0], o[0], o[1])
    if mn in ("c.sub", "c.xor", "c.or", "c.and"): return "%s %s,%s,%s" % (mn[2:], o[0], o[0], o[1])
    if mn == "c.j": return "jal x0,%s" % off(o[0])
    if mn == "c.beqz": return "beq %s,x0,%s" % (o[0], off(o[1]))
    if mn == "c.bnez": return "bne %s,x0,%s" % (o[0], off(o[1]))
    if mn == "c.jr": return "jalr x0,0(%s)" % o[0]
    if mn == "c.mv": return "add %s,x0,%s" % (o[0], o[1])
    if mn == "c.ebreak": return "ebreak"
    if mn == "c.jalr": return "jalr x1,0(%s)" % o[0]
    if mn == "c.add": return "add %s,%s,%s" % (o[0], o[0], o[1])
    sys.exit("no rule for objdump text: " + t)


def main():
    with tempfile.TemporaryDirectory() as tmp:
        expansions = build_driver(tmp)
        halves = [h for h in range(65536) if h & 3 != 3]
        text = disassemble(tmp, halves)

        failures = 0
        legality = collections.Counter()
        accepted = []                               # (halfword, objdump text, byte address)
        for idx, h in enumerate(halves):
            mine = expansions[h][0]
            if h in KNOWN_DIFFERENCES:
                if mine:
                    failures += 1
                    print("expected %04x to be rejected" % h)
                continue
            if ref_legal(text[idx]) != mine:
                legality[(split(text[idx])[0], mine)] += 1
                failures += 1
            elif mine:
                accepted.append((h, text[idx], idx * 2))
        for h in range(65536):
            if h & 3 == 3 and expansions[h][0]:
                failures += 1
                print("%04x has bits 1:0 = 11 but was accepted" % h)
        print("legality mismatches: %d %s" % (sum(legality.values()), dict(legality) if legality else ""))

        asm = os.path.join(tmp, "base.s")
        with open(asm, "w") as f:
            f.write(".option norvc\n" + "".join(base_text(t, a) + "\n" for _, t, a in accepted))
        run(["riscv64-unknown-elf-as", MARCH, ABI, asm, "-o", os.path.join(tmp, "base.o")])
        run(["riscv64-unknown-elf-objcopy", "-O", "binary", "-j", ".text",
             os.path.join(tmp, "base.o"), os.path.join(tmp, "base.bin")])
        data = open(os.path.join(tmp, "base.bin"), "rb").read()
        if len(data) != 4 * len(accepted):
            sys.exit("assembled %d bytes for %d instructions" % (len(data), len(accepted)))
        words = struct.unpack("<%dI" % len(accepted), data)
        wrong = [(h, t, w) for (h, t, _), w in zip(accepted, words) if w != expansions[h][1]]
        for h, t, w in wrong[:10]:
            print("%04x  %s  assembler %08x  expander %08x" % (h, t, w, expansions[h][1]))
        failures += len(wrong)
        print("accepted halfwords: %d, with a different word from the assembler: %d" % (len(accepted), len(wrong)))
    print("OK" if failures == 0 else "FAILED")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
