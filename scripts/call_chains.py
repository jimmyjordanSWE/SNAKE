
import os
import sys
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

    # Note: recursing into children is mostly handled by find_calls for the body 
    # but we need to find nested function definitions (unlikely in C) or just top level
    
    if node.type == 'translation_unit':
        for child in node.children:
             analyze_function(child, source_code, None, call_graph)


def main():
    parser = get_c_parser()
    project_root = os.getcwd()
    call_graph = defaultdict(set) # caller -> {callees}
    
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
                        analyze_function(tree.root_node, source_code, None, call_graph)
        else:
            pass
    
    # DFS for chains > 5
    def dfs(node, path, visited):
        if len(path) > 5:
            print(f" D: {'->'.join(path)}")
            return
        
        if node in visited:
            if node in path:
                 print(f" R: {'->'.join(path)}->{node}")
            return
            
        visited.add(node)
        if node in call_graph:
            for callee in call_graph[node]:
                dfs(callee, path + [callee], visited.copy())

    print("Calls:")
    if 'main' in call_graph:
        dfs('main', ['main'], set())
    else:
        print(" !main")

if __name__ == "__main__":
    main()
