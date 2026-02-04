#include "../include/Intel8080.h"

#include <vector>
#include <bit>

constexpr int mnemonic[72] {
        3, // MOV r1, r2
        5, // MOV r, M
        9, // MOV M, r
        13, // SPHL
        15, // MVI r, data
        19, // MVI M, data
        26, // LXI rp, data
        33, // LDA addr
        43, // STA addr
        53, // LHLD addr
        66, // SHLD addr
        79, // LDAX rp
        83, // STAX rp
        87, // XCHG
        88, // ADD r
        89, // ADD M
        93, // ADI data
        97, // ADC r
        98, // ADC M
        102, // ACI data
        106, // SUB r
        107, // SUB M
        111, // SUI data
        115, // SBB r
        116, // SBB M
        120, // SBI data
        124, // INR r
        126, // INR M
        133, // DCR r
        135, // DCR M
        142, // INX rp
        144, // DCX rp
        146, // DAD rp
        153, // DAA
        154, // ANA r
        155, // ANA M
        159, // ANI data
        163, // XRA r
        164, // XRA M
        168, // XRI data
        172, // ORA r
        173, // ORA M
        177, // ORI data
        181, // CMP r
        182, // CMP M
        186, // CPI data
        190, // RLC
        191, // RRC
        192, // RAL
        193, // RAR
        194, // CMA
        195, // CMC
        196, // STC
        197, // JMP addr
        204, // J cond addr
        211, // CALL addr
        225, // C cond addr
        239, // RET
        246, // R cond addr
        254, // RST n
        262, // PCHL
        264, // PUSH rp
        272, // PUSH PSW
        280, // POP rp
        287, // POP PSW
        294, // XTHL
        309, // IN port
        316, // OUT port
        323, // EI
        324, // DI
        325, // HLT
        328, // NOP
};

constexpr int opcode[256] {
        71, 6, 12, 30, 26, 28, 4, 46, 71, 32, 11, 31, 26, 28, 4, 47,
        71, 6, 12, 30, 26, 28, 4, 48, 71, 32, 11, 31, 26, 28, 4, 49,
        71, 6, 10, 30, 26, 28, 4, 33, 71, 32, 9, 31, 26, 28, 4, 50,
        71, 6, 8, 30, 27, 29, 5, 52, 71, 32, 7, 31, 26, 28, 4, 51,
        0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0,
        0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0,
        0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0,
        2, 2, 2, 2, 2, 2, 70, 2, 0, 0, 0, 0, 0, 0, 1, 0,
        14, 14, 14, 14, 14, 14, 15, 14, 17, 17, 17, 17, 17, 17, 18, 17,
        20, 20, 20, 20, 20, 20, 21, 20, 23, 23, 23, 23, 23, 23, 24, 23,
        34, 34, 34, 34, 34, 34, 35, 34, 37, 37, 37, 37, 37, 37, 38, 37,
        40, 40, 40, 40, 40, 40, 41, 40, 43, 43, 43, 43, 43, 43, 44, 43,
        58, 63, 54, 53, 56, 61, 16, 59, 58, 57, 54, 53, 56, 55, 19, 59,
        58, 63, 54, 67, 56, 61, 22, 59, 58, 57, 54, 66, 56, 55, 25, 59,
        58, 63, 54, 65, 56, 61, 36, 59, 58, 60, 54, 13, 56, 55, 39, 59,
        58, 64, 54, 69, 56, 62, 42, 59, 58, 3, 54, 68, 56, 55, 45, 59,
};

