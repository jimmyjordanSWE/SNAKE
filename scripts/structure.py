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
    for root, dirs, files in os.walk(project_root):
        dirs[:] = [d for d in dirs if d not in ignored_dirs]
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

    def print_hierarchical(syms, indent):
        if not syms: return
        
        # Build a trie for names
        trie = {}
        for s in syms:
            # Split repr into name and params
            # repr is "name(params)" or "name{fields}"
            # For ty/en it might just be "name"
            if '(' in s['repr']:
                name, suffix = s['repr'].split('(', 1)
                suffix = '(' + suffix
            elif '{' in s['repr']:
                name, suffix = s['repr'].split('{', 1)
                suffix = '{' + suffix
            else:
                name = s['repr']
                suffix = ""
            
            # Split name by underscores, keep trailing underscores if any
            # But usually we group by "word_"
            parts = []
            current = ""
            for char in name:
                current += char
                if char == '_':
                    parts.append(current)
                    current = ""
            if current:
                parts.append(current)
            
            curr = trie
            for i, part in enumerate(parts):
                if part not in curr: curr[part] = {}
                curr = curr[part]
                if i == len(parts) - 1:
                    if '__leaf__' not in curr: curr['__leaf__'] = []
                    curr['__leaf__'].append((suffix, s['loc']))

        def _print_trie(node, prefix_str, current_indent, needs_type=True):
            children = [k for k in node if k != '__leaf__']
            
            # Collapse single-child nodes
            if len(children) == 1 and '__leaf__' not in node:
                child_key = children[0]
                _print_trie(node[child_key], prefix_str + child_key, current_indent, needs_type)
                return

            type_tag = f"{sym_type} " if needs_type else ""

            if prefix_str:
                # If this node is a leaf with no children, combine prefix and signature
                if '__leaf__' in node and not children and len(node['__leaf__']) == 1:
                    sf, loc = node['__leaf__'][0]
                    output.append(" " * current_indent + f"{type_tag}{prefix_str}{sf}")
                else:
                    # Cluster node
                    output.append(" " * current_indent + f"{type_tag}{prefix_str}")
                    # Print leaves directly under cluster
                    for leaf in node.get('__leaf__', []):
                        sf, loc = leaf
                        output.append(" " * (current_indent + 1) + f"{sf}")
                    # Recurse children
                    for child_key in sorted(children):
                        _print_trie(node[child_key], child_key, current_indent + 1, needs_type=False)
            else:
                # Root of the group analysis
                for leaf in node.get('__leaf__', []):
                    sf, loc = leaf
                    output.append(" " * current_indent + f"{sym_type} {sf}")
                for child_key in sorted(children):
                    _print_trie(node[child_key], child_key, current_indent, needs_type=True)

        _print_trie(trie, "", indent, needs_type=True)

    def print_tree(node, name, indent=0):
        if name == '__symbols__': return
        output.append(" " * indent + name)
        if '__symbols__' in node:
            syms = node['__symbols__']
            for group_type in ['fn', 'st', 'ty', 'en']:
                type_map = {'fn':'f', 'st':'s', 'ty':'t', 'en':'e'}
                
                # Filter to unique symbols, preferring definitions and max LOC
                unique_syms = {}
                for s in syms:
                    if s['type'] != group_type: continue
                    r = s['repr']
                    if r not in unique_syms:
                        unique_syms[r] = s
                    else:
                        cur = unique_syms[r]
                        # Prefer definition over declaration
                        s_def = s.get('is_def', False)
                        cur_def = cur.get('is_def', False)
                        if s_def and not cur_def:
                            unique_syms[r] = s
                        elif s_def == cur_def:
                             if s['loc'] > cur['loc']:
                                 unique_syms[r] = s
                
                group_syms = list(unique_syms.values())

                if group_syms:
                    nonlocal sym_type
                    sym_type = type_map[group_type]
                    print_hierarchical(group_syms, indent + 1)
        
        for child_name in sorted(node.keys()):
            if child_name not in ('__symbols__', '__loc__'):
                print_tree(node[child_name], child_name, indent + 1)

    sym_type = ""
    for top_level in sorted(tree.keys()):
        print_tree(tree[top_level], top_level, 0)

    print("\n".join(output))

if __name__ == "__main__":
    main()
