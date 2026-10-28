// =========================================================================
//   Copyright (C) 2009-2026 by Anders Piniesjö <pugo@pugo.org>
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>
// =========================================================================

#include <print>
#include <format>

#include "machine.hpp"
#include "mos6502.hpp"
#include "mos6502_opcodes.hpp"
#include "mos6502_cycles.hpp"


static inline uint16_t read_word_buggy(Machine& m, uint16_t base, auto readb)
{
    // MOS 6502 has a bug that makes JMP ($xxxx) read the high byte from the same page (wraps at $xxFF).
    // Read low from base, high from (base & 0xFF00) | ((base+1) & 0x00FF)
    const uint8_t lo = readb(m, base);
    const uint16_t hi_addr = (base & 0xFF00) | ((base + 1) & 0x00FF);
    const uint8_t hi = readb(m, hi_addr);
    return (uint16_t(hi) << 8) | lo;
}

MOS6502::MOS6502(Machine& a_Machine) :
    A(0),
    X(0),
    Y(0),
    N_INTERN(0),
    Z_INTERN(0),
    V(false),
    B(false),
    D(false),
    I(true),
    C(false),
    PC(0),
    SP(0),
    machine(a_Machine),
    irq_flags(0),
    nmi_flag(false),
    do_interrupt(false),
    do_nmi(false),
    memory_read_byte_handler(nullptr),
    memory_read_byte_zp_handler(nullptr),
    memory_read_word_handler(nullptr),
    memory_read_word_zp_handler(nullptr),
    memory_write_byte_handler(nullptr),
    memory_write_byte_zp_handler(nullptr),
    memory(a_Machine.memory),
    instruction_load(true),
    instruction_cycles(0),
    current_instruction(0),
    current_cycle(0),
    has_breakpoints(false),
    skip_breakpoint(false),
    breakpoint_hit(std::nullopt)
{
}


void MOS6502::reset()
{
    A = 0;
    X = 0;
    Y = 0;
    N_INTERN = 0;
    Z_INTERN = 0;
    V = false;
    B = false;
    D = false;
    I = true;	// Block interrupts after reset.
    C = false;

    PC = memory_read_byte_handler(machine, RESET_VECTOR_L) + (memory_read_byte_handler(machine, RESET_VECTOR_H) << 8);
    SP = 0xff;
    irq_flags = 0;
    nmi_flag = false;
    do_interrupt = false;
    do_nmi = false;

    instruction_load = true;
    instruction_cycles = 0;
    current_instruction = 0;
    current_cycle = 0;
    skip_breakpoint = false;
    breakpoint_hit.reset();
}

void MOS6502::save_to_snapshot(Snapshot& snapshot) const
{
    snapshot.mos6502.A = A;
    snapshot.mos6502.X = X;
    snapshot.mos6502.Y = Y;
    snapshot.mos6502.N_INTERN = N_INTERN;
    snapshot.mos6502.Z_INTERN = Z_INTERN;
    snapshot.mos6502.V = V;
    snapshot.mos6502.B = B;
    snapshot.mos6502.D = D;
    snapshot.mos6502.I = I;
    snapshot.mos6502.C = C;

    snapshot.mos6502.PC = PC;
    snapshot.mos6502.SP = SP;
    snapshot.mos6502.irq_flags = irq_flags;
    snapshot.mos6502.nmi_flag = nmi_flag;
    snapshot.mos6502.do_interrupt = do_interrupt;
    snapshot.mos6502.do_nmi = do_nmi;

    snapshot.mos6502.instruction_load = instruction_load;
    snapshot.mos6502.instruction_cycles = instruction_cycles;
    snapshot.mos6502.current_instruction = current_instruction;
    snapshot.mos6502.current_cycle = current_cycle;
}

void MOS6502::load_from_snapshot(Snapshot& snapshot)
{
    A = snapshot.mos6502.A;
    X = snapshot.mos6502.X;
    Y = snapshot.mos6502.Y;
    N_INTERN = snapshot.mos6502.N_INTERN;
    Z_INTERN = snapshot.mos6502.Z_INTERN;
    V = snapshot.mos6502.V;
    B = snapshot.mos6502.B;
    D = snapshot.mos6502.D;
    I = snapshot.mos6502.I;
    C = snapshot.mos6502.C;

    PC = snapshot.mos6502.PC;
    SP = snapshot.mos6502.SP;
    irq_flags = snapshot.mos6502.irq_flags;
    nmi_flag = snapshot.mos6502.nmi_flag;
    do_interrupt = snapshot.mos6502.do_interrupt;
    do_nmi = snapshot.mos6502.do_nmi;

    instruction_load = snapshot.mos6502.instruction_load;
    instruction_cycles = snapshot.mos6502.instruction_cycles;
    current_instruction = snapshot.mos6502.current_instruction;
    current_cycle = snapshot.mos6502.current_cycle;
}

bool MOS6502::set_breakpoint(uint16_t address)
{
    const auto [_, inserted] = breakpoints.insert(address);
    disabled_breakpoints.erase(address);
    has_breakpoints = !breakpoints.empty();
    return inserted;
}

