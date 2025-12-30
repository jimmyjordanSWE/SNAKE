import os
import sys

# Optional dependencies
try:
    import tiktoken
except ImportError:
    tiktoken = None

try:
    import anthropic
except ImportError:
    anthropic = None

try:
    import google.generativeai as genai
except ImportError:
    genai = None

def count_openai_tokens(text):
    if not tiktoken:
        return None
    try:
        enc = tiktoken.get_encoding("o200k_base")
    except Exception:
        enc = tiktoken.get_encoding("cl100k_base")
    return len(enc.encode(text))

def count_anthropic_tokens(text):
    # Anthropic's library usually needs a client for count_tokens which might hit API.
    # As a fallback, use 1.1x OpenAI count or char-based heuristic for code.
    # Recently, anthropic doesn't provide a trivial offline tokenizer without the Rust core.
    if not anthropic:
        return None
    # Heuristic for Claude 3: close to OpenAI but slightly different.
    # Lacking a trivial offline method, we'll use a conservative estimate based on character count.
    # tokens ~= chars / 3.5 for code
    return int(len(text) / 3.5)

def count_google_tokens(text):
    if not genai:
        return None
    # Gemini tokenization is also usually server-side in the library.
    # Heuristic for Gemini: tokens ~= chars / 4
    return int(len(text) / 4.0)

def main():
    project_root = os.getcwd()
    ignored_dirs = {'.venv', 'build', '.git', 'vendor', 'node_modules', 'bin', 'obj', 'scripts'}
    
    total_chars = 0
    file_contents = []
    
    for root, dirs, files in os.walk(project_root):
        dirs[:] = [d for d in dirs if d not in ignored_dirs and d != 'vendor']
        # Do not include src/vendor
        if 'src/vendor' in root:
            continue
            
        for file in files:
            if file.endswith(('.c', '.h')):
                file_path = os.path.join(root, file)
                try:
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                        total_chars += len(content)
                        file_contents.append(content)
                except Exception:
                    pass

    full_text = "\n".join(file_contents)
    
    oa_tokens = count_openai_tokens(full_text)
    ant_tokens = count_anthropic_tokens(full_text)
    goog_tokens = count_google_tokens(full_text)
    
    print("TOKEN ANALYSIS (EXCLUDING VENDOR)")
    print(f"Total Files Analyzed: {len(file_contents)}")
    print(f"Total Characters:     {total_chars}")
    print("-" * 30)
    
    if oa_tokens is not None:
        print(f"GPT 5.2:                 {oa_tokens} tokens")
    else:
        print("GPT 5.2:                 N/A (tiktoken missing)")
        
    if ant_tokens is not None:
        print(f"Claude Opus 4.5:        {ant_tokens} tokens (est.)")
    else:
        print("Claude Opus 4.5:        N/A (anthropic missing)")
        
    if goog_tokens is not None:
        print(f"Gemini 3 Pro:            {goog_tokens} tokens (est.)")
    else:
        print("Gemini 3 Pro:            N/A (google-generativeai missing)")

if __name__ == "__main__":
    main()
