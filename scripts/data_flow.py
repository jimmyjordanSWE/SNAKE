
import os
from collections import defaultdict
from tree_sitter import Language, Parser
import tree_sitter_c as tsc

def get_c_parser():
    C_LANGUAGE = Language(tsc.language())
    parser = Parser(C_LANGUAGE)
    return parser

def infer_type(var_name):
    # Simply strip common operators to get the variable base
    return var_name.split('[')[0].split('-')[0].strip('()*& ')

def find_struct_access(node, source_code, type_accesses):
    if node.type == 'field_expression':
        parent = node.parent
        mode = "r"
        if parent and parent.type == 'assignment_expression':
            lhs = parent.child_by_field_name('left')
            if lhs == node:
                mode = "w"
        
        arg = node.child_by_field_name('argument')
        if arg:
            struct_var = source_code[arg.start_byte:arg.end_byte].decode('utf-8')
            struct_type = infer_type(struct_var)
            type_accesses[struct_type][mode] += 1

    for child in node.children:
        find_struct_access(child, source_code, type_accesses)

def main():
    parser = get_c_parser()
    project_root = os.getcwd()
    ignored_dirs = {'.venv', 'build', '.git', 'vendor', 'node_modules', 'bin', 'obj'}
    
    global_types = defaultdict(lambda: {'r': 0, 'w': 0})
    module_types = defaultdict(lambda: defaultdict(lambda: {'r': 0, 'w': 0}))
    
    included_dirs = {'src', 'include'}
    for d in sorted(included_dirs):
        dir_path = os.path.join(project_root, d)
        if not os.path.exists(dir_path): continue
        for root, _, files in os.walk(dir_path):
            for file in sorted(files):
                if file.endswith('.c'):
                    file_path = os.path.join(root, file)
                    rel_path = os.path.relpath(file_path, project_root)
                    parts = rel_path.split(os.sep)
                    module = parts[0] if len(parts) > 1 else 'root'
                
                    with open(file_path, 'rb') as f:
                        source_code = f.read()
                    tree = parser.parse(source_code)
                    
                    file_types = defaultdict(lambda: {'r': 0, 'w': 0})
                    find_struct_access(tree.root_node, source_code, file_types)
                    
                    for t, counts in file_types.items():
                        global_types[t]['r'] += counts['r']
                        global_types[t]['w'] += counts['w']
                        module_types[module][t]['r'] += counts['r']
                        module_types[module][t]['w'] += counts['w']
    
    print("Data Ownership (struct r/w totals):")
    sorted_types = sorted(global_types.items(), key=lambda x: x[1]['r']+x[1]['w'], reverse=True)
    for t, counts in sorted_types[:15]:
        if counts['r'] + counts['w'] >= 5:
            print(f"  {t}: r({counts['r']}) w({counts['w']})")
    
    print("\nModule Data Usage:")
    for mod in sorted(module_types.keys()):
        types = module_types[mod]
        top = sorted(types.items(), key=lambda x: x[1]['r']+x[1]['w'], reverse=True)[:5]
        type_strs = [f"{t}:r{c['r']}w{c['w']}" for t,c in top if c['r']+c['w'] >= 3]
        if type_strs:
            print(f"  {mod}: {', '.join(type_strs)}")

if __name__ == "__main__":
    main()
