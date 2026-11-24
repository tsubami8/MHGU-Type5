"""Build an AArch32 Forge plugin with Clang/LLD and produce its NRO image."""

import argparse
from hashlib import sha256
from pathlib import Path
from struct import pack_into, unpack_from
from subprocess import run
from tempfile import TemporaryDirectory

SOURCE = Path(__file__).resolve().parent
PAGE = 4096


def aligned(number):
    return (number + PAGE - 1) & -PAGE


def elf_to_nro(elf):
    if elf[:7] != b"\x7fELF\x01\x01\x01" or unpack_from("<H", elf, 18)[0] != 40:
        raise ValueError("Expected a little-endian ARM ELF32 file")

    phoff = unpack_from("<I", elf, 28)[0]
    phsize, phcount = unpack_from("<HH", elf, 42)
    loads = []
    for i in range(phcount):
        kind, start, addr, _, filesz, memsz, flags, _ = unpack_from(
            "<8I", elf, phoff + i * phsize
        )
        if kind == 1:
            loads.append((addr, flags, memsz, elf[start : start + filesz]))
    loads.sort()
    if len(loads) != 3 or [item[1] for item in loads] != [5, 4, 6] or loads[0][0] != 0:
        raise ValueError("Expected three NRO segments in RX, R, RW order")

    data_start, _, data_memsz, data = loads[2]
    size = aligned(data_start + len(data))
    bss = data_start + data_memsz - size
    if bss < 0 or bss % PAGE:
        raise ValueError("The linker must page-align the BSS")

    image = bytearray(size)
    for i, (addr, _, _, contents) in enumerate(loads):
        end = loads[i + 1][0] if i < 2 else size
        if addr % PAGE or addr + len(contents) > end:
            raise ValueError("Invalid NRO segment layout")
        image[addr : addr + len(contents)] = contents
        pack_into("<II", image, 0x20 + i * 8, addr, end - addr)

    mod0 = unpack_from("<I", image, 4)[0]
    if image[mod0 : mod0 + 4] != b"MOD0":
        raise ValueError("Missing MOD0 header")
    pack_into("<4sIII", image, 0x10, b"NRO0", 0, size, 0)
    pack_into("<I", image, 0x38, bss)
    image[0x40:0x60] = sha256(image[0x80:]).digest()
    return image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--clang", default="clang")
    parser.add_argument("--lld", default="ld.lld")
    parser.add_argument("--zig", help="Use Zig's Clang and LLD drivers")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    compiler = [args.clang, "--target=arm-none-eabi", "-march=armv7-a"]
    linker = [args.lld]
    if args.zig:
        compiler = [args.zig, "cc", "--target=arm-freestanding-eabi", "-mcpu=cortex_a9"]
        linker = [args.zig, "ld.lld"]

    with TemporaryDirectory(prefix="mhgu-type5-") as name:
        build = Path(name)
        objects = []
        for source in ("plugin.c", "controls.c", "bow_options.S", "target_camera.S"):
            obj = build / (source + ".o")
            run([
                *compiler, "--no-default-config", "-marm", "-mfpu=vfpv3", "-mfloat-abi=softfp", "-fPIC",
                "-ffreestanding", "-fno-builtin", "-fno-unwind-tables",
                "-fno-asynchronous-unwind-tables", "-fvisibility=hidden", "-O2",
                "-Wall", "-Wextra", "-Werror",
                *(["-std=c17"] if source.endswith(".c") else []),
                "-c", str(SOURCE / source),
                "-o", str(obj),
            ], check=True)
            objects.append(str(obj))

        elf = build / "type5.elf"
        run([
            *linker, "-shared", "-Bsymbolic", "--hash-style=sysv",
            "-z", "max-page-size=4096", "-T", str(SOURCE / "nro.ld"),
            "-o", str(elf), *objects,
        ], check=True)

        image = elf_to_nro(elf.read_bytes())
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(image)
        print(f"{args.output.resolve()} ({len(image):,} bytes)")


if __name__ == "__main__":
    main()
