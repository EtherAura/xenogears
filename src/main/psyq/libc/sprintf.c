/*
 * PSY-Q 4.0 LIBC.LIB SPRINTF.OBJ: sprintf. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

#define va_start(ap, last) (*&(ap) = (char*)(&(last) + 1))
#define va_arg(ap, type) (*(type*)((ap += sizeof(type)) - sizeof(type)))

typedef char* va_list;

typedef struct {
    u32 minus : 1;   /* 0x01 - */
    u32 plus : 1;    /* 0x02 + */
    u32 alt : 1;     /* 0x04 # */
    u32 zero : 1;    /* 0x08 0 */
    u32 hasPrec : 1; /* 0x10 .prec given */
    u32 h : 1;       /* 0x20 short */
    u32 l : 1;       /* 0x40 long */
    u32 L : 1;       /* 0x80 */
    u32 sign : 8;    /* sign/space character, 0 if none */
    s32 width;
    s32 prec;
} Spec;

/* SPRINTF.OBJ .data (0x800563F0): the empty conversion spec. */
Spec zeroSprintfSpec = {0};
s32 strlen(const char* s);
void* memchr(const void* s, s32 c, s32 n);
void* memmove(void* dst, const void* src, s32 n);

s32 sprintf(char* buf, char* fmt, ...) {
    char tmp[512];
    Spec spec;
    va_list ap;
    s32 n;
    s32 c;
    u32 v;
    char* p;
    s32 len;
    const char* digits;
    char* q;

    va_start(ap, fmt);
    n = 0;
    for (c = *fmt; c != 0; c = *++fmt) {
        if (c != '%') {
            goto literal;
        }
        spec = zeroSprintfSpec;
        for (;;) {
            c = *++fmt;
            if (c == '-') {
                spec.minus = 1;
            } else if (c == '+') {
                spec.plus = 1;
            } else if (c == ' ') {
                spec.sign = c;
            } else if (c == '#') {
                spec.alt = 1;
            } else if (c == '0') {
                spec.zero = 1;
            } else {
                break;
            }
        }
        if (c == '*') {
            spec.width = va_arg(ap, s32);
            if (spec.width < 0) {
                spec.width = -spec.width;
                spec.minus = 1;
            }
            c = *++fmt;
        } else {
            while ((u32)(c - '0') < 10) {
                spec.width = spec.width * 10 + (c - '0');
                c = *++fmt;
            }
        }
        if (c == '.') {
            c = *++fmt;
            if (c == '*') {
                spec.prec = va_arg(ap, s32);
                c = *++fmt;
            } else {
                while ((u32)(c - '0') < 10) {
                    spec.prec = spec.prec * 10 + (c - '0');
                    c = *++fmt;
                }
            }
            if (spec.prec >= 0) {
                spec.hasPrec = 1;
            }
        }
        p = tmp + sizeof(tmp);
        if (spec.minus) {
            spec.zero = 0;
        }
    again:
        switch (c) {
        case 'h':
            spec.h = 1;
            c = *++fmt;
            goto again;
        case 'l':
            spec.l = 1;
            c = *++fmt;
            goto again;
        case 'L':
            spec.L = 1;
            c = *++fmt;
            goto again;
        case 'd':
        case 'i':
            v = va_arg(ap, s32);
            do {
                if (spec.h) {
                    v = (s16)v;
                }
            } while (0);
            if ((s32)v < 0) {
                v = -v;
                spec.sign = '-';
            } else if (spec.plus) {
                spec.sign = '+';
            }
            goto decimal;
        case 'u':
            v = va_arg(ap, u32);
            do {
                if (spec.h) {
                    v = (u16)v;
                }
            } while (0);
            spec.sign = 0;
        decimal:
            if (!spec.hasPrec) {
                if (spec.zero) {
                    spec.prec = spec.width;
                    if (spec.sign) {
                        spec.prec = spec.width - 1;
                    }
                }
                if (spec.prec <= 0) {
                    spec.prec = 1;
                }
            }
            len = 0;
            while (v != 0) {
                *--p = v % 10 + '0';
                v /= 10;
                len++;
            }
            for (; len < spec.prec; len++) {
                *--p = '0';
            }
            if (spec.sign) {
                *--p = spec.sign;
                len++;
            }
            break;
        case 'o':
            v = va_arg(ap, u32);
            do {
                if (spec.h) {
                    v = (u16)v;
                }
            } while (0);
            if (!spec.hasPrec) {
                if (spec.zero) {
                    spec.prec = spec.width;
                }
                if (spec.prec <= 0) {
                    spec.prec = 1;
                }
            }
            len = 0;
            while (v != 0) {
                *--p = (v & 7) + '0';
                v >>= 3;
                len++;
            }
            if (spec.alt && len != 0 && *p != '0') {
                *--p = '0';
                len++;
            }
            for (; len < spec.prec; len++) {
                *--p = '0';
            }
            break;
        case 'p':
            spec.prec = 8;
            spec.hasPrec = 1;
            spec.l = 1;
        case 'X':
            digits = "0123456789ABCDEF";
            goto hex;
        case 'x':
            digits = "0123456789abcdef";
        hex:
            v = va_arg(ap, u32);
            do {
                if (spec.h) {
                    v = (u16)v;
                }
            } while (0);
            if (!spec.hasPrec) {
                if (spec.zero) {
                    spec.prec = spec.width;
                    if (spec.alt) {
                        spec.prec = spec.width - 2;
                    }
                }
                if (spec.prec <= 0) {
                    spec.prec = 1;
                }
            }
            len = 0;
            while (v != 0) {
                *--p = digits[v & 0xF];
                v >>= 4;
                len++;
            }
            for (; len < spec.prec; len++) {
                *--p = '0';
            }
            if (spec.alt) {
                *--p = c;
                *--p = '0';
                len += 2;
            }
            break;
        case 'c':
            *--p = va_arg(ap, s32);
            len = 1;
            break;
        case 's':
            p = va_arg(ap, char*);
            if (spec.alt) {
                len = *(u8*)p++;
                if (spec.hasPrec && spec.prec < len) {
                    len = spec.prec;
                }
            } else if (!spec.hasPrec) {
                len = strlen(p);
            } else {
                q = memchr(p, 0, spec.prec);
                if (q != 0) {
                    len = q - p;
                } else {
                    len = spec.prec;
                }
            }
            break;
        case 'n':
            p = va_arg(ap, char*);
            if (spec.h) {
                *(s16*)p = n;
            } else if (spec.l) {
                *(s32*)p = n;
            } else {
                *(s32*)p = n;
            }
            continue;
        default:
            if (c != '%') {
                goto end;
            }
        literal:
            buf[n++] = c;
            continue;
        }
        if (len < spec.width && !spec.minus) {
            do {
                buf[n++] = ' ';
                spec.width--;
            } while (len < spec.width);
        }
        memmove(buf + n, p, len);
        n += len;
        for (; len < spec.width; len++) {
            buf[n++] = ' ';
        }
    }
end:
    buf[n] = 0;
    return n;
}
