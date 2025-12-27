
import os
from tree_sitter import Language, Parser
import tree_sitter_c as tsc

# Functions known to return void or where ignoring return is OK
VOID_FUNCS = {
    # stdlib void
    'free', 'memset', 'memcpy', 'memmove', 'bzero', 'qsort', 'va_start', 'va_end',
    'exit', '_exit', 'abort', 'assert', 'perror', 'close', 'fclose', 'shutdown',
    # stdio (often ignored)
    'printf', 'fprintf', 'snprintf', 'sprintf', 'vsnprintf', 'puts', 'fputs', 'putc', 'fputc',
    # string
    'strcat', 'strcpy', 'strncpy', 'strncat',
    # SDL2 (void or ignore-ok)
    'SDL_Quit', 'SDL_DestroyWindow', 'SDL_DestroyRenderer', 'SDL_DestroyTexture',
    'SDL_Delay', 'SDL_PumpEvents', 'SDL_SetRenderDrawColor', 'SDL_RenderClear',
    'SDL_RenderPresent', 'SDL_FreeSurface', 'SDL_RenderFillRect', 'SDL_ShowWindow',
    'SDL_SetWindowAlwaysOnTop', 'SDL_RaiseWindow', 'SDL_SetHint', 'SDL_UpdateTexture',
    'SDL_RenderCopy',
    # stb
    'stbi_image_free',
}

# Set of discovered internal void functions
internal_void_funcs = set()

def get_c_parser():
    C_LANGUAGE = Language(tsc.language())
    parser = Parser(C_LANGUAGE)
    return parser

def is_void_func(name):
    if name in VOID_FUNCS:
        return True
    if name.startswith('_'): # Conventional internal/private
        return True
    if name in internal_void_funcs:
        return True
    return False

def discover_void_funcs(node, source_code):
    if node.type == 'function_definition':
        # Check return type
        type_node = node.child_by_field_name('type')
        if type_node:
            type_text = source_code[type_node.start_byte:type_node.end_byte].decode('utf-8')
            if type_text == 'void':
                decl = node.child_by_field_name('declarator')
                while decl and decl.type in ('pointer_declarator', 'parenthesized_declarator'):
                     if decl.child_count > 0: decl = decl.children[0]
                if decl and decl.type == 'function_declarator':
                     fname_node = decl.child_by_field_name('declarator')
                     if fname_node:
                         func_name = source_code[fname_node.start_byte:fname_node.end_byte].decode('utf-8')
                         internal_void_funcs.add(func_name)
    for child in node.children:
        discover_void_funcs(child, source_code)

def check_errors(node, source_code, filename, stats):
    problems = []
    
    # Ignored return values (only flag non-void externals)
    if node.type == 'expression_statement':
        if node.child_count > 0 and node.children[0].type == 'call_expression':
            call_expr = node.children[0]
            func_node = call_expr.child_by_field_name('function')
            if func_node:
                func_name = source_code[func_node.start_byte:func_node.end_byte].decode('utf-8')
                stats['calls_checked'] += 1
                if not is_void_func(func_name):
                    line = node.start_point.row + 1
                    problems.append(f"L{line}r?({func_name})")
                    
    # Missing NULL check after malloc/calloc/realloc
    # Track pending allocations and check if subsequent if-statement covers any of them
    if node.type == 'compound_statement':
        children = node.children
        pending_allocs = []  # [(line, var_name, idx)]
        
        for i in range(len(children)):
            curr = children[i]
            alloc_info = None
            
            # Check assignment: var = malloc(...)
            if curr.type == 'expression_statement' and curr.child_count > 0:
                expr = curr.children[0]
                if expr.type == 'assignment_expression':
                    rhs = expr.child_by_field_name('right')
                    if rhs and rhs.type == 'call_expression':
                        func = rhs.child_by_field_name('function')
                        if func:
                            fn = source_code[func.start_byte:func.end_byte].decode('utf-8')
                            if fn in ('malloc', 'calloc', 'realloc'):
                                lhs = expr.child_by_field_name('left')
                                if lhs:
                                    var = source_code[lhs.start_byte:lhs.end_byte].decode('utf-8')
                                    alloc_info = (curr.start_point.row + 1, var, i)
                                    stats['allocs_checked'] += 1
            
            # Check declaration: Type* var = malloc(...)
            if curr.type == 'declaration':
                for decl in curr.children:
                    if decl.type == 'init_declarator':
                        init = decl.child_by_field_name('value')
                        if init and init.type == 'call_expression':
                            func = init.child_by_field_name('function')
                            if func:
                                fn = source_code[func.start_byte:func.end_byte].decode('utf-8')
                                if fn in ('malloc', 'calloc', 'realloc'):
                                    declarator = decl.child_by_field_name('declarator')
                                    if declarator:
                                        raw = source_code[declarator.start_byte:declarator.end_byte].decode('utf-8')
                                        # Strip leading * and whitespace for pointer declarations
                                        var = raw.lstrip('* \t')
                                        alloc_info = (curr.start_point.row + 1, var, i)
                                        stats['allocs_checked'] += 1
            
            if alloc_info:
                pending_allocs.append(alloc_info)
            
            # Check if this is an if-statement that clears pending allocs
            if curr.type == 'if_statement' and pending_allocs:
                cond = curr.child_by_field_name('condition')
                if cond:
                    cond_text = source_code[cond.start_byte:cond.end_byte].decode('utf-8')
                    # Remove allocs that are checked in this condition
                    pending_allocs = [(l, v, idx) for l, v, idx in pending_allocs if v not in cond_text]
            
            # return/goto clears all pending (assumes error handling)
            if curr.type in ('return_statement', 'goto_statement'):
                pending_allocs = []
        
        # Report any unchecked allocations
        for line, var, _ in pending_allocs:
            problems.append(f"L{line}n?({var})")
                    
    for child in node.children:
        problems.extend(check_errors(child, source_code, filename, stats))
        
    return problems

def main():
    parser = get_c_parser()
    project_root = os.getcwd()
    ignored_dirs = {'.venv', 'build', '.git', 'vendor', 'node_modules', 'bin', 'obj'}
    
    seen_files = {}
    stats = {'calls_checked': 0, 'allocs_checked': 0, 'files': 0}
    
    file_list = []
    for root, dirs, files in os.walk(project_root):
        dirs[:] = [d for d in dirs if d not in ignored_dirs]
        for file in sorted(files):
            if file.endswith('.c'):
                file_list.append(os.path.join(root, file))

    # Pass 1: Discover internal void functions
    for file_path in file_list:
        with open(file_path, 'rb') as f:
            source_code = f.read()
        tree = parser.parse(source_code)
        discover_void_funcs(tree.root_node, source_code)

    # Pass 2: Check errors
    for file_path in file_list:
        rel_path = os.path.relpath(file_path, project_root)
        with open(file_path, 'rb') as f:
            source_code = f.read()
        tree = parser.parse(source_code)
        results = check_errors(tree.root_node, source_code, rel_path, stats)
        stats['files'] += 1
        if results:
            seen_files[rel_path] = results

    if seen_files:
        for path, errs in sorted(seen_files.items()):
            print(f"{path}: {' '.join(errs)}")
    else:
        print(f"# OK: {stats['files']} files, {stats['allocs_checked']} allocs, {stats['calls_checked']} calls checked")

if __name__ == "__main__":
    main()
