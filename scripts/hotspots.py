
import os
import sys
from tree_sitter import Language, Parser
import tree_sitter_c as tsc

def get_c_parser():
    C_LANGUAGE = Language(tsc.language())
    parser = Parser(C_LANGUAGE)
    return parser

def find_hotspots(node, source_code, filename, curr_func):
    hotspots = []
    
    if node.type == 'function_definition':
        decl = node.child_by_field_name('declarator')
        while decl and decl.type in ('pointer_declarator', 'parenthesized_declarator'):
             if decl.child_count > 0: decl = decl.children[0]
        if decl and decl.type == 'function_declarator':
             fname_node = decl.child_by_field_name('declarator')
             if fname_node: 
                 curr_func = source_code[fname_node.start_byte:fname_node.end_byte].decode('utf-8')
    
    # Check for nested loops (O(n^2))
    if node.type in ('for_statement', 'while_statement', 'do_statement'):
        # Check if parent chain has another loop
        p = node.parent
        depth = 0
        while p:
            if p.type in ('for_statement', 'while_statement', 'do_statement'):
                depth += 1
            p = p.parent
            
        if depth >= 1:
            line = node.start_point.row + 1
            hotspots.append(f"{curr_func}:LOOP({depth+1},L{line})")

    # Check for frequent allocations (alloc inside loop)
    if node.type == 'call_expression':
        func_node = node.child_by_field_name('function')
        if func_node:
            func_name = source_code[func_node.start_byte:func_node.end_byte].decode('utf-8')
            if func_name in ('malloc', 'calloc', 'realloc'):
                 # Check if inside loop
                 p = node.parent
                 in_loop = False
                 while p:
                     if p.type in ('for_statement', 'while_statement', 'do_statement'):
                         in_loop = True
                         break
                     p = p.parent
                 
                 if in_loop:
                      line = node.start_point.row + 1
                      hotspots.append(f"{curr_func}:ALLOC_LOOP(L{line})")

    for child in node.children:
        hotspots.extend(find_hotspots(child, source_code, filename, curr_func))
        
    return hotspots

def main():
    parser = get_c_parser()
    project_root = os.getcwd()

    print("Hotspots Analysis:")
    
    # Scan root and src
    scan_paths = [project_root, os.path.join(project_root, 'src')]
    
    seen_files = set()
    for path in scan_paths:
        if not os.path.exists(path): continue
        
        if os.path.isdir(path):
            for root, dirs, files in os.walk(path):
                # Skip .venv, build, .git directories
                if '.venv' in root or 'build' in root or '.git' in root or 'vendor' in root:
                    continue
                for file in sorted(files):
                    if file.endswith('.c'):
                        file_path = os.path.join(root, file)
                        if file_path in seen_files: continue
                        seen_files.add(file_path)
                        
                        rel_path = os.path.relpath(file_path, project_root)
                        with open(file_path, 'rb') as f:
                            source_code = f.read()
                        tree = parser.parse(source_code)
                        results = find_hotspots(tree.root_node, source_code, rel_path, None)
                        if results:
                            print(f"{rel_path}: {' '.join(results)}")
        else:
            pass

if __name__ == "__main__":
    main()
