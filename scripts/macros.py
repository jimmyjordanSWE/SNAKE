
import os
import sys
from tree_sitter import Language, Parser
import tree_sitter_c as tsc

def get_c_parser():
    C_LANGUAGE = Language(tsc.language())
    parser = Parser(C_LANGUAGE)
    return parser

def check_macros(node, source_code, filename):
    issues = []
    
    if node.type == 'preproc_def':
        name_node = node.child_by_field_name('name')
        val_node = node.child_by_field_name('value')
        
        if name_node and val_node:
            name = source_code[name_node.start_byte:name_node.end_byte].decode('utf-8')
            val = source_code[val_node.start_byte:val_node.end_byte].decode('utf-8')
            
            # 1. Check for multiple evaluations of arguments (heuristic)
            # Simplistic check: if an arg appears twice in value, it's risky unless wrapped in do-while(0) or similar context
            # This is hard to do perfectly with tree-sitter on preproc content as it's often just raw text to the parser.
            # tree-sitter-c parses preproc somewhat, but often "value" is just preproc_arg.
            
            # Simple check: control flow inside macro (if/for/while/return) - hidden logic
            if any(k in val for k in ('if ', 'for ', 'while ', 'return ', 'goto ')):
                # Exception: do { ... } while(0) is standard safe pattern
                if 'do' in val and 'while(0)' in val.replace(' ',''):
                     pass
                else:
                     line = node.start_point.row + 1
                     issues.append(f"{name}: !CTRL({line})")
                     
    for child in node.children:
        issues.extend(check_macros(child, source_code, filename))
        
    return issues

def main():
    parser = get_c_parser()
    project_root = os.getcwd()
    
    # Scan root, src, and include
    scan_paths = [project_root, os.path.join(project_root, 'src'), os.path.join(project_root, 'include')]
    
    print("Macro Audit:")
    
    seen_files = set()
    for path in scan_paths:
        if not os.path.exists(path): continue
        
        if os.path.isdir(path):
            for root, dirs, files in os.walk(path):
                # Skip specified directories
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
                        results = check_macros(tree.root_node, source_code, rel_path)
                        if results:
                            print(f"{rel_path}: {' '.join(results)}")
        else:
            pass

if __name__ == "__main__":
    main()
