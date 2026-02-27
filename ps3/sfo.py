#!/usr/bin/env python3
from __future__ import annotations
from xml.dom.minidom import Document, parse
import struct
import sys
import getopt

debug = False
pretty = False

SFO_MAGIC  = 0x46535000
SFO_STRING = 2
SFO_INT    = 4


def nullterm(data: bytes) -> str:
    z = data.find(b'\0')
    return data[:z].decode('utf-8') if z != -1 else data.decode('utf-8')


def align(num: int, alignment: int) -> int:
    return (num + alignment - 1) & ~(alignment - 1)


# ---------------------------------------------------------------------------
# Header / Entry pack/unpack (replaces the Struct base class)
# ---------------------------------------------------------------------------

HEADER_FMT = '<IIIII'   # magic, unk1, KeyOffset, ValueOffset, PairCount
HEADER_SIZE = struct.calcsize(HEADER_FMT)

ENTRY_FMT  = '<HBBIII'  # key_off, unk1, value_type, value_len, padded_len, value_off
ENTRY_SIZE = struct.calcsize(ENTRY_FMT)


def pack_header(magic, unk1, key_offset, value_offset, pair_count) -> bytes:
    return struct.pack(HEADER_FMT, magic, unk1, key_offset, value_offset, pair_count)


def unpack_header(data: bytes, offset: int = 0):
    return struct.unpack_from(HEADER_FMT, data, offset)


def pack_entry(key_off, unk1, value_type, value_len, padded_len, value_off) -> bytes:
    return struct.pack(ENTRY_FMT, key_off, unk1, value_type, value_len, padded_len, value_off)


def unpack_entry(data: bytes, offset: int):
    return struct.unpack_from(ENTRY_FMT, data, offset)


# ---------------------------------------------------------------------------
# Core helpers
# ---------------------------------------------------------------------------

def _read_sfo(data: bytes) -> list[tuple[str, int | str]]:
    """Parse raw SFO bytes into an ordered list of (key, value) pairs."""
    magic, unk1, key_offset, value_offset, pair_count = unpack_header(data)
    assert magic == SFO_MAGIC, f"Bad SFO magic: {magic:#010x}"
    assert unk1 == 0x00000101,  f"Unexpected unk1: {unk1:#010x}"

    pairs: list[tuple[str, int | str]] = []
    entry_base = HEADER_SIZE
    for _ in range(pair_count):
        key_off, e_unk1, value_type, value_len, padded_len, value_off = unpack_entry(data, entry_base)
        entry_base += ENTRY_SIZE

        key = nullterm(data[key_offset + key_off:])
        if value_type == SFO_STRING:
            value: int | str = nullterm(data[value_offset + value_off:])
        elif value_type == SFO_INT:
            value = struct.unpack_from('<I', data, value_offset + value_off)[0]
        else:
            raise ValueError(f"Unknown value type {value_type:#04x} for key '{key}'")

        pairs.append((key, value))

    return pairs


# ---------------------------------------------------------------------------
# Public commands
# ---------------------------------------------------------------------------

def list_sfo(filepath: str) -> None:
    with open(filepath, 'rb') as f:
        data = f.read()
    pairs = _read_sfo(data)
    if debug:
        magic, unk1, key_offset, value_offset, pair_count = unpack_header(data)
        print(f"Magic:        {magic:#010x}")
        print(f"Unk1:         {unk1:#010x}")
        print(f"Key Offset:   {key_offset:#010x}")
        print(f"Value Offset: {value_offset:#010x}")
        print(f"Pair Count:   {pair_count:#010x}")
        print()
        for key, value in pairs:
            print(f"  {key!r} = {value!r}")
    else:
        print(dict(pairs))


def convert_to_xml(sfo_file: str, xml_file: str) -> None:
    with open(sfo_file, 'rb') as f:
        data = f.read()
    pairs = _read_sfo(data)

    doc = Document()
    root = doc.createElement("sfo")
    doc.appendChild(root)

    for key, value in pairs:
        node = doc.createElement("value")
        node.setAttribute("name", key)
        if isinstance(value, int):
            node.setAttribute("type", "integer")
            node.appendChild(doc.createTextNode(str(value)))
        else:
            node.setAttribute("type", "string")
            node.appendChild(doc.createTextNode(value))
        root.appendChild(node)

    with open(xml_file, 'w', encoding='utf-8') as f:
        doc.writexml(f, '', '\t', '\n', encoding='utf-8')


