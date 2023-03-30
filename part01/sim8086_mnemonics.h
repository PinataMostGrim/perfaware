#ifndef SIM8086_MNEMONICS_H
#define SIM8086_MNEMONICS_H


// TODO (Aaron): Define underlying type? uint8_t?
enum operation_types
{
    Op_unknown,
    Op_mov,
    Op_add,
    Op_sub,
    Op_cmp,
    Op_jmp,
    // TODO (Aaron): Add all jumps

    Op_count,
};


enum register_id
{
    Reg_unknown,
    Reg_al,
    Reg_cl,
    Reg_dl,
    Reg_bl,
    Reg_ah,
    Reg_ch,
    Reg_dh,
    Reg_bh,
    Reg_ax,
    Reg_cx,
    Reg_dx,
    Reg_bx,
    Reg_sp,
    Reg_bp,
    Reg_si,
    Reg_di,
    Reg_bx_si,
    Reg_bx_di,
    Reg_bp_si,
    Reg_bp_di,

    Reg_mem_id_count,
};


char const *GetOpMnemonic(operation_types op);
char const *GetRegisterMnemonic(register_id regMemId);
#endif
