import os
import sys
from tree_sitter import Language, Parser
import tree_sitter_c as tsc # must use tree sittter, install in venv if not availible 

def get_c_parser():
    C_LANGUAGE = Language(tsc.language())
    parser = Parser(C_LANGUAGE)
    return parser

def get_name_and_pointer_status(node, source_code):
    """Recursively finds the identifier and whether it is a pointer."""
    name = ""
    is_ptr = False
    
    if node.type in ('field_identifier', 'identifier'):
        name = source_code[node.start_byte:node.end_byte].decode('utf-8')
    elif node.type == 'pointer_declarator':
        is_ptr = True
        for child in node.children:
            sub_name, sub_ptr = get_name_and_pointer_status(child, source_code)
            if sub_name: name = sub_name
            if sub_ptr: is_ptr = True
    elif node.type in ('parenthesized_declarator', 'array_declarator', 'function_declarator'):
        for child in node.children:
            sub_name, sub_ptr = get_name_and_pointer_status(child, source_code)
            if sub_name: name = sub_name
            if sub_ptr: is_ptr = True
            
    return name, is_ptr

def extract_fn_info(node, source_code):
    """Extracts name and params from a function_declarator node."""
    name = ""
    params = []
    
    for child in node.children:
        if child.type in ('identifier', 'pointer_declarator', 'parenthesized_declarator'):
            sub_name, _ = get_name_and_pointer_status(child, source_code)
            if sub_name: name = sub_name
        
        if child.type == 'parameter_list':
            for p in child.children:
                if p.type == 'parameter_declaration':
                    p_name = ""
                    p_ptr = False
                    for p_child in p.children:
                        if p_child.type in ('identifier', 'pointer_declarator', 'array_declarator', 'function_declarator', 'parenthesized_declarator'):
                            p_name, p_ptr = get_name_and_pointer_status(p_child, source_code)
                            break
                    if p_name:
                        prefix = "*" if p_ptr else ""
                        params.append(f"{prefix}{p_name}")
                    else:
                        content = source_code[p.start_byte:p.end_byte].decode('utf-8').strip()
                        if '*' in content: params.append("*_")
                        else: params.append("_")
    return name, params

def extract_symbols(node, source_code):
    symbols = []
    
    loc = node.end_point[0] - node.start_point[0] + 1
    
    if node.type == 'function_definition':
        for child in node.children:
            if child.type == 'function_declarator':
                name, params = extract_fn_info(child, source_code)
                if name:
                    symbols.append({'type': 'fn', 'repr': f"{name}({','.join(params)})", 'loc': loc, 'is_def': True})

    elif node.type == 'declaration':
        for child in node.children:
            if child.type == 'function_declarator':
                name, params = extract_fn_info(child, source_code)
                if name:
                    symbols.append({'type': 'fn', 'repr': f"{name}({','.join(params)})", 'loc': loc, 'is_def': False})

    elif node.type == 'struct_specifier':
        name = ""
        fields = []
        for child in node.children:
            if child.type in ('type_identifier', 'identifier'):
                name = source_code[child.start_byte:child.end_byte].decode('utf-8')
            if child.type == 'field_declaration_list':
                for field in child.children:
                    if field.type == 'field_declaration':
                        f_name = ""
                        f_ptr = False
                        for f_child in field.children:
                            if f_child.type in ('field_identifier', 'identifier', 'pointer_declarator', 'array_declarator', 'function_declarator', 'parenthesized_declarator'):
                                f_name, f_ptr = get_name_and_pointer_status(f_child, source_code)
                                break
                        if f_name:
                            prefix = "*" if f_ptr else ""
                            fields.append(f"{prefix}{f_name}")

        if name or fields:
            display_name = name if name else "<anon>"
            field_str = "{" + ",".join(fields) + "}" if fields else ""
            symbols.append({'type': 'st', 'repr': f"{display_name}{field_str}", 'loc': loc})

    elif node.type == 'enum_specifier':
        name = ""
        for child in node.children:
            if child.type == 'type_identifier':
                name = source_code[child.start_byte:child.end_byte].decode('utf-8')
                symbols.append({'type': 'en', 'repr': name, 'loc': loc})
                break

    elif node.type == 'type_definition':
        name = ""
        for child in node.children:
            if child.type == 'type_identifier':
                name = source_code[child.start_byte:child.end_byte].decode('utf-8')
        if name:
            symbols.append({'type': 'ty', 'repr': name, 'loc': loc})

    for child in node.children:
        if node.type not in ('compound_statement', 'field_declaration_list', 'parameter_list'):
            symbols.extend(extract_symbols(child, source_code))
    
    return symbols

def main():
    parser = get_c_parser()
    project_root = os.getcwd()
    output = ["STRUCTURE"]
    ignored_dirs = {'.venv', 'build', '.git', 'vendor', 'node_modules', 'bin', 'obj'}
    
    tree = {}
    included_dirs = {'src', 'include'}
    for d in sorted(included_dirs):
        dir_path = os.path.join(project_root, d)
        if not os.path.exists(dir_path): continue
        for root, _, files in os.walk(dir_path):
            for file in sorted(files):
                if file.endswith(('.c', '.h')):
                    file_path = os.path.join(root, file)
                    rel_path = os.path.relpath(file_path, project_root)
                    parts = rel_path.split(os.sep)
                    curr = tree
                    for part in parts:
                        if part not in curr: curr[part] = {}
                        curr = curr[part]
                    with open(file_path, 'rb') as f:
                        source_code = f.read()
                    ast_tree = parser.parse(source_code)
                    curr['__symbols__'] = extract_symbols(ast_tree.root_node, source_code)
                    curr['__loc__'] = len(source_code.splitlines())

    all_long_fns = []

    def traverse_collect(node, path):
        if '__symbols__' in node:
            for s in node['__symbols__']:
                if s['type'] == 'fn' and s['loc'] > 10:
                    all_long_fns.append({
                        'file': path,
                        'repr': s['repr'],
                        'loc': s['loc']
                    })
        
        children = [k for k in node.keys() if k not in ('__symbols__', '__loc__')]
        for child in sorted(children):
            traverse_collect(node[child], os.path.join(path, child))

    traverse_collect(tree, "")
    
    # Sort globally by LOC descending
    all_long_fns.sort(key=lambda x: x['loc'], reverse=True)
    
    for fn in all_long_fns:
        output.append(f"{fn['file']}: {fn['repr']} LOC: {fn['loc']}")
    
    print("\n".join(output))

if __name__ == "__main__":
    main()
