
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
    ignored_dirs = {'.venv', 'build', '.git', 'vendor', 'node_modules', 'bin', 'obj'}
    
    seen_files = {} # path -> [allocs]
    for root, dirs, files in os.walk(project_root):
        dirs[:] = [d for d in dirs if d not in ignored_dirs]
        for file in sorted(files):
            if file.endswith(('.c', '.h')):
                file_path = os.path.join(root, file)
                rel_path = os.path.relpath(file_path, project_root)
                with open(file_path, 'rb') as f:
                    source_code = f.read()
                tree = parser.parse(source_code)
                allocs = find_allocations(tree.root_node, source_code, rel_path)
                if allocs:
                    seen_files[rel_path] = allocs

    # Token-minimized output with legend
    print("# Legend: [m]alloc [c]alloc [r]ealloc [f]ree  Format: line[op]")
    
    total_allocs = 0
    total_frees = 0
    for path, allocs in sorted(seen_files.items()):
        formatted = []
        for a in allocs:
            parts = a.split()
            func = parts[0]
            line = parts[1].split(':')[-1].rstrip(')')
            op = {'malloc':'m','calloc':'c','realloc':'r','free':'f'}.get(func, func)
            formatted.append(f"{line}{op}")
            if func == 'free':
                total_frees += 1
            else:
                total_allocs += 1
        print(f"{path}: {' '.join(formatted)}")
    
    print(f"# Mem: {total_allocs}a, {total_frees}f ({len(seen_files)} files)")

if __name__ == "__main__":
    main()