void Intel8080::tick()
{
    if (pins & INT and pins & INTE) {
        intreq_ = true;
    }
    if (intreq_ and step_ == 0) {
        // ensure intff latched AFTER the last instruction has completed execution
        intff_ = true;
        intreq_ = false;
        if (stopped_) {
            stopped_ = false;
            intWhileHalt_ = true;
        }
    }
    if (stopped_) return;

    switch (step_) {
        // instruction fetch
        case 0:
            setABus_(pc);
            if (intff_) {
                setDBus(INTA|WO|M1);
                if (intWhileHalt_) {
                    intWhileHalt_ = false;
                    pins |= HLTA;
                }
                pins &= ~INTE;
            } else {
                setDBus(WO|M1|MEMR);
            }
            t1_();
            goto next;
        case 1:
            readT2_();
            intff_ ? intff_ = false : ++pc;
            goto next;
        case 2:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                ir_ = getDBus();
                step_ = mnemonic[opcode[ir_]];
                return;
            }

        // MOV r1, r2
        case 3: goto next;
        case 4:
            setReg_(dst_(), getReg(src_()));
            goto done;

        // MOV r, M
        case 5: goto next;
        case 6:
            readT1_(pair_[HL]);
            goto next;
        case 7:
            readT2_();
            goto next;
        case 8:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setReg_(dst_(), getDBus());
                goto done;
            }

        // MOV M, r
        case 9: goto next;
        case 10:
            writeT1_(pair_[HL]);
            goto next;
        case 11:
            writeT2_(getReg(src_()));
            goto next;
        case 12:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto done;
            }

        // SPHL
        case 13: goto next;
        case 14:
            pair_[SP] = pair_[HL];
            goto done;

        // MVI r, data
        case 15: goto next;
        case 16:
            readT1_(pc);
            goto next;
        case 17:
            readT2_();
            ++pc;
            goto next;
        case 18:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setReg_(dst_(), getDBus());
                goto done;
            }

        // MVI M, data
        case 19: goto next;
        case 20:
            readT1_(pc);
            goto next;
        case 21:
            readT2_();
            ++pc;
            goto next;
        case 22:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                goto next;
            }
        case 23:
            writeT1_(pair_[HL]);
            goto next;
        case 24:
            writeT2_(tmp_);
            goto next;
        case 25:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto done;
            }

        // LXI rp, data
        case 26: goto next;
        case 27:
            readT1_(pc);
            goto next;
        case 28:
            readT2_();
            ++pc;
            goto next;
        case 29:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setLo_(rp_(), getDBus());
                goto next;
            }
        case 30:
            readT1_(pc);
            goto next;
        case 31:
            readT2_();
            ++pc;
            goto next;
        case 32:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setHi_(rp_(), getDBus());
                goto done;
            }

        // LDA addr
        case 33: goto next;
        case 34:
            readT1_(pc);
            goto next;
        case 35:
            readT2_();
            ++pc;
            goto next;
        case 36:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setLo_(WZ, getDBus());
                goto next;
            }
        case 37:
            readT1_(pc);
            goto next;
        case 38:
            readT2_();
            ++pc;
            goto next;
        case 39:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setHi_(WZ, getDBus());
                goto next;
            }
        case 40:
            readT1_(pair_[WZ]);
            goto next;
        case 41:
            readT2_();
            goto next;
        case 42:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                a_ = getDBus();
                goto done;
            }

        // STA addr
        case 43: goto next;
        case 44:
            readT1_(pc);
            goto next;
        case 45:
            readT2_();
            ++pc;
            goto next;
        case 46:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setLo_(WZ, getDBus());
                goto next;
            }
        case 47:
            readT1_(pc);
            goto next;
        case 48:
            readT2_();
            ++pc;
            goto next;
        case 49:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setHi_(WZ, getDBus());
                goto next;
            }
        case 50:
            writeT1_(pair_[WZ]);
            goto next;
        case 51:
            writeT2_(a_);
            goto next;
        case 52:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto done;
            }

        // LHLD addr
        case 53: goto next;
        case 54:
            readT1_(pc);
            goto next;
        case 55:
            readT2_();
            ++pc;
            goto next;
        case 56:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setLo_(WZ, getDBus());
                goto next;
            }
        case 57:
            readT1_(pc);
            goto next;
        case 58:
            readT2_();
            ++pc;
            goto next;
        case 59:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setHi_(WZ, getDBus());
                goto next;
            }
        case 60:
            readT1_(pair_[WZ]);
            goto next;
        case 61:
            readT2_();
            ++pair_[WZ];
            goto next;
        case 62:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setLo_(HL, getDBus());
                goto next;
            }
        case 63:
            readT1_(pair_[WZ]);
            goto next;
        case 64:
            readT2_();
            goto next;
        case 65:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setHi_(HL, getDBus());
                goto done;
            }

        // SHLD addr
        case 66: goto next;
        case 67:
            readT1_(pc);
            goto next;
        case 68:
            readT2_();
            ++pc;
            goto next;
        case 69:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setLo_(WZ,getDBus());
                goto next;
            }
        case 70:
            readT1_(pc);
            goto next;
        case 71:
            readT2_();
            ++pc;
            goto next;
        case 72:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setHi_(WZ, getDBus());
                goto next;
            }
        case 73:
            writeT1_(pair_[WZ]);
            goto next;
        case 74:
            writeT2_(lo_(pair_[HL]));
            ++pair_[WZ];
            goto next;
        case 75:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto next;
            }
        case 76:
            writeT1_(pair_[WZ]);
            goto next;
        case 77:
            writeT2_(hi_(pair_[HL]));
            goto next;
        case 78:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto done;
            }

        // LDAX rp
        case 79: goto next;
        case 80:
            readT1_(pair_[rp_()]);
            goto next;
        case 81:
            readT2_();
            goto next;
        case 82:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                a_ = getDBus();
                goto done;
            }

        // STAX rp
        case 83: goto next;
        case 84:
            writeT1_(pair_[rp_()]);
            goto next;
        case 85:
            writeT2_(a_);
            goto next;
        case 86:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto done;
            }

        // XCHG
        case 87: {
            std::swap(pair_[HL], pair_[DE]);
            goto done;
        }

        // ADD r
        case 88:
            add_(getReg(src_()));
            goto done;

        // ADD M
        case 89: goto next;
        case 90:
            readT1_(pair_[HL]);
            goto next;
        case 91:
            readT2_();
            goto next;
        case 92:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                add_(tmp_);
                goto done;
            }

        // ADI data
        case 93: goto next;
        case 94:
            readT1_(pc);
            goto next;
        case 95:
            readT2_();
            ++pc;
            goto next;
        case 96:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                add_(tmp_);
                goto done;
            }

        // ADC r
        case 97:
            adc_(getReg(src_()));
            goto done;

        // ADC M
        case 98: goto next;
        case 99:
            readT1_(pair_[HL]);
            goto next;
        case 100:
            readT2_();
            goto next;
        case 101:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                adc_(tmp_);
                goto done;
            }

        // ACI data
        case 102: goto next;
        case 103:
            readT1_(pc);
            goto next;
        case 104:
            readT2_();
            ++pc;
            goto next;
        case 105:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                adc_(tmp_);
                goto done;
            }

        // SUB r
        case 106:
            sub_(getReg(src_()));
            goto done;

        // SUB M
        case 107: goto next;
        case 108:
            readT1_(pair_[HL]);
            goto next;
        case 109:
            readT2_();
            goto next;
        case 110:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                sub_(tmp_);
                goto done;
            }

        // SUI data
        case 111: goto next;
        case 112:
            readT1_(pc);
            goto next;
        case 113:
            readT2_();
            ++pc;
            goto next;
        case 114:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                sub_(tmp_);
                goto done;
            }

        // SBB r
        case 115:
            sbb_(getReg(src_()));
            goto done;

        // SBB M
        case 116: goto next;
        case 117:
            readT1_(pair_[HL]);
            goto next;
        case 118:
            readT2_();
            goto next;
        case 119:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                sbb_(tmp_);
                goto done;
            }

        // SBI data
        case 120: goto next;
        case 121:
            readT1_(pc);
            goto next;
        case 122:
            readT2_();
            ++pc;
            goto next;
        case 123:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                sbb_(tmp_);
                goto done;
            }

        // INR r
        case 124: goto next;
        case 125:
            setReg_(dst_(), inr_(getReg(dst_())));
            goto done;

        // INR M
        case 126: goto next;
        case 127:
            readT1_(pair_[HL]);
            goto next;
        case 128:
            readT2_();
            goto next;
        case 129:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                goto next;
            }
        case 130:
            writeT1_(pair_[HL]);
            goto next;
        case 131:
            writeT2_(inr_(tmp_));
            goto next;
        case 132:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto done;
            }

        // DCR r
        case 133: goto next;
        case 134:
            setReg_(dst_(), dcr_(getReg(dst_())));
            goto done;

        // DCR M
        case 135: goto next;
        case 136:
            readT1_(pair_[HL]);
            goto next;
        case 137:
            readT2_();
            goto next;
        case 138:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                goto next;
            }
        case 139:
            writeT1_(pair_[HL]);
            goto next;
        case 140:
            writeT2_(dcr_(tmp_));
            goto next;
        case 141:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto done;
            }

        // INX rp
        case 142: goto next;
        case 143:
            ++pair_[rp_()];
            goto done;

        // DCX rp
        case 144: goto next;
        case 145:
            --pair_[rp_()];
            goto done;

        // DAD
        case 146: case 147: case 148: case 149: case 150: case 151: goto next;
        case 152: {
            setCarryFlag_(pair_[HL] + pair_[rp_()] > 0xFFFF);
            pair_[HL] = pair_[HL] + pair_[rp_()];
            goto done;
        }

        // DAA
        case 153: {
            std::uint8_t addend {0};
            std::uint8_t msb {static_cast<uint8_t>(a_ & 0xF0)}, lsb {static_cast<uint8_t>(a_ & 0xF)};

            if (lsb > 9 or ac())
                addend = 6U;
            setAuxCarryFlag_(lsb + addend > 0xF);

            if (msb > 0x90 or cy() or (msb >= 0x90 and lsb > 9)) {
                addend += 0x60U;
                setCarryFlag_(true);
            }

            a_ += addend;
            zspFlags_(a_);
            goto done;
        }

        // ANA r
        case 154:
            ana_(getReg(src_()));
            goto done;

        // ANA M
        case 155: goto next;
        case 156:
            readT1_(pair_[HL]);
            goto next;
        case 157:
            readT2_();
            goto next;
        case 158:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                ana_(tmp_);
                goto done;
            }

        // ANI data
        case 159: goto next;
        case 160:
            readT1_(pc);
            goto next;
        case 161:
            readT2_();
            ++pc;
            goto next;
        case 162:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                ani_(tmp_);
                goto done;
            }

        // XRA data
        case 163:
            xra_(getReg(src_()));
            goto done;

        // XRA M
        case 164: goto next;
        case 165:
            readT1_(pair_[HL]);
            goto next;
        case 166:
            readT2_();
            goto next;
        case 167:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                xra_(tmp_);
                goto done;
            }

        // XRI data
        case 168: goto next;
        case 169:
            readT1_(pc);
            goto next;
        case 170:
            readT2_();
            ++pc;
            goto next;
        case 171:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                xra_(tmp_);
                goto done;
            }

        // ORA r
        case 172:
            ora_(getReg(src_()));
            goto done;

        // ORA M
        case 173: goto next;
        case 174:
            readT1_(pair_[HL]);
            goto next;
        case 175:
            readT2_();
            goto next;
        case 176:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                ora_(tmp_);
                goto done;
            }

        // ORI data
        case 177: goto next;
        case 178:
            readT1_(pc);
            goto next;
        case 179:
            readT2_();
            ++pc;
            goto next;
        case 180:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                ora_(tmp_);
                goto done;
            }

        // CMP r
        case 181:
            cmp_(getReg(src_()));
            goto done;

        // CMP M
        case 182: goto next;
        case 183:
            readT1_(pair_[HL]);
            goto next;
        case 184:
            readT2_();
            goto next;
        case 185:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                cmp_(tmp_);
                goto done;
            }

        // CPI data
        case 186: goto next;
        case 187:
            readT1_(pc);
            goto next;
        case 188:
            readT2_();
            ++pc;
            goto next;
        case 189:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                tmp_ = getDBus();
                cmp_(tmp_);
                goto done;
            }

        // RLC
        case 190:
            setCarryFlag_(a_ & 0x80);
            a_ = (a_ << 1U) | cy();
            goto done;

        // RRC
        case 191:
            setCarryFlag_(a_ & 0x01);
            a_ = (a_ >> 1U) | (cy() << 7U);
            goto done;

        // RAL
        case 192: {
            const std::uint8_t carry {cy()};
            setCarryFlag_(a_ & 0x80);
            a_ = (a_ << 1U) | carry;
            goto done;
        }

        // RAR
        case 193: {
            const std::uint8_t carry {cy()};
            setCarryFlag_(a_ & 0x01);
            a_ = (a_ >> 1U) | (carry << 7U);
            goto done;
        }

        // CMA
        case 194:
            a_ = ~a_;
            goto done;

        // CMC
        case 195:
            setCarryFlag_(!cy());
            goto done;

        // STC
        case 196:
            setCarryFlag_(true);
            goto done;

        // JMP addr
        case 197: goto next;
        case 198:
            readT1_(pc);
            goto next;
        case 199:
            readT2_();
            ++pc;
            goto next;
        case 200:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setLo_(WZ, getDBus());
                goto next;
            }
        case 201:
            readT1_(pc);
            goto next;
        case 202:
            readT2_();
            ++pc;
            goto next;
        case 203:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setHi_(WZ, getDBus());
                pc = pair_[WZ];
                goto done;
            }

        // J cond addr
        case 204: goto next;
        case 205:
            readT1_(pc);
            goto next;
        case 206:
            readT2_();
            ++pc;
            goto next;
        case 207:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setLo_(WZ, getDBus());
                goto next;
            }
        case 208:
            readT1_(pc);
            goto next;
        case 209:
            readT2_();
            ++pc;
            goto next;
        case 210:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setHi_(WZ, getDBus());
                if (ccc_())
                    pc = pair_[WZ];
                goto done;
            }

        // CALL addr
        case 211: goto next;
        case 212:
            --pair_[SP];
            goto next;
        case 213:
            readT1_(pc);
            goto next;
        case 214:
            readT2_();
            ++pc;
            goto next;
        case 215:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setLo_(WZ, getDBus());
                goto next;
            }
        case 216:
            readT1_(pc);
            goto next;
        case 217:
            readT2_();
            ++pc;
            goto next;
        case 218:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setHi_(WZ, getDBus());
                goto next;
            }
        case 219:
            stackWriteT1_();
            goto next;
        case 220:
            writeT2_(hi_(pc));
            --pair_[SP];
            goto next;
        case 221:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto next;
            }
        case 222:
            stackWriteT1_();
            goto next;
        case 223:
            writeT2_(lo_(pc));
            goto next;
        case 224:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                pc = pair_[WZ];
                goto done;
            }

        // CALL cond addr
        case 225: goto next;
        case 226:
            if (ccc_())
                --pair_[SP];
            goto next;
        case 227:
            readT1_(pc);
            goto next;
        case 228:
            readT2_();
            ++pc;
            goto next;
        case 229:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setLo_(WZ, getDBus());
                goto next;
            }
        case 230:
            readT1_(pc);
            goto next;
        case 231:
            readT2_();
            ++pc;
            goto next;
        case 232:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setHi_(WZ, getDBus());
                if (ccc_())
                    goto next;
                goto done;
            }
        case 233:
            stackWriteT1_();
            goto next;
        case 234:
            writeT2_(hi_(pc));
            --pair_[SP];
            goto next;
        case 235:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto next;
            }
        case 236:
            stackWriteT1_();
            goto next;
        case 237:
            writeT2_(lo_(pc));
            goto next;
        case 238:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                pc = pair_[WZ];
                goto done;
            }

        // RET
        case 239: goto next;
        case 240:
            stackReadT1_();
            goto next;
        case 241:
            readT2_();
            ++pair_[SP];
            goto next;
        case 242:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setLo_(WZ, getDBus());
                goto next;
            }
        case 243:
            stackReadT1_();
            goto next;
        case 244:
            readT2_();
            ++pair_[SP];
            goto next;
        case 245:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setHi_(WZ, getDBus());
                pc = pair_[WZ];
                goto done;
            }

        // R cond addr
        case 246: goto next;
        case 247:
            if (ccc_())
                goto next;
            goto done;
        case 248:
            stackReadT1_();
            goto next;
        case 249:
            readT2_();
            ++pair_[SP];
            goto next;
        case 250:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setLo_(WZ, getDBus());
                goto next;
            }
        case 251:
            stackReadT1_();
            goto next;
        case 252:
            readT2_();
            ++pair_[SP];
            goto next;
        case 253:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setHi_(WZ, getDBus());
                pc = pair_[WZ];
                goto done;
            }

        // RST n
        case 254: goto next;
        case 255:
            --pair_[SP];
            goto next;
        case 256:
            stackWriteT1_();
            goto next;
        case 257:
            writeT2_(hi_(pc));
            --pair_[SP];
            goto next;
        case 258:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto next;
            }
        case 259:
            stackWriteT1_();
            goto next;
        case 260:
            writeT2_(lo_(pc));
            goto next;
        case 261:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                pair_[WZ] = nnn_();
                pc = pair_[WZ];
                goto done;
            }

        // PCHL
        case 262: goto next;
        case 263:
            pc = pair_[HL];
            goto done;

        // PUSH rp
        case 264: goto next;
        case 265:
            --pair_[SP];
            goto next;
        case 266:
            stackWriteT1_();
            goto next;
        case 267:
            --pair_[SP];
            writeT2_(hi_(pair_[rp_()]));
            goto next;
        case 268:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto next;
            }
        case 269:
            stackWriteT1_();
            goto next;
        case 270:
            writeT2_(lo_(pair_[rp_()]));
            goto next;
        case 271:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto done;
            }

        // PUSH PSW
        case 272: goto next;
        case 273:
            --pair_[SP];
            goto next;
        case 274:
            stackWriteT1_();
            goto next;
        case 275:
            --pair_[SP];
            writeT2_(a_);
            goto next;
        case 276:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto next;
            }
        case 277:
            stackWriteT1_();
            goto next;
        case 278:
            writeT2_(psw_());
            goto next;
        case 279:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto done;
            }

        // POP rp
        case 280: goto next;
        case 281:
            stackReadT1_();
            goto next;
        case 282:
            ++pair_[SP];
            readT2_();
            goto next;
        case 283:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setLo_(rp_(), getDBus());
                goto next;
            }
        case 284:
            stackReadT1_();
            goto next;
        case 285:
            ++pair_[SP];
            readT2_();
            goto next;
        case 286:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setHi_(rp_(), getDBus());
                goto done;
            }

        // POP psw
        case 287: goto next;
        case 288:
            stackReadT1_();
            goto next;
        case 289:
            ++pair_[SP];
            readT2_();
            goto next;
        case 290:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                f_ = getDBus() & 0b11010111 | 0b10; // bits 3 and 5 are always zero, bit 2 is always one
                goto next;
            }
        case 291:
            stackReadT1_();
            goto next;
        case 292:
            ++pair_[SP];
            readT2_();
            goto next;
        case 293:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                a_ = getDBus();
                goto done;
            }

        // XTHL
        case 294: goto next;
        case 295:
            stackReadT1_(pair_[SP]);
            goto next;
        case 296:
            readT2_();
            goto next;
        case 297:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setLo_(WZ, getDBus());
                goto next;
            }
        case 298:
            stackReadT1_(pair_[SP] + 1);
            goto next;
        case 299:
            readT2_();
            goto next;
        case 300:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                setHi_(WZ, getDBus());
                goto next;
            }
        case 301:
            stackWriteT1_(pair_[SP] + 1);
            goto next;
        case 302:
            writeT2_(hi_(pair_[HL]));
            goto next;
        case 303:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto next;
            }
        case 304:
            stackWriteT1_(pair_[SP]);
            goto next;
        case 305:
            writeT2_(lo_(pair_[HL]));
            goto next;
        case 306:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto next;
            }
        case 307: goto next;
        case 308:
            pair_[HL] = pair_[WZ];
            goto done;

        // IN port
        case 309: goto next;
        case 310:
            readT1_(pc);
            goto next;
        case 311:
            readT2_();
            ++pc;
            goto next;
        case 312:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                pair_[WZ] = getDBus();
                goto next;
            }
        case 313:
            inputReadT1_();
            goto next;
        case 314:
            readT2_();
            goto next;
        case 315:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                a_ = getDBus();
                goto done;
            }

        // OUT port
        case 316: goto next;
        case 317:
            readT1_(pc);
            goto next;
        case 318:
            readT2_();
            ++pc;
            goto next;
        case 319:
            if (waiting_()) goto wait;
            else {
                stopDataIn_();
                pair_[WZ] = getDBus();
                goto next;
            }
        case 320:
            outputWriteT1_();
            goto next;
        case 321:
            writeT2_(a_);
            goto next;
        case 322:
            if (waiting_()) goto wait;
            else {
                stopDataOut_();
                goto done;
            }

        // EI
        case 323:
            pins |= INTE;
            goto done;

        // DI
        case 324:
            pins &= ~INTE;
            goto done;

        // HLT
        case 325: goto next;
        case 326:
            setABus_(pc);
            setDBus(WO|HLTA|MEMR);
            status = getDBus();
            goto next;
        case 327:
            stopped_ = true;
            goto done;

        // NOP
        case 328: goto done;
    }

    // see (https://floooh.github.io/2021/12/17/cycle-stepped-z80.html)
    wait:
        if (pins & READY)
            pins &= ~WAIT;
        return;
    next:
        ++step_;
        return;
    done:
        step_ = 0;
        return;
}