bool MOS6502::set_breakpoint_enabled(uint16_t address, bool enabled)
{
    if (!breakpoints.contains(address)) {
        return false;
    }

    if (enabled) {
        disabled_breakpoints.erase(address);
    }
    else {
        disabled_breakpoints.insert(address);
    }
    return true;
}

bool MOS6502::breakpoint_enabled(uint16_t address) const
{
    return breakpoints.contains(address) && !disabled_breakpoints.contains(address);
}

bool MOS6502::set_temporary_breakpoint(uint16_t address)
{
    temporary_breakpoints.insert(address);
    breakpoints.insert(address);
    has_breakpoints = true;
    return true;
}

bool MOS6502::clear_breakpoint(uint16_t address)
{
    const bool removed = breakpoints.erase(address) != 0;
    disabled_breakpoints.erase(address);
    temporary_breakpoints.erase(address);
    has_breakpoints = !breakpoints.empty();
    return removed;
}

void MOS6502::clear_breakpoints()
{
    breakpoints.clear();
    disabled_breakpoints.clear();
    temporary_breakpoints.clear();
    has_breakpoints = false;
}

std::string MOS6502::get_register_summary()
{
    return std::format("[A: {:02X}, X: {:02X}, Y: {:02X}  |  N: {}, Z: {}, C: {}, V: {}  |  SP: {:02X}]",
                       A, X, Y, (int)get_n(), (int)get_z(), (int)C, (int)V, SP);
}


//   7                           0
// +---+---+---+---+---+---+---+---+
// | N | V |   | B | D | I | Z | C |
// +---+---+---+---+---+---+---+---+
uint8_t MOS6502::get_p() const
{
    uint8_t result = 0;
    result |= get_n() ? FLAG_N : 0;
    result |= V ? FLAG_V : 0;
    result |= B ? FLAG_B : 0;
    result |= D ? FLAG_D : 0;
    result |= I ? FLAG_I : 0;
    result |= get_z() ? FLAG_Z : 0;
    result |= C ? FLAG_C : 0;
    return result;
}

void MOS6502::set_p(uint8_t p)
{
    N_INTERN = (p & FLAG_N) ? FLAG_N : 0;
    V = !! (p & FLAG_V);
    B = !! (p & FLAG_B);
    D = !! (p & FLAG_D);
    I = !! (p & FLAG_I);
    Z_INTERN = (p & FLAG_Z) ? 0 : 1;
    C = !! (p & FLAG_C);
}

void MOS6502::ADC(uint8_t value)
{
    uint16_t sum = A + value + (C ? 1 : 0);

    if (D) { // Decimal mode
        // V from binary sum (before BCD adjust)
        V = (~(A ^ value) & (A ^ sum) & 0x80) != 0;

        uint16_t adj = sum;

        // low-nibble adjust
        if ( ((A & 0x0F) + (value & 0x0F) + (C ? 1 : 0)) > 9 ) {
            adj += 0x06;
        }

        // high-nibble / carry adjust
        if ( adj > 0x99 ) {
            adj += 0x60;
        }

        C = adj > 0x99;           // BCD carry out
        A = static_cast<uint8_t>(adj & 0xFF);
        set_flag_nz(A);           // N/Z from final (adjusted) result
    }
    else { // Normal mode
        C = sum > 0xFF;
        V = (~(A ^ value) & (A ^ sum) & 0x80) != 0;
        A = static_cast<uint8_t>(sum);
        set_flag_nz(A);
    }
}

void MOS6502::SBC(uint8_t value)
{
    uint16_t diff = A - value - (C ? 0 : 1);

    if (D) { // Decimal mode
        // V from binary diff (before BCD adjust)
        V = ((A ^ value) & (A ^ diff) & 0x80) != 0;

        // Start from the binary result in 8 bits
        uint16_t adj = diff & 0xFF;

        // low nibble borrow adjust
        if ( ((A & 0x0F) - (C ? 0 : 1)) < (value & 0x0F) ) {
            adj = (adj - 0x06) & 0xFF;
        }

        // high nibble borrow / 100 adjust
        if ( diff > 0xFF ) {
            // borrow occurred (i.e., negative in 9-bit)
            adj = (adj - 0x60) & 0xFF;
        }

        C = diff < 0x100;                    // set if no borrow
        A = static_cast<uint8_t>(adj);
        set_flag_nz(A);                      // N/Z from final (adjusted) result
    }
    else { // Normal mode
        C = diff < 0x100;
        V = ((A ^ value) & (A ^ diff) & 0x80) != 0;
        A = static_cast<uint8_t>(diff);
        set_flag_nz(A);
    }
}

