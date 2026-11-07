#!/usr/bin/env python3
"""Split a C file into several at function names (a new source-file boundary).

    python tools/split_c.py SPEC.json

SPEC is JSON:
    {"path": "src/mod/x_80001000.c",
     "drop": ["exact text removed from the file first", ...],
     "parts": [{"at": "func_80002000", "name": "x_80002000",
                "header": "/* file comment */\n",
                "kr": {"func_80001234": "void"}}, ...]}

Each new file starts at its "at" function (with the comment lines above it)
and gets: its header, the original file's declarations up to the first
function, every top-level declaration that preceded its cut (outside #if
blocks), and prototypes for the functions defined in earlier parts that it
uses (a definition was their prototype in the single file). "kr" lists
functions this file calls without a prototype: their prototypes are left
out, their NAME_np( and ((T (*)())NAME)( calls become plain calls, and a
K&R declaration "T NAME();" is added. Nonmatching paths are renamed. Then
add the subsegments to both versions' yamls (tools/jp_layout.py maps JP
offsets), configure, and check with ninja.
"""
import re, sys, json
spec=json.load(open(sys.argv[1]))
p=spec['path']
t=open(p).read()
for d in spec.get('drop',[]):
    assert t.count(d)==1, d
    t=t.replace(d,'')
s=t.split('\n')
old=p.split('/')[-1][:-2]
def defline(i):
    return re.match(r'^([A-Za-z_][\w \*]*?\b(\w+))\((.*)\)\s*\{\s*$',s[i]) or re.match(r'^INCLUDE_ASM\("[^"]+", (\w+)\);',s[i])
first=next(i for i in range(len(s)) if defline(i))
while s[first-1].startswith('/*') or s[first-1].startswith(' *'): first-=1
inc=next(i for i,l in enumerate(s) if l.startswith('#include'))
top=s[inc:first]
cuts=[]
for part in spec['parts']:
    c=next(i for i,l in enumerate(s) if re.match(r'^[A-Za-z_][\w \*]*\b%s\(.*\)\s*\{\s*$'%part['at'],l) or re.match(r'^INCLUDE_ASM\("[^"]+", %s\);'%part['at'],l))
    while s[c-1].startswith('/*') or s[c-1].startswith(' *'): c-=1
    cuts.append(c)
bounds=[0]+cuts+[len(s)]
decl_re=re.compile(r'^(extern |#include|#define|[A-Za-z_][A-Za-z0-9_ \*]*\(.*\);)')
strip=lambda l: re.sub(r"/\*.*?\*/|\"(?:\\.|[^\"\\])*\"|'(?:\\.|[^'\\])*'","",l)
# top-level declarations and definitions by line
mid=[]; defs={}; depth=0; cond=0
for i in range(first,len(s)):
    l=s[i]
    if re.match(r'^#\s*if',l): cond+=1
    elif re.match(r'^#\s*endif',l): cond-=1
    if depth==0 and cond==0:
        if decl_re.match(l) and not l.startswith('INCLUDE_'): mid.append((i,l))
        m=re.match(r'^([A-Za-z_][\w \*]*?\b(\w+))\((.*)\)\s*\{\s*$',l)
        if m: defs[m.group(2)]=(i,l.rstrip(' {').rstrip()+';')
    depth+=strip(l).count('{')-strip(l).count('}')
out={}
out[old]='\n'.join(s[:cuts[0]]).rstrip('\n')+'\n'
for k,part in enumerate(spec['parts']):
    a,b=bounds[k+1],bounds[k+2]
    body='\n'.join(s[a:b])
    m=[]
    for i,l in mid:
        if i<a and l not in top and l not in m: m.append(l)
    decls=top+m
    kr=part.get('kr',{})
    if kr:
        decls=[l for l in decls if not any(re.match(r'^(extern )?[A-Za-z_][\w \*]*\b%s\(.*\);'%f,l) or re.match(r'^#define %s_np\b'%f,l) for f in kr)]
        for f in kr:
            body=body.replace(f+'_np(',f+'(')
            body=re.sub(r'\(\((?:s32|void|u16|u8|u32) \(\*\)\(\)\)%s\)\('%f,f+'(',body)
    declared='\n'.join(decls)
    protos=[d for f,(i,d) in defs.items() if i<a and f not in kr and re.search(r'\b%s\b'%f,body) and not re.search(r'\b%s\s*\('%f,declared)]
    text=part['header']+'\n'.join(decls).rstrip('\n')+'\n'
    if kr:
        text+=('\n/* This file calls these without prototypes: retail passes unmasked ints\n'
               ' * for their u8/u16 parameters and uses their results unmasked. */\n'
               + ''.join(f'{r} {f}();\n' for f,r in kr.items()))
    if protos:
        text+=f'\n/* defined in earlier files */\n'+'\n'.join(protos)+'\n'
    text+='\n'+body
    text=text.replace(f'/nonmatchings/{old}"',f'/nonmatchings/{part["name"]}"').rstrip('\n')+'\n'
    out[part['name']]=text
for n,tx in out.items():
    open(p.rsplit('/',1)[0]+'/'+n+'.c','w').write(tx)
    print(n, tx.count('\n'), 'lines')
