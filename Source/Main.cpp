#include <Windows.h>
#include <stdio.h>

#include "General.h"

typedef void (*JITEntry)();

enum {
    CODE_SIZE = KiloBytes(4),
    DATA_SIZE = KiloBytes(4),
};

enum {
    RAX, RCX, RDX, RBX,
    RSP, RBP, RSI, RDI,
    R8, R9, R10, R11,
    R12, R13, R14, R15
};
typedef u8 Register;

enum : u8 {
    INDIRECT,
    INDIRECT_8,
    INDIRECT_32,
    DIRECT
};

enum {
    SCALE_1,
    SCALE_2,
    SCALE_4,
    SCALE_8
};

enum {
    REXW = 0x48,
};

enum : u8 {
    JB = 0x2,
    JAE = 0x3,
    JE = 0x4,
    JNE = 0x5,
    JBE = 0x6,
    JA = 0x7,
    JL = 0xc,
    JGE = 0xd,
    JLE = 0xe,
    JG = 0xf,
};
typedef u8 Condition;

struct RegMem {
    u8 mod;
    Register reg;
    Register rm;
    union {
        s8 disp8;
        s32 disp32;
    };
    u8 scale;
    u8 index;
    u8 base;
};

global u8 *code;
global u8 *code_end;
global u8 *wptr;

global u8 *data;
global u8 *data_end;
global u8 *data_wptr;

void EmitCode(u64 bytes, int cnt) {
    Assert(wptr + cnt < code_end);
    *((u64 *)wptr) = bytes;
    wptr += cnt;
}

void EmitREX(RegMem rm) {
    // r8=8, so >=r8 have the fourth bit set, which means we can use it to figure out if we need the rex bit set
    EmitCode(REXW | ((rm.reg >> 3) << 2) | ((rm.rm >> 3)), 1);
}

void EmitREXW() {
    EmitCode(REXW, 1);
}

void EmitOpcode(u64 opcode) {
    EmitCode(opcode, 1);
}

void EmitModRM(RegMem rm) {
    EmitCode(((rm.mod & 0b11) << 6) | ((rm.reg & 0b111) << 3) | (rm.rm & 0b111), 1);

    if (rm.rm == RSP && rm.mod != DIRECT) {
        EmitCode(((rm.scale & 0b11) << 6) | ((rm.index & 0b111) << 3) | (rm.base & 0b111), 1);
    }

    if (rm.mod == INDIRECT_8) {
        EmitCode(u64(rm.disp8), 1);
    } else if (rm.mod == INDIRECT_32) {
        EmitCode(u64(rm.disp32), 4);
    }
}

void EmitModRMOpcode(RegMem rm, u64 opcode) {
    EmitREX(rm);
    EmitOpcode(opcode);
    EmitModRM(rm);
}

u8 *WriteStringToData(const char *str) {
    u8 *result = data_wptr;

    int len = strlen(str);
    for (int i = 0; i < len; ++i) {
        result[i] = str[i];
    }
    result[len] = 0;
    data_wptr += len + 1;

    return result;
}

// The abbreviation after INST_ stands for the encoding listed in the intel instruction set manual
#define INST_ZO(name, opcode) void name() { \
    EmitOpcode(opcode); \
}

#define INST_O(name, opcode) void name(Register reg) { \
    EmitOpcode(opcode + reg); \
}

#define INST_D(name, opcode, offset_len) void name(s32 offset) { \
    EmitOpcode(opcode); \
    EmitCode(offset, offset_len); \
}

#define INST_OI(name, opcode) void name(Register reg, u64 imm) { \
    EmitREXW(); \
    EmitOpcode(opcode + reg); \
    EmitCode(imm, 8); \
}

#define INST_RM(name, opcode) void name(RegMem rm) { \
    EmitModRMOpcode(rm, opcode); \
}