bool Intel8080::ccc_() const {
    switch (ir_ >> 3U & 7U) {
        // NZ - not zero (Z = 0)
        case 0b000: return (z()) == 0;
        // Z - zero (Z = 1)
        case 0b001: return (z()) != 0;
        // NC - no carry (CY = 0)
        case 0b010: return (cy()) == 0;
        // C - carry (CY = 1)
        case 0b011: return (cy()) != 0;
        // PO - parity odd (P = 0)
        case 0b100: return (p()) == 0;
        // PE - parity even (P = 1)
        case 0b101: return (p()) != 0;
        // P - plus (S = 0)
        case 0b110: return (s()) == 0;
        // M - minus (S = 1)
        case 0b111: return (s()) != 0;
        default: return false;
    }
}

void Intel8080::setReg_(const std::uint8_t r, const std::uint8_t val) {
    switch (r) {
        case B: setHi_(BC, val); break;
        case C: setLo_(BC, val); break;
        case D: setHi_(DE, val); break;
        case E: setLo_(DE, val); break;
        case H: setHi_(HL, val); break;
        case L: setLo_(HL, val); break;
        case A: a_ = val; break;
        case F: f_ = val; break;
        default: break;
    }
}

std::uint8_t Intel8080::getReg(const std::uint8_t r) const {
    switch (r) {
        case B: return hi_(pair_[BC]);
        case C: return lo_(pair_[BC]);
        case D: return hi_(pair_[DE]);
        case E: return lo_(pair_[DE]);
        case H: return hi_(pair_[HL]);
        case L: return lo_(pair_[HL]);
        case A: return a_;
        case F: return f_;
        default: return -1;
    }
}

