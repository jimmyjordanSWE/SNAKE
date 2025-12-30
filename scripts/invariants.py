
import os
from tree_sitter import Language, Parser
import tree_sitter_c as tsc

def get_c_parser():
    C_LANGUAGE = Language(tsc.language())
    parser = Parser(C_LANGUAGE)
    return parser

# Keywords that suggest state variables
STATE_KEYWORDS = ('state', 'status', 'mode', 'phase', 'active', 'enabled', 
                  'initialized', 'running', 'paused', 'ready', 'valid', 'dirty')

def find_transitions(node, source_code, filename, transitions):
    # Detect assignments to state-like variables
    if node.type == 'assignment_expression':
        left = node.child_by_field_name('left')
        right = node.child_by_field_name('right')
        
        if left and right:
            lhs_name = source_code[left.start_byte:left.end_byte].decode('utf-8')
            rhs_name = source_code[right.start_byte:right.end_byte].decode('utf-8')
            
            lhs_lower = lhs_name.lower()
            is_state_var = any(kw in lhs_lower for kw in STATE_KEYWORDS)
            
            if is_state_var:
                # Check if RHS is enum (UPPERCASE), boolean, or NULL
                is_enum = rhs_name.isupper() and len(rhs_name) > 1 and not rhs_name.isdigit()
                is_bool = rhs_name in ('true', 'false', '0', '1')
                is_null = rhs_name == 'NULL'
                
                if is_enum or is_bool or is_null:
                    line = node.start_point.row + 1
                    transitions.append(f"{lhs_name}->{rhs_name}(L{line})")
    
    # Detect function calls that set state (e.g., set_state(X))
    if node.type == 'call_expression':
        func = node.child_by_field_name('function')
        if func:
            func_name = source_code[func.start_byte:func.end_byte].decode('utf-8')
            if 'set_state' in func_name.lower() or 'set_status' in func_name.lower():
                args = node.child_by_field_name('arguments')
                if args:
                    arg_text = source_code[args.start_byte:args.end_byte].decode('utf-8')
                    line = node.start_point.row + 1
                    transitions.append(f"{func_name}{arg_text}(L{line})")

    for child in node.children:
        find_transitions(child, source_code, filename, transitions)

def main():
    parser = get_c_parser()
    project_root = os.getcwd()
    ignored_dirs = {'.venv', 'build', '.git', 'vendor', 'node_modules', 'bin', 'obj'}
    
    all_transitions = {}
    
    included_dirs = {'src', 'include'}
    for d in sorted(included_dirs):
        dir_path = os.path.join(project_root, d)
        if not os.path.exists(dir_path): continue
        for root, _, files in os.walk(dir_path):
            for file in sorted(files):
                if file.endswith('.c'):
                    file_path = os.path.join(root, file)
                    rel_path = os.path.relpath(file_path, project_root)
                    with open(file_path, 'rb') as f:
                        source_code = f.read()
                    tree = parser.parse(source_code)
                    results = []
                    find_transitions(tree.root_node, source_code, rel_path, results)
                    if results:
                        all_transitions[rel_path] = results

    print("# State Transitions (state/status/mode/active/valid/etc)")
    total = 0
    for path, trans in sorted(all_transitions.items()):
        print(f"{path}: {' '.join(trans)}")
        total += len(trans)
    print(f"# Total: {total} transitions in {len(all_transitions)} files")

if __name__ == "__main__":
    main()
