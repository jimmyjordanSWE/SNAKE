
import os
import sys
from tree_sitter import Language, Parser
import tree_sitter_c as tsc

def get_c_parser():
    C_LANGUAGE = Language(tsc.language())
    parser = Parser(C_LANGUAGE)
    return parser

def find_transitions(node, source_code, filename, transitions):
    # Heuristic: look for assignments to a variable named 'state' or similar,
    # or assignments involving enum values.
    
    if node.type == 'assignment_expression':
        left = node.child_by_field_name('left')
        right = node.child_by_field_name('right')
        
        if left and right:
            lhs_name = source_code[left.start_byte:left.end_byte].decode('utf-8')
            rhs_name = source_code[right.start_byte:right.end_byte].decode('utf-8')
            
            # Check if lhs is likely a state variable
            if 'state' in lhs_name.lower() or 'status' in lhs_name.lower():
                # Check if RHS is likely an enum (UPPERCASE)
                if rhs_name.isupper() and len(rhs_name) > 1 and not rhs_name.isdigit():
                     line = node.start_point.row + 1
                     transitions.append(f"{lhs_name}->{rhs_name}(L{line})")

    for child in node.children:
        find_transitions(child, source_code, filename, transitions)

def main():
    parser = get_c_parser()
    project_root = os.getcwd()
    
    print("Invariants (State Transitions):")
    
    # Scan root and src
    scan_paths = [project_root, os.path.join(project_root, 'src')]
    
    seen_files = set()
    for path in scan_paths:
        if not os.path.exists(path): continue
        if os.path.isdir(path):
            for root, dirs, files in os.walk(path):
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
                        results = []
                        find_transitions(tree.root_node, source_code, rel_path, results)
                        if results:
                            print(f"{rel_path}: {' '.join(results)}")
        else:
            pass

if __name__ == "__main__":
    main()
