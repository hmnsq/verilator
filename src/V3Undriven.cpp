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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Undriven.h"

#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3UndrivenCapture.h"
#ಅನ್ತರ್ಭಾವ "V3Width.h"

#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Class for every variable we may process

ವರ್ಗ Undriven_Var_ನಮೂದು final {
    // MEMBERS
    AstVar* ಸ್ಥಿರಾಂಕ m_varp;  // Variable this tracks
    std::ಸದಿಶ<ತಾರ್ಕಿಕ> m_ಸಂಪೂರ್ಣ_Flags;  // Used/Driven on whole vector
    std::ಸದಿಶ<ತಾರ್ಕಿಕ> m_ಬಿಟ್_Flags;  // Used/Driven on each subbit
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* m_ಬಳಸಿದ_Not_Drivenp = ಶೂನ್ಯಸೂಚಕ;  // First read before any write
    ಸ್ಥಿರಾಂಕ AstAlways* m_alwCombp
        = ಶೂನ್ಯಸೂಚಕ;  // always_comb of var if driven within always_comb, else nullptr
    ಸ್ಥಿರಾಂಕ AstAlways* m_alwFFp = ಶೂನ್ಯಸೂಚಕ;  // always_ff of var if driven within always_ff
    ಸ್ಥಿರಾಂಕ AstAlways* m_alwPlainp = ಶೂನ್ಯಸೂಚಕ;  // plain always of var if driven within plain always
    ಸ್ಥಿರಾಂಕ AstClocking* m_clockingp = ಶೂನ್ಯಸೂಚಕ;  // clocking block of var if driven as output
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* m_nodep = ಶೂನ್ಯಸೂಚಕ;  // varref if driven, else nullptr
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* m_initStaticp = ಶೂನ್ಯಸೂಚಕ;  // varref if in InitialStatic driven
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* m_initialp = ಶೂನ್ಯಸೂಚಕ;  // varref if driven in an explicit initial block
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* m_contAssignp = ಶೂನ್ಯಸೂಚಕ;  // varref if in continuous assignment driven
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* m_procWritep = ಶೂನ್ಯಸೂಚಕ;  // varref if written in process
    ತಾರ್ಕಿಕ m_underGen = ಸುಳ್ಳು;  // Under a generate
    ತಾರ್ಕಿಕ m_ftaskDriven = ಸುಳ್ಳು;  // Last driven by function or task

    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask_Ref* m_callNodep = ಶೂನ್ಯಸೂಚಕ;  // Call node if driven via writeSummary

    ಎಣಿಕೆ : uint8_t {
        FLAG_ಬಳಸಿದ = 0,  // Signal or bit has been read/observed
        FLAG_DRIVEN = 1,  // Signal or bit has been written/driven
        FLAG_DRIVEN_ALWCOMB = 2,  // Whole signal has been driven from always_comb
        FLAG_DRIVEN_ALWFF = 3,  // Whole signal has been driven from always_ff
        FLAG_DRIVEN_ALWPLAIN = 4,  // Whole signal has been driven from a plain always
        FLAG_DRIVEN_CLOCKING = 5,  // Whole signal has been driven as a clocking block output
        FLAGS_ಪ್ರತಿ_ಬಿಟ್ = 6  // Number of flags stored for each tracked bit
    };

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ Undriven_Var_ನಮೂದು(AstVar* varp)
        : m_varp{varp} {  // Construction for when a var is used
        UINFO(9, "create " << varp);
        m_ಸಂಪೂರ್ಣ_Flags.resize(FLAGS_ಪ್ರತಿ_ಬಿಟ್);
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < FLAGS_ಪ್ರತಿ_ಬಿಟ್; i++) m_ಸಂಪೂರ್ಣ_Flags[i] = ಸುಳ್ಳು;
        m_ಬಿಟ್_Flags.resize(varp->ಅಗಲ() * FLAGS_ಪ್ರತಿ_ಬಿಟ್);
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < varp->ಅಗಲ() * FLAGS_ಪ್ರತಿ_ಬಿಟ್; i++) m_ಬಿಟ್_Flags[i] = ಸುಳ್ಳು;
    }
    ~Undriven_Var_ನಮೂದು() = ಪೂರ್ವನಿಯೋಜಿತ;