def convert_to_sfo(xml_file: str, sfo_file: str,
                   force_title: str | None = None,
                   force_appid: str | None = None) -> None:
    dom = parse(xml_file)
    nodes = dom.getElementsByTagName("value")

    kvs: list[tuple[str, int | str]] = []
    for node in nodes:
        if not node.hasAttributes():
            continue
        vtype = node.getAttribute("type") or None
        name  = node.getAttribute("name") or None
        if not name or not vtype:
            continue

        text = ''.join(
            n.data for n in node.childNodes if n.nodeType == n.TEXT_NODE
        ).strip()

        if name == "TITLE" and force_title is not None:
            kvs.append((name, force_title))
        elif name == "TITLE_ID" and force_appid is not None:
            kvs.append((name, force_appid))
        elif vtype == "string":
            kvs.append((name, text))
        elif vtype == "integer":
            kvs.append((name, int(text)))

    # --- build entry metadata ---
    def padded_len_for(key: str, value: int | str) -> int:
        if isinstance(value, int):
            return 4
        raw = len(value.encode('utf-8')) + 1  # +1 for null terminator
        if key == "TITLE":
            return align(raw, 0x80)
        elif key == "LICENSE":
            return align(raw, 0x200)
        elif key == "TITLE_ID":
            return align(raw, 0x10)
        return align(raw, 4)

    key_off_acc   = 0
    value_off_acc = 0
    entries: list[tuple] = []

    for key, value in kvs:
        pl = padded_len_for(key, value)
        vl = 4 if isinstance(value, int) else len(value.encode('utf-8')) + 1
        vt = SFO_INT if isinstance(value, int) else SFO_STRING
        entries.append((key_off_acc, 4, vt, vl, pl, value_off_acc))
        key_off_acc   += len(key.encode('utf-8')) + 1
        value_off_acc += pl

    pair_count   = len(kvs)
    header_end   = HEADER_SIZE + ENTRY_SIZE * pair_count
    key_section  = align(header_end + key_off_acc, 4)
    key_pad      = key_section - (header_end + key_off_acc)
    value_section_start = key_section

    with open(sfo_file, 'wb') as f:
        f.write(pack_header(SFO_MAGIC, 0x00000101,
                            header_end, value_section_start, pair_count))

        for e in entries:
            f.write(pack_entry(*e))

        for key, _ in kvs:
            f.write(key.encode('utf-8') + b'\0')
        f.write(b'\0' * key_pad)

        for (key, value), (_, _, vt, vl, pl, _) in zip(kvs, entries):
            if isinstance(value, int):
                f.write(struct.pack('<I', value))
            else:
                encoded = value.encode('utf-8') + b'\0'
                f.write(encoded)
                f.write(b'\0' * (pl - len(encoded)))


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def usage() -> None:
    print("""usage: sfo.py [options] [args]

  -l / --list <file>          Print key/value pairs from a PARAM.SFO
  -t / --toxml   <sfo> <xml>  Convert PARAM.SFO -> XML
  -f / --fromxml <xml> <sfo>  Convert XML -> PARAM.SFO
       --title <title>        Override TITLE when building SFO
       --appid <id>           Override TITLE_ID when building SFO
  -d / --debug                Enable debug output
  -p / --pretty               Pretty-print debug output
  -v / --version              Print version
  -h / --help                 Print this help
""")


def version() -> None:
    print("sfo.py 0.3 (Python 3 rewrite)")


def main() -> None:
    global debug, pretty

    try:
        opts, args = getopt.getopt(
            sys.argv[1:], "hdvpl:tf",
            ["help", "debug", "version", "pretty",
             "list=", "toxml", "fromxml", "title=", "appid="]
        )
    except getopt.GetoptError as e:
        print(f"Error: {e}", file=sys.stderr)
        usage()
        sys.exit(2)

    do_list   = False
    list_file = None
    to_xml    = False
    from_xml  = False
    force_title = None
    force_appid = None

    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage(); sys.exit(0)
        elif opt in ("-v", "--version"):
            version(); sys.exit(0)
        elif opt in ("-d", "--debug"):
            debug = True
        elif opt in ("-p", "--pretty"):
            pretty = True
        elif opt in ("-l", "--list"):
            do_list = True
            list_file = arg
        elif opt in ("-t", "--toxml"):
            to_xml = True
        elif opt in ("-f", "--fromxml"):
            from_xml = True
        elif opt == "--title":
            force_title = arg
        elif opt == "--appid":
            force_appid = arg

    if do_list and list_file:
        list_sfo(list_file)
    elif to_xml and not from_xml and len(args) == 2:
        convert_to_xml(args[0], args[1])
    elif from_xml and not to_xml and len(args) == 2:
        convert_to_sfo(args[0], args[1], force_title, force_appid)
    else:
        usage()
        sys.exit(2)


if __name__ == "__main__":
    main()