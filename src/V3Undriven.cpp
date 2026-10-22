// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Check for unused/undriven signals
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2004-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// V3Undriven's Transformations:
//
// Netlist:
//      Make vector for all variables
//      SEL(VARREF(...))) mark only some bits as used/driven
//      else VARREF(...) mark all bits as used/driven
//      Report unused/undriven nets
//
//*************************************************************************

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Undriven.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UndrivenCapture.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Width.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Class for every variable we may process

ਵਰਗ Undriven_Var_ਇੰਦਰਾਜ final {
    // MEMBERS
    AstVar* ਅਚਲ m_varp;  // Variable this tracks
    std::ਸਦਿਸ਼<ਤਾਰਕਿਕ> m_ਸਾਰਾ_Flags;  // Used/Driven on whole vector
    std::ਸਦਿਸ਼<ਤਾਰਕਿਕ> m_ਦੋਆਧਾਰੀ_ਅੰਕ_Flags;  // Used/Driven on each subbit
    ਅਚਲ Ast_ਗੰਢ* m_ਵਰਤਿਆ_Not_Drivenp = ਖਾਲੀ_ਸੂਚਕ;  // First read before any write
    ਅਚਲ AstAlways* m_alwCombp
        = ਖਾਲੀ_ਸੂਚਕ;  // always_comb of var if driven within always_comb, else nullptr
    ਅਚਲ AstAlways* m_alwFFp = ਖਾਲੀ_ਸੂਚਕ;  // always_ff of var if driven within always_ff
    ਅਚਲ AstAlways* m_alwPlainp = ਖਾਲੀ_ਸੂਚਕ;  // plain always of var if driven within plain always
    ਅਚਲ AstClocking* m_clockingp = ਖਾਲੀ_ਸੂਚਕ;  // clocking block of var if driven as output
    ਅਚਲ Ast_ਗੰਢ_Var_Ref* m_nodep = ਖਾਲੀ_ਸੂਚਕ;  // varref if driven, else nullptr
    ਅਚਲ Ast_ਗੰਢ* m_initStaticp = ਖਾਲੀ_ਸੂਚਕ;  // varref if in InitialStatic driven
    ਅਚਲ Ast_ਗੰਢ* m_initialp = ਖਾਲੀ_ਸੂਚਕ;  // varref if driven in an explicit initial block
    ਅਚਲ Ast_ਗੰਢ* m_contAssignp = ਖਾਲੀ_ਸੂਚਕ;  // varref if in continuous assignment driven
    ਅਚਲ Ast_ਗੰਢ* m_procWritep = ਖਾਲੀ_ਸੂਚਕ;  // varref if written in process
    ਤਾਰਕਿਕ m_underGen = ਝੂਠ;  // Under a generate
    ਤਾਰਕਿਕ m_ftaskDriven = ਝੂਠ;  // Last driven by function or task

    ਅਚਲ Ast_ਗੰਢ_FTask_Ref* m_callNodep = ਖਾਲੀ_ਸੂਚਕ;  // Call node if driven via writeSummary

    ਗਿਣਤੀ : uint8_t {
        FLAG_ਵਰਤਿਆ = 0,  // Signal or bit has been read/observed
        FLAG_DRIVEN = 1,  // Signal or bit has been written/driven
        FLAG_DRIVEN_ALWCOMB = 2,  // Whole signal has been driven from always_comb
        FLAG_DRIVEN_ALWFF = 3,  // Whole signal has been driven from always_ff
        FLAG_DRIVEN_ALWPLAIN = 4,  // Whole signal has been driven from a plain always
        FLAG_DRIVEN_CLOCKING = 5,  // Whole signal has been driven as a clocking block output
        FLAGS_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ = 6  // Number of flags stored for each tracked bit
    };

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ Undriven_Var_ਇੰਦਰਾਜ(AstVar* varp)
        : m_varp{varp} {  // Construction for when a var is used
        UINFO(9, "create " << varp);
        m_ਸਾਰਾ_Flags.resize(FLAGS_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ);
        ਲਈ (ਪੂਰਨ i = 0; i < FLAGS_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ; i++) m_ਸਾਰਾ_Flags[i] = ਝੂਠ;
        m_ਦੋਆਧਾਰੀ_ਅੰਕ_Flags.resize(varp->ਚੌੜਾਈ() * FLAGS_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ);
        ਲਈ (ਪੂਰਨ i = 0; i < varp->ਚੌੜਾਈ() * FLAGS_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ; i++) m_ਦੋਆਧਾਰੀ_ਅੰਕ_Flags[i] = ਝੂਠ;
    }
    ~Undriven_Var_ਇੰਦਰਾਜ() = ਮੂਲ;

ਨਿਜੀ:
    // METHODS
    ਤਾਰਕਿਕ ਦੋਆਧਾਰੀ_ਅੰਕ_ਅੰਕ_Ok(ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2) ਅਚਲ {
        ਵਾਪਸ ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= 0 && (ਦੋਆਧਾਰੀ_ਅੰਕ_2 * FLAGS_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ < static_cast<ਪੂਰਨ>(m_ਦੋਆਧਾਰੀ_ਅੰਕ_Flags.ਮਾਪ()));
    }
    ਤਾਰਕਿਕ ਵਰਤਿਆ_Flag(ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2) ਅਚਲ {
        ਵਾਪਸ m_ਸਾਰਾ_Flags[FLAG_ਵਰਤਿਆ] || m_ਦੋਆਧਾਰੀ_ਅੰਕ_Flags[ਦੋਆਧਾਰੀ_ਅੰਕ_2 * FLAGS_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ + FLAG_ਵਰਤਿਆ];
    }
    ਤਾਰਕਿਕ drivenFlag(ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2) ਅਚਲ {
        ਵਾਪਸ m_ਸਾਰਾ_Flags[FLAG_DRIVEN] || m_ਦੋਆਧਾਰੀ_ਅੰਕ_Flags[ਦੋਆਧਾਰੀ_ਅੰਕ_2 * FLAGS_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ + FLAG_DRIVEN];
    }
    ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_ਗਿਣਤੀ() ਅਚਲ { ਵਾਪਸ m_ਦੋਆਧਾਰੀ_ਅੰਕ_Flags.ਮਾਪ() / FLAGS_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ; }
    ਖਾਲੀ ਰਿਕਾਰਡ_ਵਰਤਿਆ_Not_Driven(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਜੇ (!m_ਵਰਤਿਆ_Not_Drivenp) m_ਵਰਤਿਆ_Not_Drivenp = nodep;
    }
    ਗਿਣਤੀ ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਾਂ_Which : uint8_t { BN_ਅਣਵਰਤਿਆ, BN_UNDRIVEN, BN_BOTH };
    string ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਾਂ(ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਾਂ_Which which) {
        string ਬਿੱਟ;
        ਤਾਰਕਿਕ prev = ਝੂਠ;
        ਪੂਰਨ msb = 0;
        // bit==-1 loops below; we do one extra iteration so end with prev=false
        ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = (m_ਦੋਆਧਾਰੀ_ਅੰਕ_Flags.ਮਾਪ() / FLAGS_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ) - 1; ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= -1; --ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
            ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= 0
                && ((which == BN_ਅਣਵਰਤਿਆ && !ਵਰਤਿਆ_Flag(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && drivenFlag(ਦੋਆਧਾਰੀ_ਅੰਕ_2))
                    || (which == BN_UNDRIVEN && ਵਰਤਿਆ_Flag(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && !drivenFlag(ਦੋਆਧਾਰੀ_ਅੰਕ_2))
                    || (which == BN_BOTH && !ਵਰਤਿਆ_Flag(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && !drivenFlag(ਦੋਆਧਾਰੀ_ਅੰਕ_2)))) {
                ਜੇ (!prev) {
                    prev = ਸੱਚ;
                    msb = ਦੋਆਧਾਰੀ_ਅੰਕ_2;
                }
            } ਨਹੀਂਤਾਂ ਜੇ (prev) {
                ਅਚਲ AstBasicDType* ਅਚਲ bdtypep = m_varp->basicp();
                ਅਚਲ ਪੂਰਨ lsb = ਦੋਆਧਾਰੀ_ਅੰਕ_2 + 1;
                ਜੇ (ਬਿੱਟ != "") ਬਿੱਟ += ",";
                ਜੇ (lsb == msb) {
                    ਬਿੱਟ += cvt_ਮੰਜ਼ਿਲ_Str(lsb + bdtypep->lo());
                } ਨਹੀਂਤਾਂ {
                    ਜੇ (bdtypep->ascending()) {
                        ਬਿੱਟ
                            += cvt_ਮੰਜ਼ਿਲ_Str(lsb + bdtypep->lo()) + ":" + cvt_ਮੰਜ਼ਿਲ_Str(msb + bdtypep->lo());
                    } ਨਹੀਂਤਾਂ {
                        ਬਿੱਟ
                            += cvt_ਮੰਜ਼ਿਲ_Str(msb + bdtypep->lo()) + ":" + cvt_ਮੰਜ਼ਿਲ_Str(lsb + bdtypep->lo());
                    }
                }
                prev = ਝੂਠ;
            }
        }
        ਵਾਪਸ "[" + ਬਿੱਟ + "]";
    }

ਜਨਤਕ:
    ਖਾਲੀ ਵਰਤਿਆ_ਸਾਰਾ(ਅਚਲ Ast_ਗੰਢ* nodep) {
        UINFO(9, "set u[*] " << m_varp->name() << " " << nodep);
        ਜੇ (!m_ਸਾਰਾ_Flags[FLAG_DRIVEN]) {
            ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < ਦੋਆਧਾਰੀ_ਅੰਕ_ਗਿਣਤੀ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
                ਜੇ (!drivenFlag(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
                    ਰਿਕਾਰਡ_ਵਰਤਿਆ_Not_Driven(nodep);
                    ਰੋਕੋ;
                }
            }
        }
        m_ਸਾਰਾ_Flags[FLAG_ਵਰਤਿਆ] = ਸੱਚ;
    }
    ਖਾਲੀ driven_ਸਾਰਾ(ਅਚਲ Ast_ਗੰਢ* nodep) {
        UINFO(9, "set d[*] " << m_varp->name() << " " << nodep);
        m_ਸਾਰਾ_Flags[FLAG_DRIVEN] = ਸੱਚ;
    }
    ਖਾਲੀ driven_ਸਾਰਾ(ਅਚਲ Ast_ਗੰਢ_Var_Ref* nodep, ਤਾਰਕਿਕ ftaskDef) {
        ਅਚਲ ਤਾਰਕਿਕ wasDriven = is_Driven_ਸਾਰਾ();
        driven_ਸਾਰਾ(nodep);
        // A write in a task/function definition is not an executed process, so it must not
        // displace an already recorded write, nor clear the flag that keeps such a write
        // from being reported as the other write of a MULTIDRIVEN
        ਜੇ (ftaskDef && wasDriven) ਵਾਪਸ;
        m_ftaskDriven = ftaskDef;
        m_nodep = nodep;
    }
    ਖਾਲੀ driven_Always_Comb_ਸਾਰਾ(ਅਚਲ AstAlways* alwCombp) {
        m_ਸਾਰਾ_Flags[FLAG_DRIVEN_ALWCOMB] = ਸੱਚ;
        m_alwCombp = alwCombp;
    }
    ਖਾਲੀ drivenAlwaysFFWhole(ਅਚਲ AstAlways* alwFFp, ਅਚਲ AstVar* varp) {
        m_ਸਾਰਾ_Flags[FLAG_DRIVEN_ALWFF] = ਸੱਚ;
        m_alwFFp = alwFFp;
    }
    ਖਾਲੀ driven_Always_Plain_ਸਾਰਾ(ਅਚਲ AstAlways* alwPlainp) {
        m_ਸਾਰਾ_Flags[FLAG_DRIVEN_ALWPLAIN] = ਸੱਚ;
        m_alwPlainp = alwPlainp;
    }
    ਖਾਲੀ driven_Clocking_ਸਾਰਾ(ਅਚਲ AstClocking* clockingp) {
        m_ਸਾਰਾ_Flags[FLAG_DRIVEN_CLOCKING] = ਸੱਚ;
        m_clockingp = clockingp;
    }

    ਅਚਲ Ast_ਗੰਢ* initStaticp() ਅਚਲ { ਵਾਪਸ m_initStaticp; }
    ਖਾਲੀ initStaticp(ਅਚਲ Ast_ਗੰਢ* nodep) { m_initStaticp = nodep; }
    ਅਚਲ Ast_ਗੰਢ* initialp() ਅਚਲ { ਵਾਪਸ m_initialp; }
    ਖਾਲੀ initialp(ਅਚਲ Ast_ਗੰਢ* nodep) { m_initialp = nodep; }
    ਅਚਲ Ast_ਗੰਢ* contAssignp() ਅਚਲ { ਵਾਪਸ m_contAssignp; }
    ਖਾਲੀ contAssignp(ਅਚਲ Ast_ਗੰਢ* nodep) { m_contAssignp = nodep; }
    ਅਚਲ Ast_ਗੰਢ* procWritep() ਅਚਲ { ਵਾਪਸ m_procWritep; }
    ਖਾਲੀ procWritep(ਅਚਲ Ast_ਗੰਢ* nodep) { m_procWritep = nodep; }
    ਖਾਲੀ underGenerate() { m_underGen = ਸੱਚ; }
    ਤਾਰਕਿਕ isUnderGen() ਅਚਲ { ਵਾਪਸ m_underGen; }
    ਤਾਰਕਿਕ is_Driven_ਸਾਰਾ() ਅਚਲ { ਵਾਪਸ m_ਸਾਰਾ_Flags[FLAG_DRIVEN]; }
    ਤਾਰਕਿਕ is_Driven_Always_Comb_ਸਾਰਾ() ਅਚਲ { ਵਾਪਸ m_ਸਾਰਾ_Flags[FLAG_DRIVEN_ALWCOMB]; }
    ਤਾਰਕਿਕ isDrivenAlwaysFFWhole() ਅਚਲ { ਵਾਪਸ m_ਸਾਰਾ_Flags[FLAG_DRIVEN_ALWFF]; }
    ਤਾਰਕਿਕ is_Driven_Always_Plain_ਸਾਰਾ() ਅਚਲ { ਵਾਪਸ m_ਸਾਰਾ_Flags[FLAG_DRIVEN_ALWPLAIN]; }
    ਤਾਰਕਿਕ is_Driven_Clocking_ਸਾਰਾ() ਅਚਲ { ਵਾਪਸ m_ਸਾਰਾ_Flags[FLAG_DRIVEN_CLOCKING]; }
    ਤਾਰਕਿਕ isFtaskDriven() ਅਚਲ { ਵਾਪਸ m_ftaskDriven; }
    ਅਚਲ Ast_ਗੰਢ_Var_Ref* getNodep() ਅਚਲ { ਵਾਪਸ m_nodep; }
    ਅਚਲ AstAlways* getAlwCombp() ਅਚਲ { ਵਾਪਸ m_alwCombp; }
    ਅਚਲ AstAlways* getAlwFFp() ਅਚਲ { ਵਾਪਸ m_alwFFp; }
    ਅਚਲ AstAlways* getAlwPlainp() ਅਚਲ { ਵਾਪਸ m_alwPlainp; }
    ਅਚਲ AstClocking* getClockingp() ਅਚਲ { ਵਾਪਸ m_clockingp; }
    ਖਾਲੀ ਵਰਤਿਆ_ਦੋਆਧਾਰੀ_ਅੰਕ_2(ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2, ਪੂਰਨ ਚੌੜਾਈ, ਅਚਲ Ast_ਗੰਢ* nodep) {
        UINFO(9, "set u[" << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 + ਚੌੜਾਈ - 1) << ":" << ਦੋਆਧਾਰੀ_ਅੰਕ_2 << "] " << m_varp->name());
        ਲਈ (ਪੂਰਨ i = 0; i < ਚੌੜਾਈ; i++) {
            ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_ਅੰਕ_Ok(ਦੋਆਧਾਰੀ_ਅੰਕ_2 + i)) {
                ਜੇ (!drivenFlag(ਦੋਆਧਾਰੀ_ਅੰਕ_2 + i)) ਰਿਕਾਰਡ_ਵਰਤਿਆ_Not_Driven(nodep);
                m_ਦੋਆਧਾਰੀ_ਅੰਕ_Flags[(ਦੋਆਧਾਰੀ_ਅੰਕ_2 + i) * FLAGS_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ + FLAG_ਵਰਤਿਆ] = ਸੱਚ;
            }
        }
    }
    ਖਾਲੀ driven_ਦੋਆਧਾਰੀ_ਅੰਕ(ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2, ਪੂਰਨ ਚੌੜਾਈ) {
        UINFO(9, "set d[" << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 + ਚੌੜਾਈ - 1) << ":" << ਦੋਆਧਾਰੀ_ਅੰਕ_2 << "] " << m_varp->name());
        ਲਈ (ਪੂਰਨ i = 0; i < ਚੌੜਾਈ; i++) {
            ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_ਅੰਕ_Ok(ਦੋਆਧਾਰੀ_ਅੰਕ_2 + i)) m_ਦੋਆਧਾਰੀ_ਅੰਕ_Flags[(ਦੋਆਧਾਰੀ_ਅੰਕ_2 + i) * FLAGS_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ + FLAG_DRIVEN] = ਸੱਚ;
        }
    }
    ਤਾਰਕਿਕ is_ਵਰਤਿਆ_Not_Driven_ਦੋਆਧਾਰੀ_ਅੰਕ(ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2, ਪੂਰਨ ਚੌੜਾਈ) ਅਚਲ {
        ਲਈ (ਪੂਰਨ i = 0; i < ਚੌੜਾਈ; i++) {
            ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_ਅੰਕ_Ok(ਦੋਆਧਾਰੀ_ਅੰਕ_2 + i)
                && (m_ਸਾਰਾ_Flags[FLAG_ਵਰਤਿਆ] || m_ਦੋਆਧਾਰੀ_ਅੰਕ_Flags[(ਦੋਆਧਾਰੀ_ਅੰਕ_2 + i) * FLAGS_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ + FLAG_ਵਰਤਿਆ])
                && !(m_ਸਾਰਾ_Flags[FLAG_DRIVEN]
                     || m_ਦੋਆਧਾਰੀ_ਅੰਕ_Flags[(ਦੋਆਧਾਰੀ_ਅੰਕ_2 + i) * FLAGS_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ + FLAG_DRIVEN]))
                ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }
    ਤਾਰਕਿਕ is_ਵਰਤਿਆ_Not_Driven_Any() ਅਚਲ {
        ਵਾਪਸ is_ਵਰਤਿਆ_Not_Driven_ਦੋਆਧਾਰੀ_ਅੰਕ(0, m_ਦੋਆਧਾਰੀ_ਅੰਕ_Flags.ਮਾਪ() / FLAGS_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ);
    }
    ਅਚਲ Ast_ਗੰਢ* ਪਹਿਲਾ_ਵਰਤਿਆ_Not_Drivenp() ਅਚਲ { ਵਾਪਸ m_ਵਰਤਿਆ_Not_Drivenp; }
    ਸਥਿਰ ਤਾਰਕਿਕ ਅਣਵਰਤਿਆ_ਮੇਲ(AstVar* nodep) {
        ਅਚਲ string regexp = v3Global.opt.ਅਣਵਰਤਿਆ_Regexp();
        ਜੇ (regexp == "") ਵਾਪਸ ਝੂਠ;
        ਅਚਲ string prettyName = nodep->prettyName();
        ਵਾਪਸ VString::wildmatch(prettyName.c_str(), regexp.c_str());
    }
    ਖਾਲੀ reportViolations() {
        // Combine bits into overall state
        AstVar* ਅਚਲ nodep = m_varp;

        ਜੇ (initStaticp() && procWritep() && nodep->hasUserInit() && !nodep->isClassMember()
            && !nodep->isFuncLocal()) {
            initStaticp()->v3warn(
                PROCASSINIT,
                "Procedural assignment to declaration with initial value: "
                    << nodep->prettyNameQ() << '\n'
                    << initStaticp()->warn_ਹੋਰ() << "... Location of variable initialization\n"
                    << initStaticp()->warnContextPrimary() << '\n'
                    << procWritep()->warnOther() << "... Location of variable process write\n"
                    << procWritep()->warn_ਹੋਰ()
                    << "... Perhaps should initialize instead using a reset in this process\n"
                    << procWritep()->warnContextSecondary());
        }
        ਅਚਲ Ast_ਗੰਢ* ਅਚਲ initp = nodep->hasUserInit() ? initStaticp() : initialp();
        ਜੇ (initp && contAssignp() && !nodep->isClassMember() && !nodep->isFuncLocal()) {
            initp->v3warn(E_CONTASSINIT, "Continuous assignment to variable with initial value: "
                                             << nodep->prettyNameQ() << '\n'
                                             << initp->warn_ਹੋਰ()
                                             << "... Location of variable initialization\n"
                                             << initp->warnContextPrimary() << '\n'
                                             << contAssignp()->warnOther()
                                             << "... Location of continuous assignment\n"
                                             << contAssignp()->warnContextSecondary());
        }
        ਜੇ (nodep->isGenVar()) {  // Genvar
            ਜੇ (!nodep->isIfaceRef() && !nodep->is_ਵਰਤਿਆ_Param() && !ਅਣਵਰਤਿਆ_ਮੇਲ(nodep)) {
                nodep->v3warn(UNUSEDGENVAR, "Genvar is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::UNUSEDGENVAR,
                                                 ਸੱਚ);  // Warn only once
            }
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isParam()) {  // Parameter
            ਜੇ (!nodep->isIfaceRef() && !nodep->is_ਵਰਤਿਆ_Param() && !ਅਣਵਰਤਿਆ_ਮੇਲ(nodep)) {
                nodep->v3warn(UNUSEDPARAM, "Parameter is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::UNUSEDPARAM,
                                                 ਸੱਚ);  // Warn only once
            }
        } ਨਹੀਂਤਾਂ {  // Signal
            ਅਚਲ string varType{nodep->isFuncLocal() ? "Function variable" : "Signal"};
            ਅਚਲ ਤਾਰਕਿਕ funcInout = nodep->isFuncLocal() && nodep->isInout();
            ਤਾਰਕਿਕ allU = ਸੱਚ;
            ਤਾਰਕਿਕ allD = ਸੱਚ;
            ਤਾਰਕਿਕ anyU = m_ਸਾਰਾ_Flags[FLAG_ਵਰਤਿਆ];
            ਤਾਰਕਿਕ anyD = m_ਸਾਰਾ_Flags[FLAG_DRIVEN];
            ਤਾਰਕਿਕ anyUnotD = ਝੂਠ;
            ਤਾਰਕਿਕ anyDnotU = ਝੂਠ;
            ਤਾਰਕਿਕ anynotDU = ਝੂਠ;
            ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < m_ਦੋਆਧਾਰੀ_ਅੰਕ_Flags.ਮਾਪ() / FLAGS_ਪ੍ਰਤੀ_ਦੋਆਧਾਰੀ_ਅੰਕ; ਦੋਆਧਾਰੀ_ਅੰਕ_2++) {
                ਅਚਲ ਤਾਰਕਿਕ ਵਰਤਿਆ = ਵਰਤਿਆ_Flag(ਦੋਆਧਾਰੀ_ਅੰਕ_2);
                ਅਚਲ ਤਾਰਕਿਕ driv = drivenFlag(ਦੋਆਧਾਰੀ_ਅੰਕ_2);
                allU &= ਵਰਤਿਆ;
                anyU |= ਵਰਤਿਆ;
                allD &= driv;
                anyD |= driv;
                anyUnotD |= ਵਰਤਿਆ && !driv;
                anyDnotU |= !ਵਰਤਿਆ && driv;
                anynotDU |= !ਵਰਤਿਆ && !driv;
            }
            ਜੇ (funcInout) {
                ਜੇ (anyD) allU = ਸੱਚ;
                allD = ਸੱਚ;
            }
            ਜੇ (allU) m_ਸਾਰਾ_Flags[FLAG_ਵਰਤਿਆ] = ਸੱਚ;
            ਜੇ (allD) m_ਸਾਰਾ_Flags[FLAG_DRIVEN] = ਸੱਚ;
            // Test results
            ਜੇ (nodep->isIfaceRef()) {
                // For interface top level we don't do any tracking
                // Ideally we'd report unused instance cells, but presumably a signal inside one
                // would get reported as unused
            } ਨਹੀਂਤਾਂ ਜੇ (allU && allD) {
                // It's fine
            } ਨਹੀਂਤਾਂ ਜੇ (!anyD && !anyU) {
                // UNDRIVEN is considered more serious - as is more likely a bug,
                // thus undriven+unused bits get UNUSED warnings, as they're not as buggy.
                ਜੇ (!ਅਣਵਰਤਿਆ_ਮੇਲ(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not driven, nor used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::UNUSEDSIGNAL,
                                                     ਸੱਚ);  // Warn only once
                }
            } ਨਹੀਂਤਾਂ ਜੇ (allD && !anyU) {
                ਜੇ (!ਅਣਵਰਤਿਆ_ਮੇਲ(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::UNUSEDSIGNAL,
                                                     ਸੱਚ);  // Warn only once
                }
            } ਨਹੀਂਤਾਂ ਜੇ (!anyD && allU) {
                nodep->v3warn(UNDRIVEN, varType << " is not driven: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::UNDRIVEN, ਸੱਚ);  // Warn only once
            } ਨਹੀਂਤਾਂ ਜੇ (!funcInout) {
                // Bits have different dispositions
                ਅਚਲ std::string var_Type_ਹੇਠਲਾ = [&varType]() {
                    std::string str = varType;
                    str[0] = std::tolower(static_cast<ਬਿਨਾਨਿਸ਼ਾਨੀ ਅੱਖਰ>(str[0]));
                    ਵਾਪਸ str;
                }();
                ਤਾਰਕਿਕ ਸੈੱਟ_U = ਝੂਠ;
                ਤਾਰਕਿਕ ਸੈੱਟ_D = ਝੂਠ;
                ਜੇ (anynotDU && !ਅਣਵਰਤਿਆ_ਮੇਲ(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  "Bits of " << var_Type_ਹੇਠਲਾ << " are not driven, nor used: "
                                             << nodep->prettyNameQ() << ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਾਂ(BN_BOTH));
                    ਸੈੱਟ_U = ਸੱਚ;
                }
                ਜੇ (anyDnotU && !ਅਣਵਰਤਿਆ_ਮੇਲ(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL, "Bits of " << var_Type_ਹੇਠਲਾ << " are not used: "
                                                           << nodep->prettyNameQ()
                                                           << ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਾਂ(BN_ਅਣਵਰਤਿਆ));
                    ਸੈੱਟ_U = ਸੱਚ;
                }
                ਜੇ (anyUnotD) {
                    nodep->v3warn(UNDRIVEN, "Bits of " << var_Type_ਹੇਠਲਾ << " are not driven: "
                                                       << nodep->prettyNameQ()
                                                       << ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਾਂ(BN_UNDRIVEN));
                    ਸੈੱਟ_D = ਸੱਚ;
                }
                ਜੇ (ਸੈੱਟ_U) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::UNUSEDSIGNAL, ਸੱਚ);
                }
                ਜੇ (ਸੈੱਟ_D) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::UNDRIVEN, ਸੱਚ);
                }
            }
        }
    }

    ਖਾਲੀ driven_ਰਾਹੀਂ_Call(ਅਚਲ Ast_ਗੰਢ_FTask_Ref* nodep) {
        driven_ਸਾਰਾ(nodep);
        ਜੇ (!m_callNodep) m_callNodep = nodep;
    }
    ਅਚਲ Ast_ਗੰਢ_FTask_Ref* callNodep() ਅਚਲ { ਵਾਪਸ m_callNodep; }
};

