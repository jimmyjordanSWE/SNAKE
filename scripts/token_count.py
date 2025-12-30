import os
import sys
import re
import tiktoken
from collections import defaultdict

def count_tokens(text, encoding_name):
    try:
        enc = tiktoken.get_encoding(encoding_name)
    except Exception:
        enc = tiktoken.get_encoding("cl100k_base")
    return len(enc.encode(text))

def get_sloc(content):
    # Remove multi-line comments: /* ... */
    # This regex is non-greedy and handles multi-line blocks
    content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
    
    # Remove single-line comments: // ...
    content = re.sub(r'//.*', '', content)
    
    # Count non-empty, non-whitespace lines
    sloc = 0
    for line in content.splitlines():
        if line.strip():
            sloc += 1
    return sloc

def main():
    project_root = os.getcwd()
    # Inclusion-based approach: only these top-level directories will be scanned
    included_dirs = {'src', 'include'}
    
    total_chars = 0
    total_lines = 0
    file_contents = []
    
    # Store stats per included directory
    dir_stats = defaultdict(lambda: {"files": 0, "lines": 0})
    
    for d in sorted(included_dirs):
        dir_path = os.path.join(project_root, d)
        if not os.path.exists(dir_path):
            continue
            
        for root, _, files in os.walk(dir_path):
            for file in files:
                if file.endswith(('.c', '.h')):
                    file_path = os.path.join(root, file)
                    try:
                        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                            content = f.read()
                            
                            # Calculate SLOC (excluding comments and empty lines)
                            lines = get_sloc(content)
                            
                            total_chars += len(content)
                            total_lines += lines
                            dir_stats[d]["files"] += 1
                            dir_stats[d]["lines"] += lines
                            file_contents.append(content)
                    except Exception:
                        pass

    if not file_contents:
        print("TOKEN ANALYSIS (INCLUSION: src, include)")
        print("No files found in included directories.")
        return

    full_text = "\n".join(file_contents)
    
    gpt_gemini_tokens = count_tokens(full_text, "o200k_base")
    claude_tokens = count_tokens(full_text, "cl100k_base")
    
    print("TOKEN ANALYSIS (INCLUSION: src, include)")
    print(f"Total Files Analyzed: {len(file_contents)}")
    print(f"Total lines of code (after make format-llm): {total_lines}")
    print(f"Total Characters:     {total_chars}")
    print("-" * 30)
    
    # Accurate tokenization using tiktoken encodings
    print(f"GPT 5.2:                 {gpt_gemini_tokens} tokens")
    print(f"Claude Opus 4.5:        {claude_tokens} tokens (est.)")
    print(f"Gemini 3 Pro:            {gpt_gemini_tokens} tokens (est.)")
    print("-" * 30)
    print("BREAKDOWN BY DIRECTORY (SLOC):")
    for d in sorted(included_dirs):
        stats = dir_stats[d]
        if stats["files"] > 0:
            print(f"  {d:15}: {stats['lines']:5} lines ({stats['files']:3} files)")

if __name__ == "__main__":
    main()
