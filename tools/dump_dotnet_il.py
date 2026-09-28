#!/usr/bin/env python3
"""Dump selected .NET methods with metadata-token names (analysis helper)."""

from __future__ import annotations

import argparse

import dnfile
from dncil.cil.body.reader import read_method_body_from_bytes
from dncil.clr.token import StringToken, Token


TABLES = {
    0x00: "Module",
    0x01: "TypeRef",
    0x02: "TypeDef",
    0x04: "Field",
    0x06: "MethodDef",
    0x0A: "MemberRef",
    0x1B: "TypeSpec",
    0x2B: "MethodSpec",
}


def row_name(row: object) -> str:
    parts = []
    for attr in ("TypeNamespace", "TypeName", "Name"):
        value = getattr(row, attr, None)
        if value is not None and str(value):
            parts.append(str(value))
    parent = getattr(row, "Class", None)
    if parent is not None:
        target = getattr(parent, "row", None)
        if target is not None:
            prefix = row_name(target)
            if prefix:
                parts.insert(0, prefix)
    method = getattr(row, "Method", None)
    if method is not None and getattr(method, "row", None) is not None:
        parts.insert(0, row_name(method.row))
    return "::".join(parts) or repr(row)


def resolve_token(pe: dnfile.dnPE, token: Token) -> str:
    value = token.value
    table_id = value >> 24
    row_index = value & 0xFFFFFF
    if isinstance(token, StringToken):
        try:
            return repr(pe.net.user_strings.get(row_index).value)
        except Exception:
            return repr(token)
    table_name = TABLES.get(table_id)
    table = getattr(pe.net.mdtables, table_name, None) if table_name else None
    if table is None or row_index <= 0 or row_index > len(table.rows):
        return repr(token)
    return f"{table_name}[{row_index}] {row_name(table.rows[row_index - 1])}"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("assembly")
    parser.add_argument("type_name")
    parser.add_argument("method_name", nargs="?")
    args = parser.parse_args()

    pe = dnfile.dnPE(args.assembly)
    for typedef in pe.net.mdtables.TypeDef.rows:
        if str(typedef.TypeName) != args.type_name:
            continue
        for method_index in typedef.MethodList:
            method = method_index.row
            if args.method_name and str(method.Name) != args.method_name:
                continue
            print(f"\n{typedef.TypeNamespace}.{typedef.TypeName}::{method.Name} RVA=0x{method.Rva:X}")
            if not method.Rva:
                continue
            offset = pe.get_offset_from_rva(method.Rva)
            body = read_method_body_from_bytes(pe.__data__[offset:])
            for instruction in body.instructions:
                operand = instruction.operand
                if isinstance(operand, Token):
                    operand_text = resolve_token(pe, operand)
                elif operand is None:
                    operand_text = ""
                else:
                    operand_text = str(operand)
                print(f"  IL_{instruction.offset:04X}: {instruction.opcode} {operand_text}".rstrip())


if __name__ == "__main__":
    main()
