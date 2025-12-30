
import os
from collections import defaultdict
from tree_sitter import Language, Parser
import tree_sitter_c as tsc

def get_c_parser():
    C_LANGUAGE = Language(tsc.language())
    parser = Parser(C_LANGUAGE)
    return parser

def find_calls(node, source_code, calls_from_function):
    if node.type == 'call_expression':
        func_node = node.child_by_field_name('function')
        if func_node:
           func_name = source_code[func_node.start_byte:func_node.end_byte].decode('utf-8')
           calls_from_function.add(func_name)
           
    for child in node.children:
        find_calls(child, source_code, calls_from_function)

def analyze_function(node, source_code, current_function_name, call_graph):
    if node.type == 'function_definition':
        decl = node.child_by_field_name('declarator')
        while decl and decl.type in ('pointer_declarator', 'parenthesized_declarator'):
             if decl.child_count > 0: decl = decl.children[0]
             
        if decl and decl.type == 'function_declarator':
             fname_node = decl.child_by_field_name('declarator')
             if fname_node: 
                 current_function_name = source_code[fname_node.start_byte:fname_node.end_byte].decode('utf-8')
    
    if current_function_name:
         calls = set()
         find_calls(node, source_code, calls)
         if calls:
             if current_function_name not in call_graph:
                 call_graph[current_function_name] = set()
             call_graph[current_function_name].update(calls)

    if node.type == 'translation_unit':
        for child in node.children:
             analyze_function(child, source_code, None, call_graph)


def main():
    parser = get_c_parser()
    project_root = os.getcwd()
    call_graph = defaultdict(set)
    func_to_module = {}
    ignored_dirs = {'.venv', 'build', '.git', 'vendor', 'node_modules', 'bin', 'obj'}
    
    included_dirs = {'src', 'include'}
    for d in sorted(included_dirs):
        dir_path = os.path.join(project_root, d)
        if not os.path.exists(dir_path): continue
        for root, _, files in os.walk(dir_path):
            for file in sorted(files):
                if file.endswith('.c'):
                    file_path = os.path.join(root, file)
                    module = os.path.splitext(file)[0]
                    with open(file_path, 'rb') as f:
                        source_code = f.read()
                    tree = parser.parse(source_code)
                    
                    # First pass: find all function definitions in this file
                    def find_defs(node):
                        if node.type == 'function_definition':
                            decl = node.child_by_field_name('declarator')
                            while decl and decl.type in ('pointer_declarator', 'parenthesized_declarator'):
                                 if decl.child_count > 0: decl = decl.children[0]
                            if decl and decl.type == 'function_declarator':
                                 fname_node = decl.child_by_field_name('declarator')
                                 if fname_node:
                                     func_name = source_code[fname_node.start_byte:fname_node.end_byte].decode('utf-8')
                                     func_to_module[func_name] = module
                        for child in node.children: find_defs(child)
                    find_defs(tree.root_node)
                    
                    # Second pass: analyze calls
                    analyze_function(tree.root_node, source_code, None, call_graph)

    def get_module(fn):
        return func_to_module.get(fn, 'stdlib')
    
    module_graph = defaultdict(set)
    for caller, callees in call_graph.items():
        caller_mod = get_module(caller)
        for callee in callees:
            callee_mod = get_module(callee)
            if callee_mod != caller_mod and callee_mod != 'stdlib':
                module_graph[caller_mod].add(callee_mod)
    
    print("Flow:")
    for mod in sorted(module_graph.keys()):
        targets = sorted(module_graph[mod])
        print(f" {mod}->{','.join(targets)}")
    
    print("\nTree(main):")
    global_expanded = set()
    def print_tree(fn, indent, path):
        if fn in path:
            print(f"{'  '*indent}{fn} (recursive)")
            return
        if fn in global_expanded:
            print(f"{'  '*indent}{fn} (see above)")
            return

        callees = []
        if fn in call_graph:
            callees = sorted(call_graph[fn])
            callees = [c for c in callees if c in call_graph or get_module(c) != 'stdlib']
        
        if not callees:
            print(f"{'  '*indent}{fn}")
            return

        print(f"{'  '*indent}{fn}")
        global_expanded.add(fn)
        new_path = path | {fn}
        for c in callees:
            print_tree(c, indent + 1, new_path)
    
    if 'main' in call_graph:
        print_tree('main', 1, set())
    else:
        print("  !main")
    
if __name__ == "__main__":
    main()
