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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Undriven.h"

#ضمن "V3Stats.h"
#ضمن "V3UndrivenCapture.h"
#ضمن "V3Width.h"

#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Class for every variable we may process

صنف Undriven_Var_ثبت final {
    // MEMBERS
    AstVar* ثابت m_varp;  // Variable this tracks
    std::ویکټور<منطقي> m_ټولیز_Flags;  // Used/Driven on whole vector
    std::ویکټور<منطقي> m_بټ_Flags;  // Used/Driven on each subbit
    ثابت Ast_غوټه* m_کارول_شوی_Not_Drivenp = تش_اشاره;  // First read before any write
    ثابت AstAlways* m_alwCombp
        = تش_اشاره;  // always_comb of var if driven within always_comb, else nullptr
    ثابت AstAlways* m_alwFFp = تش_اشاره;  // always_ff of var if driven within always_ff
    ثابت AstAlways* m_alwPlainp = تش_اشاره;  // plain always of var if driven within plain always
    ثابت AstClocking* m_clockingp = تش_اشاره;  // clocking block of var if driven as output
    ثابت Ast_غوټه_Var_Ref* m_nodep = تش_اشاره;  // varref if driven, else nullptr
    ثابت Ast_غوټه* m_initStaticp = تش_اشاره;  // varref if in InitialStatic driven
    ثابت Ast_غوټه* m_initialp = تش_اشاره;  // varref if driven in an explicit initial block
    ثابت Ast_غوټه* m_contAssignp = تش_اشاره;  // varref if in continuous assignment driven
    ثابت Ast_غوټه* m_procWritep = تش_اشاره;  // varref if written in process
    منطقي m_underGen = خطأ;  // Under a generate
    منطقي m_ftaskDriven = خطأ;  // Last driven by function or task

    ثابت Ast_غوټه_FTask_Ref* m_callNodep = تش_اشاره;  // Call node if driven via writeSummary

    تعداد : uint8_t {
        FLAG_کارول_شوی = 0,  // Signal or bit has been read/observed
        FLAG_DRIVEN = 1,  // Signal or bit has been written/driven
        FLAG_DRIVEN_ALWCOMB = 2,  // Whole signal has been driven from always_comb
        FLAG_DRIVEN_ALWFF = 3,  // Whole signal has been driven from always_ff
        FLAG_DRIVEN_ALWPLAIN = 4,  // Whole signal has been driven from a plain always
        FLAG_DRIVEN_CLOCKING = 5,  // Whole signal has been driven as a clocking block output
        FLAGS_فی_بټ = 6  // Number of flags stored for each tracked bit
    };

عام:
    // CONSTRUCTORS
    څرګند Undriven_Var_ثبت(AstVar* varp)
        : m_varp{varp} {  // Construction for when a var is used
        UINFO(9, "create " << varp);
        m_ټولیز_Flags.resize(FLAGS_فی_بټ);
        لكل (صحيح i = 0; i < FLAGS_فی_بټ; i++) m_ټولیز_Flags[i] = خطأ;
        m_بټ_Flags.resize(varp->پلنوالی() * FLAGS_فی_بټ);
        لكل (صحيح i = 0; i < varp->پلنوالی() * FLAGS_فی_بټ; i++) m_بټ_Flags[i] = خطأ;
    }
    ~Undriven_Var_ثبت() = افتراضي;

خاص:
    // METHODS
    منطقي بټ_عدد_Ok(صحيح بټ_2) ثابت {
        أرجع بټ_2 >= 0 && (بټ_2 * FLAGS_فی_بټ < static_cast<صحيح>(m_بټ_Flags.اندازه()));
    }
    منطقي کارول_شوی_Flag(صحيح بټ_2) ثابت {
        أرجع m_ټولیز_Flags[FLAG_کارول_شوی] || m_بټ_Flags[بټ_2 * FLAGS_فی_بټ + FLAG_کارول_شوی];
    }
    منطقي drivenFlag(صحيح بټ_2) ثابت {
        أرجع m_ټولیز_Flags[FLAG_DRIVEN] || m_بټ_Flags[بټ_2 * FLAGS_فی_بټ + FLAG_DRIVEN];
    }
    صحيح بټ_شمېر() ثابت { أرجع m_بټ_Flags.اندازه() / FLAGS_فی_بټ; }
    فراغ ریکارډ_کارول_شوی_Not_Driven(ثابت Ast_غوټه* nodep) {
        إذا (!m_کارول_شوی_Not_Drivenp) m_کارول_شوی_Not_Drivenp = nodep;
    }
    تعداد بټ_نومونه_Which : uint8_t { BN_ناکارول_شوی, BN_UNDRIVEN, BN_BOTH };
    string بټ_نومونه(بټ_نومونه_Which which) {
        string بټونه;
        منطقي prev = خطأ;
        صحيح msb = 0;
        // bit==-1 loops below; we do one extra iteration so end with prev=false
        لكل (صحيح بټ_2 = (m_بټ_Flags.اندازه() / FLAGS_فی_بټ) - 1; بټ_2 >= -1; --بټ_2) {
            إذا (بټ_2 >= 0
                && ((which == BN_ناکارول_شوی && !کارول_شوی_Flag(بټ_2) && drivenFlag(بټ_2))
                    || (which == BN_UNDRIVEN && کارول_شوی_Flag(بټ_2) && !drivenFlag(بټ_2))
                    || (which == BN_BOTH && !کارول_شوی_Flag(بټ_2) && !drivenFlag(بټ_2)))) {
                إذا (!prev) {
                    prev = ریښتیا;
                    msb = بټ_2;
                }
            } وإلا إذا (prev) {
                ثابت AstBasicDType* ثابت bdtypep = m_varp->basicp();
                ثابت صحيح lsb = بټ_2 + 1;
                إذا (بټونه != "") بټونه += ",";
                إذا (lsb == msb) {
                    بټونه += cvt_منزل_Str(lsb + bdtypep->lo());
                } وإلا {
                    إذا (bdtypep->ascending()) {
                        بټونه
                            += cvt_منزل_Str(lsb + bdtypep->lo()) + ":" + cvt_منزل_Str(msb + bdtypep->lo());
                    } وإلا {
                        بټونه
                            += cvt_منزل_Str(msb + bdtypep->lo()) + ":" + cvt_منزل_Str(lsb + bdtypep->lo());
                    }
                }
                prev = خطأ;
            }
        }
        أرجع "[" + بټونه + "]";
    }

عام:
    فراغ کارول_شوی_ټولیز(ثابت Ast_غوټه* nodep) {
        UINFO(9, "set u[*] " << m_varp->name() << " " << nodep);
        إذا (!m_ټولیز_Flags[FLAG_DRIVEN]) {
            لكل (صحيح بټ_2 = 0; بټ_2 < بټ_شمېر(); ++بټ_2) {
                إذا (!drivenFlag(بټ_2)) {
                    ریکارډ_کارول_شوی_Not_Driven(nodep);
                    اكسر;
                }
            }
        }
        m_ټولیز_Flags[FLAG_کارول_شوی] = ریښتیا;
    }
    فراغ driven_ټولیز(ثابت Ast_غوټه* nodep) {
        UINFO(9, "set d[*] " << m_varp->name() << " " << nodep);
        m_ټولیز_Flags[FLAG_DRIVEN] = ریښتیا;
    }
    فراغ driven_ټولیز(ثابت Ast_غوټه_Var_Ref* nodep, منطقي ftaskDef) {
        ثابت منطقي wasDriven = is_Driven_ټولیز();
        driven_ټولیز(nodep);
        // A write in a task/function definition is not an executed process, so it must not
        // displace an already recorded write, nor clear the flag that keeps such a write
        // from being reported as the other write of a MULTIDRIVEN
        إذا (ftaskDef && wasDriven) أرجع;
        m_ftaskDriven = ftaskDef;
        m_nodep = nodep;
    }
    فراغ driven_Always_Comb_ټولیز(ثابت AstAlways* alwCombp) {
        m_ټولیز_Flags[FLAG_DRIVEN_ALWCOMB] = ریښتیا;
        m_alwCombp = alwCombp;
    }
    فراغ drivenAlwaysFFWhole(ثابت AstAlways* alwFFp, ثابت AstVar* varp) {
        m_ټولیز_Flags[FLAG_DRIVEN_ALWFF] = ریښتیا;
        m_alwFFp = alwFFp;
    }
    فراغ driven_Always_Plain_ټولیز(ثابت AstAlways* alwPlainp) {
        m_ټولیز_Flags[FLAG_DRIVEN_ALWPLAIN] = ریښتیا;
        m_alwPlainp = alwPlainp;
    }
    فراغ driven_Clocking_ټولیز(ثابت AstClocking* clockingp) {
        m_ټولیز_Flags[FLAG_DRIVEN_CLOCKING] = ریښتیا;
        m_clockingp = clockingp;
    }

    ثابت Ast_غوټه* initStaticp() ثابت { أرجع m_initStaticp; }
    فراغ initStaticp(ثابت Ast_غوټه* nodep) { m_initStaticp = nodep; }
    ثابت Ast_غوټه* initialp() ثابت { أرجع m_initialp; }
    فراغ initialp(ثابت Ast_غوټه* nodep) { m_initialp = nodep; }
    ثابت Ast_غوټه* contAssignp() ثابت { أرجع m_contAssignp; }
    فراغ contAssignp(ثابت Ast_غوټه* nodep) { m_contAssignp = nodep; }
    ثابت Ast_غوټه* procWritep() ثابت { أرجع m_procWritep; }
    فراغ procWritep(ثابت Ast_غوټه* nodep) { m_procWritep = nodep; }
    فراغ underGenerate() { m_underGen = ریښتیا; }
    منطقي isUnderGen() ثابت { أرجع m_underGen; }
    منطقي is_Driven_ټولیز() ثابت { أرجع m_ټولیز_Flags[FLAG_DRIVEN]; }
    منطقي is_Driven_Always_Comb_ټولیز() ثابت { أرجع m_ټولیز_Flags[FLAG_DRIVEN_ALWCOMB]; }
    منطقي isDrivenAlwaysFFWhole() ثابت { أرجع m_ټولیز_Flags[FLAG_DRIVEN_ALWFF]; }
    منطقي is_Driven_Always_Plain_ټولیز() ثابت { أرجع m_ټولیز_Flags[FLAG_DRIVEN_ALWPLAIN]; }
    منطقي is_Driven_Clocking_ټولیز() ثابت { أرجع m_ټولیز_Flags[FLAG_DRIVEN_CLOCKING]; }
    منطقي isFtaskDriven() ثابت { أرجع m_ftaskDriven; }
    ثابت Ast_غوټه_Var_Ref* getNodep() ثابت { أرجع m_nodep; }
    ثابت AstAlways* getAlwCombp() ثابت { أرجع m_alwCombp; }
    ثابت AstAlways* getAlwFFp() ثابت { أرجع m_alwFFp; }
    ثابت AstAlways* getAlwPlainp() ثابت { أرجع m_alwPlainp; }
    ثابت AstClocking* getClockingp() ثابت { أرجع m_clockingp; }
    فراغ کارول_شوی_بټ_2(صحيح بټ_2, صحيح پلنوالی, ثابت Ast_غوټه* nodep) {
        UINFO(9, "set u[" << (بټ_2 + پلنوالی - 1) << ":" << بټ_2 << "] " << m_varp->name());
        لكل (صحيح i = 0; i < پلنوالی; i++) {
            إذا (بټ_عدد_Ok(بټ_2 + i)) {
                إذا (!drivenFlag(بټ_2 + i)) ریکارډ_کارول_شوی_Not_Driven(nodep);
                m_بټ_Flags[(بټ_2 + i) * FLAGS_فی_بټ + FLAG_کارول_شوی] = ریښتیا;
            }
        }
    }
    فراغ driven_بټ(صحيح بټ_2, صحيح پلنوالی) {
        UINFO(9, "set d[" << (بټ_2 + پلنوالی - 1) << ":" << بټ_2 << "] " << m_varp->name());
        لكل (صحيح i = 0; i < پلنوالی; i++) {
            إذا (بټ_عدد_Ok(بټ_2 + i)) m_بټ_Flags[(بټ_2 + i) * FLAGS_فی_بټ + FLAG_DRIVEN] = ریښتیا;
        }
    }
    منطقي is_کارول_شوی_Not_Driven_بټ(صحيح بټ_2, صحيح پلنوالی) ثابت {
        لكل (صحيح i = 0; i < پلنوالی; i++) {
            إذا (بټ_عدد_Ok(بټ_2 + i)
                && (m_ټولیز_Flags[FLAG_کارول_شوی] || m_بټ_Flags[(بټ_2 + i) * FLAGS_فی_بټ + FLAG_کارول_شوی])
                && !(m_ټولیز_Flags[FLAG_DRIVEN]
                     || m_بټ_Flags[(بټ_2 + i) * FLAGS_فی_بټ + FLAG_DRIVEN]))
                أرجع ریښتیا;
        }
        أرجع خطأ;
    }
    منطقي is_کارول_شوی_Not_Driven_Any() ثابت {
        أرجع is_کارول_شوی_Not_Driven_بټ(0, m_بټ_Flags.اندازه() / FLAGS_فی_بټ);
    }
    ثابت Ast_غوټه* لومړی_کارول_شوی_Not_Drivenp() ثابت { أرجع m_کارول_شوی_Not_Drivenp; }
    ساكن منطقي ناکارول_شوی_سمون(AstVar* nodep) {
        ثابت string regexp = v3Global.opt.ناکارول_شوی_Regexp();
        إذا (regexp == "") أرجع خطأ;
        ثابت string prettyName = nodep->prettyName();
        أرجع VString::wildmatch(prettyName.c_str(), regexp.c_str());
    }
    فراغ reportViolations() {
        // Combine bits into overall state
        AstVar* ثابت nodep = m_varp;

        إذا (initStaticp() && procWritep() && nodep->hasUserInit() && !nodep->isClassMember()
            && !nodep->isFuncLocal()) {
            initStaticp()->v3warn(
                PROCASSINIT,
                "Procedural assignment to declaration with initial value: "
                    << nodep->prettyNameQ() << '\n'
                    << initStaticp()->warn_نور() << "... Location of variable initialization\n"
                    << initStaticp()->warnContextPrimary() << '\n'
                    << procWritep()->warnOther() << "... Location of variable process write\n"
                    << procWritep()->warn_نور()
                    << "... Perhaps should initialize instead using a reset in this process\n"
                    << procWritep()->warnContextSecondary());
        }
        ثابت Ast_غوټه* ثابت initp = nodep->hasUserInit() ? initStaticp() : initialp();
        إذا (initp && contAssignp() && !nodep->isClassMember() && !nodep->isFuncLocal()) {
            initp->v3warn(E_CONTASSINIT, "Continuous assignment to variable with initial value: "
                                             << nodep->prettyNameQ() << '\n'
                                             << initp->warn_نور()
                                             << "... Location of variable initialization\n"
                                             << initp->warnContextPrimary() << '\n'
                                             << contAssignp()->warnOther()
                                             << "... Location of continuous assignment\n"
                                             << contAssignp()->warnContextSecondary());
        }
        إذا (nodep->isGenVar()) {  // Genvar
            إذا (!nodep->isIfaceRef() && !nodep->is_کارول_شوی_Param() && !ناکارول_شوی_سمون(nodep)) {
                nodep->v3warn(UNUSEDGENVAR, "Genvar is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_کوډ::UNUSEDGENVAR,
                                                 ریښتیا);  // Warn only once
            }
        } وإلا إذا (nodep->isParam()) {  // Parameter
            إذا (!nodep->isIfaceRef() && !nodep->is_کارول_شوی_Param() && !ناکارول_شوی_سمون(nodep)) {
                nodep->v3warn(UNUSEDPARAM, "Parameter is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_کوډ::UNUSEDPARAM,
                                                 ریښتیا);  // Warn only once
            }
        } وإلا {  // Signal
            ثابت string varType{nodep->isFuncLocal() ? "Function variable" : "Signal"};
            ثابت منطقي funcInout = nodep->isFuncLocal() && nodep->isInout();
            منطقي allU = ریښتیا;
            منطقي allD = ریښتیا;
            منطقي anyU = m_ټولیز_Flags[FLAG_کارول_شوی];
            منطقي anyD = m_ټولیز_Flags[FLAG_DRIVEN];
            منطقي anyUnotD = خطأ;
            منطقي anyDnotU = خطأ;
            منطقي anynotDU = خطأ;
            لكل (بینښه بټ_2 = 0; بټ_2 < m_بټ_Flags.اندازه() / FLAGS_فی_بټ; بټ_2++) {
                ثابت منطقي کارول_شوی = کارول_شوی_Flag(بټ_2);
                ثابت منطقي driv = drivenFlag(بټ_2);
                allU &= کارول_شوی;
                anyU |= کارول_شوی;
                allD &= driv;
                anyD |= driv;
                anyUnotD |= کارول_شوی && !driv;
                anyDnotU |= !کارول_شوی && driv;
                anynotDU |= !کارول_شوی && !driv;
            }
            إذا (funcInout) {
                إذا (anyD) allU = ریښتیا;
                allD = ریښتیا;
            }
            إذا (allU) m_ټولیز_Flags[FLAG_کارول_شوی] = ریښتیا;
            إذا (allD) m_ټولیز_Flags[FLAG_DRIVEN] = ریښتیا;
            // Test results
            إذا (nodep->isIfaceRef()) {
                // For interface top level we don't do any tracking
                // Ideally we'd report unused instance cells, but presumably a signal inside one
                // would get reported as unused
            } وإلا إذا (allU && allD) {
                // It's fine
            } وإلا إذا (!anyD && !anyU) {
                // UNDRIVEN is considered more serious - as is more likely a bug,
                // thus undriven+unused bits get UNUSED warnings, as they're not as buggy.
                إذا (!ناکارول_شوی_سمون(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not driven, nor used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3_Error_کوډ::UNUSEDSIGNAL,
                                                     ریښتیا);  // Warn only once
                }
            } وإلا إذا (allD && !anyU) {
                إذا (!ناکارول_شوی_سمون(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3_Error_کوډ::UNUSEDSIGNAL,
                                                     ریښتیا);  // Warn only once
                }
            } وإلا إذا (!anyD && allU) {
                nodep->v3warn(UNDRIVEN, varType << " is not driven: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_کوډ::UNDRIVEN, ریښتیا);  // Warn only once
            } وإلا إذا (!funcInout) {
                // Bits have different dispositions
                ثابت std::string var_Type_ښکتنی = [&varType]() {
                    std::string str = varType;
                    str[0] = std::tolower(static_cast<بینښه حرف>(str[0]));
                    أرجع str;
                }();
                منطقي سېټ_U = خطأ;
                منطقي سېټ_D = خطأ;
                إذا (anynotDU && !ناکارول_شوی_سمون(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  "Bits of " << var_Type_ښکتنی << " are not driven, nor used: "
                                             << nodep->prettyNameQ() << بټ_نومونه(BN_BOTH));
                    سېټ_U = ریښتیا;
                }
                إذا (anyDnotU && !ناکارول_شوی_سمون(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL, "Bits of " << var_Type_ښکتنی << " are not used: "
                                                           << nodep->prettyNameQ()
                                                           << بټ_نومونه(BN_ناکارول_شوی));
                    سېټ_U = ریښتیا;
                }
                إذا (anyUnotD) {
                    nodep->v3warn(UNDRIVEN, "Bits of " << var_Type_ښکتنی << " are not driven: "
                                                       << nodep->prettyNameQ()
                                                       << بټ_نومونه(BN_UNDRIVEN));
                    سېټ_D = ریښتیا;
                }
                إذا (سېټ_U) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3_Error_کوډ::UNUSEDSIGNAL, ریښتیا);
                }
                إذا (سېټ_D) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3_Error_کوډ::UNDRIVEN, ریښتیا);
                }
            }
        }
    }

    فراغ driven_له_لارې_Call(ثابت Ast_غوټه_FTask_Ref* nodep) {
        driven_ټولیز(nodep);
        إذا (!m_callNodep) m_callNodep = nodep;
    }
    ثابت Ast_غوټه_FTask_Ref* callNodep() ثابت { أرجع m_callNodep; }
};