uint8_t MOS6502::time_instruction()
{
    uint16_t _pc = PC;
    uint8_t extra = 0;

    if (nmi_flag) {
        _pc = memory_read_word_handler(machine, NMI_VECTOR_L);
        extra += 7;
        do_interrupt = true;
    }
    else if (irq_flags && !I) {
        _pc = memory_read_word_handler(machine, IRQ_VECTOR_L);
        extra += 7;
        do_interrupt = true;
    }

    uint8_t b1;
    uint16_t addr;
    current_instruction = memory_read_byte_handler(machine, _pc);
    current_instruction_addr = PC;

    switch(current_instruction)
    {
        case LDA_ABS_X:
        case LDY_ABS_X:
        case ADC_ABS_X:
        case SBC_ABS_X:
        case AND_ABS_X:
        case ORA_ABS_X:
        case EOR_ABS_X:
        case CMP_ABS_X:
        case ILL_NOP_ABS_X_1C:
        case ILL_NOP_ABS_X_3C:
        case ILL_NOP_ABS_X_5C:
        case ILL_NOP_ABS_X_7C:
        case ILL_NOP_ABS_X_DC:
        case ILL_NOP_ABS_X_FC:
            addr = peek_addr_abs(_pc);
            extra += pagecheck(addr, X) ? 1 : 0;
            break;

        case LDA_ABS_Y:
        case LDX_ABS_Y:
        case ADC_ABS_Y:
        case SBC_ABS_Y:
        case AND_ABS_Y:
        case ORA_ABS_Y:
        case EOR_ABS_Y:
        case CMP_ABS_Y:
            addr = peek_addr_abs(_pc);
            extra += pagecheck(addr, Y) ? 1 : 0;
            break;

        case LDA_IND_Y:
        case ADC_IND_Y:
        case SBC_IND_Y:
        case AND_IND_Y:
        case ORA_IND_Y:
        case EOR_IND_Y:
        case CMP_IND_Y:
            addr = memory_read_word_zp_handler(machine, memory_read_byte_handler(machine, _pc + 1));
            extra += pagecheck(addr, Y) ? 1 : 0;
            break;


        case BCC:
            if (!C) {
                extra += pagecheck2(peek_jump_addr(_pc), (_pc + 2)) ? 2 : 1;
            }
            break;
        case BCS:
            if (C) {
                extra += pagecheck2(peek_jump_addr(_pc), (_pc + 2)) ? 2 : 1;
            }
            break;
        case BEQ:
            if (get_z()) {
                extra += pagecheck2(peek_jump_addr(_pc), (_pc + 2)) ? 2 : 1;
            }
            break;
        case BNE:
            if (! get_z()) {
                extra += pagecheck2(peek_jump_addr(_pc), (_pc + 2)) ? 2 : 1;
            }
            break;

        case BMI:
            if (get_n()) {
                extra += pagecheck2(peek_jump_addr(_pc), (_pc + 2)) ? 2 : 1;
            }
            break;

        case BPL:
            if (! get_n()) {
                extra += pagecheck2(peek_jump_addr(_pc), (_pc + 2)) ? 2 : 1;
            }
            break;

        case BVC:
            if (!V) {
                extra += pagecheck2(peek_jump_addr(_pc), (_pc + 2)) ? 2 : 1;
            }
            break;

        case BVS:
            if (V) {
                extra += pagecheck2(peek_jump_addr(_pc), (_pc + 2)) ? 2 : 1;
            }
            break;
        case ILL_LAX_ABS_Y:
            // ABS,X / ABS,Y page-cross penalties
            extra += pagecheck(peek_addr_abs(_pc), Y) ? 1 : 0;
            break;

        case ILL_LAX_IND_Y:
            // (zp),Y page-cross penalty
            addr = memory_read_word_zp_handler(machine, memory_read_byte_handler(machine, _pc + 1));
            extra += pagecheck(addr, Y) ? 1 : 0;
            break;

        default:
            break;
    }

    instruction_cycles = opcode_cycles[current_instruction] + extra;
    return instruction_cycles;
}


