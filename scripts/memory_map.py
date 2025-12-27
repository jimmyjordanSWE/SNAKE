
import os
import sys
from tree_sitter import Language, Parser
import tree_sitter_c as tsc

def get_c_parser():
    C_LANGUAGE = Language(tsc.language())
    parser = Parser(C_LANGUAGE)
    return parser

def find_allocations(node, source_code, filename):
    allocs = []
    
    if node.type == 'call_expression':
        func_node = node.child_by_field_name('function')
        if func_node:
            func_name = source_code[func_node.start_byte:func_node.end_byte].decode('utf-8')
            if func_name in ('malloc', 'calloc', 'realloc', 'free'):
                line = node.start_point.row + 1
                allocs.append(f"{func_name} ({filename}:{line})")

    for child in node.children:
        allocs.extend(find_allocations(child, source_code, filename))
        
    return allocs

def main():
    parser = get_c_parser()
    project_root = os.getcwd()
    
    # Scan root, src, and include
    scan_paths = [project_root, os.path.join(project_root, 'src'), os.path.join(project_root, 'include')]
    
    seen_files = {} # path -> [allocs]
    for path in scan_paths:
        if not os.path.exists(path): continue
        if os.path.isdir(path):
            for root, dirs, files in os.walk(path):
                if '.venv' in root or 'build' in root or '.git' in root or 'vendor' in root:
                    continue
                for file in sorted(files):
                    if file.endswith(('.c', '.h')):
                        file_path = os.path.join(root, file)
                        if file_path in seen_files: continue
                        rel_path = os.path.relpath(file_path, project_root)
                        with open(file_path, 'rb') as f:
                            source_code = f.read()
                        tree = parser.parse(source_code)
                        allocs = find_allocations(tree.root_node, source_code, rel_path)
                        if allocs:
                            seen_files[rel_path] = allocs

    # Token-minimized output
    for path, allocs in sorted(seen_files.items()):
        formatted = []
        for a in allocs:
            # a is "func (path:line)"
            parts = a.split()
            func = parts[0]
            line = parts[1].split(':')[-1].rstrip(')')
            op = {'malloc':'m','calloc':'c','realloc':'r','free':'f'}.get(func, func)
            formatted.append(f"{line}[{op}]")
        print(f"{path}: {' '.join(formatted)}")

if __name__ == "__main__":
    main()