inline void Intel8080::t1_()
{
    pins |= SYNC;
    status = getDBus(); // technically status is latched in phi1 of T2 but im not implementing two clocks
                        // and the status needs to be ready in the same phase as the SYNC signal
}

inline void Intel8080::readT1_(const std::uint16_t addr)
{
    setABus_(addr);
    setDBus(WO|MEMR);
    t1_();
}

inline void Intel8080::writeT1_(const std::uint16_t addr)
{
    setABus_(addr);
    setDBus(uint_fast64_t(0ULL));
    t1_();
}

inline void Intel8080::stackWriteT1_()
{
    setABus_(pair_[SP]);
    setDBus(STACK);
    t1_();
}

inline void Intel8080::stackWriteT1_(const std::uint16_t addr)
{
    setABus_(addr);
    setDBus(STACK);
    t1_();
}

inline void Intel8080::stackReadT1_()
{
    setABus_(pair_[SP]);
    setDBus(WO|STACK|MEMR);
    t1_();
}

inline void Intel8080::stackReadT1_(const std::uint16_t addr)
{
    setABus_(addr);
    setDBus(WO|STACK|MEMR);
    t1_();
}

inline void Intel8080::inputReadT1_()
{
    setABus_(pair_[WZ]);
    setDBus(WO|INP);
    t1_();
}

