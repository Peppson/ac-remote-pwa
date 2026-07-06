Import("env")  # type: ignore

from pathlib import Path
import re

ICONS_DIR = Path("images") / "icons"
OUTPUT_DIR = Path("include") / "embeddedWeb"
INDEX_HEADER_PATH = OUTPUT_DIR / "embedded.h"

SUPPORTED_EXTENSIONS = {".png", ".jpg", ".jpeg", ".webp", ".gif", ".ico", ".bmp"}
BYTES_PER_LINE = 12


def sanitize_symbol_name(filename: Path) -> str:
    raw = f"{filename.stem}_{filename.suffix.lstrip('.')}"
    sanitized = re.sub(r"[^A-Za-z0-9]+", "_", raw).strip("_").upper()
    return f"EMBEDDED_{sanitized}"


def make_header_filename(filename: Path) -> str:
    return f"{filename.stem}.h"


def format_byte_array(data: bytes) -> str:
    lines = []
    for i in range(0, len(data), BYTES_PER_LINE):
        chunk = data[i : i + BYTES_PER_LINE]
        hex_values = ", ".join(f"0x{byte:02x}" for byte in chunk)
        lines.append(f"    {hex_values},")
    if lines:
        # Trim the trailing comma for cleaner generated output.
        lines[-1] = lines[-1].rstrip(",")
    return "\n".join(lines)


def generate_header_content(source: Path, symbol: str, data: bytes) -> str:
    length_symbol = f"{symbol}_LEN"
    byte_array = format_byte_array(data)

    return (
        "#pragma once\n"
        "#include <cstddef>\n\n"
        f"// Generated from: {source.as_posix()}\n"
        f"constexpr unsigned char {symbol}[] = {{\n"
        f"{byte_array}\n"
        "};\n"
        f"constexpr std::size_t {length_symbol} = sizeof({symbol});\n"
    )


def generate_index_header(entries: list[tuple[str, str]]) -> str:
    include_lines = [f'#include "{header_name}"' for header_name, _ in entries]

    includes_block = "\n".join(include_lines)

    return (
        "#pragma once\n"
        "#include <cstddef>\n\n"
        "// Minified webpage\n"
        "#include \"webpage.h\"\n\n"
        "// Auto-generated index of embedded icon headers.\n"
        f"{includes_block}\n"
    )


def write_if_changed(path: Path, content: str) -> bool:
    path.parent.mkdir(parents=True, exist_ok=True)

    if path.exists() and path.read_text(encoding="utf-8") == content:
        return False

    path.write_text(content, encoding="utf-8", newline="\n")
    return True


def main() -> None:
    project_dir = Path(env["PROJECT_DIR"])  # type: ignore
    icons_src_dir = project_dir / ICONS_DIR
    output_dir = project_dir / OUTPUT_DIR

    if not icons_src_dir.exists():
        raise FileNotFoundError(f"Missing icons source folder: {icons_src_dir}")

    source_files = sorted(
        p for p in icons_src_dir.iterdir() 
            if p.is_file() and p.suffix.lower() in SUPPORTED_EXTENSIONS
    )

    if not source_files:
        raise RuntimeError(f"No icon files found in {icons_src_dir}")

    generated_entries: list[tuple[str, str]] = []

    for source_file in source_files:
        data = source_file.read_bytes()
        if not data:
            raise RuntimeError(f"Icon file is empty: {source_file}")

        symbol = sanitize_symbol_name(source_file)
        header_name = make_header_filename(source_file)
        header_path = output_dir / header_name

        header_content = generate_header_content(source_file.relative_to(project_dir), symbol, data)

        if write_if_changed(header_path, header_content):
            print(f"[sync_icons] Updated {header_path.relative_to(project_dir)}")
        else:
            print(f"[sync_icons] {header_path.relative_to(project_dir)} already up to date")

        generated_entries.append((header_name, symbol))

    index_content = generate_index_header(generated_entries)
    index_path = project_dir / INDEX_HEADER_PATH

    if write_if_changed(index_path, index_content):
        print(f"[sync_icons] Updated {index_path.relative_to(project_dir)}")
    else:
        print(f"[sync_icons] {index_path.relative_to(project_dir)} already up to date")


main()
