#!/usr/bin/env python3
import sys,io,os,re

EXTS = ['.c','.h','.js']
EXCLUDE_DIRS = {'build','.git','node_modules'}

# Remove C-style (/* */) and C++-style (//) comments but preserve string/char literals

def strip_comments(src):
    out=[]
    i=0
    n=len(src)
    state='code'
    while i<n:
        ch=src[i]
        nxt=src[i+1] if i+1<n else ''
        if state=='code':
            if ch=='"':
                out.append(ch); i+=1; state='string'
            elif ch=="'":
                out.append(ch); i+=1; state='char'
            elif ch=='/' and nxt=='/':
                # skip line comment
                i+=2
                while i<n and src[i] not in '\n': i+=1
            elif ch=='/' and nxt=='*':
                # skip block comment
                i+=2
                while i<n-1 and not (src[i]=='*' and src[i+1]=='/'):
                    i+=1
                i+=2 if i<n else 0
            else:
                out.append(ch); i+=1
        elif state=='string':
            out.append(ch)
            if ch=='\\':
                # escaped char
                if i+1<n: out.append(src[i+1]); i+=2; continue
            if ch=='"': state='code'
            i+=1
        elif state=='char':
            out.append(ch)
            if ch=='\\':
                if i+1<n: out.append(src[i+1]); i+=2; continue
            if ch=="'": state='code'
            i+=1
    return ''.join(out)


def process_file(path):
    with open(path,'r',encoding='utf-8') as f:
        src=f.read()
    new=strip_comments(src)
    if new!=src:
        with open(path,'w',encoding='utf-8') as f:
            f.write(new)
        return True
    return False

changed=[]
for root,dirs,files in os.walk('.'):
    # skip exclude dirs
    parts=root.split(os.sep)
    if any(p in EXCLUDE_DIRS for p in parts):
        continue
    for fn in files:
        _,ext=os.path.splitext(fn)
        if ext.lower() in EXTS:
            path=os.path.join(root,fn)
            if process_file(path): changed.append(path)

print('Modified',len(changed),'files')
for p in changed: print(p)