void Intel8080::outputWriteT1_()
{
    setABus_(pair_[WZ]);
    setDBus(OUT);
    t1_();
}

inline void Intel8080::t2_()
{
    pins &= ~SYNC;
    if (!(pins & READY))
        pins |= WAIT;
}

inline void Intel8080::readT2_()
{
    t2_();
    pins |= DBIN;
}

inline void Intel8080::writeT2_(const std::uint8_t r)
{
    t2_();
    pins |= WR;
    setDBus(r);
}

inline void Intel8080::add_(const std::uint8_t addend)
{
    carryFlagsAdd_(addend);
    a_ += addend;
    zspFlags_(a_);
}

inline void Intel8080::adc_(const std::uint8_t addend)
{
    std::uint8_t carry {cy()}; // need temp because carry is updated before accumulator
    carryFlagsAdd_(addend, carry);
    a_ += addend + carry;
    zspFlags_(a_);
}

inline void Intel8080::sub_(const std::uint8_t subtrahend)
{
    carryFlagsSub_(subtrahend);
    a_ -= subtrahend;
    zspFlags_(a_);
}

inline void Intel8080::sbb_(const std::uint8_t subtrahend)
{
    std::uint8_t carry {cy()}; // need temp because carry is updated before accumulator
    carryFlagsSub_(subtrahend, carry);
    a_ = a_ - subtrahend - carry;
    zspFlags_(a_);
}

