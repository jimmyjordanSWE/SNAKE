import os
import sys
from tree_sitter import Language, Parser
import tree_sitter_c as tsc

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
    
    if node.type == 'function_definition':
        for child in node.children:
            if child.type == 'function_declarator':
                name, params = extract_fn_info(child, source_code)
                if name:
                    symbols.append({'type': 'fn', 'repr': f"{name}({','.join(params)})"})

    elif node.type == 'declaration':
        for child in node.children:
            if child.type == 'function_declarator':
                name, params = extract_fn_info(child, source_code)
                if name:
                    symbols.append({'type': 'fn', 'repr': f"{name}({','.join(params)})"})

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
            symbols.append({'type': 'st', 'repr': f"{display_name}{field_str}"})

    elif node.type == 'enum_specifier':
        name = ""
        for child in node.children:
            if child.type == 'type_identifier':
                name = source_code[child.start_byte:child.end_byte].decode('utf-8')
                symbols.append({'type': 'en', 'repr': name})
                break

    elif node.type == 'type_definition':
        name = ""
        for child in node.children:
            if child.type == 'type_identifier':
                name = source_code[child.start_byte:child.end_byte].decode('utf-8')
        if name:
            symbols.append({'type': 'ty', 'repr': name})

    for child in node.children:
        if node.type not in ('compound_statement', 'field_declaration_list', 'parameter_list'):
            symbols.extend(extract_symbols(child, source_code))
    
    return symbols

def main():
    context_file = "PROJECT_CONTEXT.txt"
    if os.path.exists(context_file):
        os.remove(context_file)
        
    parser = get_c_parser()
    project_root = os.getcwd()
    target_dirs = ['src', 'include']
    output = ["PROJECT_STRUCTURE_AST"]
    
    for target in target_dirs:
        dir_path = os.path.join(project_root, target)
        if not os.path.exists(dir_path): continue
            
        tree = {}
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

        def print_tree(node, name, indent=0):
            if name == '__symbols__': return
            output.append("  " * indent + name)
            
            if '__symbols__' in node:
                syms = node['__symbols__']
                for group_type in ['fn', 'st', 'ty', 'en']:
                    seen = set()
                    group_syms = []
                    for s in syms:
                        if s['type'] == group_type and s['repr'] not in seen:
                            group_syms.append(s)
                            seen.add(s['repr'])
                            
                    if group_syms:
                        output.append("  " * (indent + 1) + group_type)
                        for s in group_syms:
                            output.append("  " * (indent + 2) + s['repr'])
                            
            for child_name in sorted(node.keys()):
                if child_name != '__symbols__':
                    print_tree(node[child_name], child_name, indent + 1)

        if target in tree:
            print_tree(tree[target], target, 0)

    with open(context_file, "w") as f:
        f.write("\n".join(output))
    print(f"Project context generated in {context_file}")

if __name__ == "__main__":
    main()
