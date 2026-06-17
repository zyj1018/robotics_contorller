#!/usr/bin/env python3
"""数据字典代码生成器 — YAML → C/C++ 头文件"""
import yaml, os, sys
from datetime import datetime

C_TYPES = {
    'uint8_t': 'uint8_t', 'uint16_t': 'uint16_t', 'uint32_t': 'uint32_t',
    'int8_t': 'int8_t', 'int16_t': 'int16_t', 'int32_t': 'int32_t',
    'float': 'float', 'double': 'double', 'bool': 'bool',
}

def load_yaml(path):
    with open(path, 'r', encoding='utf-8') as f:
        return yaml.safe_load(f)

def generate_enums_c(data):
    lines = ['/* Auto-generated enum definitions — DO NOT EDIT */',
             f'/* Generated: {datetime.now().isoformat()} */',
             '#ifndef GENERATED_ENUMS_H', '#define GENERATED_ENUMS_H',
             '#include <stdint.h>', '']
    for name, info in data.get('enums', {}).items():
        lines.append(f'/* {name} */')
        if isinstance(info, dict):
            for k, v in info.get('values', {}).items():
                lines.append(f'#define {name}_{k} {v}')
        elif isinstance(info, list):
            for item in info:
                if isinstance(item, dict):
                    lines.append(f'#define {name}_{item["name"]} {item["id"]}')
                else:
                    # auto-numbered
                    pass
        lines.append('')
    lines.append('#endif /* GENERATED_ENUMS_H */')
    return '\n'.join(lines)

def generate_messages_c(data):
    lines = ['/* Auto-generated message definitions — DO NOT EDIT */',
             f'/* Generated: {datetime.now().isoformat()} */',
             '#ifndef GENERATED_MESSAGES_H', '#define GENERATED_MESSAGES_H',
             '#include <stdint.h>', '#include "protocol_frame.h"', '']
    for name, info in data.get('messages', {}).items():
        msg_id = info.get('id', 0)
        lines.append(f'/* {info.get("desc", name)} */')
        lines.append(f'#define MSG_ID_{name.upper()} 0x{msg_id:04X}')
        lines.append(f'typedef struct {{')
        for field in info.get('fields', []):
            ctype = C_TYPES.get(field.get('type', 'uint32_t'), field.get('type'))
            lines.append(f'    {ctype} {field["name"]};  /* {field.get("desc","")} */')
        lines.append(f'}} {name};')
        lines.append('')
    lines.append('#endif /* GENERATED_MESSAGES_H */')
    return '\n'.join(lines)

def generate_fault_codes_c(data):
    lines = ['/* Auto-generated fault codes — DO NOT EDIT */',
             f'/* Generated: {datetime.now().isoformat()} */',
             '#ifndef GENERATED_FAULT_CODES_H', '#define GENERATED_FAULT_CODES_H', '']
    for fault in data.get('faults', []):
        lines.append(f'#define FAULT_{fault["module"]}_{fault["name"]} 0x{fault["code"]:04X}')
    lines.append('')
    lines.append('/* Fault code lookup */')
    lines.append('typedef struct {')
    lines.append('    uint16_t code; const char *module; const char *desc; uint8_t level;')
    lines.append('} fault_code_entry_t;')
    lines.append('')
    entries = [f'    {{0x{f["code"]:04X}, "{f["module"]}", "{f["desc"]}", {f["level"]}}}'
               for f in data.get('faults', [])]
    lines.append('static const fault_code_entry_t fault_code_table[] = {')
    lines.append(',\n'.join(entries))
    lines.append('};')
    lines.append('#endif /* GENERATED_FAULT_CODES_H */')
    return '\n'.join(lines)

def generate_cpp_header(data):
    lines = ['// Auto-generated C++ definitions — DO NOT EDIT',
             f'// Generated: {datetime.now().isoformat()}',
             '#pragma once', '#include <cstdint>', '',
             'namespace robot_controller { namespace generated {', '']
    for name, info in data.get('enums', {}).items():
        lines.append(f'enum class {name} : uint8_t {{')
        if isinstance(info, dict):
            vals = [f'    {k} = {v}' for k, v in info.get('values', {}).items()]
        elif isinstance(info, list):
            vals = [f'    {item["name"]} = {item["id"]}' for item in info]
        lines.append(',\n'.join(vals))
        lines.append('};')
        lines.append('')
    lines.append('}} // namespace robot_controller::generated')
    return '\n'.join(lines)

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.normpath(os.path.join(script_dir, '..', '..'))
    idl_dir = os.path.join(repo_root, 'idl')
    out_dir_c = os.path.join(repo_root, 'generated', 'mcu_common')
    out_dir_cpp = os.path.join(repo_root, 'generated', 'rk3588')

    # 加载所有 YAML
    data = {'enums': {}, 'messages': {}, 'parameters': [], 'faults': []}
    for fname in ['enums.yaml', 'messages.yaml', 'parameters.yaml', 'faults.yaml']:
        path = os.path.join(idl_dir, fname)
        if os.path.exists(path):
            d = load_yaml(path)
            for key in data:
                if key in d:
                    if isinstance(data[key], dict):
                        data[key].update(d[key])
                    elif isinstance(data[key], list):
                        data[key].extend(d[key])
    # Also try single file
    single = os.path.join(idl_dir, 'data_dictionary.yaml')
    if os.path.exists(single):
        d = load_yaml(single)
        for key in data:
            if key in d:
                if isinstance(data[key], dict) and isinstance(d[key], dict):
                    data[key].update(d[key])
                elif isinstance(data[key], list) and isinstance(d[key], list):
                    data[key].extend(d[key])

    os.makedirs(out_dir_c, exist_ok=True)
    os.makedirs(out_dir_cpp, exist_ok=True)

    # Generate C headers
    for gen_func, fname in [
        (generate_enums_c, 'generated_enums.h'),
        (generate_messages_c, 'generated_messages.h'),
        (generate_fault_codes_c, 'generated_fault_codes.h'),
    ]:
        path = os.path.join(out_dir_c, fname)
        with open(path, 'w', encoding='utf-8') as f:
            f.write(gen_func(data))
            f.write('\n')
        print(f'  Generated: {path}')

    # Generate C++ header
    path = os.path.join(out_dir_cpp, 'generated_enums.hpp')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(generate_cpp_header(data))
        f.write('\n')
    print(f'  Generated: {path}')

    print(f'\nDone. {len(data["enums"])} enums, {len(data["messages"])} messages, '
          f'{len(data["parameters"])} params, {len(data["faults"])} faults.')

if __name__ == '__main__':
    main()