//######################################################################
// Undriven state, as a visitor of each AstNode

ਵਰਗ UndrivenVisitor final : ਜਨਤਕ VNVisitorConst {
    // NODE STATE
    // Netlist:
    //  AstVar::user1p          -> UndrivenVar* for usage var, 0=not set yet
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    // Each always:
    //  AstNode::user2p         -> UndrivenVar* for usage var, 0=not set yet
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;

    // STATE
    std::array<std::ਸਦਿਸ਼<Undriven_Var_ਇੰਦਰਾਜ*>, 3> m_entryps = {};  // Nodes to delete when finished
    ਤਾਰਕਿਕ m_ਵਿੱਚ_BBox = ਝੂਠ;  // In black box; mark as driven+used
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Cont_ਸੌਂਪੋ = ਝੂਠ;  // In continuous assignment
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Initial = ਝੂਠ;  // In explicit initial block
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Initial_ਤਿਆਰੀ = ਝੂਠ;  // In InitialAutomatic*/InitialStatic* assignment LHS
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Initial_Static = ਝੂਠ;  // In InitialStatic
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Proc_ਸੌਂਪੋ = ਝੂਠ;  // In procedural assignment
    ਤਾਰਕਿਕ m_ਵਿੱਚ_FTask_Ref = ਝੂਠ;  // In function or task call
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Inout_Or_Ref_Pin = ਝੂਠ;  // Connected to pin that is inout
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Sel_Lhs = ਝੂਠ;  // Iterating the fromp of an AstSel (a partial-bit write target)
    ਅਚਲ Ast_ਗੰਢ_FTask* m_taskp = ਖਾਲੀ_ਸੂਚਕ;  // Current task
    ਅਚਲ AstAlways* m_alwaysp = ਖਾਲੀ_ਸੂਚਕ;  // Current always of either type
    ਅਚਲ AstAlways* m_alwaysCombp = ਖਾਲੀ_ਸੂਚਕ;  // Current always if combo, otherwise nullptr
    ਅਚਲ AstAlways* m_alwaysFFp = ਖਾਲੀ_ਸੂਚਕ;  // Current always if ff, otherwise nullptr
    ਅਚਲ AstAlways* m_alwaysPlainp = ਖਾਲੀ_ਸੂਚਕ;  // Current always if plain (not comb/ff/latch)
    ਅਚਲ AstClocking* m_clockingp = ਖਾਲੀ_ਸੂਚਕ;  // Current clocking block, otherwise nullptr

    V3UndrivenCapture* ਅਚਲ m_capturep = ਖਾਲੀ_ਸੂਚਕ;  // Capture object.  'nullptr' if disabled.

    // METHODS

    Undriven_Var_ਇੰਦਰਾਜ* getEntryp(AstVar* nodep, ਪੂਰਨ which_user) {
        ਜੇ (!(which_user == 1 ? nodep->user1p() : nodep->user2p())) {
            Undriven_Var_ਇੰਦਰਾਜ* ਅਚਲ entryp = ਨਵਾਂ Undriven_Var_ਇੰਦਰਾਜ{nodep};
            // UINFO(9," Associate u="<<which_user<<" "<<cvtToHex(this)<<" "<<nodep->name()<<endl);
            m_entryps[which_user].ਧੱਕੋ_ਪਿੱਠ(entryp);
            ਜੇ (which_user == 1) {
                nodep->user1p(entryp);
            } ਨਹੀਂਤਾਂ ਜੇ (which_user == 2) {
                nodep->user2p(entryp);
            } ਨਹੀਂਤਾਂ {
                nodep->v3fatalSrc("Bad case");
            }
            ਵਾਪਸ entryp;
        } ਨਹੀਂਤਾਂ {
            Undriven_Var_ਇੰਦਰਾਜ* ਅਚਲ entryp = reinterpret_cast<Undriven_Var_ਇੰਦਰਾਜ*>(
                which_user == 1 ? nodep->user1p() : nodep->user2p());
            ਵਾਪਸ entryp;
        }
    }

    ਖਾਲੀ warn_Alw_Comb_ਕ੍ਰਮ(Ast_ਗੰਢ_Var_Ref* nodep, ਅਚਲ Ast_ਗੰਢ* readp) {
        AstVar* ਅਚਲ varp = nodep->varp();
        ਜੇ (!varp->isParam() && !varp->isGenVar() && !varp->is_ਵਰਤਿਆ_Loop_Idx()
            && !varp->ignoreSchedWrite()
            && !m_ਵਿੱਚ_BBox  // We may have falsely considered a SysIgnore as a driver
            && !VN_IS(nodep, VarXRef)  // Xrefs might point at two different instances
            && !varp->fileline()->warnIsOff(
                V3_Error_ਕੋਡ::ALWCOMBORDER)) {  // Warn only once per variable
            nodep->v3warn(ALWCOMBORDER,
                          "always_comb reads "
                              << nodep->prettyNameQ()
                              << " before assigning it later in the same block; behavior "
                                 "may imply latch/state-like behavior and is not purely "
                                 "combinational"
                              << (readp ? "\n" + readp->warnOther()
                                              + "... Location of earlier read\n"
                                              + readp->warnContextSecondary()
                                        : ""));
            varp->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::ALWCOMBORDER,
                                            ਸੱਚ);  // Complain just once for any usage
        }
    }
    // A clocking block 'output' is an additional driver of the signal it names.
    // The conflict is found from whichever driver is reached second.
    ਤਾਰਕਿਕ clockingDrivesOther(ਅਚਲ Undriven_Var_ਇੰਦਰਾਜ* entryp, ਤਾਰਕਿਕ otherWriteIsStaticInit) ਅਚਲ {
        ਅਚਲ ਤਾਰਕਿਕ otherIsExplicitProc
            = entryp->is_Driven_Always_Comb_ਸਾਰਾ() || entryp->isDrivenAlwaysFFWhole();
        ਵਾਪਸ m_clockingp && !otherIsExplicitProc && !otherWriteIsStaticInit
               && m_clockingp != entryp->getClockingp();
    }
    ਤਾਰਕਿਕ otherDrivesClocking(ਅਚਲ Undriven_Var_ਇੰਦਰਾਜ* entryp) ਅਚਲ {
        ਵਾਪਸ !m_clockingp && !m_alwaysCombp && !m_alwaysFFp && !m_ਵਿੱਚ_Initial_Static
               && entryp->is_Driven_Clocking_ਸਾਰਾ();
    }
    // A continuous assignment contending with a clocking block 'output' is an
    // unambiguous driver conflict. Multiple clocking blocks may drive the same
    // signal, the last drive wins (IEEE 1800-2023 14.16.2).
    ਖਾਲੀ warnClockingDriven(Ast_ਗੰਢ_Var_Ref* nodep, ਅਚਲ Undriven_Var_ਇੰਦਰਾਜ* entryp,
                            ਅਚਲ Ast_ਗੰਢ* otherWritep, ਤਾਰਕਿਕ otherWriteIsStaticInit) {
        ਜੇ (clockingDrivesOther(entryp, otherWriteIsStaticInit)
            && otherWritep == entryp->contAssignp()) {
            nodep->v3warn(MULTIDRIVEN,
                          "Variable written to in clocking block also driven by continuous "
                          "assignment"
                              << " (IEEE 1800-2023 14.3): " << nodep->prettyNameQ() << '\n'
                              << nodep->warnOther() << '\n'
                              << nodep->warnContextPrimary() << '\n'
                              << otherWritep->warnOther()
                              << "... Location of continuous assignment\n"
                              << otherWritep->warnContextSecondary());
        }
        ਜੇ (otherDrivesClocking(entryp) && m_ਵਿੱਚ_Cont_ਸੌਂਪੋ) {
            nodep->v3warn(MULTIDRIVEN,
                          "Variable driven by continuous assignment also written to in "
                          "clocking block"
                              << " (IEEE 1800-2023 14.3): " << nodep->prettyNameQ() << '\n'
                              << nodep->warnOther() << '\n'
                              << nodep->warnContextPrimary() << '\n'
                              << otherWritep->warnOther()
                              << "... Location of clocking block output\n"
                              << otherWritep->warnContextSecondary());
        }
    }
    // Driving a signal from both a clocking block and a plain process is a deliberate
    // idiom in some testbenches, as with the plain always conflicts above.
    ਖਾਲੀ warnClockingDrivenProc(Ast_ਗੰਢ_Var_Ref* nodep, ਅਚਲ Undriven_Var_ਇੰਦਰਾਜ* entryp,
                                ਅਚਲ Ast_ਗੰਢ* otherWritep, ਤਾਰਕਿਕ otherWriteIsStaticInit) {
        ਜੇ (clockingDrivesOther(entryp, otherWriteIsStaticInit) && !entryp->getClockingp()
            && otherWritep != entryp->contAssignp()) {
            nodep->v3warn(MULTIDRIVENPROC,
                          "Variable written to in clocking block also written by another "
                          "process"
                              << " (IEEE 1800-2023 14.3): " << nodep->prettyNameQ() << '\n'
                              << nodep->warnOther() << '\n'
                              << nodep->warnContextPrimary() << '\n'
                              << otherWritep->warnOther() << "... Location of other write\n"
                              << otherWritep->warnContextSecondary());
        }
        ਜੇ (otherDrivesClocking(entryp) && !m_ਵਿੱਚ_Cont_ਸੌਂਪੋ) {
            nodep->v3warn(MULTIDRIVENPROC,
                          "Variable written to in process also written to in clocking block"
                              << " (IEEE 1800-2023 14.3): " << nodep->prettyNameQ() << '\n'
                              << nodep->warnOther() << '\n'
                              << nodep->warnContextPrimary() << '\n'
                              << otherWritep->warnOther()
                              << "... Location of clocking block output\n"
                              << otherWritep->warnContextSecondary());
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        ਅਚਲ ਤਾਰਕਿਕ funcInout = nodep->isFuncLocal() && nodep->isInout();
        ਲਈ (ਪੂਰਨ usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            // For assigns and non-combo always, do just usr==1, to look
            // for module-wide undriven etc.
            // For combo always, run both usr==1 for above, and also
            // usr==2 for always-only checks.
            Undriven_Var_ਇੰਦਰਾਜ* ਅਚਲ entryp = getEntryp(nodep, usr);
            ਜੇ ((nodep->isNonOutput() && !funcInout) || nodep->is_Sig_ਜਨਤਕ()
                || nodep->hasUserInit() || nodep->isSigUserRWPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->driven_ਸਾਰਾ(nodep);
            }
            ਜੇ ((nodep->isWritable() && !funcInout) || nodep->is_Sig_ਜਨਤਕ()
                || nodep->isSigUserRWPublic() || nodep->is_Sig_User_Rd_ਜਨਤਕ()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->ਵਰਤਿਆ_ਸਾਰਾ(nodep);
            }
            ਜੇ (nodep->valuep()) entryp->driven_ਸਾਰਾ(nodep->valuep());
        }
        // Discover variables used in bit definitions, etc
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstArraySel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਟੁਕੜਾ_Sel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override {
        Ast_ਗੰਢ_Var_Ref* ਅਚਲ varrefp = VN_CAST(nodep->fromp(), ਗੰਢ_Var_Ref);
        AstConst* ਅਚਲ constp = VN_CAST(nodep->lsbp(), Const);
        ਜੇ (varrefp && constp && !constp->ਅੰਕ().isFourState()) {
            ਲਈ (ਪੂਰਨ usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                Undriven_Var_ਇੰਦਰਾਜ* ਅਚਲ entryp = getEntryp(varrefp->varp(), usr);
                ਅਚਲ ਪੂਰਨ lsb = constp->ਮੰਜ਼ਿਲ_UInt();
                ਜੇ (m_ਵਿੱਚ_BBox || varrefp->access().isWriteOrRW()) {
                    // Don't warn if already driven earlier as "a=0; if(a) a=1;" is fine.
                    ਜੇ (usr == 2 && m_alwaysCombp
                        && entryp->is_ਵਰਤਿਆ_Not_Driven_ਦੋਆਧਾਰੀ_ਅੰਕ(lsb, nodep->ਚੌੜਾਈ())) {
                        UINFO(9, " Select.  Entryp=" << cvt_ਮੰਜ਼ਿਲ_Hex(entryp));
                        warn_Alw_Comb_ਕ੍ਰਮ(varrefp, entryp->ਪਹਿਲਾ_ਵਰਤਿਆ_Not_Drivenp());
                    }
                    entryp->driven_ਦੋਆਧਾਰੀ_ਅੰਕ(lsb, nodep->ਚੌੜਾਈ());
                }
                ਜੇ (m_ਵਿੱਚ_BBox || !varrefp->access().isWriteOrRW())
                    entryp->ਵਰਤਿਆ_ਦੋਆਧਾਰੀ_ਅੰਕ_2(lsb, nodep->ਚੌੜਾਈ(), varrefp);
            }
        } ਨਹੀਂਤਾਂ {
            // skip over static longest static prefix
            iterateConst(nodep->lsbp());
            VL_RESTORER(m_ਵਿੱਚ_Sel_Lhs);
            m_ਵਿੱਚ_Sel_Lhs = !V3_ਚੌੜਾਈ::selectNonConstantRecurse(nodep->lsbp(), /*inSel=*/ਸੱਚ);
            iterateConst(nodep->fromp());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        // Any variable
        ਜੇ (nodep->access().isWriteOrRW()
            && !VN_IS(nodep, VarXRef)) {  // Ignore interface variables and similar ugly items
            ਜੇ (m_ਵਿੱਚ_Proc_ਸੌਂਪੋ && !nodep->varp()->varType().isProcAssignable()
                && !nodep->varp()->isDeclTyped()  //
                && !nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()) {
                nodep->v3warn(PROCASSWIRE, "Procedural assignment to wire, perhaps intended var"
                                               << " (IEEE 1800-2023 6.5): "
                                               << nodep->prettyNameQ());
            } ਨਹੀਂਤਾਂ ਜੇ (m_ਵਿੱਚ_Cont_ਸੌਂਪੋ && !nodep->varp()->varType().isContAssignable()
                       && !nodep->fileline()->language().systemVerilog()) {
                nodep->v3warn(CONTASSREG,
                              "Continuous assignment to reg, perhaps intended wire"
                                  << " (IEEE 1364-2005 6.1; Verilog only, legal in SV): "
                                  << nodep->prettyNameQ());
            }
            ਜੇ (m_ਵਿੱਚ_FTask_Ref && nodep->varp()->varType().isNet()) {
                nodep->v3warn(
                    PROCASSWIRE,
                    "Passed wire on output or inout subroutine argument, expected expression that "
                    "is valid on the left hand side of a procedural assignment"
                        << " (IEEE 1800-2023 13.5): " << nodep->prettyNameQ());
            }
        }

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Remember that anything driven here doesn't count toward MULTIDRIVEN.
        ਤਾਰਕਿਕ ftaskDef = ਝੂਠ;
        ਜੇ (m_taskp && !m_alwaysp && !m_ਵਿੱਚ_Cont_ਸੌਂਪੋ && !m_ਵਿੱਚ_Initial_Static && !m_ਵਿੱਚ_BBox
            && !m_taskp->dpiExport()) {
            AstVar* ਅਚਲ retVarp = VN_CAST(m_taskp->fvarp(), Var);
            ਜੇ (!retVarp || nodep->varp() != retVarp) ftaskDef = ਸੱਚ;
        }

        ਲਈ (ਪੂਰਨ usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            Undriven_Var_ਇੰਦਰਾਜ* ਅਚਲ entryp = getEntryp(nodep->varp(), usr);
            ਅਚਲ ਤਾਰਕਿਕ fdrv = nodep->access().isWriteOrRW()
                              && nodep->varp()->attrFileDescr();  // FD's are also being read from
            ਜੇ (m_ਵਿੱਚ_BBox || nodep->access().isWriteOrRW()) {
                ਜੇ (usr == 2 && m_alwaysCombp && entryp->is_ਵਰਤਿਆ_Not_Driven_Any()) {
                    UINFO(9, " Full bus.  Entryp=" << cvt_ਮੰਜ਼ਿਲ_Hex(entryp));
                    warn_Alw_Comb_ਕ੍ਰਮ(nodep, entryp->ਪਹਿਲਾ_ਵਰਤਿਆ_Not_Drivenp());
                }
                ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ otherVarRefp = entryp->getNodep();
                ਅਚਲ Ast_ਗੰਢ* ਅਚਲ otherWritep = otherVarRefp
                                                       ? static_cast<ਅਚਲ Ast_ਗੰਢ*>(otherVarRefp)
                                                       : entryp->callNodep();
                ਅਚਲ ਤਾਰਕਿਕ same_File_ਸਤਰ
                    = otherVarRefp && nodep->fileline() == otherVarRefp->fileline();
                // Preconditions shared by MULTIDRIVEN and MULTIDRIVENPROC.
                ਅਚਲ ਤਾਰਕਿਕ multidrivenCommon
                    = entryp->is_Driven_ਸਾਰਾ() && !m_ਵਿੱਚ_BBox && !VN_IS(nodep, VarXRef)
                      && !VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType) && !same_File_ਸਤਰ
                      && !entryp->isUnderGen() && otherWritep && !entryp->isFtaskDriven()
                      && !ftaskDef && !m_ਵਿੱਚ_Sel_Lhs;
                // The two warnings are gated independently on the variable
                // declaration's fileline, as v3warn suppression will check
                // the driving fileline and still warn even if the warning
                // was suppressed with lint_off at the declaration.
                ਅਚਲ ਤਾਰਕਿਕ otherWriteIsStaticInit
                    = nodep->varp()->hasUserInit() && otherWritep == entryp->initStaticp();
                ਜੇ (multidrivenCommon
                    && !nodep->varp()->fileline()->warnIsOff(V3_Error_ਕੋਡ::MULTIDRIVEN)) {
                    ਜੇ (m_alwaysCombp
                        && (!entryp->is_Driven_Always_Comb_ਸਾਰਾ()
                            || (m_alwaysCombp != entryp->getAlwCombp()
                                && m_alwaysCombp->fileline()
                                       != entryp->getAlwCombp()->fileline()))) {
                        nodep->v3warn(
                            MULTIDRIVEN,
                            "Variable written to in always_comb also written by other process"
                                << " (IEEE 1800-2023 9.2.2.2): " << nodep->prettyNameQ() << '\n'
                                << nodep->warnOther() << '\n'
                                << nodep->warnContextPrimary() << '\n'
                                << otherWritep->warnOther() << "... Location of other write\n"
                                << otherWritep->warnContextSecondary());
                    }
                    ਜੇ (!m_alwaysCombp && entryp->is_Driven_Always_Comb_ਸਾਰਾ()) {
                        nodep->v3warn(MULTIDRIVEN, "Variable also written to in always_comb"
                                                       << " (IEEE 1800-2023 9.2.2.2): "
                                                       << nodep->prettyNameQ() << '\n'
                                                       << nodep->warnOther() << '\n'
                                                       << nodep->warnContextPrimary() << '\n'
                                                       << otherWritep->warnOther()
                                                       << "... Location of always_comb write\n"
                                                       << otherWritep->warnContextSecondary());
                    }
                    ਜੇ (m_alwaysFFp && !otherWriteIsStaticInit
                        && (!entryp->isDrivenAlwaysFFWhole()
                            || (m_alwaysFFp != entryp->getAlwFFp()
                                && m_alwaysFFp->fileline() != entryp->getAlwFFp()->fileline()))) {
                        nodep->v3warn(
                            MULTIDRIVEN,
                            "Variable written to in always_ff also written by other process"
                                << " (IEEE 1800-2023 9.2.2.4): " << nodep->prettyNameQ() << '\n'
                                << nodep->warnOther() << '\n'
                                << nodep->warnContextPrimary() << '\n'
                                << otherWritep->warnOther() << "... Location of other write\n"
                                << otherWritep->warnContextSecondary());
                    }
                    ਜੇ (!m_alwaysFFp && !m_ਵਿੱਚ_Initial_Static && entryp->isDrivenAlwaysFFWhole()) {
                        nodep->v3warn(MULTIDRIVEN, "Variable also written to in always_ff"
                                                       << " (IEEE 1800-2023 9.2.2.4): "
                                                       << nodep->prettyNameQ() << '\n'
                                                       << nodep->warnOther() << '\n'
                                                       << nodep->warnContextPrimary() << '\n'
                                                       << otherWritep->warnOther()
                                                       << "... Location of always_ff write\n"
                                                       << otherWritep->warnContextSecondary());
                    }
                    warnClockingDriven(nodep, entryp, otherWritep, otherWriteIsStaticInit);
                }
                ਜੇ (multidrivenCommon && !nodep->varp()->is_ਵਰਤਿਆ_Loop_Idx()
                    && !nodep->varp()->fileline()->warnIsOff(V3_Error_ਕੋਡ::MULTIDRIVENPROC)) {
                    // Two plain always blocks driving the whole signal: legal
                    // SystemVerilog, but a driver conflict for synthesis. The
                    // always_ff/always_comb cases above already cover mixes with
                    // an explicit process, so only warn for plain+plain here.
                    // When the two blocks are clocked differently, the
                    // (on-by-default) MULTIDRIVEN check in V3Delayed already reports
                    // the conflict, so don't also emit MULTIDRIVENPROC for that case.
                    ਅਚਲ AstAlways* ਅਚਲ otherAlwaysp = entryp->getAlwPlainp();
                    ਅਚਲ Ast_Sen_ਰੁੱਖ* ਅਚਲ senp
                        = m_alwaysPlainp ? m_alwaysPlainp->sentreep() : ਖਾਲੀ_ਸੂਚਕ;
                    ਅਚਲ Ast_Sen_ਰੁੱਖ* ਅਚਲ otherSenp
                        = otherAlwaysp ? otherAlwaysp->sentreep() : ਖਾਲੀ_ਸੂਚਕ;
                    ਅਚਲ ਤਾਰਕਿਕ differentClocking = senp && otherSenp && senp->hasClocked()
                                                   && otherSenp->hasClocked()
                                                   && !senp->same_ਰੁੱਖ(otherSenp);
                    ਜੇ (m_alwaysPlainp && entryp->is_Driven_Always_Plain_ਸਾਰਾ()
                        && m_alwaysPlainp != otherAlwaysp
                        && m_alwaysPlainp->fileline() != otherAlwaysp->fileline()
                        && !differentClocking) {
                        nodep->v3warn(
                            MULTIDRIVENPROC,
                            "Variable written to in always block also written by another always "
                            "block: "
                                << nodep->prettyNameQ() << '\n'
                                << nodep->warnOther() << '\n'
                                << nodep->warnContextPrimary() << '\n'
                                << otherWritep->warnOther() << "... Location of other write\n"
                                << otherWritep->warnContextSecondary());
                    }
                    warnClockingDrivenProc(nodep, entryp, otherWritep, otherWriteIsStaticInit);
                }
                ਜੇ (!m_ਵਿੱਚ_Initial_ਤਿਆਰੀ || nodep->varp()->hasUserInit()) {
                    // Else don't count default initialization as a driver to a net/variable
                    entryp->driven_ਸਾਰਾ(nodep, ftaskDef);
                }
                ਜੇ (m_alwaysCombp && entryp->is_Driven_Always_Comb_ਸਾਰਾ()
                    && m_alwaysCombp != entryp->getAlwCombp()
                    && m_alwaysCombp->fileline() == entryp->getAlwCombp()->fileline())
                    entryp->underGenerate();
                ਜੇ (m_alwaysCombp) entryp->driven_Always_Comb_ਸਾਰਾ(m_alwaysCombp);
                ਜੇ (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, nodep->varp());
                ਜੇ (m_alwaysPlainp) entryp->driven_Always_Plain_ਸਾਰਾ(m_alwaysPlainp);
                ਜੇ (m_clockingp) entryp->driven_Clocking_ਸਾਰਾ(m_clockingp);
            }
            ਜੇ (nodep->access().isWriteOrRW() && !VN_IS(nodep, VarXRef)) {
                // Ignoring xrefs as the initial and assignment to track might refer to two
                // different instances.  Ideally all of V3Undriven would move after V3Scope,
                // then could use VarScope tracking instead.
                ਜੇ (m_ਵਿੱਚ_Initial_Static && !entryp->initStaticp()) entryp->initStaticp(nodep);
                ਜੇ (m_ਵਿੱਚ_Initial && !entryp->initialp()) entryp->initialp(nodep);
                ਜੇ (m_ਵਿੱਚ_Cont_ਸੌਂਪੋ && !entryp->contAssignp()) entryp->contAssignp(nodep);
                ਜੇ (m_alwaysp && m_ਵਿੱਚ_Proc_ਸੌਂਪੋ && !entryp->procWritep())
                    entryp->procWritep(nodep);
            }
            ਜੇ ((!m_ਵਿੱਚ_Initial_ਤਿਆਰੀ || nodep->varp()->hasUserInit())
                && (m_ਵਿੱਚ_BBox || nodep->access().isReadOrRW()
                    || fdrv
                    // Inouts have only isWrite set, as we don't have more
                    // information and operating on module boundary, treat as
                    // both read and writing
                    || m_ਵਿੱਚ_Inout_Or_Ref_Pin))
                entryp->ਵਰਤਿਆ_ਸਾਰਾ(nodep);
        }
    }

    // Don't know what black boxed calls do, assume in+out
    ਖਾਲੀ ਦੇਖੋ(AstSysIgnore* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_BBox);
        m_ਵਿੱਚ_BBox = ਸੱਚ;
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Proc_ਸੌਂਪੋ);
        m_ਵਿੱਚ_Proc_ਸੌਂਪੋ = ਸੱਚ;
        {
            VL_RESTORER(m_ਵਿੱਚ_Initial_ਤਿਆਰੀ);
            m_ਵਿੱਚ_Initial_ਤਿਆਰੀ = ਝੂਠ;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Dly* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Proc_ਸੌਂਪੋ);
        m_ਵਿੱਚ_Proc_ਸੌਂਪੋ = ਸੱਚ;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Force* nodep) override {
        iterateConst(nodep->rhsp());
        VL_RESTORER(m_ਵਿੱਚ_Initial);
        m_ਵਿੱਚ_Initial = ਝੂਠ;
        iterateConst(nodep->lhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_W* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Cont_ਸੌਂਪੋ);
        m_ਵਿੱਚ_Cont_ਸੌਂਪੋ = ਸੱਚ;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRelease* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Initial);
        m_ਵਿੱਚ_Initial = ਝੂਠ;
        iterateConst(nodep->lhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitial* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Initial);
        m_ਵਿੱਚ_Initial = ਸੱਚ;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitialAutomatic* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Initial_ਤਿਆਰੀ);
        m_ਵਿੱਚ_Initial_ਤਿਆਰੀ = ਸੱਚ;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Initial_ਤਿਆਰੀ);
        m_ਵਿੱਚ_Initial_ਤਿਆਰੀ = ਸੱਚ;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitialStatic* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Initial_Static);
        m_ਵਿੱਚ_Initial_Static = ਸੱਚ;
        VL_RESTORER(m_ਵਿੱਚ_Initial_ਤਿਆਰੀ);
        m_ਵਿੱਚ_Initial_ਤਿਆਰੀ = ਸੱਚ;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Initial_ਤਿਆਰੀ);
        m_ਵਿੱਚ_Initial_ਤਿਆਰੀ = ਸੱਚ;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        VL_RESTORER(m_alwaysCombp);
        VL_RESTORER(m_alwaysFFp);
        VL_RESTORER(m_alwaysPlainp);
        Ast_ਗੰਢ::user2_Clear_ਰੁੱਖ();
        m_alwaysp = nodep;
        ਜੇ (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            UINFO(9, "   " << nodep);
            m_alwaysCombp = nodep;
        } ਨਹੀਂਤਾਂ {
            m_alwaysCombp = ਖਾਲੀ_ਸੂਚਕ;
        }
        m_alwaysFFp = nodep->keyword() == VAlwaysKwd::ALWAYS_FF ? nodep : ਖਾਲੀ_ਸੂਚਕ;
        m_alwaysPlainp = nodep->keyword() == VAlwaysKwd::ALWAYS ? nodep : ਖਾਲੀ_ਸੂਚਕ;
        iterateChildrenConst(nodep);
        ਜੇ (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) UINFO(9, "   Done " << nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstClocking* nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_FTask_Ref);
        m_ਵਿੱਚ_FTask_Ref = ਸੱਚ;

        iterateChildrenConst(nodep);

        ਜੇ (!m_capturep) ਵਾਪਸ;

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Do not apply writeSummary at calls inside a task definition, or they will look like
        // independent drivers (phantom MULTIDRIVEN).
        ਅਚਲ ਤਾਰਕਿਕ ਵਿੱਚ_Executed_Context
            = !(m_taskp && !m_alwaysp && !m_ਵਿੱਚ_Cont_ਸੌਂਪੋ && !m_ਵਿੱਚ_Initial_Static && !m_ਵਿੱਚ_BBox
                && !m_taskp->dpiExport());

        ਜੇ (!ਵਿੱਚ_Executed_Context) ਵਾਪਸ;

        Ast_ਗੰਢ_FTask* ਅਚਲ calleep = nodep->taskp();
        ਜੇ (!calleep) ਵਾਪਸ;

        ਅਚਲ ਸਵੈਚਾਲਿਤ& vars = m_capturep->writeSummary(calleep);
        ਲਈ (AstVar* ਅਚਲ varp : vars) {
            ਲਈ (ਪੂਰਨ usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                Undriven_Var_ਇੰਦਰਾਜ* ਅਚਲ entryp = getEntryp(varp, usr);
                entryp->driven_ਰਾਹੀਂ_Call(nodep);
                ਜੇ (m_alwaysCombp) entryp->driven_Always_Comb_ਸਾਰਾ(m_alwaysCombp);
                ਜੇ (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, varp);
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        VL_RESTORER(m_taskp);
        m_taskp = nodep;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstPin* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Inout_Or_Ref_Pin);
        m_ਵਿੱਚ_Inout_Or_Ref_Pin = nodep->modVarp()->isInoutOrRef();
        iterateChildrenConst(nodep);
    }

    // Until we support tables, primitives will have undriven and unused I/Os
    ਖਾਲੀ ਦੇਖੋ(AstPrimitive*) override {}

    // Coverage artifacts etc shouldn't count as a sink
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cover_Decl*) override {}
    ਖਾਲੀ ਦੇਖੋ(AstCoverInc*) override {}
    ਖਾਲੀ ਦੇਖੋ(AstCoverToggle*) override {}
    ਖਾਲੀ ਦੇਖੋ(AstTraceDecl* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    ਖਾਲੀ ਦੇਖੋ(AstTraceInc* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }

    // iterate
    ਖਾਲੀ ਦੇਖੋ(AstConst* nodep) override {}
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ UndrivenVisitor(AstNetlist* nodep, V3UndrivenCapture* capturep)
        : m_capturep{capturep} {
        iterateConst(nodep);
    }

    ~UndrivenVisitor() override {
        ਲਈ (Undriven_Var_ਇੰਦਰਾਜ* ip : m_entryps[1]) ip->reportViolations();
        ਲਈ (ਪੂਰਨ usr = 1; usr < 3; ++usr) {
            ਲਈ (Undriven_Var_ਇੰਦਰਾਜ* ip : m_entryps[usr]) ਮਿਟਾਓ ip;
        }
    }
};

//######################################################################
// Undriven class functions

ਖਾਲੀ V3Undriven::undrivenAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    V3UndrivenCapture capture{nodep};
    { UndrivenVisitor{nodep, &capture}; }

    ਜੇ (v3Global.opt.stats()) V3Stats::statsStage("undriven");
}