ಖಾಸಗಿ:
    // METHODS
    ತಾರ್ಕಿಕ ಬಿಟ್_ಅಂಕೆ_Ok(ಪೂರ್ಣಾಂಕ ಬಿಟ್_2) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ ಬಿಟ್_2 >= 0 && (ಬಿಟ್_2 * FLAGS_ಪ್ರತಿ_ಬಿಟ್ < static_cast<ಪೂರ್ಣಾಂಕ>(m_ಬಿಟ್_Flags.ಗಾತ್ರ_2()));
    }
    ತಾರ್ಕಿಕ ಬಳಸಿದ_Flag(ಪೂರ್ಣಾಂಕ ಬಿಟ್_2) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_ಸಂಪೂರ್ಣ_Flags[FLAG_ಬಳಸಿದ] || m_ಬಿಟ್_Flags[ಬಿಟ್_2 * FLAGS_ಪ್ರತಿ_ಬಿಟ್ + FLAG_ಬಳಸಿದ];
    }
    ತಾರ್ಕಿಕ drivenFlag(ಪೂರ್ಣಾಂಕ ಬಿಟ್_2) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_ಸಂಪೂರ್ಣ_Flags[FLAG_DRIVEN] || m_ಬಿಟ್_Flags[ಬಿಟ್_2 * FLAGS_ಪ್ರತಿ_ಬಿಟ್ + FLAG_DRIVEN];
    }
    ಪೂರ್ಣಾಂಕ ಬಿಟ್_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಿಟ್_Flags.ಗಾತ್ರ_2() / FLAGS_ಪ್ರತಿ_ಬಿಟ್; }
    ಶೂನ್ಯ ದಾಖಲೆ_ಬಳಸಿದ_Not_Driven(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಆದರೆ (!m_ಬಳಸಿದ_Not_Drivenp) m_ಬಳಸಿದ_Not_Drivenp = nodep;
    }
    ಎಣಿಕೆ ಬಿಟ್_ಹೆಸರುಗಳು_Which : uint8_t { BN_ಬಳಸದ, BN_UNDRIVEN, BN_BOTH };
    string ಬಿಟ್_ಹೆಸರುಗಳು(ಬಿಟ್_ಹೆಸರುಗಳು_Which which) {
        string ಬಿಟ್ಟುಗಳು;
        ತಾರ್ಕಿಕ prev = ಸುಳ್ಳು;
        ಪೂರ್ಣಾಂಕ msb = 0;
        // bit==-1 loops below; we do one extra iteration so end with prev=false
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = (m_ಬಿಟ್_Flags.ಗಾತ್ರ_2() / FLAGS_ಪ್ರತಿ_ಬಿಟ್) - 1; ಬಿಟ್_2 >= -1; --ಬಿಟ್_2) {
            ಆದರೆ (ಬಿಟ್_2 >= 0
                && ((which == BN_ಬಳಸದ && !ಬಳಸಿದ_Flag(ಬಿಟ್_2) && drivenFlag(ಬಿಟ್_2))
                    || (which == BN_UNDRIVEN && ಬಳಸಿದ_Flag(ಬಿಟ್_2) && !drivenFlag(ಬಿಟ್_2))
                    || (which == BN_BOTH && !ಬಳಸಿದ_Flag(ಬಿಟ್_2) && !drivenFlag(ಬಿಟ್_2)))) {
                ಆದರೆ (!prev) {
                    prev = ನಿಜ;
                    msb = ಬಿಟ್_2;
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (prev) {
                ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ bdtypep = m_varp->basicp();
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb = ಬಿಟ್_2 + 1;
                ಆದರೆ (ಬಿಟ್ಟುಗಳು != "") ಬಿಟ್ಟುಗಳು += ",";
                ಆದರೆ (lsb == msb) {
                    ಬಿಟ್ಟುಗಳು += cvt_ಗಮ್ಯ_Str(lsb + bdtypep->lo());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆದರೆ (bdtypep->ascending()) {
                        ಬಿಟ್ಟುಗಳು
                            += cvt_ಗಮ್ಯ_Str(lsb + bdtypep->lo()) + ":" + cvt_ಗಮ್ಯ_Str(msb + bdtypep->lo());
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ಬಿಟ್ಟುಗಳು
                            += cvt_ಗಮ್ಯ_Str(msb + bdtypep->lo()) + ":" + cvt_ಗಮ್ಯ_Str(lsb + bdtypep->lo());
                    }
                }
                prev = ಸುಳ್ಳು;
            }
        }
        ಹಿಂತಿರುಗಿ "[" + ಬಿಟ್ಟುಗಳು + "]";
    }

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ ಬಳಸಿದ_ಸಂಪೂರ್ಣ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        UINFO(9, "set u[*] " << m_varp->name() << " " << nodep);
        ಆದರೆ (!m_ಸಂಪೂರ್ಣ_Flags[FLAG_DRIVEN]) {
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < ಬಿಟ್_ಎಣಿಕೆ(); ++ಬಿಟ್_2) {
                ಆದರೆ (!drivenFlag(ಬಿಟ್_2)) {
                    ದಾಖಲೆ_ಬಳಸಿದ_Not_Driven(nodep);
                    ನಿಲ್ಲಿಸು;
                }
            }
        }
        m_ಸಂಪೂರ್ಣ_Flags[FLAG_ಬಳಸಿದ] = ನಿಜ;
    }
    ಶೂನ್ಯ driven_ಸಂಪೂರ್ಣ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        UINFO(9, "set d[*] " << m_varp->name() << " " << nodep);
        m_ಸಂಪೂರ್ಣ_Flags[FLAG_DRIVEN] = ನಿಜ;
    }
    ಶೂನ್ಯ driven_ಸಂಪೂರ್ಣ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* nodep, ತಾರ್ಕಿಕ ftaskDef) {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ wasDriven = is_Driven_ಸಂಪೂರ್ಣ();
        driven_ಸಂಪೂರ್ಣ(nodep);
        // A write in a task/function definition is not an executed process, so it must not
        // displace an already recorded write, nor clear the flag that keeps such a write
        // from being reported as the other write of a MULTIDRIVEN
        ಆದರೆ (ftaskDef && wasDriven) ಹಿಂತಿರುಗಿ;
        m_ftaskDriven = ftaskDef;
        m_nodep = nodep;
    }
    ಶೂನ್ಯ driven_Always_Comb_ಸಂಪೂರ್ಣ(ಸ್ಥಿರಾಂಕ AstAlways* alwCombp) {
        m_ಸಂಪೂರ್ಣ_Flags[FLAG_DRIVEN_ALWCOMB] = ನಿಜ;
        m_alwCombp = alwCombp;
    }
    ಶೂನ್ಯ drivenAlwaysFFWhole(ಸ್ಥಿರಾಂಕ AstAlways* alwFFp, ಸ್ಥಿರಾಂಕ AstVar* varp) {
        m_ಸಂಪೂರ್ಣ_Flags[FLAG_DRIVEN_ALWFF] = ನಿಜ;
        m_alwFFp = alwFFp;
    }
    ಶೂನ್ಯ driven_Always_Plain_ಸಂಪೂರ್ಣ(ಸ್ಥಿರಾಂಕ AstAlways* alwPlainp) {
        m_ಸಂಪೂರ್ಣ_Flags[FLAG_DRIVEN_ALWPLAIN] = ನಿಜ;
        m_alwPlainp = alwPlainp;
    }
    ಶೂನ್ಯ driven_Clocking_ಸಂಪೂರ್ಣ(ಸ್ಥಿರಾಂಕ AstClocking* clockingp) {
        m_ಸಂಪೂರ್ಣ_Flags[FLAG_DRIVEN_CLOCKING] = ನಿಜ;
        m_clockingp = clockingp;
    }

    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* initStaticp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_initStaticp; }
    ಶೂನ್ಯ initStaticp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) { m_initStaticp = nodep; }
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* initialp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_initialp; }
    ಶೂನ್ಯ initialp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) { m_initialp = nodep; }
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* contAssignp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_contAssignp; }
    ಶೂನ್ಯ contAssignp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) { m_contAssignp = nodep; }
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* procWritep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_procWritep; }
    ಶೂನ್ಯ procWritep(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) { m_procWritep = nodep; }
    ಶೂನ್ಯ underGenerate() { m_underGen = ನಿಜ; }
    ತಾರ್ಕಿಕ isUnderGen() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_underGen; }
    ತಾರ್ಕಿಕ is_Driven_ಸಂಪೂರ್ಣ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸಂಪೂರ್ಣ_Flags[FLAG_DRIVEN]; }
    ತಾರ್ಕಿಕ is_Driven_Always_Comb_ಸಂಪೂರ್ಣ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸಂಪೂರ್ಣ_Flags[FLAG_DRIVEN_ALWCOMB]; }
    ತಾರ್ಕಿಕ isDrivenAlwaysFFWhole() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸಂಪೂರ್ಣ_Flags[FLAG_DRIVEN_ALWFF]; }
    ತಾರ್ಕಿಕ is_Driven_Always_Plain_ಸಂಪೂರ್ಣ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸಂಪೂರ್ಣ_Flags[FLAG_DRIVEN_ALWPLAIN]; }
    ತಾರ್ಕಿಕ is_Driven_Clocking_ಸಂಪೂರ್ಣ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸಂಪೂರ್ಣ_Flags[FLAG_DRIVEN_CLOCKING]; }
    ತಾರ್ಕಿಕ isFtaskDriven() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ftaskDriven; }
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* getNodep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_nodep; }
    ಸ್ಥಿರಾಂಕ AstAlways* getAlwCombp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_alwCombp; }
    ಸ್ಥಿರಾಂಕ AstAlways* getAlwFFp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_alwFFp; }
    ಸ್ಥಿರಾಂಕ AstAlways* getAlwPlainp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_alwPlainp; }
    ಸ್ಥಿರಾಂಕ AstClocking* getClockingp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_clockingp; }
    ಶೂನ್ಯ ಬಳಸಿದ_ಬಿಟ್_2(ಪೂರ್ಣಾಂಕ ಬಿಟ್_2, ಪೂರ್ಣಾಂಕ ಅಗಲ, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        UINFO(9, "set u[" << (ಬಿಟ್_2 + ಅಗಲ - 1) << ":" << ಬಿಟ್_2 << "] " << m_varp->name());
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಅಗಲ; i++) {
            ಆದರೆ (ಬಿಟ್_ಅಂಕೆ_Ok(ಬಿಟ್_2 + i)) {
                ಆದರೆ (!drivenFlag(ಬಿಟ್_2 + i)) ದಾಖಲೆ_ಬಳಸಿದ_Not_Driven(nodep);
                m_ಬಿಟ್_Flags[(ಬಿಟ್_2 + i) * FLAGS_ಪ್ರತಿ_ಬಿಟ್ + FLAG_ಬಳಸಿದ] = ನಿಜ;
            }
        }
    }
    ಶೂನ್ಯ driven_ಬಿಟ್(ಪೂರ್ಣಾಂಕ ಬಿಟ್_2, ಪೂರ್ಣಾಂಕ ಅಗಲ) {
        UINFO(9, "set d[" << (ಬಿಟ್_2 + ಅಗಲ - 1) << ":" << ಬಿಟ್_2 << "] " << m_varp->name());
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಅಗಲ; i++) {
            ಆದರೆ (ಬಿಟ್_ಅಂಕೆ_Ok(ಬಿಟ್_2 + i)) m_ಬಿಟ್_Flags[(ಬಿಟ್_2 + i) * FLAGS_ಪ್ರತಿ_ಬಿಟ್ + FLAG_DRIVEN] = ನಿಜ;
        }
    }
    ತಾರ್ಕಿಕ is_ಬಳಸಿದ_Not_Driven_ಬಿಟ್(ಪೂರ್ಣಾಂಕ ಬಿಟ್_2, ಪೂರ್ಣಾಂಕ ಅಗಲ) ಸ್ಥಿರಾಂಕ {
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಅಗಲ; i++) {
            ಆದರೆ (ಬಿಟ್_ಅಂಕೆ_Ok(ಬಿಟ್_2 + i)
                && (m_ಸಂಪೂರ್ಣ_Flags[FLAG_ಬಳಸಿದ] || m_ಬಿಟ್_Flags[(ಬಿಟ್_2 + i) * FLAGS_ಪ್ರತಿ_ಬಿಟ್ + FLAG_ಬಳಸಿದ])
                && !(m_ಸಂಪೂರ್ಣ_Flags[FLAG_DRIVEN]
                     || m_ಬಿಟ್_Flags[(ಬಿಟ್_2 + i) * FLAGS_ಪ್ರತಿ_ಬಿಟ್ + FLAG_DRIVEN]))
                ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ತಾರ್ಕಿಕ is_ಬಳಸಿದ_Not_Driven_Any() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ is_ಬಳಸಿದ_Not_Driven_ಬಿಟ್(0, m_ಬಿಟ್_Flags.ಗಾತ್ರ_2() / FLAGS_ಪ್ರತಿ_ಬಿಟ್);
    }
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಮೊದಲ_ಬಳಸಿದ_Not_Drivenp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಳಸಿದ_Not_Drivenp; }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಬಳಸದ_ಹೊಂದಿಕೆ(AstVar* nodep) {
        ಸ್ಥಿರಾಂಕ string regexp = v3Global.opt.ಬಳಸದ_Regexp();
        ಆದರೆ (regexp == "") ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ string prettyName = nodep->prettyName();
        ಹಿಂತಿರುಗಿ VString::wildmatch(prettyName.c_str(), regexp.c_str());
    }
    ಶೂನ್ಯ reportViolations() {
        // Combine bits into overall state
        AstVar* ಸ್ಥಿರಾಂಕ nodep = m_varp;

        ಆದರೆ (initStaticp() && procWritep() && nodep->hasUserInit() && !nodep->isClassMember()
            && !nodep->isFuncLocal()) {
            initStaticp()->v3warn(
                PROCASSINIT,
                "Procedural assignment to declaration with initial value: "
                    << nodep->prettyNameQ() << '\n'
                    << initStaticp()->warn_ಇನ್ನಷ್ಟು() << "... Location of variable initialization\n"
                    << initStaticp()->warnContextPrimary() << '\n'
                    << procWritep()->warnOther() << "... Location of variable process write\n"
                    << procWritep()->warn_ಇನ್ನಷ್ಟು()
                    << "... Perhaps should initialize instead using a reset in this process\n"
                    << procWritep()->warnContextSecondary());
        }
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ initp = nodep->hasUserInit() ? initStaticp() : initialp();
        ಆದರೆ (initp && contAssignp() && !nodep->isClassMember() && !nodep->isFuncLocal()) {
            initp->v3warn(E_CONTASSINIT, "Continuous assignment to variable with initial value: "
                                             << nodep->prettyNameQ() << '\n'
                                             << initp->warn_ಇನ್ನಷ್ಟು()
                                             << "... Location of variable initialization\n"
                                             << initp->warnContextPrimary() << '\n'
                                             << contAssignp()->warnOther()
                                             << "... Location of continuous assignment\n"
                                             << contAssignp()->warnContextSecondary());
        }
        ಆದರೆ (nodep->isGenVar()) {  // Genvar
            ಆದರೆ (!nodep->isIfaceRef() && !nodep->is_ಬಳಸಿದ_Param() && !ಬಳಸದ_ಹೊಂದಿಕೆ(nodep)) {
                nodep->v3warn(UNUSEDGENVAR, "Genvar is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNUSEDGENVAR,
                                                 ನಿಜ);  // Warn only once
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isParam()) {  // Parameter
            ಆದರೆ (!nodep->isIfaceRef() && !nodep->is_ಬಳಸಿದ_Param() && !ಬಳಸದ_ಹೊಂದಿಕೆ(nodep)) {
                nodep->v3warn(UNUSEDPARAM, "Parameter is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNUSEDPARAM,
                                                 ನಿಜ);  // Warn only once
            }
        } ಇಲ್ಲದಿದ್ದರೆ {  // Signal
            ಸ್ಥಿರಾಂಕ string varType{nodep->isFuncLocal() ? "Function variable" : "Signal"};
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ funcInout = nodep->isFuncLocal() && nodep->isInout();
            ತಾರ್ಕಿಕ allU = ನಿಜ;
            ತಾರ್ಕಿಕ allD = ನಿಜ;
            ತಾರ್ಕಿಕ anyU = m_ಸಂಪೂರ್ಣ_Flags[FLAG_ಬಳಸಿದ];
            ತಾರ್ಕಿಕ anyD = m_ಸಂಪೂರ್ಣ_Flags[FLAG_DRIVEN];
            ತಾರ್ಕಿಕ anyUnotD = ಸುಳ್ಳು;
            ತಾರ್ಕಿಕ anyDnotU = ಸುಳ್ಳು;
            ತಾರ್ಕಿಕ anynotDU = ಸುಳ್ಳು;
            ಗಾಗಿ (ಚಿಹ್ನೆರಹಿತ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < m_ಬಿಟ್_Flags.ಗಾತ್ರ_2() / FLAGS_ಪ್ರತಿ_ಬಿಟ್; ಬಿಟ್_2++) {
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಬಳಸಿದ = ಬಳಸಿದ_Flag(ಬಿಟ್_2);
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ driv = drivenFlag(ಬಿಟ್_2);
                allU &= ಬಳಸಿದ;
                anyU |= ಬಳಸಿದ;
                allD &= driv;
                anyD |= driv;
                anyUnotD |= ಬಳಸಿದ && !driv;
                anyDnotU |= !ಬಳಸಿದ && driv;
                anynotDU |= !ಬಳಸಿದ && !driv;
            }
            ಆದರೆ (funcInout) {
                ಆದರೆ (anyD) allU = ನಿಜ;
                allD = ನಿಜ;
            }
            ಆದರೆ (allU) m_ಸಂಪೂರ್ಣ_Flags[FLAG_ಬಳಸಿದ] = ನಿಜ;
            ಆದರೆ (allD) m_ಸಂಪೂರ್ಣ_Flags[FLAG_DRIVEN] = ನಿಜ;
            // Test results
            ಆದರೆ (nodep->isIfaceRef()) {
                // For interface top level we don't do any tracking
                // Ideally we'd report unused instance cells, but presumably a signal inside one
                // would get reported as unused
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (allU && allD) {
                // It's fine
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!anyD && !anyU) {
                // UNDRIVEN is considered more serious - as is more likely a bug,
                // thus undriven+unused bits get UNUSED warnings, as they're not as buggy.
                ಆದರೆ (!ಬಳಸದ_ಹೊಂದಿಕೆ(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not driven, nor used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNUSEDSIGNAL,
                                                     ನಿಜ);  // Warn only once
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (allD && !anyU) {
                ಆದರೆ (!ಬಳಸದ_ಹೊಂದಿಕೆ(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNUSEDSIGNAL,
                                                     ನಿಜ);  // Warn only once
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!anyD && allU) {
                nodep->v3warn(UNDRIVEN, varType << " is not driven: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNDRIVEN, ನಿಜ);  // Warn only once
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!funcInout) {
                // Bits have different dispositions
                ಸ್ಥಿರಾಂಕ std::string var_Type_ಅಧೋ = [&varType]() {
                    std::string str = varType;
                    str[0] = std::tolower(static_cast<ಚಿಹ್ನೆರಹಿತ ಅಕ್ಷರ>(str[0]));
                    ಹಿಂತಿರುಗಿ str;
                }();
                ತಾರ್ಕಿಕ ಗಣ_U = ಸುಳ್ಳು;
                ತಾರ್ಕಿಕ ಗಣ_D = ಸುಳ್ಳು;
                ಆದರೆ (anynotDU && !ಬಳಸದ_ಹೊಂದಿಕೆ(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  "Bits of " << var_Type_ಅಧೋ << " are not driven, nor used: "
                                             << nodep->prettyNameQ() << ಬಿಟ್_ಹೆಸರುಗಳು(BN_BOTH));
                    ಗಣ_U = ನಿಜ;
                }
                ಆದರೆ (anyDnotU && !ಬಳಸದ_ಹೊಂದಿಕೆ(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL, "Bits of " << var_Type_ಅಧೋ << " are not used: "
                                                           << nodep->prettyNameQ()
                                                           << ಬಿಟ್_ಹೆಸರುಗಳು(BN_ಬಳಸದ));
                    ಗಣ_U = ನಿಜ;
                }
                ಆದರೆ (anyUnotD) {
                    nodep->v3warn(UNDRIVEN, "Bits of " << var_Type_ಅಧೋ << " are not driven: "
                                                       << nodep->prettyNameQ()
                                                       << ಬಿಟ್_ಹೆಸರುಗಳು(BN_UNDRIVEN));
                    ಗಣ_D = ನಿಜ;
                }
                ಆದರೆ (ಗಣ_U) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNUSEDSIGNAL, ನಿಜ);
                }
                ಆದರೆ (ಗಣ_D) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNDRIVEN, ನಿಜ);
                }
            }
        }
    }

    ಶೂನ್ಯ driven_ಮೂಲಕ_Call(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask_Ref* nodep) {
        driven_ಸಂಪೂರ್ಣ(nodep);
        ಆದರೆ (!m_callNodep) m_callNodep = nodep;
    }
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask_Ref* callNodep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_callNodep; }
};