bool MOS6502::exec(bool break_on_brk, bool& do_break, bool ignore_breakpoint)
{
    current_cycle = 0;

    if (do_interrupt) {
        do_interrupt = false;

        push_byte_stack(PC >> 8);
        push_byte_stack(PC & 0xff);
        push_byte_stack((get_p() & ~FLAG_B) | 0x20);  // B=0, bit5=1

        I = true;            // mask further IRQs
        D = false;           // NMOS quirk: clear decimal on interrupt

        if (nmi_flag) {
            PC = memory_read_word_handler(machine, NMI_VECTOR_L);
            nmi_flag = false;
            std::println("NMI interrupt");
        }

        else if (irq_flags) {
            PC = memory_read_word_handler(machine, IRQ_VECTOR_L);
        }
    }

    const bool skipped_breakpoint = skip_breakpoint;
    skip_breakpoint = false;
    if (!ignore_breakpoint && !skipped_breakpoint && has_breakpoints && breakpoint_enabled(PC)) {
        breakpoint_hit = PC;
        if (temporary_breakpoints.erase(PC) != 0) {
            breakpoints.erase(PC);
            has_breakpoints = !breakpoints.empty();
        }
        do_break = true;
        return false;
    }

    uint8_t b1, b2;
    uint16_t addr;
    int i;

    ++PC;

    switch(current_instruction)
    {
        case LDA_IMM:
            set_flag_nz(A = read_byte_imm());
            break;
        case LDA_ZP:
            set_flag_nz(A = read_byte_zp());
            break;
        case LDA_ZP_X:
            set_flag_nz(A = read_byte_zp_x());
            break;
        case LDA_ABS:
            set_flag_nz(A = read_byte_abs());
            break;
        case LDA_ABS_X:
            set_flag_nz(A = memory_read_byte_handler(machine, read_addr_abs_x()));
            break;
        case LDA_ABS_Y:
            set_flag_nz(A = memory_read_byte_handler(machine, read_addr_abs_y()));
            break;
        case LDA_IND_X:
            set_flag_nz(A = read_byte_ind_x());
            break;
        case LDA_IND_Y:
            set_flag_nz(A = memory_read_byte_handler(machine, read_addr_ind_y()));
            break;

        case LDX_IMM:
            set_flag_nz(X = read_byte_imm());
            break;
        case LDX_ZP:
            set_flag_nz(X = read_byte_zp());
            break;
        case LDX_ZP_Y:
            set_flag_nz(X = read_byte_zp_y());
            break;
        case LDX_ABS:
            set_flag_nz(X = read_byte_abs());
            break;
        case LDX_ABS_Y:
            set_flag_nz(X = memory_read_byte_handler(machine, read_addr_abs_y()));
            break;


        case LDY_IMM:
            set_flag_nz(Y = read_byte_imm());
            break;
        case LDY_ZP:
            set_flag_nz(Y = read_byte_zp());
            break;
        case LDY_ZP_X:
            set_flag_nz(Y = read_byte_zp_x());
            break;
        case LDY_ABS:
            set_flag_nz(Y = read_byte_abs());
            break;
        case LDY_ABS_X:
            set_flag_nz(Y = memory_read_byte_handler(machine, read_addr_abs_x()));
            break;

        case STA_ZP:
            memory_write_byte_handler(machine, read_addr_zp(), A);
            break;
        case STA_ZP_X:
            memory_write_byte_handler(machine, read_addr_zp_x(), A);
            break;
        case STA_ABS:
            memory_write_byte_handler(machine, read_addr_abs(), A);
            break;
        case STA_ABS_X:
            memory_write_byte_handler(machine, read_addr_abs_x(), A);
            break;
        case STA_ABS_Y:
            memory_write_byte_handler(machine, read_addr_abs_y(), A);
            break;
        case STA_IND_X:
            memory_write_byte_handler(machine, read_addr_ind_x(), A);
            break;
        case STA_IND_Y:
            memory_write_byte_handler(machine, read_addr_ind_y(), A);
            break;

        case STX_ZP:
            memory_write_byte_handler(machine, read_addr_zp(), X);
            break;
        case STX_ZP_Y:
            memory_write_byte_handler(machine, read_addr_zp_y(), X);
            break;
        case STX_ABS:
            memory_write_byte_handler(machine, read_addr_abs(), X);
            break;

        case STY_ZP:
            memory_write_byte_handler(machine, read_addr_zp(), Y);
            break;
        case STY_ZP_X:
            memory_write_byte_handler(machine, read_addr_zp_x(), Y);
            break;
        case STY_ABS:
            memory_write_byte_handler(machine, read_addr_abs(), Y);
            break;

            // ADD to accumulator with carry
        case ADC_IMM:
            ADC(read_byte_imm());
            break;
        case ADC_ZP:
            ADC(read_byte_zp());
            break;
        case ADC_ZP_X:
            ADC(read_byte_zp_x());
            break;
        case ADC_ABS:
            ADC(read_byte_abs());
            break;
        case ADC_ABS_X:
            ADC(memory_read_byte_handler(machine, read_addr_abs_x()));
            break;
        case ADC_ABS_Y:
            ADC(memory_read_byte_handler(machine, read_addr_abs_y()));
            break;
        case ADC_IND_X:
            ADC(read_byte_ind_x());
            break;
        case ADC_IND_Y:
            ADC(memory_read_byte_handler(machine, read_addr_ind_y()));
            break;

            // Subtract from accumulator with borrow
        case SBC_IMM:
            SBC(read_byte_imm());
            break;
        case SBC_ZP:
            SBC(read_byte_zp());
            break;
        case SBC_ZP_X:
            SBC(read_byte_zp_x());
            break;
        case SBC_ABS:
            SBC(read_byte_abs());
            break;
        case SBC_ABS_X:
            SBC(memory_read_byte_handler(machine, read_addr_abs_x()));
            break;
        case SBC_ABS_Y:
            SBC(memory_read_byte_handler(machine, read_addr_abs_y()));
            break;
        case SBC_IND_X:
            SBC(read_byte_ind_x());
            break;
        case SBC_IND_Y:
            SBC(memory_read_byte_handler(machine, read_addr_ind_y()));
            break;

            // Increment memory by one
        case INC_ZP:
            addr = read_addr_zp();
            memory_write_byte_handler(machine, addr, set_flag_nz(memory_read_byte_handler(machine, addr) + 1));
            break;
        case INC_ZP_X:
            addr = read_addr_zp_x();
            memory_write_byte_handler(machine, addr, set_flag_nz(memory_read_byte_handler(machine, addr) + 1));
            break;
        case INC_ABS:
            addr = read_addr_abs();
            memory_write_byte_handler(machine, addr, set_flag_nz(memory_read_byte_handler(machine, addr) + 1));
            break;
        case INC_ABS_X:
            addr = read_addr_abs_x();
            memory_write_byte_handler(machine, addr, set_flag_nz(memory_read_byte_handler(machine, addr) + 1));
            break;

            // Decrease memory by one
        case DEC_ZP:
            addr = read_addr_zp();
            memory_write_byte_handler(machine, addr, set_flag_nz(memory_read_byte_handler(machine, addr) - 1));
            break;
        case DEC_ZP_X:
            addr = read_addr_zp_x();
            memory_write_byte_handler(machine, addr, set_flag_nz(memory_read_byte_handler(machine, addr) - 1));
            break;
        case DEC_ABS:
            addr = read_addr_abs();
            memory_write_byte_handler(machine, addr, set_flag_nz(memory_read_byte_handler(machine, addr) - 1));
            break;
        case DEC_ABS_X:
            addr = read_addr_abs_x();
            memory_write_byte_handler(machine, addr, set_flag_nz(memory_read_byte_handler(machine, addr) - 1));
            break;

            // Increase X by one
        case INX:
            set_flag_nz(++X);
            break;
            // Decrease X by one
        case DEX:
            set_flag_nz(--X);
            break;
            // Increase Y by one
        case INY:
            set_flag_nz(++Y);
            break;
            // Decrease Y by one
        case DEY:
            set_flag_nz(--Y);
            break;

            // And accumulator with memory
        case AND_IMM:
            set_flag_nz(A &= read_byte_imm());
            break;
        case AND_ZP:
            set_flag_nz(A &= read_byte_zp());
            break;
        case AND_ZP_X:
            set_flag_nz(A &= read_byte_zp_x());
            break;
        case AND_ABS:
            set_flag_nz(A &= read_byte_abs());
            break;
        case AND_ABS_X:
            set_flag_nz(A &= memory_read_byte_handler(machine, read_addr_abs_x()));
            break;
        case AND_ABS_Y:
            set_flag_nz(A &= memory_read_byte_handler(machine, read_addr_abs_y()));
            break;
        case AND_IND_X:
            set_flag_nz(A &= read_byte_ind_x());
            break;
        case AND_IND_Y :
            set_flag_nz(A &= memory_read_byte_handler(machine, read_addr_ind_y()));
            break;

            // Or accumulator with memory
        case ORA_IMM:
            set_flag_nz(A |= read_byte_imm());
            break;
        case ORA_ZP:
            set_flag_nz(A |= read_byte_zp());
            break;
        case ORA_ZP_X:
            set_flag_nz(A |= read_byte_zp_x());
            break;
        case ORA_ABS:
            set_flag_nz(A |= read_byte_abs());
            break;
        case ORA_ABS_X:
            set_flag_nz(A |= memory_read_byte_handler(machine, read_addr_abs_x()));
            break;
        case ORA_ABS_Y:
            set_flag_nz(A |= memory_read_byte_handler(machine, read_addr_abs_y()));
            break;
        case ORA_IND_X:
            set_flag_nz(A |= read_byte_ind_x());
            break;
        case ORA_IND_Y:
            set_flag_nz(A |= memory_read_byte_handler(machine, read_addr_ind_y()));
            break;

            // Exclusive or accumulator with memory
        case EOR_IMM:
            set_flag_nz(A ^= read_byte_imm());
            break;
        case EOR_ZP:
            set_flag_nz(A ^= read_byte_zp());
            break;
        case EOR_ZP_X:
            set_flag_nz(A ^= read_byte_zp_x());
            break;
        case EOR_ABS:
            set_flag_nz(A ^= read_byte_abs());
            break;
        case EOR_ABS_X:
            set_flag_nz(A ^= memory_read_byte_handler(machine, read_addr_abs_x()));
            break;
        case EOR_ABS_Y:
            set_flag_nz(A ^= memory_read_byte_handler(machine, read_addr_abs_y()));
            break;
        case EOR_IND_X:
            set_flag_nz(A ^= read_byte_ind_x());
            break;
        case EOR_IND_Y:
            set_flag_nz(A ^= memory_read_byte_handler(machine, read_addr_ind_y()));
            break;

            //      +-+-+-+-+-+-+-+-+
            // C <- |7|6|5|4|3|2|1|0| <- 0              N Z C I D V
            //      +-+-+-+-+-+-+-+-+                   / / / _ _ _
        case ASL_ACC:
            C = A & 0x80;
            set_flag_nz(A <<= 1);
            break;
        case ASL_ZP:
            b1 = memory_read_byte_zp_handler(machine, addr = read_addr_zp());
            C = b1 & 0x80;
            memory_write_byte_zp_handler(machine, addr, set_flag_nz(b1 <<= 1));
            break;
        case ASL_ZP_X:
            b1 = memory_read_byte_zp_handler(machine, addr = read_addr_zp_x());
            C = b1 & 0x80;
            memory_write_byte_zp_handler(machine, addr, set_flag_nz(b1 <<= 1));
            break;
        case ASL_ABS:
            addr = read_addr_abs();
            b1 = memory_read_byte_handler(machine, addr);
            C = b1 & 0x80;
            memory_write_byte_handler(machine, addr, set_flag_nz(b1 <<= 1));
            break;
        case ASL_ABS_X:
            addr = read_addr_abs_x();
            b1 = memory_read_byte_handler(machine, addr);
            C = b1 & 0x80;
            memory_write_byte_handler(machine, addr, set_flag_nz(b1 <<= 1));
            break;

            //      +-+-+-+-+-+-+-+-+
            // 0 -> |7|6|5|4|3|2|1|0| -> C              N Z C I D V
            //      +-+-+-+-+-+-+-+-+                   0 / / _ _ _
        case LSR_ACC:
            C = A & 0x01;
            set_flag_nz(A >>= 1);
            break;
        case LSR_ZP:
            b1 = memory_read_byte_zp_handler(machine, addr = read_addr_zp());
            C = b1 & 0x01;
            memory_write_byte_zp_handler(machine, addr, set_flag_nz(b1 >>= 1));
            break;
        case LSR_ZP_X:
            b1 = memory_read_byte_zp_handler(machine, addr = read_addr_zp_x());
            C = b1 & 0x01;
            memory_write_byte_zp_handler(machine, addr, set_flag_nz(b1 >>= 1));
            break;
        case LSR_ABS:
            addr = read_addr_abs();
            b1 = memory_read_byte_handler(machine, addr);
            C = b1 & 0x01;
            memory_write_byte_handler(machine, addr, set_flag_nz(b1 >>= 1));
            break;
        case LSR_ABS_X:
            addr = read_addr_abs_x();
            b1 = memory_read_byte_handler(machine, addr);
            C = b1 & 0x01;
            memory_write_byte_handler(machine, addr, set_flag_nz(b1 >>= 1));
            break;

            // +------------------------------+
            // |         M or A               |
            // |   +-+-+-+-+-+-+-+-+    +-+   |
            // +-< |7|6|5|4|3|2|1|0| <- |C| <-+         N Z C I D V
            //     +-+-+-+-+-+-+-+-+    +-+             / / / _ _ _
        case ROL_ACC:
            b2 = A & 0x80;
            set_flag_nz(A = C ? (A<<=1) + 1 : A<<=1);
            C = b2;
            break;
        case ROL_ZP:
            b1 = memory_read_byte_zp_handler(machine, addr = read_addr_zp());
            b2 = b1 & 0x80;
            memory_write_byte_zp_handler(machine, addr, set_flag_nz(C ? (b1<<=1) + 1 : b1<<=1));
            C = b2;
            break;
        case ROL_ZP_X:
            b1 = memory_read_byte_zp_handler(machine, addr = read_addr_zp_x());
            b2 = b1 & 0x80;
            memory_write_byte_zp_handler(machine, addr, set_flag_nz(C ? (b1<<=1) + 1 : b1<<=1));
            C = b2;
            break;
        case ROL_ABS:
            addr = read_addr_abs();
            b1 = memory_read_byte_handler(machine, addr);
            b2 = b1 & 0x80;
            memory_write_byte_handler(machine, addr, set_flag_nz(C ? (b1<<=1) + 1 : b1<<=1));
            C = b2;
            break;
        case ROL_ABS_X:
            addr = read_addr_abs_x();
            b1 = memory_read_byte_handler(machine, addr);
            b2 = b1 & 0x80;
            memory_write_byte_handler(machine, addr, set_flag_nz(C ? (b1<<=1) + 1 : b1<<=1));
            C = b2;
            break;

            // +------------------------------+
            // |                              |
            // |   +-+    +-+-+-+-+-+-+-+-+   |
            // +-> |C| -> |7|6|5|4|3|2|1|0| >-+         N Z C I D V
            //     +-+    +-+-+-+-+-+-+-+-+             / / / _ _ _
        case ROR_ACC:
            b2 = A & 0x01;
            set_flag_nz(A = C ? (A>>=1)|0x80 : A>>=1);
            C = b2;
            break;
        case ROR_ZP:
            b1 = memory_read_byte_zp_handler(machine, addr = read_addr_zp());
            b2 = b1 & 0x01;
            memory_write_byte_zp_handler(machine, addr, set_flag_nz(C ? (b1>>=1) | 0x80 : b1>>=1));
            C = b2;
            break;
        case ROR_ZP_X:
            b1 = memory_read_byte_zp_handler(machine, addr = read_addr_zp_x());
            b2 = b1 & 0x01;
            memory_write_byte_zp_handler(machine, addr, set_flag_nz(C ? (b1>>=1) | 0x80 : b1>>=1));
            C = b2;
            break;
        case ROR_ABS:
            addr = read_addr_abs();
            b1 = memory_read_byte_handler(machine, addr);
            b2 = b1 & 0x01;
            memory_write_byte_handler(machine, addr, set_flag_nz(C ? (b1>>=1) | 0x80 : b1>>=1));
            C = b2;
            break;
        case ROR_ABS_X:
            addr = read_addr_abs_x();
            b1 = memory_read_byte_handler(machine, addr);
            b2 = b1 & 0x01;
            memory_write_byte_handler(machine, addr, set_flag_nz(C ? (b1>>=1) | 0x80 : b1>>=1));
            C = b2;
            break;

            // Branches
        case BCC:
            if (!C) {
                PC = read_jump_addr();
            }
            else
                ++PC;
            break;
        case BCS:
            if (C) {
                PC = read_jump_addr();
            }
            else
                ++PC;
            break;
        case BEQ:
            if (get_z()) {
                PC = read_jump_addr();
            }
            else
                ++PC;
            break;
        case BNE:
            if (! get_z()) {
                PC = read_jump_addr();
            }
            else
                ++PC;
            break;

        case BMI:
            if (get_n()) {
                PC = read_jump_addr();
            }
            else
                ++PC;
            break;

        case BPL:
            if (! get_n()) {
                PC = read_jump_addr();
            }
            else
                ++PC;
            break;
        case BVC:

            if (!V) {
                PC = read_jump_addr();
            }
            else
                ++PC;
            break;

        case BVS:
            if (V) {
                PC = read_jump_addr();
            }
            else
                ++PC;
            break;

        case BIT_ZP:
            b1 = read_byte_zp();
            N_INTERN = b1;
            Z_INTERN = A & b1;
            V = b1 & FLAG_V;  // bit 6 -> V
            break;

        case BIT_ABS:
            b1 = read_byte_abs();
            N_INTERN = b1;
            Z_INTERN = A & b1;
            V = b1 & FLAG_V;  // bit 6 -> V
            break;

        case SEC: // Set carry flag
            C = true;
            break;
        case SED: // Set decimal flag
            D = true;
            break;
        case SEI: // Set interrupt flag
            I = true;
            break;

        case CLC: // Clear carry flag
            C = false;
            break;
        case CLD: // Clear decimal flag
            D = false;
            break;
        case CLI: // Clear interrupt flag
            I = false;
            break;
        case CLV: // Clear overflow flag
            V = false;
            break;

        case CMP_IMM:
            i = A - read_byte_imm();
            C = i >= 0;
            set_flag_nz((uint8_t)i);
            break;
        case CMP_ZP:
            i = A - read_byte_zp();
            C = i >= 0;
            set_flag_nz((uint8_t)i);
            break;
        case CMP_ZP_X:
            i = A - read_byte_zp_x();
            C = i >= 0;
            set_flag_nz((uint8_t)i);
            break;
        case CMP_ABS:
            i = A - read_byte_abs();
            C = i >= 0;
            set_flag_nz((uint8_t)i);
            break;
        case CMP_ABS_X:
            i = A - memory_read_byte_handler(machine, read_addr_abs_x());
            C = i >= 0;
            set_flag_nz((uint8_t)i);
            break;
        case CMP_ABS_Y:
            i = A - memory_read_byte_handler(machine, read_addr_abs_y());
            C = i >= 0;
            set_flag_nz((uint8_t)i);
            break;
        case CMP_IND_X:
            i = A - read_byte_ind_x();
            C = i >= 0;
            set_flag_nz((uint8_t)i);
            break;
        case CMP_IND_Y:
            i = A - memory_read_byte_handler(machine, read_addr_ind_y());
            C = i >= 0;
            set_flag_nz((uint8_t)i);
            break;

        case CPX_IMM:
            i = X - read_byte_imm();
            C = i >= 0;
            set_flag_nz((uint8_t)i);
            break;
        case CPX_ZP:
            i = X - read_byte_zp();
            C = i >= 0;
            set_flag_nz((uint8_t)i);
            break;
        case CPX_ABS:
            i = X - read_byte_abs();
            C = i >= 0;
            set_flag_nz((uint8_t)i);
            break;

        case CPY_IMM:
            i = Y - read_byte_imm();
            C = i >= 0;
            set_flag_nz((uint8_t)i);
            break;
        case CPY_ZP:
            i = Y - read_byte_zp();
            C = i >= 0;
            set_flag_nz((uint8_t)i);
            break;
        case CPY_ABS:
            i = Y - read_byte_abs();
            C = i >= 0;
            set_flag_nz((uint8_t)i);
            break;

        case JMP_ABS:
            PC = read_addr_abs();
            break;
        case JMP_IND:
            PC = read_word_buggy(machine, read_addr_abs(), memory_read_byte_handler);
            break;

        case JSR:
            push_byte_stack((PC+1) >> 8); // Store 1 before next instruction
            push_byte_stack((PC+1) & 0xff);
            PC = read_addr_abs();
            break;

        case RTS:
            PC = pop_byte_stack();
            PC += (pop_byte_stack() << 8) + 1;
            break;

        case BRK:
            push_byte_stack((PC+1) >> 8); // Byte after BRK will not be executed on return!
            push_byte_stack(PC+1);
            push_byte_stack(get_p() | FLAG_B | 0x20);
            I = true;
            PC = memory_read_word_handler(machine, IRQ_VECTOR_L);
            if (break_on_brk) {
                do_break = true;
            }
            break;

        case RTI:  // Return from interrupt
            set_p(pop_byte_stack()); //  & 0xdb);
            PC = pop_byte_stack();
            PC += (pop_byte_stack() << 8);
            break;

        case NOP:
            break;

        case PHA:  // Push accumulator to stack
            push_byte_stack(A);
            break;
        case PLA:  // Pull accumulator from stack
            A = pop_byte_stack();
            set_flag_nz(A);
            break;
        case PHP:  // Push status to stack
            push_byte_stack(get_p() | FLAG_B | 0x20);  // B=1, bit5=1
            break;
        case PLP:  // Pull status from stack
            set_p(pop_byte_stack() | 0x20);;
            break;

        case TAX:  // Transfer A to X
            set_flag_nz(X = A);
            break;
        case TXA:  // Transfer X to A
            set_flag_nz(A = X);
            break;
        case TAY:  // Transfer A to Y
            set_flag_nz(Y = A);
            break;
        case TYA:  // Transfer Y to A
            set_flag_nz(A = Y);
            break;
        case TXS:  // Transfer X to SP
            SP = X;
            break;
        case TSX:  // Transfer SP to X
            set_flag_nz(X = SP);
            break;

//        case ILL_SBX:
//            i = (A & X) - read_byte_imm();
//            set_flag_nz(i);
//            C = i >= 0;
//            X = i;
//            break;

        case ILL_LAX_ZP: {
            uint8_t v = memory_read_byte_zp_handler(machine, read_addr_zp());
            A = X = v; set_flag_nz(A);
            break;
        }
        case ILL_LAX_ZP_Y: {
            uint8_t v = memory_read_byte_zp_handler(machine, read_addr_zp_y());
            A = X = v; set_flag_nz(A);
            break;
        }
        case ILL_LAX_ABS: {
            A = X = read_byte_abs(); set_flag_nz(A);
            break;
        }
        case ILL_LAX_ABS_Y: {
            uint8_t v = memory_read_byte_handler(machine, read_addr_abs_y());
            A = X = v; set_flag_nz(A);
            break;
        }
        case ILL_LAX_IND_X: {
            uint8_t v = memory_read_byte_handler(machine, read_addr_ind_x());
            A = X = v; set_flag_nz(A);
            break;
        }
        case ILL_LAX_IND_Y: {              // $B3
            uint8_t v = memory_read_byte_handler(machine, read_addr_ind_y());
            A = X = v; set_flag_nz(A);
            break;
        }


        case ILL_SLO_IND_X:
            b1 = memory_read_byte_handler(machine, addr = read_addr_ind_x());
            C = (b1 & 0x80) != 0;
            b1 <<= 1;
            memory_write_byte_handler(machine, addr, b1);
            set_flag_nz(A |= b1);
            break;

        case ILL_SLO_IND_Y:
            b1 = memory_read_byte_handler(machine, addr = read_addr_ind_y());
            C = (b1 & 0x80) != 0;
            b1 <<= 1;
            memory_write_byte_handler(machine, addr, b1);
            set_flag_nz(A |= b1);
            break;

        case ILL_RLA_IND_Y:
            b1 = memory_read_byte_handler(machine, addr = read_addr_ind_y());
            b2 = b1 & 0x80;
            b1 <<= 1;
            if (C) { b1 |= 0x01; }
            memory_write_byte_handler(machine, addr, b1);
            C = b2 != 0;
            set_flag_nz(A &= b1);
            break;

        case ILL_NOP_IMP_1A:
        case ILL_NOP_IMP_3A:
        case ILL_NOP_IMP_5A:
        case ILL_NOP_IMP_7A:
        case ILL_NOP_IMP_DA:
        case ILL_NOP_IMP_FA:
            break;

        case ILL_NOP_IMM_80:
        case ILL_NOP_IMM_82:
        case ILL_NOP_IMM_89:
        case ILL_NOP_IMM_C2:
        case ILL_NOP_IMM_E2:
            i = read_byte_imm();
            break;

        case ILL_NOP_ZP_04:
        case ILL_NOP_ZP_44:
        case ILL_NOP_ZP_64:
            addr = read_addr_zp();
            break;

        case ILL_NOP_ZPX_14:
        case ILL_NOP_ZPX_34:
        case ILL_NOP_ZPX_54:
        case ILL_NOP_ZPX_74:
        case ILL_NOP_ZPX_D4:
        case ILL_NOP_ZPX_F4:
            addr = read_addr_zp_x();
            break;

        case ILL_NOP_ABS_0C:
            addr = read_addr_abs();
            break;

        case ILL_NOP_ABS_X_1C:
        case ILL_NOP_ABS_X_3C:
        case ILL_NOP_ABS_X_5C:
        case ILL_NOP_ABS_X_7C:
        case ILL_NOP_ABS_X_DC:
        case ILL_NOP_ABS_X_FC:
            addr = read_addr_abs_x();
            break;

        default:
            std::println("Unhandled illegal opcode: ${:02X}\n", current_instruction);
            do_break = true;
            break;
    }

    return true;
}
