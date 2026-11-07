#!/usr/bin/env python3
"""Tidy a spliced C file: include the project headers it needs, drop
declarations those headers already provide, drop declarations nothing uses,
and order types before other declarations.

    python tools/tidy_decls.py src/main/foo.c [...]
"""
import sys, re
sys.path.insert(0, 'tools'); import splice
HEADERS = ['psyq.h', 'main/memory.h', 'main/filesystem.h', 'main/task.h', 'main/game_state.h', 'main/menu.h', 'main/battle.h', 'main/sprite.h']
provided = {}
for h in HEADERS:
    for it in splice.top_level_items(open('include/' + h).read()):
        n = splice.declared_name(it)
        if n and not it.startswith('#'):
            provided[n] = h
def tidy(path):
    text = open(path).read()
    items = splice.top_level_items(text)
    # Functions, INCLUDE_* lines and initialized data definitions keep their
    # place: their order sets the order of .text/.rodata/.data/.sdata.
    def initialized(it):
        code = splice.strip_comments(it).strip()
        return '=' in code.split('{')[0] and not code.startswith(('typedef', 'extern', '#'))
    def conditional(it):
        return re.match(r'#\s*(if|ifdef|ifndef|elif|else|endif)\b', splice.strip_comments(it).strip()) is not None
    body = [it for it in items if splice.function_name(it) or initialized(it) or conditional(it)
            or it.lstrip().startswith(('INCLUDE_ASM', 'INCLUDE_RODATA'))]
    decls = [it for it in items if it not in body and not splice.strip_comments(it).strip().startswith('#include')]
    m = re.match(r'\s*(/\*.*?\*/)', text, re.S)
    header_comment = m.group(1) if m else ''
    used_text = '\n'.join(body)
    need = set()
    keep = []
    # iterate until stable: a declaration is kept if its name is used by functions or kept decls
    names = {id(d): splice.declared_name(d) for d in decls}
    for d in decls:
        n = names[id(d)]
        if n in provided:
            continue
        keep.append(d)
    changed = True
    while changed:
        changed = False
        ref = used_text + '\n'.join(keep)
        for d in list(keep):
            n = names[id(d)]
            others = ref.replace(d, '')
            if n and not re.search(rf'\b{re.escape(n)}\b', others):
                keep.remove(d); changed = True
    ref = used_text + '\n'.join(keep)
    incs = ['common.h'] + [h for h in HEADERS if any(re.search(rf'\b{re.escape(n)}\b', ref) for n, hh in provided.items() if hh == h)]
    # Keep includes this script doesn't manage (e.g. gte.h, whose macros
    # would otherwise compile as implicit function calls).
    existing = re.findall(r'^#include "([^"]+)"', text, re.M)
    incs += [h for h in existing if h not in incs and h not in HEADERS]
    if 'gte.h' not in incs and re.search(r'\bgte_\w+\(', used_text):
        incs.append('gte.h')
    if 'stack.h' not in incs and re.search(r'\b(STACK_SWITCH|STACK_RESTORE|GET_RA)\(', used_text):
        incs.append('stack.h')
    # header files include their own deps; keep includes deterministic
    out = header_comment + '\n' + ''.join(f'#include "{h}"\n' for h in incs) + '\n'
    macros = [d for d in keep if splice.strip_comments(d).strip().startswith('#')]
    types = [d for d in keep if d not in macros and 'typedef' in splice.strip_comments(d)]
    others = [d for d in keep if d not in types and d not in macros]
    if macros: out += '\n'.join(macros) + '\n\n'
    if types: out += '\n\n'.join(types) + '\n\n'
    if others: out += '\n'.join(others) + '\n\n'
    out += '\n\n'.join(body) + '\n'
    open(path, 'w').write(out)
    print(f'{path}: kept {len(keep)} of {len(decls)} declarations; includes {incs}')
for p in sys.argv[1:]:
    tidy(p)
