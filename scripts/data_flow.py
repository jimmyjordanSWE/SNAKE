
import os
import sys
from collections import defaultdict
from tree_sitter import Language, Parser
import tree_sitter_c as tsc

def get_c_parser():
    C_LANGUAGE = Language(tsc.language())
    parser = Parser(C_LANGUAGE)
    return parser

def find_struct_access(node, source_code, accesses, parent_struct=None):
    # This is a heuristic. We identify "struct.field" or "struct->field" patterns.
    # We assign "read" or "write" based on whether it's on the LHS of an assignment.
    
    if node.type == 'field_expression':
        # container.field or container->field
        # check if this field_expression is LHS of assignment
        parent = node.parent
        mode = "read"
        if parent and parent.type == 'assignment_expression':
            lhs = parent.child_by_field_name('left')
            if lhs == node:
                mode = "write"
        
        # Extract field name
        field = node.child_by_field_name('field')
        if field:
             field_name = source_code[field.start_byte:field.end_byte].decode('utf-8')
             arg = node.child_by_field_name('argument')
             if arg:
                 struct_var = source_code[arg.start_byte:arg.end_byte].decode('utf-8')
                 accesses[struct_var].append(f"{field_name} ({mode})")

    for child in node.children:
        find_struct_access(child, source_code, accesses)

def main():
    parser = get_c_parser()
    project_root = os.getcwd()
    
    # Scan root and src
    scan_paths = [project_root, os.path.join(project_root, 'src')]
    
    seen_files = set()
    print("Data Flow (Struct Access):")
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
                        accesses = defaultdict(lambda: {'read': 0, 'write': 0})
                        
                        # We need to adapt find_struct_access to use this dict structure or just use list
                        # The original used defaultdict(list). Let's stick to that and summarize in main.
                        raw_accesses = defaultdict(list)
                        find_struct_access(tree.root_node, source_code, raw_accesses)
                        
                        if raw_accesses:
                            print(f" {rel_path}:")
                            for struct, ops in raw_accesses.items():
                                reads = [o for o in ops if 'read' in o]
                                writes = [o for o in ops if 'write' in o]
                                if reads or writes:
                                    print(f"  {struct}: r({len(reads)}) w({len(writes)})")
        else:
            pass

if __name__ == "__main__":
    main()