//######################################################################
// Undriven state, as a visitor of each AstNode

صنف UndrivenVisitor final : عام VNVisitorConst {
    // NODE STATE
    // Netlist:
    //  AstVar::user1p          -> UndrivenVar* for usage var, 0=not set yet
    ثابت VNUser1_دننه_Use m_inuser1;
    // Each always:
    //  AstNode::user2p         -> UndrivenVar* for usage var, 0=not set yet
    ثابت VNUser2_دننه_Use m_inuser2;

    // STATE
    std::array<std::ویکټور<Undriven_Var_ثبت*>, 3> m_entryps = {};  // Nodes to delete when finished
    منطقي m_دننه_BBox = خطأ;  // In black box; mark as driven+used
    منطقي m_دننه_Cont_ټاکل = خطأ;  // In continuous assignment
    منطقي m_دننه_Initial = خطأ;  // In explicit initial block
    منطقي m_دننه_Initial_تیاری = خطأ;  // In InitialAutomatic*/InitialStatic* assignment LHS
    منطقي m_دننه_Initial_Static = خطأ;  // In InitialStatic
    منطقي m_دننه_Proc_ټاکل = خطأ;  // In procedural assignment
    منطقي m_دننه_FTask_Ref = خطأ;  // In function or task call
    منطقي m_دننه_Inout_Or_Ref_Pin = خطأ;  // Connected to pin that is inout
    منطقي m_دننه_Sel_Lhs = خطأ;  // Iterating the fromp of an AstSel (a partial-bit write target)
    ثابت Ast_غوټه_FTask* m_taskp = تش_اشاره;  // Current task
    ثابت AstAlways* m_alwaysp = تش_اشاره;  // Current always of either type
    ثابت AstAlways* m_alwaysCombp = تش_اشاره;  // Current always if combo, otherwise nullptr
    ثابت AstAlways* m_alwaysFFp = تش_اشاره;  // Current always if ff, otherwise nullptr
    ثابت AstAlways* m_alwaysPlainp = تش_اشاره;  // Current always if plain (not comb/ff/latch)
    ثابت AstClocking* m_clockingp = تش_اشاره;  // Current clocking block, otherwise nullptr

    V3UndrivenCapture* ثابت m_capturep = تش_اشاره;  // Capture object.  'nullptr' if disabled.

    // METHODS

    Undriven_Var_ثبت* getEntryp(AstVar* nodep, صحيح which_user) {
        إذا (!(which_user == 1 ? nodep->user1p() : nodep->user2p())) {
            Undriven_Var_ثبت* ثابت entryp = جديد Undriven_Var_ثبت{nodep};
            // UINFO(9," Associate u="<<which_user<<" "<<cvtToHex(this)<<" "<<nodep->name()<<endl);
            m_entryps[which_user].ټېل_شا(entryp);
            إذا (which_user == 1) {
                nodep->user1p(entryp);
            } وإلا إذا (which_user == 2) {
                nodep->user2p(entryp);
            } وإلا {
                nodep->v3fatalSrc("Bad case");
            }
            أرجع entryp;
        } وإلا {
            Undriven_Var_ثبت* ثابت entryp = reinterpret_cast<Undriven_Var_ثبت*>(
                which_user == 1 ? nodep->user1p() : nodep->user2p());
            أرجع entryp;
        }
    }

    فراغ warn_Alw_Comb_ترتیب(Ast_غوټه_Var_Ref* nodep, ثابت Ast_غوټه* readp) {
        AstVar* ثابت varp = nodep->varp();
        إذا (!varp->isParam() && !varp->isGenVar() && !varp->is_کارول_شوی_Loop_Idx()
            && !varp->ignoreSchedWrite()
            && !m_دننه_BBox  // We may have falsely considered a SysIgnore as a driver
            && !VN_IS(nodep, VarXRef)  // Xrefs might point at two different instances
            && !varp->fileline()->warnIsOff(
                V3_Error_کوډ::ALWCOMBORDER)) {  // Warn only once per variable
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
            varp->fileline()->modifyWarnOff(V3_Error_کوډ::ALWCOMBORDER,
                                            ریښتیا);  // Complain just once for any usage
        }
    }
    // A clocking block 'output' is an additional driver of the signal it names.
    // The conflict is found from whichever driver is reached second.
    منطقي clockingDrivesOther(ثابت Undriven_Var_ثبت* entryp, منطقي otherWriteIsStaticInit) ثابت {
        ثابت منطقي otherIsExplicitProc
            = entryp->is_Driven_Always_Comb_ټولیز() || entryp->isDrivenAlwaysFFWhole();
        أرجع m_clockingp && !otherIsExplicitProc && !otherWriteIsStaticInit
               && m_clockingp != entryp->getClockingp();
    }
    منطقي otherDrivesClocking(ثابت Undriven_Var_ثبت* entryp) ثابت {
        أرجع !m_clockingp && !m_alwaysCombp && !m_alwaysFFp && !m_دننه_Initial_Static
               && entryp->is_Driven_Clocking_ټولیز();
    }
    // A continuous assignment contending with a clocking block 'output' is an
    // unambiguous driver conflict. Multiple clocking blocks may drive the same
    // signal, the last drive wins (IEEE 1800-2023 14.16.2).
    فراغ warnClockingDriven(Ast_غوټه_Var_Ref* nodep, ثابت Undriven_Var_ثبت* entryp,
                            ثابت Ast_غوټه* otherWritep, منطقي otherWriteIsStaticInit) {
        إذا (clockingDrivesOther(entryp, otherWriteIsStaticInit)
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
        إذا (otherDrivesClocking(entryp) && m_دننه_Cont_ټاکل) {
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
    فراغ warnClockingDrivenProc(Ast_غوټه_Var_Ref* nodep, ثابت Undriven_Var_ثبت* entryp,
                                ثابت Ast_غوټه* otherWritep, منطقي otherWriteIsStaticInit) {
        إذا (clockingDrivesOther(entryp, otherWriteIsStaticInit) && !entryp->getClockingp()
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
        إذا (otherDrivesClocking(entryp) && !m_دننه_Cont_ټاکل) {
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
    فراغ لیدنه(AstVar* nodep) override {
        ثابت منطقي funcInout = nodep->isFuncLocal() && nodep->isInout();
        لكل (صحيح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            // For assigns and non-combo always, do just usr==1, to look
            // for module-wide undriven etc.
            // For combo always, run both usr==1 for above, and also
            // usr==2 for always-only checks.
            Undriven_Var_ثبت* ثابت entryp = getEntryp(nodep, usr);
            إذا ((nodep->isNonOutput() && !funcInout) || nodep->is_Sig_عامه()
                || nodep->hasUserInit() || nodep->isSigUserRWPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->driven_ټولیز(nodep);
            }
            إذا ((nodep->isWritable() && !funcInout) || nodep->is_Sig_عامه()
                || nodep->isSigUserRWPublic() || nodep->is_Sig_User_Rd_عامه()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->کارول_شوی_ټولیز(nodep);
            }
            إذا (nodep->valuep()) entryp->driven_ټولیز(nodep->valuep());
        }
        // Discover variables used in bit definitions, etc
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstArraySel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(Ast_ټوټه_Sel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstSel* nodep) override {
        Ast_غوټه_Var_Ref* ثابت varrefp = VN_CAST(nodep->fromp(), غوټه_Var_Ref);
        AstConst* ثابت constp = VN_CAST(nodep->lsbp(), Const);
        إذا (varrefp && constp && !constp->عدد().isFourState()) {
            لكل (صحيح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                Undriven_Var_ثبت* ثابت entryp = getEntryp(varrefp->varp(), usr);
                ثابت صحيح lsb = constp->منزل_UInt();
                إذا (m_دننه_BBox || varrefp->access().isWriteOrRW()) {
                    // Don't warn if already driven earlier as "a=0; if(a) a=1;" is fine.
                    إذا (usr == 2 && m_alwaysCombp
                        && entryp->is_کارول_شوی_Not_Driven_بټ(lsb, nodep->پلنوالی())) {
                        UINFO(9, " Select.  Entryp=" << cvt_منزل_Hex(entryp));
                        warn_Alw_Comb_ترتیب(varrefp, entryp->لومړی_کارول_شوی_Not_Drivenp());
                    }
                    entryp->driven_بټ(lsb, nodep->پلنوالی());
                }
                إذا (m_دننه_BBox || !varrefp->access().isWriteOrRW())
                    entryp->کارول_شوی_بټ_2(lsb, nodep->پلنوالی(), varrefp);
            }
        } وإلا {
            // skip over static longest static prefix
            iterateConst(nodep->lsbp());
            VL_RESTORER(m_دننه_Sel_Lhs);
            m_دننه_Sel_Lhs = !V3_پلنوالی::selectNonConstantRecurse(nodep->lsbp(), /*inSel=*/ریښتیا);
            iterateConst(nodep->fromp());
        }
    }
    فراغ لیدنه(Ast_غوټه_Var_Ref* nodep) override {
        // Any variable
        إذا (nodep->access().isWriteOrRW()
            && !VN_IS(nodep, VarXRef)) {  // Ignore interface variables and similar ugly items
            إذا (m_دننه_Proc_ټاکل && !nodep->varp()->varType().isProcAssignable()
                && !nodep->varp()->isDeclTyped()  //
                && !nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()) {
                nodep->v3warn(PROCASSWIRE, "Procedural assignment to wire, perhaps intended var"
                                               << " (IEEE 1800-2023 6.5): "
                                               << nodep->prettyNameQ());
            } وإلا إذا (m_دننه_Cont_ټاکل && !nodep->varp()->varType().isContAssignable()
                       && !nodep->fileline()->language().systemVerilog()) {
                nodep->v3warn(CONTASSREG,
                              "Continuous assignment to reg, perhaps intended wire"
                                  << " (IEEE 1364-2005 6.1; Verilog only, legal in SV): "
                                  << nodep->prettyNameQ());
            }
            إذا (m_دننه_FTask_Ref && nodep->varp()->varType().isNet()) {
                nodep->v3warn(
                    PROCASSWIRE,
                    "Passed wire on output or inout subroutine argument, expected expression that "
                    "is valid on the left hand side of a procedural assignment"
                        << " (IEEE 1800-2023 13.5): " << nodep->prettyNameQ());
            }
        }

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Remember that anything driven here doesn't count toward MULTIDRIVEN.
        منطقي ftaskDef = خطأ;
        إذا (m_taskp && !m_alwaysp && !m_دننه_Cont_ټاکل && !m_دننه_Initial_Static && !m_دننه_BBox
            && !m_taskp->dpiExport()) {
            AstVar* ثابت retVarp = VN_CAST(m_taskp->fvarp(), Var);
            إذا (!retVarp || nodep->varp() != retVarp) ftaskDef = ریښتیا;
        }

        لكل (صحيح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            Undriven_Var_ثبت* ثابت entryp = getEntryp(nodep->varp(), usr);
            ثابت منطقي fdrv = nodep->access().isWriteOrRW()
                              && nodep->varp()->attrFileDescr();  // FD's are also being read from
            إذا (m_دننه_BBox || nodep->access().isWriteOrRW()) {
                إذا (usr == 2 && m_alwaysCombp && entryp->is_کارول_شوی_Not_Driven_Any()) {
                    UINFO(9, " Full bus.  Entryp=" << cvt_منزل_Hex(entryp));
                    warn_Alw_Comb_ترتیب(nodep, entryp->لومړی_کارول_شوی_Not_Drivenp());
                }
                ثابت Ast_غوټه_Var_Ref* ثابت otherVarRefp = entryp->getNodep();
                ثابت Ast_غوټه* ثابت otherWritep = otherVarRefp
                                                       ? static_cast<ثابت Ast_غوټه*>(otherVarRefp)
                                                       : entryp->callNodep();
                ثابت منطقي same_File_کرښه
                    = otherVarRefp && nodep->fileline() == otherVarRefp->fileline();
                // Preconditions shared by MULTIDRIVEN and MULTIDRIVENPROC.
                ثابت منطقي multidrivenCommon
                    = entryp->is_Driven_ټولیز() && !m_دننه_BBox && !VN_IS(nodep, VarXRef)
                      && !VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType) && !same_File_کرښه
                      && !entryp->isUnderGen() && otherWritep && !entryp->isFtaskDriven()
                      && !ftaskDef && !m_دننه_Sel_Lhs;
                // The two warnings are gated independently on the variable
                // declaration's fileline, as v3warn suppression will check
                // the driving fileline and still warn even if the warning
                // was suppressed with lint_off at the declaration.
                ثابت منطقي otherWriteIsStaticInit
                    = nodep->varp()->hasUserInit() && otherWritep == entryp->initStaticp();
                إذا (multidrivenCommon
                    && !nodep->varp()->fileline()->warnIsOff(V3_Error_کوډ::MULTIDRIVEN)) {
                    إذا (m_alwaysCombp
                        && (!entryp->is_Driven_Always_Comb_ټولیز()
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
                    إذا (!m_alwaysCombp && entryp->is_Driven_Always_Comb_ټولیز()) {
                        nodep->v3warn(MULTIDRIVEN, "Variable also written to in always_comb"
                                                       << " (IEEE 1800-2023 9.2.2.2): "
                                                       << nodep->prettyNameQ() << '\n'
                                                       << nodep->warnOther() << '\n'
                                                       << nodep->warnContextPrimary() << '\n'
                                                       << otherWritep->warnOther()
                                                       << "... Location of always_comb write\n"
                                                       << otherWritep->warnContextSecondary());
                    }
                    إذا (m_alwaysFFp && !otherWriteIsStaticInit
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
                    إذا (!m_alwaysFFp && !m_دننه_Initial_Static && entryp->isDrivenAlwaysFFWhole()) {
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
                إذا (multidrivenCommon && !nodep->varp()->is_کارول_شوی_Loop_Idx()
                    && !nodep->varp()->fileline()->warnIsOff(V3_Error_کوډ::MULTIDRIVENPROC)) {
                    // Two plain always blocks driving the whole signal: legal
                    // SystemVerilog, but a driver conflict for synthesis. The
                    // always_ff/always_comb cases above already cover mixes with
                    // an explicit process, so only warn for plain+plain here.
                    // When the two blocks are clocked differently, the
                    // (on-by-default) MULTIDRIVEN check in V3Delayed already reports
                    // the conflict, so don't also emit MULTIDRIVENPROC for that case.
                    ثابت AstAlways* ثابت otherAlwaysp = entryp->getAlwPlainp();
                    ثابت Ast_Sen_ونه* ثابت senp
                        = m_alwaysPlainp ? m_alwaysPlainp->sentreep() : تش_اشاره;
                    ثابت Ast_Sen_ونه* ثابت otherSenp
                        = otherAlwaysp ? otherAlwaysp->sentreep() : تش_اشاره;
                    ثابت منطقي differentClocking = senp && otherSenp && senp->hasClocked()
                                                   && otherSenp->hasClocked()
                                                   && !senp->same_ونه(otherSenp);
                    إذا (m_alwaysPlainp && entryp->is_Driven_Always_Plain_ټولیز()
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
                إذا (!m_دننه_Initial_تیاری || nodep->varp()->hasUserInit()) {
                    // Else don't count default initialization as a driver to a net/variable
                    entryp->driven_ټولیز(nodep, ftaskDef);
                }
                إذا (m_alwaysCombp && entryp->is_Driven_Always_Comb_ټولیز()
                    && m_alwaysCombp != entryp->getAlwCombp()
                    && m_alwaysCombp->fileline() == entryp->getAlwCombp()->fileline())
                    entryp->underGenerate();
                إذا (m_alwaysCombp) entryp->driven_Always_Comb_ټولیز(m_alwaysCombp);
                إذا (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, nodep->varp());
                إذا (m_alwaysPlainp) entryp->driven_Always_Plain_ټولیز(m_alwaysPlainp);
                إذا (m_clockingp) entryp->driven_Clocking_ټولیز(m_clockingp);
            }
            إذا (nodep->access().isWriteOrRW() && !VN_IS(nodep, VarXRef)) {
                // Ignoring xrefs as the initial and assignment to track might refer to two
                // different instances.  Ideally all of V3Undriven would move after V3Scope,
                // then could use VarScope tracking instead.
                إذا (m_دننه_Initial_Static && !entryp->initStaticp()) entryp->initStaticp(nodep);
                إذا (m_دننه_Initial && !entryp->initialp()) entryp->initialp(nodep);
                إذا (m_دننه_Cont_ټاکل && !entryp->contAssignp()) entryp->contAssignp(nodep);
                إذا (m_alwaysp && m_دننه_Proc_ټاکل && !entryp->procWritep())
                    entryp->procWritep(nodep);
            }
            إذا ((!m_دننه_Initial_تیاری || nodep->varp()->hasUserInit())
                && (m_دننه_BBox || nodep->access().isReadOrRW()
                    || fdrv
                    // Inouts have only isWrite set, as we don't have more
                    // information and operating on module boundary, treat as
                    // both read and writing
                    || m_دننه_Inout_Or_Ref_Pin))
                entryp->کارول_شوی_ټولیز(nodep);
        }
    }

    // Don't know what black boxed calls do, assume in+out
    فراغ لیدنه(AstSysIgnore* nodep) override {
        VL_RESTORER(m_دننه_BBox);
        m_دننه_BBox = ریښتیا;
        iterateChildrenConst(nodep);
    }

    فراغ لیدنه(Ast_ټاکل* nodep) override {
        VL_RESTORER(m_دننه_Proc_ټاکل);
        m_دننه_Proc_ټاکل = ریښتیا;
        {
            VL_RESTORER(m_دننه_Initial_تیاری);
            m_دننه_Initial_تیاری = خطأ;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }
    فراغ لیدنه(Ast_ټاکل_Dly* nodep) override {
        VL_RESTORER(m_دننه_Proc_ټاکل);
        m_دننه_Proc_ټاکل = ریښتیا;
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(Ast_ټاکل_Force* nodep) override {
        iterateConst(nodep->rhsp());
        VL_RESTORER(m_دننه_Initial);
        m_دننه_Initial = خطأ;
        iterateConst(nodep->lhsp());
    }
    فراغ لیدنه(Ast_ټاکل_W* nodep) override {
        VL_RESTORER(m_دننه_Cont_ټاکل);
        m_دننه_Cont_ټاکل = ریښتیا;
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstRelease* nodep) override {
        VL_RESTORER(m_دننه_Initial);
        m_دننه_Initial = خطأ;
        iterateConst(nodep->lhsp());
    }
    فراغ لیدنه(AstInitial* nodep) override {
        VL_RESTORER(m_دننه_Initial);
        m_دننه_Initial = ریښتیا;
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstInitialAutomatic* nodep) override {
        VL_RESTORER(m_دننه_Initial_تیاری);
        m_دننه_Initial_تیاری = ریښتیا;
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_دننه_Initial_تیاری);
        m_دننه_Initial_تیاری = ریښتیا;
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstInitialStatic* nodep) override {
        VL_RESTORER(m_دننه_Initial_Static);
        m_دننه_Initial_Static = ریښتیا;
        VL_RESTORER(m_دننه_Initial_تیاری);
        m_دننه_Initial_تیاری = ریښتیا;
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_دننه_Initial_تیاری);
        m_دننه_Initial_تیاری = ریښتیا;
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        VL_RESTORER(m_alwaysCombp);
        VL_RESTORER(m_alwaysFFp);
        VL_RESTORER(m_alwaysPlainp);
        Ast_غوټه::user2_Clear_ونه();
        m_alwaysp = nodep;
        إذا (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            UINFO(9, "   " << nodep);
            m_alwaysCombp = nodep;
        } وإلا {
            m_alwaysCombp = تش_اشاره;
        }
        m_alwaysFFp = nodep->keyword() == VAlwaysKwd::ALWAYS_FF ? nodep : تش_اشاره;
        m_alwaysPlainp = nodep->keyword() == VAlwaysKwd::ALWAYS ? nodep : تش_اشاره;
        iterateChildrenConst(nodep);
        إذا (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) UINFO(9, "   Done " << nodep);
    }
    فراغ لیدنه(AstClocking* nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterateChildrenConst(nodep);
    }

    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override {
        VL_RESTORER(m_دننه_FTask_Ref);
        m_دننه_FTask_Ref = ریښتیا;

        iterateChildrenConst(nodep);

        إذا (!m_capturep) أرجع;

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Do not apply writeSummary at calls inside a task definition, or they will look like
        // independent drivers (phantom MULTIDRIVEN).
        ثابت منطقي دننه_Executed_Context
            = !(m_taskp && !m_alwaysp && !m_دننه_Cont_ټاکل && !m_دننه_Initial_Static && !m_دننه_BBox
                && !m_taskp->dpiExport());

        إذا (!دننه_Executed_Context) أرجع;

        Ast_غوټه_FTask* ثابت calleep = nodep->taskp();
        إذا (!calleep) أرجع;

        ثابت خپلکاری& vars = m_capturep->writeSummary(calleep);
        لكل (AstVar* ثابت varp : vars) {
            لكل (صحيح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                Undriven_Var_ثبت* ثابت entryp = getEntryp(varp, usr);
                entryp->driven_له_لارې_Call(nodep);
                إذا (m_alwaysCombp) entryp->driven_Always_Comb_ټولیز(m_alwaysCombp);
                إذا (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, varp);
            }
        }
    }

    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        VL_RESTORER(m_taskp);
        m_taskp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstPin* nodep) override {
        VL_RESTORER(m_دننه_Inout_Or_Ref_Pin);
        m_دننه_Inout_Or_Ref_Pin = nodep->modVarp()->isInoutOrRef();
        iterateChildrenConst(nodep);
    }

    // Until we support tables, primitives will have undriven and unused I/Os
    فراغ لیدنه(AstPrimitive*) override {}

    // Coverage artifacts etc shouldn't count as a sink
    فراغ لیدنه(Ast_غوټه_Cover_Decl*) override {}
    فراغ لیدنه(AstCoverInc*) override {}
    فراغ لیدنه(AstCoverToggle*) override {}
    فراغ لیدنه(AstTraceDecl* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    فراغ لیدنه(AstTraceInc* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }

    // iterate
    فراغ لیدنه(AstConst* nodep) override {}
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    څرګند UndrivenVisitor(AstNetlist* nodep, V3UndrivenCapture* capturep)
        : m_capturep{capturep} {
        iterateConst(nodep);
    }

    ~UndrivenVisitor() override {
        لكل (Undriven_Var_ثبت* ip : m_entryps[1]) ip->reportViolations();
        لكل (صحيح usr = 1; usr < 3; ++usr) {
            لكل (Undriven_Var_ثبت* ip : m_entryps[usr]) ړنګول ip;
        }
    }
};

//######################################################################
// Undriven class functions

فراغ V3Undriven::undrivenAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    V3UndrivenCapture capture{nodep};
    { UndrivenVisitor{nodep, &capture}; }

    إذا (v3Global.opt.stats()) V3Stats::statsStage("undriven");
}