// they have a set value for the reg field (regv) of modrm, defined by /<number> in the manual
#define INST_M(name, opcode, regv) void name(RegMem rm) { \
    rm.reg = regv; \
    EmitOpcode(opcode); \
    EmitModRM(rm); \
}

#define INST_MI(name, opcode, regv) void name(RegMem rm, u32 imm) { \
    rm.reg = regv; \
    EmitModRMOpcode(rm, opcode); \
    EmitCode(imm, 4); \
}

INST_O(Push, 0x50)
INST_O(Pop, 0x58)
INST_ZO(Ret, 0xc3)

INST_OI(MovRegImm, 0xb8)
INST_MI(MovRMImm, 0xc7, 0)
INST_RM(MovRMReg, 0x89)
INST_RM(MovRegRM, 0x8b)

INST_MI(AddRMImm, 0x81, 0)
INST_RM(AddRMReg, 0x01)
INST_RM(AddRegRM, 0x03)

INST_MI(SubRMImm, 0x81, 5)
INST_RM(SubRMReg, 0x29)
INST_RM(SubRegRM, 0x2b)

INST_MI(CmpRMImm, 0x81, 7)
INST_RM(CmpRMReg, 0x39)
INST_RM(CmpRegRM, 0x3b)

INST_D(JmpRel8, 0xeb, 1)
INST_D(JmpRel32, 0xe9, 4)

INST_M(Call, 0xff, 2)

void JmpCond(Condition cond, u8 *target) {
    s32 max_offset = target - (wptr + 6);
    if (u32(max_offset) < 255) {
        EmitOpcode(0x70 + cond);
        EmitCode(target - (wptr + 1), 1);
    } else {
        EmitOpcode(0x0f);
        EmitOpcode(0x80 + cond);
        EmitCode(target - (wptr + 4), 4);
    }
}

int main() {
    code = (u8 *) VirtualAlloc(0, CODE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    wptr = code;
    code_end = code + CODE_SIZE;

    data = (u8 *) VirtualAlloc(0, DATA_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    data_wptr = data;
    data_end = data + DATA_SIZE;

    u8 *int_format_string = WriteStringToData("%d\n");
    u8 *printf_addr = (u8 *) printf;

    Push(RBP);
    MovRegRM({.mod=DIRECT, .reg=RBP, .rm=RSP});

    int a[5] = {5, 2, 3, 1, 4};
    for (int i = 0; i < 5; ++i) {
        MovRMImm({.mod=INDIRECT_8, .rm=RBP, .disp8=s8(-0x40+i*8)}, a[i]);
    }

    MovRMImm({.mod=INDIRECT_8, .rm=RBP, .disp8=-0x8}, 0);
    MovRMImm({.mod=INDIRECT_8, .rm=RBP, .disp8=-0x10}, 0);

    JmpRel8(0);
    u8 *to_patch = wptr - 1;

    u8 *target = wptr;
    MovRegRM({.mod=INDIRECT_8, .reg=RAX, .rm=RBP, .disp8=-0x10});
    MovRegRM({.mod=INDIRECT_8, .reg=RAX, .rm=RSP, .disp8=-0x40, .scale=SCALE_8, .index=RAX, .base=RBP});
    AddRMReg({.mod=INDIRECT_8, .reg=RAX, .rm=RBP, .disp8=-0x8});
    AddRMImm({.mod=INDIRECT_8, .rm=RBP, .disp8=-0x10}, 0x1);

    *to_patch = wptr - (to_patch + 1);
    CmpRMImm({.mod=INDIRECT_8, .rm=RBP, .disp8=-0x10}, 0x4);
    JmpCond(JLE, target);

    MovRegRM({.mod=INDIRECT_8, .reg=RDX, .rm=RBP, .disp8=-0x8});
    MovRegImm(RCX, u64(int_format_string));
    MovRegImm(RAX, u64(printf_addr));
    Call({.mod=DIRECT, .rm=RAX});

    Pop(RBP);
    Ret();

    JITEntry Entry = (JITEntry)code;
    Entry();

    return 0;
}
