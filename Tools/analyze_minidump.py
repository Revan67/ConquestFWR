import struct
import sys

from minidump.minidumpfile import MinidumpFile


def module_for(modules, address):
    for module in modules:
        if module.baseaddress <= address < module.endaddress:
            return module
    return None


def describe(modules, address):
    module = module_for(modules, address)
    if module is None:
        return ""
    return f"{module.name}+0x{address - module.baseaddress:X}"


def main(path):
    dump = MinidumpFile.parse(path)
    reader = dump.get_reader()
    exception = dump.exception.exception_records[0]
    thread = next(item for item in dump.threads.threads if item.ThreadId == exception.ThreadId)
    context = thread.ContextObject
    modules = dump.modules.modules

    print(f"exception_thread=0x{thread.ThreadId:X}")
    for name in ("Eip", "Esp", "Ebp", "Eax", "Ebx", "Ecx", "Edx", "Esi", "Edi", "EFlags"):
        value = getattr(context, name)
        print(f"{name}=0x{value:08X} {describe(modules, value)}")

    def dump_dwords(label, address, count):
        print(f"\n{label} at 0x{address:08X}:")
        try:
            data = reader.read(address, count * 4)
        except Exception as error:
            print(f"unreadable: {error}")
            return
        for offset in range(0, len(data), 4):
            value = struct.unpack_from("<I", data, offset)[0]
            print(
                f"+0x{offset:03X} 0x{value:08X} "
                f"{describe(modules, value)}"
            )

    # The current Debug build uses an EBP frame for ObjectExtent::initExtents.
    # These ranges expose its locals, the init record argument, and the object
    # fields without requiring an interactive debugger.
    dump_dwords("exception frame locals", context.Ebp - 0x180, 0x188 // 4)
    try:
        init_record = struct.unpack("<I", reader.read(context.Ebp + 8, 4))[0]
        dump_dwords("init record", init_record, 0x130 // 4)
        mesh_renders = struct.unpack("<I", reader.read(context.Ebp - 0xA8, 4))[0]
        dump_dwords("mesh renderer pointer array", mesh_renders, 16)
        object_address = struct.unpack("<I", reader.read(context.Ebp - 0x90, 4))[0]
        dump_dwords("object mesh fields", object_address + 0x220, 32)
    except Exception as error:
        print(f"\nUnable to inspect initExtents data: {error}")

    print("\nEBP chain:")
    ebp = context.Ebp
    for index in range(64):
        try:
            data = reader.read(ebp, 8)
        except Exception as error:
            print(f"{index:02d} EBP=0x{ebp:08X} unreadable: {error}")
            break
        next_ebp, return_address = struct.unpack("<II", data)
        print(
            f"{index:02d} EBP=0x{ebp:08X} next=0x{next_ebp:08X} "
            f"return=0x{return_address:08X} {describe(modules, return_address)}"
        )
        if next_ebp <= ebp or next_ebp - ebp > 0x100000:
            break
        ebp = next_ebp

    print("\nModule-looking values in first 4 KB of stack:")
    stack = reader.read(context.Esp, 4096)
    for offset in range(0, len(stack), 4):
        value = struct.unpack_from("<I", stack, offset)[0]
        description = describe(modules, value)
        if description:
            print(f"ESP+0x{offset:03X} 0x{value:08X} {description}")


if __name__ == "__main__":
    main(sys.argv[1])
