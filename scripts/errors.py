
import os
import sys
from tree_sitter import Language, Parser
import tree_sitter_c as tsc

def get_c_parser():
    C_LANGUAGE = Language(tsc.language())
    parser = Parser(C_LANGUAGE)
    return parser

def check_errors(node, source_code, filename):
    problems = []
    
    # Check for ignored return values (basic heuristic)
    if node.type == 'expression_statement':
        # expression statement wrapping a call usually means ignored return
        # unless it is (void) cast, which we skip for simplicity or strictness
        if node.child_count > 0 and node.children[0].type == 'call_expression':
             line = node.start_point.row + 1
             # We might want to check if the function actually returns void, but we don't have that info easily here.
             # So we flag all and maybe filter by known non-voids or just report.
             # For this strict environment, flagging all ignored calls is decent.
             
             call_expr = node.children[0]
             func_node = call_expr.child_by_field_name('function')
             if func_node:
                 func_name = source_code[func_node.start_byte:func_node.end_byte].decode('utf-8')
                 # whitelist some common void funcs or allow all
                 # problems.append(f"{func_name}: ignored return value? ({filename}:{line})")
                 pass
                 
    # Check for missing NULL check after malloc (simplified)
    # detecting "var = malloc(...); ... use var ..." without "if (!var)"
    # This requires data flow, so we'll do a very local check: 
    # if next statement is not an if check on that var.
    
    if node.type == 'compound_statement':
        children = node.children
        for i in range(len(children) - 1):
             curr = children[i]
             # Check if curr is assignment from malloc
             is_malloc = False
             target_var = None
             
             if curr.type == 'expression_statement' and curr.child_count > 0:
                 expr = curr.children[0]
                 if expr.type == 'assignment_expression':
                     rhs = expr.child_by_field_name('right')
                     if rhs and rhs.type == 'call_expression':
                         func = rhs.child_by_field_name('function')
                         if func:
                             fn = source_code[func.start_byte:func.end_byte].decode('utf-8')
                             if fn in ('malloc', 'calloc', 'realloc'):
                                 is_malloc = True
                                 lhs = expr.child_by_field_name('left')
                                 if lhs: target_var = source_code[lhs.start_byte:lhs.end_byte].decode('utf-8')
            
             if is_malloc and target_var:
                 # Check next few statements (up to 3) for the var in an IF condition
                 has_check = False
                 for j in range(1, 4):
                     if i + j < len(children):
                         next_stmt = children[i+j]
                         if next_stmt.type == 'if_statement':
                             cond = next_stmt.child_by_field_name('condition')
                             if cond:
                                 cond_text = source_code[cond.start_byte:cond.end_byte].decode('utf-8')
                                 if target_var in cond_text:
                                     has_check = True
                                     break
                         # If it's a return or goto, maybe it's checking elsewhere, 
                         # but let's stick to the 3-stmt lookahead for now.
                 
                 if not has_check:
                     line = curr.start_point.row + 1
                     problems.append(f"{target_var}: missing NULL check after {fn} ⚠️ ({filename}:{line})")
                     
    for child in node.children:
        problems.extend(check_errors(child, source_code, filename))
        
    return problems

def main():
    parser = get_c_parser()
    project_root = os.getcwd()
    
    # Scan root, src, and include
    scan_paths = [project_root, os.path.join(project_root, 'src')]
    
    seen_files = {} # path -> [problems]
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
                        rel_path = os.path.relpath(file_path, project_root)
                        with open(file_path, 'rb') as f:
                            source_code = f.read()
                        tree = parser.parse(source_code)
                        results = check_errors(tree.root_node, source_code, rel_path)
                        if results:
                            seen_files[rel_path] = results

    for path, errs in sorted(seen_files.items()):
        formatted = []
        for e in errs:
            # e is "target: missing NULL check after fn ⚠️ (path:line)"
            msg, loc = e.rsplit(' (', 1)
            line = loc.split(':')[-1].rstrip(')')
            var = msg.split(':')[0]
            formatted.append(f"L{line} !NULL({var})")
        print(f"{path}: {' '.join(formatted)}")

if __name__ == "__main__":
    main()