inline std::uint8_t Intel8080::inr_(std::uint8_t operand)
{
    ++operand;
    setAuxCarryFlag_((operand & 0xFU) == 0); // only case for half carry is 01111 + 1 = 10000
    zspFlags_(operand);
    return operand;
}

inline std::uint8_t Intel8080::dcr_(std::uint8_t operand)
{
    --operand;
    setAuxCarryFlag_((operand & 0xFU) != 0xF); // only case for half borrow is 10000 - 1 = 01111
    zspFlags_(operand);
    return operand;
}

inline void Intel8080::ana_(const std::uint8_t operand)
{
    carryFlagsAnd_(operand);
    a_ &= operand;
    zspFlags_(a_);
}

inline void Intel8080::ani_(const std::uint8_t operand)
{
    carryFlagsAnd_(operand);
    a_ &= operand;
    zspFlags_(a_);
}

inline void Intel8080::xra_(const std::uint8_t operand)
{
    carryFlagsAlg_();
    a_ ^= operand;
    zspFlags_(a_);
}

inline void Intel8080::ora_(const std::uint8_t operand)
{
    carryFlagsAlg_();
    a_ |= operand;
    zspFlags_(a_);
}

inline void Intel8080::cmp_(const std::uint8_t operand)
{
    carryFlagsSub_(operand);
    zspFlags_(a_ - operand);
}

inline void Intel8080::carryFlagsAlg_() {
    setAuxCarryFlag_(false);
    setCarryFlag_(false);
}

inline void Intel8080::carryFlagsAnd_(const std::uint8_t operand)
{
    setAuxCarryFlag_((a_ | operand) & 0b1000U);
    setCarryFlag_(false);
}

inline void Intel8080::carryFlagsAdd_(const std::uint8_t addend, std::uint8_t cy)
{
    std::uint16_t res {static_cast<uint16_t>(a_ + addend + cy)};
    setAuxCarryFlag_((a_ ^ addend ^ res) & 0x10U);
    setCarryFlag_(res > 0xFF);
}

inline void Intel8080::carryFlagsSub_(const std::uint8_t subtrahend, std::uint8_t cy)
{
    std::uint16_t res {static_cast<uint16_t>(a_ - subtrahend - cy)};
    setAuxCarryFlag_(~(a_ ^ subtrahend ^ res) & 0x10U);
    setCarryFlag_(a_ < subtrahend + cy);
}

inline void Intel8080::zspFlags_(const std::uint8_t val)
{
    setZeroFlag_(val == 0);
    setSignFlag_(val & 0x80U);
    setParityFlag_(std::popcount(val) % 2 == 0);
}