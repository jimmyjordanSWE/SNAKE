
import os
import sys
from tree_sitter import Language, Parser
import tree_sitter_c as tsc

def get_c_parser():
    C_LANGUAGE = Language(tsc.language())
    parser = Parser(C_LANGUAGE)
    return parser

def find_includes(node, source_code):
    includes = []
    
    if node.type == 'preproc_include':
        path_node = node.child_by_field_name('path')
        if path_node:
            path_val = source_code[path_node.start_byte:path_node.end_byte].decode('utf-8')
            # remove quotes or brackets
            path_val = path_val.strip('<>"')
            includes.append(path_val)
            
    for child in node.children:
        includes.extend(find_includes(child, source_code))
        
    return includes

def main():
    parser = get_c_parser()
    project_root = os.getcwd()
    file_deps = {} # file -> [included_files]
    
    # Scan root, src, and include
    scan_paths = [project_root, os.path.join(project_root, 'src'), os.path.join(project_root, 'include')]
    
    seen_files = set()
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
                        seen_files.add(file_path)
                        
                        rel_path = os.path.relpath(file_path, project_root)
                        with open(file_path, 'rb') as f:
                            source_code = f.read()
                        tree = parser.parse(source_code)
                        file_deps[rel_path] = find_includes(tree.root_node, source_code)
        else:
            pass

    print("Deps:")
    for f, deps in sorted(file_deps.items()):
        print(f" {f}: {' '.join(sorted(deps))}")
    
    # Cycle detection logic
    # We need to map include paths back to actual files. This is tricky without include path logic.
    # We'll do a best-effort mapping    # Cycle detection logic
    # Build graph
    simplified_map = {} # filename -> rel_path
    for f in file_deps:
        simplified_map[os.path.basename(f)] = f
        
    adj = {} # rel_path -> list of rel_paths
    for f, d_list in file_deps.items():
        adj[f] = []
        for inc in d_list:
            base = os.path.basename(inc)
            if base in simplified_map:
                adj[f].append(simplified_map[base])
        
    visited = set()
    rec_stack = set()
    
    def detect_cycle(u, path):
        visited.add(u)
        rec_stack.add(u)
        
        for v in adj.get(u, []):
            if v not in visited:
                if detect_cycle(v, path + [v]): return True
            elif v in rec_stack:
                print(f"  Cycle detected: {'->'.join(path)}->{v}")
                return True
                
        rec_stack.remove(u)
        return False

    for f in adj:
        if f not in visited:
            detect_cycle(f, [f])

if __name__ == "__main__":
    main()