//######################################################################
// Undriven state, as a visitor of each AstNode

ವರ್ಗ UndrivenVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE
    // Netlist:
    //  AstVar::user1p          -> UndrivenVar* for usage var, 0=not set yet
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    // Each always:
    //  AstNode::user2p         -> UndrivenVar* for usage var, 0=not set yet
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;

    // STATE
    std::array<std::ಸದಿಶ<Undriven_Var_ನಮೂದು*>, 3> m_entryps = {};  // Nodes to delete when finished
    ತಾರ್ಕಿಕ m_ಒಳ_BBox = ಸುಳ್ಳು;  // In black box; mark as driven+used
    ತಾರ್ಕಿಕ m_ಒಳ_Cont_ನಿಯೋಜಿಸು = ಸುಳ್ಳು;  // In continuous assignment
    ತಾರ್ಕಿಕ m_ಒಳ_Initial = ಸುಳ್ಳು;  // In explicit initial block
    ತಾರ್ಕಿಕ m_ಒಳ_Initial_ಸಿದ್ಧತೆ = ಸುಳ್ಳು;  // In InitialAutomatic*/InitialStatic* assignment LHS
    ತಾರ್ಕಿಕ m_ಒಳ_Initial_Static = ಸುಳ್ಳು;  // In InitialStatic
    ತಾರ್ಕಿಕ m_ಒಳ_Proc_ನಿಯೋಜಿಸು = ಸುಳ್ಳು;  // In procedural assignment
    ತಾರ್ಕಿಕ m_ಒಳ_FTask_Ref = ಸುಳ್ಳು;  // In function or task call
    ತಾರ್ಕಿಕ m_ಒಳ_Inout_Or_Ref_Pin = ಸುಳ್ಳು;  // Connected to pin that is inout
    ತಾರ್ಕಿಕ m_ಒಳ_Sel_Lhs = ಸುಳ್ಳು;  // Iterating the fromp of an AstSel (a partial-bit write target)
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* m_taskp = ಶೂನ್ಯಸೂಚಕ;  // Current task
    ಸ್ಥಿರಾಂಕ AstAlways* m_alwaysp = ಶೂನ್ಯಸೂಚಕ;  // Current always of either type
    ಸ್ಥಿರಾಂಕ AstAlways* m_alwaysCombp = ಶೂನ್ಯಸೂಚಕ;  // Current always if combo, otherwise nullptr
    ಸ್ಥಿರಾಂಕ AstAlways* m_alwaysFFp = ಶೂನ್ಯಸೂಚಕ;  // Current always if ff, otherwise nullptr
    ಸ್ಥಿರಾಂಕ AstAlways* m_alwaysPlainp = ಶೂನ್ಯಸೂಚಕ;  // Current always if plain (not comb/ff/latch)
    ಸ್ಥಿರಾಂಕ AstClocking* m_clockingp = ಶೂನ್ಯಸೂಚಕ;  // Current clocking block, otherwise nullptr

    V3UndrivenCapture* ಸ್ಥಿರಾಂಕ m_capturep = ಶೂನ್ಯಸೂಚಕ;  // Capture object.  'nullptr' if disabled.

    // METHODS

    Undriven_Var_ನಮೂದು* getEntryp(AstVar* nodep, ಪೂರ್ಣಾಂಕ which_user) {
        ಆದರೆ (!(which_user == 1 ? nodep->user1p() : nodep->user2p())) {
            Undriven_Var_ನಮೂದು* ಸ್ಥಿರಾಂಕ entryp = ಹೊಸ Undriven_Var_ನಮೂದು{nodep};
            // UINFO(9," Associate u="<<which_user<<" "<<cvtToHex(this)<<" "<<nodep->name()<<endl);
            m_entryps[which_user].ತಳ್ಳು_ಹಿಂಭಾಗ(entryp);
            ಆದರೆ (which_user == 1) {
                nodep->user1p(entryp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (which_user == 2) {
                nodep->user2p(entryp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3fatalSrc("Bad case");
            }
            ಹಿಂತಿರುಗಿ entryp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            Undriven_Var_ನಮೂದು* ಸ್ಥಿರಾಂಕ entryp = reinterpret_cast<Undriven_Var_ನಮೂದು*>(
                which_user == 1 ? nodep->user1p() : nodep->user2p());
            ಹಿಂತಿರುಗಿ entryp;
        }
    }

    ಶೂನ್ಯ warn_Alw_Comb_ಕ್ರಮ(Ast_ಗಂಟು_Var_Ref* nodep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* readp) {
        AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        ಆದರೆ (!varp->isParam() && !varp->isGenVar() && !varp->is_ಬಳಸಿದ_Loop_Idx()
            && !varp->ignoreSchedWrite()
            && !m_ಒಳ_BBox  // We may have falsely considered a SysIgnore as a driver
            && !VN_IS(nodep, VarXRef)  // Xrefs might point at two different instances
            && !varp->fileline()->warnIsOff(
                V3_Error_ಸಂಕೇತ::ALWCOMBORDER)) {  // Warn only once per variable
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
            varp->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::ALWCOMBORDER,
                                            ನಿಜ);  // Complain just once for any usage
        }
    }
    // A clocking block 'output' is an additional driver of the signal it names.
    // The conflict is found from whichever driver is reached second.
    ತಾರ್ಕಿಕ clockingDrivesOther(ಸ್ಥಿರಾಂಕ Undriven_Var_ನಮೂದು* entryp, ತಾರ್ಕಿಕ otherWriteIsStaticInit) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ otherIsExplicitProc
            = entryp->is_Driven_Always_Comb_ಸಂಪೂರ್ಣ() || entryp->isDrivenAlwaysFFWhole();
        ಹಿಂತಿರುಗಿ m_clockingp && !otherIsExplicitProc && !otherWriteIsStaticInit
               && m_clockingp != entryp->getClockingp();
    }
    ತಾರ್ಕಿಕ otherDrivesClocking(ಸ್ಥಿರಾಂಕ Undriven_Var_ನಮೂದು* entryp) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ !m_clockingp && !m_alwaysCombp && !m_alwaysFFp && !m_ಒಳ_Initial_Static
               && entryp->is_Driven_Clocking_ಸಂಪೂರ್ಣ();
    }
    // A continuous assignment contending with a clocking block 'output' is an
    // unambiguous driver conflict. Multiple clocking blocks may drive the same
    // signal, the last drive wins (IEEE 1800-2023 14.16.2).
    ಶೂನ್ಯ warnClockingDriven(Ast_ಗಂಟು_Var_Ref* nodep, ಸ್ಥಿರಾಂಕ Undriven_Var_ನಮೂದು* entryp,
                            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* otherWritep, ತಾರ್ಕಿಕ otherWriteIsStaticInit) {
        ಆದರೆ (clockingDrivesOther(entryp, otherWriteIsStaticInit)
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
        ಆದರೆ (otherDrivesClocking(entryp) && m_ಒಳ_Cont_ನಿಯೋಜಿಸು) {
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
    ಶೂನ್ಯ warnClockingDrivenProc(Ast_ಗಂಟು_Var_Ref* nodep, ಸ್ಥಿರಾಂಕ Undriven_Var_ನಮೂದು* entryp,
                                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* otherWritep, ತಾರ್ಕಿಕ otherWriteIsStaticInit) {
        ಆದರೆ (clockingDrivesOther(entryp, otherWriteIsStaticInit) && !entryp->getClockingp()
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
        ಆದರೆ (otherDrivesClocking(entryp) && !m_ಒಳ_Cont_ನಿಯೋಜಿಸು) {
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
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ funcInout = nodep->isFuncLocal() && nodep->isInout();
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            // For assigns and non-combo always, do just usr==1, to look
            // for module-wide undriven etc.
            // For combo always, run both usr==1 for above, and also
            // usr==2 for always-only checks.
            Undriven_Var_ನಮೂದು* ಸ್ಥಿರಾಂಕ entryp = getEntryp(nodep, usr);
            ಆದರೆ ((nodep->isNonOutput() && !funcInout) || nodep->is_Sig_ಬಹಿರಂಗ()
                || nodep->hasUserInit() || nodep->isSigUserRWPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->driven_ಸಂಪೂರ್ಣ(nodep);
            }
            ಆದರೆ ((nodep->isWritable() && !funcInout) || nodep->is_Sig_ಬಹಿರಂಗ()
                || nodep->isSigUserRWPublic() || nodep->is_Sig_User_Rd_ಬಹಿರಂಗ()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->ಬಳಸಿದ_ಸಂಪೂರ್ಣ(nodep);
            }
            ಆದರೆ (nodep->valuep()) entryp->driven_ಸಂಪೂರ್ಣ(nodep->valuep());
        }
        // Discover variables used in bit definitions, etc
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstArraySel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ತುಂಡು_Sel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSel* nodep) override {
        Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(nodep->fromp(), ಗಂಟು_Var_Ref);
        AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->lsbp(), Const);
        ಆದರೆ (varrefp && constp && !constp->ಅಂಕೆ().isFourState()) {
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                Undriven_Var_ನಮೂದು* ಸ್ಥಿರಾಂಕ entryp = getEntryp(varrefp->varp(), usr);
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb = constp->ಗಮ್ಯ_UInt();
                ಆದರೆ (m_ಒಳ_BBox || varrefp->access().isWriteOrRW()) {
                    // Don't warn if already driven earlier as "a=0; if(a) a=1;" is fine.
                    ಆದರೆ (usr == 2 && m_alwaysCombp
                        && entryp->is_ಬಳಸಿದ_Not_Driven_ಬಿಟ್(lsb, nodep->ಅಗಲ())) {
                        UINFO(9, " Select.  Entryp=" << cvt_ಗಮ್ಯ_Hex(entryp));
                        warn_Alw_Comb_ಕ್ರಮ(varrefp, entryp->ಮೊದಲ_ಬಳಸಿದ_Not_Drivenp());
                    }
                    entryp->driven_ಬಿಟ್(lsb, nodep->ಅಗಲ());
                }
                ಆದರೆ (m_ಒಳ_BBox || !varrefp->access().isWriteOrRW())
                    entryp->ಬಳಸಿದ_ಬಿಟ್_2(lsb, nodep->ಅಗಲ(), varrefp);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // skip over static longest static prefix
            iterateConst(nodep->lsbp());
            VL_RESTORER(m_ಒಳ_Sel_Lhs);
            m_ಒಳ_Sel_Lhs = !V3_ಅಗಲ::selectNonConstantRecurse(nodep->lsbp(), /*inSel=*/ನಿಜ);
            iterateConst(nodep->fromp());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        // Any variable
        ಆದರೆ (nodep->access().isWriteOrRW()
            && !VN_IS(nodep, VarXRef)) {  // Ignore interface variables and similar ugly items
            ಆದರೆ (m_ಒಳ_Proc_ನಿಯೋಜಿಸು && !nodep->varp()->varType().isProcAssignable()
                && !nodep->varp()->isDeclTyped()  //
                && !nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()) {
                nodep->v3warn(PROCASSWIRE, "Procedural assignment to wire, perhaps intended var"
                                               << " (IEEE 1800-2023 6.5): "
                                               << nodep->prettyNameQ());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ಒಳ_Cont_ನಿಯೋಜಿಸು && !nodep->varp()->varType().isContAssignable()
                       && !nodep->fileline()->language().systemVerilog()) {
                nodep->v3warn(CONTASSREG,
                              "Continuous assignment to reg, perhaps intended wire"
                                  << " (IEEE 1364-2005 6.1; Verilog only, legal in SV): "
                                  << nodep->prettyNameQ());
            }
            ಆದರೆ (m_ಒಳ_FTask_Ref && nodep->varp()->varType().isNet()) {
                nodep->v3warn(
                    PROCASSWIRE,
                    "Passed wire on output or inout subroutine argument, expected expression that "
                    "is valid on the left hand side of a procedural assignment"
                        << " (IEEE 1800-2023 13.5): " << nodep->prettyNameQ());
            }
        }

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Remember that anything driven here doesn't count toward MULTIDRIVEN.
        ತಾರ್ಕಿಕ ftaskDef = ಸುಳ್ಳು;
        ಆದರೆ (m_taskp && !m_alwaysp && !m_ಒಳ_Cont_ನಿಯೋಜಿಸು && !m_ಒಳ_Initial_Static && !m_ಒಳ_BBox
            && !m_taskp->dpiExport()) {
            AstVar* ಸ್ಥಿರಾಂಕ retVarp = VN_CAST(m_taskp->fvarp(), Var);
            ಆದರೆ (!retVarp || nodep->varp() != retVarp) ftaskDef = ನಿಜ;
        }

        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            Undriven_Var_ನಮೂದು* ಸ್ಥಿರಾಂಕ entryp = getEntryp(nodep->varp(), usr);
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ fdrv = nodep->access().isWriteOrRW()
                              && nodep->varp()->attrFileDescr();  // FD's are also being read from
            ಆದರೆ (m_ಒಳ_BBox || nodep->access().isWriteOrRW()) {
                ಆದರೆ (usr == 2 && m_alwaysCombp && entryp->is_ಬಳಸಿದ_Not_Driven_Any()) {
                    UINFO(9, " Full bus.  Entryp=" << cvt_ಗಮ್ಯ_Hex(entryp));
                    warn_Alw_Comb_ಕ್ರಮ(nodep, entryp->ಮೊದಲ_ಬಳಸಿದ_Not_Drivenp());
                }
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ otherVarRefp = entryp->getNodep();
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ otherWritep = otherVarRefp
                                                       ? static_cast<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*>(otherVarRefp)
                                                       : entryp->callNodep();
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ same_File_ಸಾಲು
                    = otherVarRefp && nodep->fileline() == otherVarRefp->fileline();
                // Preconditions shared by MULTIDRIVEN and MULTIDRIVENPROC.
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ multidrivenCommon
                    = entryp->is_Driven_ಸಂಪೂರ್ಣ() && !m_ಒಳ_BBox && !VN_IS(nodep, VarXRef)
                      && !VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType) && !same_File_ಸಾಲು
                      && !entryp->isUnderGen() && otherWritep && !entryp->isFtaskDriven()
                      && !ftaskDef && !m_ಒಳ_Sel_Lhs;
                // The two warnings are gated independently on the variable
                // declaration's fileline, as v3warn suppression will check
                // the driving fileline and still warn even if the warning
                // was suppressed with lint_off at the declaration.
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ otherWriteIsStaticInit
                    = nodep->varp()->hasUserInit() && otherWritep == entryp->initStaticp();
                ಆದರೆ (multidrivenCommon
                    && !nodep->varp()->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::MULTIDRIVEN)) {
                    ಆದರೆ (m_alwaysCombp
                        && (!entryp->is_Driven_Always_Comb_ಸಂಪೂರ್ಣ()
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
                    ಆದರೆ (!m_alwaysCombp && entryp->is_Driven_Always_Comb_ಸಂಪೂರ್ಣ()) {
                        nodep->v3warn(MULTIDRIVEN, "Variable also written to in always_comb"
                                                       << " (IEEE 1800-2023 9.2.2.2): "
                                                       << nodep->prettyNameQ() << '\n'
                                                       << nodep->warnOther() << '\n'
                                                       << nodep->warnContextPrimary() << '\n'
                                                       << otherWritep->warnOther()
                                                       << "... Location of always_comb write\n"
                                                       << otherWritep->warnContextSecondary());
                    }
                    ಆದರೆ (m_alwaysFFp && !otherWriteIsStaticInit
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
                    ಆದರೆ (!m_alwaysFFp && !m_ಒಳ_Initial_Static && entryp->isDrivenAlwaysFFWhole()) {
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
                ಆದರೆ (multidrivenCommon && !nodep->varp()->is_ಬಳಸಿದ_Loop_Idx()
                    && !nodep->varp()->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::MULTIDRIVENPROC)) {
                    // Two plain always blocks driving the whole signal: legal
                    // SystemVerilog, but a driver conflict for synthesis. The
                    // always_ff/always_comb cases above already cover mixes with
                    // an explicit process, so only warn for plain+plain here.
                    // When the two blocks are clocked differently, the
                    // (on-by-default) MULTIDRIVEN check in V3Delayed already reports
                    // the conflict, so don't also emit MULTIDRIVENPROC for that case.
                    ಸ್ಥಿರಾಂಕ AstAlways* ಸ್ಥಿರಾಂಕ otherAlwaysp = entryp->getAlwPlainp();
                    ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senp
                        = m_alwaysPlainp ? m_alwaysPlainp->sentreep() : ಶೂನ್ಯಸೂಚಕ;
                    ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ otherSenp
                        = otherAlwaysp ? otherAlwaysp->sentreep() : ಶೂನ್ಯಸೂಚಕ;
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ differentClocking = senp && otherSenp && senp->hasClocked()
                                                   && otherSenp->hasClocked()
                                                   && !senp->same_ವೃಕ್ಷ(otherSenp);
                    ಆದರೆ (m_alwaysPlainp && entryp->is_Driven_Always_Plain_ಸಂಪೂರ್ಣ()
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
                ಆದರೆ (!m_ಒಳ_Initial_ಸಿದ್ಧತೆ || nodep->varp()->hasUserInit()) {
                    // Else don't count default initialization as a driver to a net/variable
                    entryp->driven_ಸಂಪೂರ್ಣ(nodep, ftaskDef);
                }
                ಆದರೆ (m_alwaysCombp && entryp->is_Driven_Always_Comb_ಸಂಪೂರ್ಣ()
                    && m_alwaysCombp != entryp->getAlwCombp()
                    && m_alwaysCombp->fileline() == entryp->getAlwCombp()->fileline())
                    entryp->underGenerate();
                ಆದರೆ (m_alwaysCombp) entryp->driven_Always_Comb_ಸಂಪೂರ್ಣ(m_alwaysCombp);
                ಆದರೆ (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, nodep->varp());
                ಆದರೆ (m_alwaysPlainp) entryp->driven_Always_Plain_ಸಂಪೂರ್ಣ(m_alwaysPlainp);
                ಆದರೆ (m_clockingp) entryp->driven_Clocking_ಸಂಪೂರ್ಣ(m_clockingp);
            }
            ಆದರೆ (nodep->access().isWriteOrRW() && !VN_IS(nodep, VarXRef)) {
                // Ignoring xrefs as the initial and assignment to track might refer to two
                // different instances.  Ideally all of V3Undriven would move after V3Scope,
                // then could use VarScope tracking instead.
                ಆದರೆ (m_ಒಳ_Initial_Static && !entryp->initStaticp()) entryp->initStaticp(nodep);
                ಆದರೆ (m_ಒಳ_Initial && !entryp->initialp()) entryp->initialp(nodep);
                ಆದರೆ (m_ಒಳ_Cont_ನಿಯೋಜಿಸು && !entryp->contAssignp()) entryp->contAssignp(nodep);
                ಆದರೆ (m_alwaysp && m_ಒಳ_Proc_ನಿಯೋಜಿಸು && !entryp->procWritep())
                    entryp->procWritep(nodep);
            }
            ಆದರೆ ((!m_ಒಳ_Initial_ಸಿದ್ಧತೆ || nodep->varp()->hasUserInit())
                && (m_ಒಳ_BBox || nodep->access().isReadOrRW()
                    || fdrv
                    // Inouts have only isWrite set, as we don't have more
                    // information and operating on module boundary, treat as
                    // both read and writing
                    || m_ಒಳ_Inout_Or_Ref_Pin))
                entryp->ಬಳಸಿದ_ಸಂಪೂರ್ಣ(nodep);
        }
    }

    // Don't know what black boxed calls do, assume in+out
    ಶೂನ್ಯ ಭೇಟಿ(AstSysIgnore* nodep) override {
        VL_RESTORER(m_ಒಳ_BBox);
        m_ಒಳ_BBox = ನಿಜ;
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು* nodep) override {
        VL_RESTORER(m_ಒಳ_Proc_ನಿಯೋಜಿಸು);
        m_ಒಳ_Proc_ನಿಯೋಜಿಸು = ನಿಜ;
        {
            VL_RESTORER(m_ಒಳ_Initial_ಸಿದ್ಧತೆ);
            m_ಒಳ_Initial_ಸಿದ್ಧತೆ = ಸುಳ್ಳು;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Dly* nodep) override {
        VL_RESTORER(m_ಒಳ_Proc_ನಿಯೋಜಿಸು);
        m_ಒಳ_Proc_ನಿಯೋಜಿಸು = ನಿಜ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Force* nodep) override {
        iterateConst(nodep->rhsp());
        VL_RESTORER(m_ಒಳ_Initial);
        m_ಒಳ_Initial = ಸುಳ್ಳು;
        iterateConst(nodep->lhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_W* nodep) override {
        VL_RESTORER(m_ಒಳ_Cont_ನಿಯೋಜಿಸು);
        m_ಒಳ_Cont_ನಿಯೋಜಿಸು = ನಿಜ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRelease* nodep) override {
        VL_RESTORER(m_ಒಳ_Initial);
        m_ಒಳ_Initial = ಸುಳ್ಳು;
        iterateConst(nodep->lhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitial* nodep) override {
        VL_RESTORER(m_ಒಳ_Initial);
        m_ಒಳ_Initial = ನಿಜ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialAutomatic* nodep) override {
        VL_RESTORER(m_ಒಳ_Initial_ಸಿದ್ಧತೆ);
        m_ಒಳ_Initial_ಸಿದ್ಧತೆ = ನಿಜ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_ಒಳ_Initial_ಸಿದ್ಧತೆ);
        m_ಒಳ_Initial_ಸಿದ್ಧತೆ = ನಿಜ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialStatic* nodep) override {
        VL_RESTORER(m_ಒಳ_Initial_Static);
        m_ಒಳ_Initial_Static = ನಿಜ;
        VL_RESTORER(m_ಒಳ_Initial_ಸಿದ್ಧತೆ);
        m_ಒಳ_Initial_ಸಿದ್ಧತೆ = ನಿಜ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_ಒಳ_Initial_ಸಿದ್ಧತೆ);
        m_ಒಳ_Initial_ಸಿದ್ಧತೆ = ನಿಜ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        VL_RESTORER(m_alwaysCombp);
        VL_RESTORER(m_alwaysFFp);
        VL_RESTORER(m_alwaysPlainp);
        Ast_ಗಂಟು::user2_Clear_ವೃಕ್ಷ();
        m_alwaysp = nodep;
        ಆದರೆ (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            UINFO(9, "   " << nodep);
            m_alwaysCombp = nodep;
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_alwaysCombp = ಶೂನ್ಯಸೂಚಕ;
        }
        m_alwaysFFp = nodep->keyword() == VAlwaysKwd::ALWAYS_FF ? nodep : ಶೂನ್ಯಸೂಚಕ;
        m_alwaysPlainp = nodep->keyword() == VAlwaysKwd::ALWAYS ? nodep : ಶೂನ್ಯಸೂಚಕ;
        iterateChildrenConst(nodep);
        ಆದರೆ (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) UINFO(9, "   Done " << nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClocking* nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        VL_RESTORER(m_ಒಳ_FTask_Ref);
        m_ಒಳ_FTask_Ref = ನಿಜ;

        iterateChildrenConst(nodep);

        ಆದರೆ (!m_capturep) ಹಿಂತಿರುಗಿ;

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Do not apply writeSummary at calls inside a task definition, or they will look like
        // independent drivers (phantom MULTIDRIVEN).
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಒಳ_Executed_Context
            = !(m_taskp && !m_alwaysp && !m_ಒಳ_Cont_ನಿಯೋಜಿಸು && !m_ಒಳ_Initial_Static && !m_ಒಳ_BBox
                && !m_taskp->dpiExport());

        ಆದರೆ (!ಒಳ_Executed_Context) ಹಿಂತಿರುಗಿ;

        Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ calleep = nodep->taskp();
        ಆದರೆ (!calleep) ಹಿಂತಿರುಗಿ;

        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& vars = m_capturep->writeSummary(calleep);
        ಗಾಗಿ (AstVar* ಸ್ಥಿರಾಂಕ varp : vars) {
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                Undriven_Var_ನಮೂದು* ಸ್ಥಿರಾಂಕ entryp = getEntryp(varp, usr);
                entryp->driven_ಮೂಲಕ_Call(nodep);
                ಆದರೆ (m_alwaysCombp) entryp->driven_Always_Comb_ಸಂಪೂರ್ಣ(m_alwaysCombp);
                ಆದರೆ (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, varp);
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_taskp);
        m_taskp = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPin* nodep) override {
        VL_RESTORER(m_ಒಳ_Inout_Or_Ref_Pin);
        m_ಒಳ_Inout_Or_Ref_Pin = nodep->modVarp()->isInoutOrRef();
        iterateChildrenConst(nodep);
    }

    // Until we support tables, primitives will have undriven and unused I/Os
    ಶೂನ್ಯ ಭೇಟಿ(AstPrimitive*) override {}

    // Coverage artifacts etc shouldn't count as a sink
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cover_Decl*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverInc*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverToggle*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstTraceDecl* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    ಶೂನ್ಯ ಭೇಟಿ(AstTraceInc* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }

    // iterate
    ಶೂನ್ಯ ಭೇಟಿ(AstConst* nodep) override {}
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ UndrivenVisitor(AstNetlist* nodep, V3UndrivenCapture* capturep)
        : m_capturep{capturep} {
        iterateConst(nodep);
    }

    ~UndrivenVisitor() override {
        ಗಾಗಿ (Undriven_Var_ನಮೂದು* ip : m_entryps[1]) ip->reportViolations();
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ usr = 1; usr < 3; ++usr) {
            ಗಾಗಿ (Undriven_Var_ನಮೂದು* ip : m_entryps[usr]) ಅಳಿಸು ip;
        }
    }
};

//######################################################################
// Undriven class functions

ಶೂನ್ಯ V3Undriven::undrivenAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    V3UndrivenCapture capture{nodep};
    { UndrivenVisitor{nodep, &capture}; }

    ಆದರೆ (v3Global.opt.stats()) V3Stats::statsStage("undriven");
}
