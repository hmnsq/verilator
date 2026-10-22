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

صنف Undriven_Var_مدخل final {
    // MEMBERS
    AstVar* ثابت m_varp;  // Variable this tracks
    std::وکتور<منطقي> m_کامل_Flags;  // Used/Driven on whole vector
    std::وکتور<منطقي> m_بیت_Flags;  // Used/Driven on each subbit
    ثابت Ast_گره* m_استفاده_شده_Not_Drivenp = اشاره_تهی;  // First read before any write
    ثابت AstAlways* m_alwCombp
        = اشاره_تهی;  // always_comb of var if driven within always_comb, else nullptr
    ثابت AstAlways* m_alwFFp = اشاره_تهی;  // always_ff of var if driven within always_ff
    ثابت AstAlways* m_alwPlainp = اشاره_تهی;  // plain always of var if driven within plain always
    ثابت AstClocking* m_clockingp = اشاره_تهی;  // clocking block of var if driven as output
    ثابت Ast_گره_Var_Ref* m_nodep = اشاره_تهی;  // varref if driven, else nullptr
    ثابت Ast_گره* m_initStaticp = اشاره_تهی;  // varref if in InitialStatic driven
    ثابت Ast_گره* m_initialp = اشاره_تهی;  // varref if driven in an explicit initial block
    ثابت Ast_گره* m_contAssignp = اشاره_تهی;  // varref if in continuous assignment driven
    ثابت Ast_گره* m_procWritep = اشاره_تهی;  // varref if written in process
    منطقي m_underGen = خطأ;  // Under a generate
    منطقي m_ftaskDriven = خطأ;  // Last driven by function or task

    ثابت Ast_گره_FTask_Ref* m_callNodep = اشاره_تهی;  // Call node if driven via writeSummary

    تعداد : uint8_t {
        FLAG_استفاده_شده = 0,  // Signal or bit has been read/observed
        FLAG_DRIVEN = 1,  // Signal or bit has been written/driven
        FLAG_DRIVEN_ALWCOMB = 2,  // Whole signal has been driven from always_comb
        FLAG_DRIVEN_ALWFF = 3,  // Whole signal has been driven from always_ff
        FLAG_DRIVEN_ALWPLAIN = 4,  // Whole signal has been driven from a plain always
        FLAG_DRIVEN_CLOCKING = 5,  // Whole signal has been driven as a clocking block output
        FLAGS_در_هر_بیت = 6  // Number of flags stored for each tracked bit
    };

عام:
    // CONSTRUCTORS
    صریح Undriven_Var_مدخل(AstVar* varp)
        : m_varp{varp} {  // Construction for when a var is used
        UINFO(9, "create " << varp);
        m_کامل_Flags.resize(FLAGS_در_هر_بیت);
        لكل (صحيح i = 0; i < FLAGS_در_هر_بیت; i++) m_کامل_Flags[i] = خطأ;
        m_بیت_Flags.resize(varp->عرض() * FLAGS_در_هر_بیت);
        لكل (صحيح i = 0; i < varp->عرض() * FLAGS_در_هر_بیت; i++) m_بیت_Flags[i] = خطأ;
    }
    ~Undriven_Var_مدخل() = افتراضي;

خاص:
    // METHODS
    منطقي بیت_رقم_Ok(صحيح بیت_2) ثابت {
        أرجع بیت_2 >= 0 && (بیت_2 * FLAGS_در_هر_بیت < static_cast<صحيح>(m_بیت_Flags.اندازه()));
    }
    منطقي استفاده_شده_Flag(صحيح بیت_2) ثابت {
        أرجع m_کامل_Flags[FLAG_استفاده_شده] || m_بیت_Flags[بیت_2 * FLAGS_در_هر_بیت + FLAG_استفاده_شده];
    }
    منطقي drivenFlag(صحيح بیت_2) ثابت {
        أرجع m_کامل_Flags[FLAG_DRIVEN] || m_بیت_Flags[بیت_2 * FLAGS_در_هر_بیت + FLAG_DRIVEN];
    }
    صحيح بیت_شمار() ثابت { أرجع m_بیت_Flags.اندازه() / FLAGS_در_هر_بیت; }
    فراغ رکورد_استفاده_شده_Not_Driven(ثابت Ast_گره* nodep) {
        إذا (!m_استفاده_شده_Not_Drivenp) m_استفاده_شده_Not_Drivenp = nodep;
    }
    تعداد بیت_نامها_Which : uint8_t { BN_استفادهنشده, BN_UNDRIVEN, BN_BOTH };
    string بیت_نامها(بیت_نامها_Which which) {
        string بیتها;
        منطقي prev = خطأ;
        صحيح msb = 0;
        // bit==-1 loops below; we do one extra iteration so end with prev=false
        لكل (صحيح بیت_2 = (m_بیت_Flags.اندازه() / FLAGS_در_هر_بیت) - 1; بیت_2 >= -1; --بیت_2) {
            إذا (بیت_2 >= 0
                && ((which == BN_استفادهنشده && !استفاده_شده_Flag(بیت_2) && drivenFlag(بیت_2))
                    || (which == BN_UNDRIVEN && استفاده_شده_Flag(بیت_2) && !drivenFlag(بیت_2))
                    || (which == BN_BOTH && !استفاده_شده_Flag(بیت_2) && !drivenFlag(بیت_2)))) {
                إذا (!prev) {
                    prev = درست;
                    msb = بیت_2;
                }
            } وإلا إذا (prev) {
                ثابت AstBasicDType* ثابت bdtypep = m_varp->basicp();
                ثابت صحيح lsb = بیت_2 + 1;
                إذا (بیتها != "") بیتها += ",";
                إذا (lsb == msb) {
                    بیتها += cvt_مقصد_Str(lsb + bdtypep->lo());
                } وإلا {
                    إذا (bdtypep->ascending()) {
                        بیتها
                            += cvt_مقصد_Str(lsb + bdtypep->lo()) + ":" + cvt_مقصد_Str(msb + bdtypep->lo());
                    } وإلا {
                        بیتها
                            += cvt_مقصد_Str(msb + bdtypep->lo()) + ":" + cvt_مقصد_Str(lsb + bdtypep->lo());
                    }
                }
                prev = خطأ;
            }
        }
        أرجع "[" + بیتها + "]";
    }

عام:
    فراغ استفاده_شده_کامل(ثابت Ast_گره* nodep) {
        UINFO(9, "set u[*] " << m_varp->name() << " " << nodep);
        إذا (!m_کامل_Flags[FLAG_DRIVEN]) {
            لكل (صحيح بیت_2 = 0; بیت_2 < بیت_شمار(); ++بیت_2) {
                إذا (!drivenFlag(بیت_2)) {
                    رکورد_استفاده_شده_Not_Driven(nodep);
                    اكسر;
                }
            }
        }
        m_کامل_Flags[FLAG_استفاده_شده] = درست;
    }
    فراغ driven_کامل(ثابت Ast_گره* nodep) {
        UINFO(9, "set d[*] " << m_varp->name() << " " << nodep);
        m_کامل_Flags[FLAG_DRIVEN] = درست;
    }
    فراغ driven_کامل(ثابت Ast_گره_Var_Ref* nodep, منطقي ftaskDef) {
        ثابت منطقي wasDriven = is_Driven_کامل();
        driven_کامل(nodep);
        // A write in a task/function definition is not an executed process, so it must not
        // displace an already recorded write, nor clear the flag that keeps such a write
        // from being reported as the other write of a MULTIDRIVEN
        إذا (ftaskDef && wasDriven) أرجع;
        m_ftaskDriven = ftaskDef;
        m_nodep = nodep;
    }
    فراغ driven_Always_Comb_کامل(ثابت AstAlways* alwCombp) {
        m_کامل_Flags[FLAG_DRIVEN_ALWCOMB] = درست;
        m_alwCombp = alwCombp;
    }
    فراغ drivenAlwaysFFWhole(ثابت AstAlways* alwFFp, ثابت AstVar* varp) {
        m_کامل_Flags[FLAG_DRIVEN_ALWFF] = درست;
        m_alwFFp = alwFFp;
    }
    فراغ driven_Always_Plain_کامل(ثابت AstAlways* alwPlainp) {
        m_کامل_Flags[FLAG_DRIVEN_ALWPLAIN] = درست;
        m_alwPlainp = alwPlainp;
    }
    فراغ driven_Clocking_کامل(ثابت AstClocking* clockingp) {
        m_کامل_Flags[FLAG_DRIVEN_CLOCKING] = درست;
        m_clockingp = clockingp;
    }

    ثابت Ast_گره* initStaticp() ثابت { أرجع m_initStaticp; }
    فراغ initStaticp(ثابت Ast_گره* nodep) { m_initStaticp = nodep; }
    ثابت Ast_گره* initialp() ثابت { أرجع m_initialp; }
    فراغ initialp(ثابت Ast_گره* nodep) { m_initialp = nodep; }
    ثابت Ast_گره* contAssignp() ثابت { أرجع m_contAssignp; }
    فراغ contAssignp(ثابت Ast_گره* nodep) { m_contAssignp = nodep; }
    ثابت Ast_گره* procWritep() ثابت { أرجع m_procWritep; }
    فراغ procWritep(ثابت Ast_گره* nodep) { m_procWritep = nodep; }
    فراغ underGenerate() { m_underGen = درست; }
    منطقي isUnderGen() ثابت { أرجع m_underGen; }
    منطقي is_Driven_کامل() ثابت { أرجع m_کامل_Flags[FLAG_DRIVEN]; }
    منطقي is_Driven_Always_Comb_کامل() ثابت { أرجع m_کامل_Flags[FLAG_DRIVEN_ALWCOMB]; }
    منطقي isDrivenAlwaysFFWhole() ثابت { أرجع m_کامل_Flags[FLAG_DRIVEN_ALWFF]; }
    منطقي is_Driven_Always_Plain_کامل() ثابت { أرجع m_کامل_Flags[FLAG_DRIVEN_ALWPLAIN]; }
    منطقي is_Driven_Clocking_کامل() ثابت { أرجع m_کامل_Flags[FLAG_DRIVEN_CLOCKING]; }
    منطقي isFtaskDriven() ثابت { أرجع m_ftaskDriven; }
    ثابت Ast_گره_Var_Ref* getNodep() ثابت { أرجع m_nodep; }
    ثابت AstAlways* getAlwCombp() ثابت { أرجع m_alwCombp; }
    ثابت AstAlways* getAlwFFp() ثابت { أرجع m_alwFFp; }
    ثابت AstAlways* getAlwPlainp() ثابت { أرجع m_alwPlainp; }
    ثابت AstClocking* getClockingp() ثابت { أرجع m_clockingp; }
    فراغ استفاده_شده_بیت_2(صحيح بیت_2, صحيح عرض, ثابت Ast_گره* nodep) {
        UINFO(9, "set u[" << (بیت_2 + عرض - 1) << ":" << بیت_2 << "] " << m_varp->name());
        لكل (صحيح i = 0; i < عرض; i++) {
            إذا (بیت_رقم_Ok(بیت_2 + i)) {
                إذا (!drivenFlag(بیت_2 + i)) رکورد_استفاده_شده_Not_Driven(nodep);
                m_بیت_Flags[(بیت_2 + i) * FLAGS_در_هر_بیت + FLAG_استفاده_شده] = درست;
            }
        }
    }
    فراغ driven_بیت(صحيح بیت_2, صحيح عرض) {
        UINFO(9, "set d[" << (بیت_2 + عرض - 1) << ":" << بیت_2 << "] " << m_varp->name());
        لكل (صحيح i = 0; i < عرض; i++) {
            إذا (بیت_رقم_Ok(بیت_2 + i)) m_بیت_Flags[(بیت_2 + i) * FLAGS_در_هر_بیت + FLAG_DRIVEN] = درست;
        }
    }
    منطقي is_استفاده_شده_Not_Driven_بیت(صحيح بیت_2, صحيح عرض) ثابت {
        لكل (صحيح i = 0; i < عرض; i++) {
            إذا (بیت_رقم_Ok(بیت_2 + i)
                && (m_کامل_Flags[FLAG_استفاده_شده] || m_بیت_Flags[(بیت_2 + i) * FLAGS_در_هر_بیت + FLAG_استفاده_شده])
                && !(m_کامل_Flags[FLAG_DRIVEN]
                     || m_بیت_Flags[(بیت_2 + i) * FLAGS_در_هر_بیت + FLAG_DRIVEN]))
                أرجع درست;
        }
        أرجع خطأ;
    }
    منطقي is_استفاده_شده_Not_Driven_Any() ثابت {
        أرجع is_استفاده_شده_Not_Driven_بیت(0, m_بیت_Flags.اندازه() / FLAGS_در_هر_بیت);
    }
    ثابت Ast_گره* اولی_استفاده_شده_Not_Drivenp() ثابت { أرجع m_استفاده_شده_Not_Drivenp; }
    ساكن منطقي استفادهنشده_تطابق(AstVar* nodep) {
        ثابت string regexp = v3Global.opt.استفادهنشده_Regexp();
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
                    << initStaticp()->warn_بیشتر() << "... Location of variable initialization\n"
                    << initStaticp()->warnContextPrimary() << '\n'
                    << procWritep()->warnOther() << "... Location of variable process write\n"
                    << procWritep()->warn_بیشتر()
                    << "... Perhaps should initialize instead using a reset in this process\n"
                    << procWritep()->warnContextSecondary());
        }
        ثابت Ast_گره* ثابت initp = nodep->hasUserInit() ? initStaticp() : initialp();
        إذا (initp && contAssignp() && !nodep->isClassMember() && !nodep->isFuncLocal()) {
            initp->v3warn(E_CONTASSINIT, "Continuous assignment to variable with initial value: "
                                             << nodep->prettyNameQ() << '\n'
                                             << initp->warn_بیشتر()
                                             << "... Location of variable initialization\n"
                                             << initp->warnContextPrimary() << '\n'
                                             << contAssignp()->warnOther()
                                             << "... Location of continuous assignment\n"
                                             << contAssignp()->warnContextSecondary());
        }
        إذا (nodep->isGenVar()) {  // Genvar
            إذا (!nodep->isIfaceRef() && !nodep->is_استفاده_شده_Param() && !استفادهنشده_تطابق(nodep)) {
                nodep->v3warn(UNUSEDGENVAR, "Genvar is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_کد::UNUSEDGENVAR,
                                                 درست);  // Warn only once
            }
        } وإلا إذا (nodep->isParam()) {  // Parameter
            إذا (!nodep->isIfaceRef() && !nodep->is_استفاده_شده_Param() && !استفادهنشده_تطابق(nodep)) {
                nodep->v3warn(UNUSEDPARAM, "Parameter is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_کد::UNUSEDPARAM,
                                                 درست);  // Warn only once
            }
        } وإلا {  // Signal
            ثابت string varType{nodep->isFuncLocal() ? "Function variable" : "Signal"};
            ثابت منطقي funcInout = nodep->isFuncLocal() && nodep->isInout();
            منطقي allU = درست;
            منطقي allD = درست;
            منطقي anyU = m_کامل_Flags[FLAG_استفاده_شده];
            منطقي anyD = m_کامل_Flags[FLAG_DRIVEN];
            منطقي anyUnotD = خطأ;
            منطقي anyDnotU = خطأ;
            منطقي anynotDU = خطأ;
            لكل (بیعلامت بیت_2 = 0; بیت_2 < m_بیت_Flags.اندازه() / FLAGS_در_هر_بیت; بیت_2++) {
                ثابت منطقي استفاده_شده = استفاده_شده_Flag(بیت_2);
                ثابت منطقي driv = drivenFlag(بیت_2);
                allU &= استفاده_شده;
                anyU |= استفاده_شده;
                allD &= driv;
                anyD |= driv;
                anyUnotD |= استفاده_شده && !driv;
                anyDnotU |= !استفاده_شده && driv;
                anynotDU |= !استفاده_شده && !driv;
            }
            إذا (funcInout) {
                إذا (anyD) allU = درست;
                allD = درست;
            }
            إذا (allU) m_کامل_Flags[FLAG_استفاده_شده] = درست;
            إذا (allD) m_کامل_Flags[FLAG_DRIVEN] = درست;
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
                إذا (!استفادهنشده_تطابق(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not driven, nor used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3_Error_کد::UNUSEDSIGNAL,
                                                     درست);  // Warn only once
                }
            } وإلا إذا (allD && !anyU) {
                إذا (!استفادهنشده_تطابق(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3_Error_کد::UNUSEDSIGNAL,
                                                     درست);  // Warn only once
                }
            } وإلا إذا (!anyD && allU) {
                nodep->v3warn(UNDRIVEN, varType << " is not driven: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_کد::UNDRIVEN, درست);  // Warn only once
            } وإلا إذا (!funcInout) {
                // Bits have different dispositions
                ثابت std::string var_Type_پایینی = [&varType]() {
                    std::string str = varType;
                    str[0] = std::tolower(static_cast<بیعلامت حرف>(str[0]));
                    أرجع str;
                }();
                منطقي مجموعه_U = خطأ;
                منطقي مجموعه_D = خطأ;
                إذا (anynotDU && !استفادهنشده_تطابق(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  "Bits of " << var_Type_پایینی << " are not driven, nor used: "
                                             << nodep->prettyNameQ() << بیت_نامها(BN_BOTH));
                    مجموعه_U = درست;
                }
                إذا (anyDnotU && !استفادهنشده_تطابق(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL, "Bits of " << var_Type_پایینی << " are not used: "
                                                           << nodep->prettyNameQ()
                                                           << بیت_نامها(BN_استفادهنشده));
                    مجموعه_U = درست;
                }
                إذا (anyUnotD) {
                    nodep->v3warn(UNDRIVEN, "Bits of " << var_Type_پایینی << " are not driven: "
                                                       << nodep->prettyNameQ()
                                                       << بیت_نامها(BN_UNDRIVEN));
                    مجموعه_D = درست;
                }
                إذا (مجموعه_U) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3_Error_کد::UNUSEDSIGNAL, درست);
                }
                إذا (مجموعه_D) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3_Error_کد::UNDRIVEN, درست);
                }
            }
        }
    }

    فراغ driven_از_طریق_Call(ثابت Ast_گره_FTask_Ref* nodep) {
        driven_کامل(nodep);
        إذا (!m_callNodep) m_callNodep = nodep;
    }
    ثابت Ast_گره_FTask_Ref* callNodep() ثابت { أرجع m_callNodep; }
};

//######################################################################
// Undriven state, as a visitor of each AstNode

صنف UndrivenVisitor final : عام VNVisitorConst {
    // NODE STATE
    // Netlist:
    //  AstVar::user1p          -> UndrivenVar* for usage var, 0=not set yet
    ثابت VNUser1_درون_Use m_inuser1;
    // Each always:
    //  AstNode::user2p         -> UndrivenVar* for usage var, 0=not set yet
    ثابت VNUser2_درون_Use m_inuser2;

    // STATE
    std::array<std::وکتور<Undriven_Var_مدخل*>, 3> m_entryps = {};  // Nodes to delete when finished
    منطقي m_درون_BBox = خطأ;  // In black box; mark as driven+used
    منطقي m_درون_Cont_تخصیص_بده = خطأ;  // In continuous assignment
    منطقي m_درون_Initial = خطأ;  // In explicit initial block
    منطقي m_درون_Initial_راهاندازی = خطأ;  // In InitialAutomatic*/InitialStatic* assignment LHS
    منطقي m_درون_Initial_Static = خطأ;  // In InitialStatic
    منطقي m_درون_Proc_تخصیص_بده = خطأ;  // In procedural assignment
    منطقي m_درون_FTask_Ref = خطأ;  // In function or task call
    منطقي m_درون_Inout_Or_Ref_Pin = خطأ;  // Connected to pin that is inout
    منطقي m_درون_Sel_Lhs = خطأ;  // Iterating the fromp of an AstSel (a partial-bit write target)
    ثابت Ast_گره_FTask* m_taskp = اشاره_تهی;  // Current task
    ثابت AstAlways* m_alwaysp = اشاره_تهی;  // Current always of either type
    ثابت AstAlways* m_alwaysCombp = اشاره_تهی;  // Current always if combo, otherwise nullptr
    ثابت AstAlways* m_alwaysFFp = اشاره_تهی;  // Current always if ff, otherwise nullptr
    ثابت AstAlways* m_alwaysPlainp = اشاره_تهی;  // Current always if plain (not comb/ff/latch)
    ثابت AstClocking* m_clockingp = اشاره_تهی;  // Current clocking block, otherwise nullptr

    V3UndrivenCapture* ثابت m_capturep = اشاره_تهی;  // Capture object.  'nullptr' if disabled.

    // METHODS

    Undriven_Var_مدخل* getEntryp(AstVar* nodep, صحيح which_user) {
        إذا (!(which_user == 1 ? nodep->user1p() : nodep->user2p())) {
            Undriven_Var_مدخل* ثابت entryp = جديد Undriven_Var_مدخل{nodep};
            // UINFO(9," Associate u="<<which_user<<" "<<cvtToHex(this)<<" "<<nodep->name()<<endl);
            m_entryps[which_user].افزودن_پشت(entryp);
            إذا (which_user == 1) {
                nodep->user1p(entryp);
            } وإلا إذا (which_user == 2) {
                nodep->user2p(entryp);
            } وإلا {
                nodep->v3fatalSrc("Bad case");
            }
            أرجع entryp;
        } وإلا {
            Undriven_Var_مدخل* ثابت entryp = reinterpret_cast<Undriven_Var_مدخل*>(
                which_user == 1 ? nodep->user1p() : nodep->user2p());
            أرجع entryp;
        }
    }

    فراغ warn_Alw_Comb_ترتیب(Ast_گره_Var_Ref* nodep, ثابت Ast_گره* readp) {
        AstVar* ثابت varp = nodep->varp();
        إذا (!varp->isParam() && !varp->isGenVar() && !varp->is_استفاده_شده_Loop_Idx()
            && !varp->ignoreSchedWrite()
            && !m_درون_BBox  // We may have falsely considered a SysIgnore as a driver
            && !VN_IS(nodep, VarXRef)  // Xrefs might point at two different instances
            && !varp->fileline()->warnIsOff(
                V3_Error_کد::ALWCOMBORDER)) {  // Warn only once per variable
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
            varp->fileline()->modifyWarnOff(V3_Error_کد::ALWCOMBORDER,
                                            درست);  // Complain just once for any usage
        }
    }
    // A clocking block 'output' is an additional driver of the signal it names.
    // The conflict is found from whichever driver is reached second.
    منطقي clockingDrivesOther(ثابت Undriven_Var_مدخل* entryp, منطقي otherWriteIsStaticInit) ثابت {
        ثابت منطقي otherIsExplicitProc
            = entryp->is_Driven_Always_Comb_کامل() || entryp->isDrivenAlwaysFFWhole();
        أرجع m_clockingp && !otherIsExplicitProc && !otherWriteIsStaticInit
               && m_clockingp != entryp->getClockingp();
    }
    منطقي otherDrivesClocking(ثابت Undriven_Var_مدخل* entryp) ثابت {
        أرجع !m_clockingp && !m_alwaysCombp && !m_alwaysFFp && !m_درون_Initial_Static
               && entryp->is_Driven_Clocking_کامل();
    }
    // A continuous assignment contending with a clocking block 'output' is an
    // unambiguous driver conflict. Multiple clocking blocks may drive the same
    // signal, the last drive wins (IEEE 1800-2023 14.16.2).
    فراغ warnClockingDriven(Ast_گره_Var_Ref* nodep, ثابت Undriven_Var_مدخل* entryp,
                            ثابت Ast_گره* otherWritep, منطقي otherWriteIsStaticInit) {
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
        إذا (otherDrivesClocking(entryp) && m_درون_Cont_تخصیص_بده) {
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
    فراغ warnClockingDrivenProc(Ast_گره_Var_Ref* nodep, ثابت Undriven_Var_مدخل* entryp,
                                ثابت Ast_گره* otherWritep, منطقي otherWriteIsStaticInit) {
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
        إذا (otherDrivesClocking(entryp) && !m_درون_Cont_تخصیص_بده) {
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
    فراغ بازدید(AstVar* nodep) override {
        ثابت منطقي funcInout = nodep->isFuncLocal() && nodep->isInout();
        لكل (صحيح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            // For assigns and non-combo always, do just usr==1, to look
            // for module-wide undriven etc.
            // For combo always, run both usr==1 for above, and also
            // usr==2 for always-only checks.
            Undriven_Var_مدخل* ثابت entryp = getEntryp(nodep, usr);
            إذا ((nodep->isNonOutput() && !funcInout) || nodep->is_Sig_عمومی()
                || nodep->hasUserInit() || nodep->isSigUserRWPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->driven_کامل(nodep);
            }
            إذا ((nodep->isWritable() && !funcInout) || nodep->is_Sig_عمومی()
                || nodep->isSigUserRWPublic() || nodep->is_Sig_User_Rd_عمومی()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->استفاده_شده_کامل(nodep);
            }
            إذا (nodep->valuep()) entryp->driven_کامل(nodep->valuep());
        }
        // Discover variables used in bit definitions, etc
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstArraySel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_برش_Sel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstSel* nodep) override {
        Ast_گره_Var_Ref* ثابت varrefp = VN_CAST(nodep->fromp(), گره_Var_Ref);
        AstConst* ثابت constp = VN_CAST(nodep->lsbp(), Const);
        إذا (varrefp && constp && !constp->رقم().isFourState()) {
            لكل (صحيح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                Undriven_Var_مدخل* ثابت entryp = getEntryp(varrefp->varp(), usr);
                ثابت صحيح lsb = constp->مقصد_UInt();
                إذا (m_درون_BBox || varrefp->access().isWriteOrRW()) {
                    // Don't warn if already driven earlier as "a=0; if(a) a=1;" is fine.
                    إذا (usr == 2 && m_alwaysCombp
                        && entryp->is_استفاده_شده_Not_Driven_بیت(lsb, nodep->عرض())) {
                        UINFO(9, " Select.  Entryp=" << cvt_مقصد_Hex(entryp));
                        warn_Alw_Comb_ترتیب(varrefp, entryp->اولی_استفاده_شده_Not_Drivenp());
                    }
                    entryp->driven_بیت(lsb, nodep->عرض());
                }
                إذا (m_درون_BBox || !varrefp->access().isWriteOrRW())
                    entryp->استفاده_شده_بیت_2(lsb, nodep->عرض(), varrefp);
            }
        } وإلا {
            // skip over static longest static prefix
            iterateConst(nodep->lsbp());
            VL_RESTORER(m_درون_Sel_Lhs);
            m_درون_Sel_Lhs = !V3_عرض::selectNonConstantRecurse(nodep->lsbp(), /*inSel=*/درست);
            iterateConst(nodep->fromp());
        }
    }
    فراغ بازدید(Ast_گره_Var_Ref* nodep) override {
        // Any variable
        إذا (nodep->access().isWriteOrRW()
            && !VN_IS(nodep, VarXRef)) {  // Ignore interface variables and similar ugly items
            إذا (m_درون_Proc_تخصیص_بده && !nodep->varp()->varType().isProcAssignable()
                && !nodep->varp()->isDeclTyped()  //
                && !nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()) {
                nodep->v3warn(PROCASSWIRE, "Procedural assignment to wire, perhaps intended var"
                                               << " (IEEE 1800-2023 6.5): "
                                               << nodep->prettyNameQ());
            } وإلا إذا (m_درون_Cont_تخصیص_بده && !nodep->varp()->varType().isContAssignable()
                       && !nodep->fileline()->language().systemVerilog()) {
                nodep->v3warn(CONTASSREG,
                              "Continuous assignment to reg, perhaps intended wire"
                                  << " (IEEE 1364-2005 6.1; Verilog only, legal in SV): "
                                  << nodep->prettyNameQ());
            }
            إذا (m_درون_FTask_Ref && nodep->varp()->varType().isNet()) {
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
        إذا (m_taskp && !m_alwaysp && !m_درون_Cont_تخصیص_بده && !m_درون_Initial_Static && !m_درون_BBox
            && !m_taskp->dpiExport()) {
            AstVar* ثابت retVarp = VN_CAST(m_taskp->fvarp(), Var);
            إذا (!retVarp || nodep->varp() != retVarp) ftaskDef = درست;
        }

        لكل (صحيح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            Undriven_Var_مدخل* ثابت entryp = getEntryp(nodep->varp(), usr);
            ثابت منطقي fdrv = nodep->access().isWriteOrRW()
                              && nodep->varp()->attrFileDescr();  // FD's are also being read from
            إذا (m_درون_BBox || nodep->access().isWriteOrRW()) {
                إذا (usr == 2 && m_alwaysCombp && entryp->is_استفاده_شده_Not_Driven_Any()) {
                    UINFO(9, " Full bus.  Entryp=" << cvt_مقصد_Hex(entryp));
                    warn_Alw_Comb_ترتیب(nodep, entryp->اولی_استفاده_شده_Not_Drivenp());
                }
                ثابت Ast_گره_Var_Ref* ثابت otherVarRefp = entryp->getNodep();
                ثابت Ast_گره* ثابت otherWritep = otherVarRefp
                                                       ? static_cast<ثابت Ast_گره*>(otherVarRefp)
                                                       : entryp->callNodep();
                ثابت منطقي same_File_سطر
                    = otherVarRefp && nodep->fileline() == otherVarRefp->fileline();
                // Preconditions shared by MULTIDRIVEN and MULTIDRIVENPROC.
                ثابت منطقي multidrivenCommon
                    = entryp->is_Driven_کامل() && !m_درون_BBox && !VN_IS(nodep, VarXRef)
                      && !VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType) && !same_File_سطر
                      && !entryp->isUnderGen() && otherWritep && !entryp->isFtaskDriven()
                      && !ftaskDef && !m_درون_Sel_Lhs;
                // The two warnings are gated independently on the variable
                // declaration's fileline, as v3warn suppression will check
                // the driving fileline and still warn even if the warning
                // was suppressed with lint_off at the declaration.
                ثابت منطقي otherWriteIsStaticInit
                    = nodep->varp()->hasUserInit() && otherWritep == entryp->initStaticp();
                إذا (multidrivenCommon
                    && !nodep->varp()->fileline()->warnIsOff(V3_Error_کد::MULTIDRIVEN)) {
                    إذا (m_alwaysCombp
                        && (!entryp->is_Driven_Always_Comb_کامل()
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
                    إذا (!m_alwaysCombp && entryp->is_Driven_Always_Comb_کامل()) {
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
                    إذا (!m_alwaysFFp && !m_درون_Initial_Static && entryp->isDrivenAlwaysFFWhole()) {
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
                إذا (multidrivenCommon && !nodep->varp()->is_استفاده_شده_Loop_Idx()
                    && !nodep->varp()->fileline()->warnIsOff(V3_Error_کد::MULTIDRIVENPROC)) {
                    // Two plain always blocks driving the whole signal: legal
                    // SystemVerilog, but a driver conflict for synthesis. The
                    // always_ff/always_comb cases above already cover mixes with
                    // an explicit process, so only warn for plain+plain here.
                    // When the two blocks are clocked differently, the
                    // (on-by-default) MULTIDRIVEN check in V3Delayed already reports
                    // the conflict, so don't also emit MULTIDRIVENPROC for that case.
                    ثابت AstAlways* ثابت otherAlwaysp = entryp->getAlwPlainp();
                    ثابت Ast_Sen_درخت* ثابت senp
                        = m_alwaysPlainp ? m_alwaysPlainp->sentreep() : اشاره_تهی;
                    ثابت Ast_Sen_درخت* ثابت otherSenp
                        = otherAlwaysp ? otherAlwaysp->sentreep() : اشاره_تهی;
                    ثابت منطقي differentClocking = senp && otherSenp && senp->hasClocked()
                                                   && otherSenp->hasClocked()
                                                   && !senp->same_درخت(otherSenp);
                    إذا (m_alwaysPlainp && entryp->is_Driven_Always_Plain_کامل()
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
                إذا (!m_درون_Initial_راهاندازی || nodep->varp()->hasUserInit()) {
                    // Else don't count default initialization as a driver to a net/variable
                    entryp->driven_کامل(nodep, ftaskDef);
                }
                إذا (m_alwaysCombp && entryp->is_Driven_Always_Comb_کامل()
                    && m_alwaysCombp != entryp->getAlwCombp()
                    && m_alwaysCombp->fileline() == entryp->getAlwCombp()->fileline())
                    entryp->underGenerate();
                إذا (m_alwaysCombp) entryp->driven_Always_Comb_کامل(m_alwaysCombp);
                إذا (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, nodep->varp());
                إذا (m_alwaysPlainp) entryp->driven_Always_Plain_کامل(m_alwaysPlainp);
                إذا (m_clockingp) entryp->driven_Clocking_کامل(m_clockingp);
            }
            إذا (nodep->access().isWriteOrRW() && !VN_IS(nodep, VarXRef)) {
                // Ignoring xrefs as the initial and assignment to track might refer to two
                // different instances.  Ideally all of V3Undriven would move after V3Scope,
                // then could use VarScope tracking instead.
                إذا (m_درون_Initial_Static && !entryp->initStaticp()) entryp->initStaticp(nodep);
                إذا (m_درون_Initial && !entryp->initialp()) entryp->initialp(nodep);
                إذا (m_درون_Cont_تخصیص_بده && !entryp->contAssignp()) entryp->contAssignp(nodep);
                إذا (m_alwaysp && m_درون_Proc_تخصیص_بده && !entryp->procWritep())
                    entryp->procWritep(nodep);
            }
            إذا ((!m_درون_Initial_راهاندازی || nodep->varp()->hasUserInit())
                && (m_درون_BBox || nodep->access().isReadOrRW()
                    || fdrv
                    // Inouts have only isWrite set, as we don't have more
                    // information and operating on module boundary, treat as
                    // both read and writing
                    || m_درون_Inout_Or_Ref_Pin))
                entryp->استفاده_شده_کامل(nodep);
        }
    }

    // Don't know what black boxed calls do, assume in+out
    فراغ بازدید(AstSysIgnore* nodep) override {
        VL_RESTORER(m_درون_BBox);
        m_درون_BBox = درست;
        iterateChildrenConst(nodep);
    }

    فراغ بازدید(Ast_تخصیص_بده* nodep) override {
        VL_RESTORER(m_درون_Proc_تخصیص_بده);
        m_درون_Proc_تخصیص_بده = درست;
        {
            VL_RESTORER(m_درون_Initial_راهاندازی);
            m_درون_Initial_راهاندازی = خطأ;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }
    فراغ بازدید(Ast_تخصیص_بده_Dly* nodep) override {
        VL_RESTORER(m_درون_Proc_تخصیص_بده);
        m_درون_Proc_تخصیص_بده = درست;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_تخصیص_بده_Force* nodep) override {
        iterateConst(nodep->rhsp());
        VL_RESTORER(m_درون_Initial);
        m_درون_Initial = خطأ;
        iterateConst(nodep->lhsp());
    }
    فراغ بازدید(Ast_تخصیص_بده_W* nodep) override {
        VL_RESTORER(m_درون_Cont_تخصیص_بده);
        m_درون_Cont_تخصیص_بده = درست;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstRelease* nodep) override {
        VL_RESTORER(m_درون_Initial);
        m_درون_Initial = خطأ;
        iterateConst(nodep->lhsp());
    }
    فراغ بازدید(AstInitial* nodep) override {
        VL_RESTORER(m_درون_Initial);
        m_درون_Initial = درست;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstInitialAutomatic* nodep) override {
        VL_RESTORER(m_درون_Initial_راهاندازی);
        m_درون_Initial_راهاندازی = درست;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_درون_Initial_راهاندازی);
        m_درون_Initial_راهاندازی = درست;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstInitialStatic* nodep) override {
        VL_RESTORER(m_درون_Initial_Static);
        m_درون_Initial_Static = درست;
        VL_RESTORER(m_درون_Initial_راهاندازی);
        m_درون_Initial_راهاندازی = درست;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_درون_Initial_راهاندازی);
        m_درون_Initial_راهاندازی = درست;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        VL_RESTORER(m_alwaysCombp);
        VL_RESTORER(m_alwaysFFp);
        VL_RESTORER(m_alwaysPlainp);
        Ast_گره::user2_Clear_درخت();
        m_alwaysp = nodep;
        إذا (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            UINFO(9, "   " << nodep);
            m_alwaysCombp = nodep;
        } وإلا {
            m_alwaysCombp = اشاره_تهی;
        }
        m_alwaysFFp = nodep->keyword() == VAlwaysKwd::ALWAYS_FF ? nodep : اشاره_تهی;
        m_alwaysPlainp = nodep->keyword() == VAlwaysKwd::ALWAYS ? nodep : اشاره_تهی;
        iterateChildrenConst(nodep);
        إذا (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) UINFO(9, "   Done " << nodep);
    }
    فراغ بازدید(AstClocking* nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterateChildrenConst(nodep);
    }

    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        VL_RESTORER(m_درون_FTask_Ref);
        m_درون_FTask_Ref = درست;

        iterateChildrenConst(nodep);

        إذا (!m_capturep) أرجع;

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Do not apply writeSummary at calls inside a task definition, or they will look like
        // independent drivers (phantom MULTIDRIVEN).
        ثابت منطقي درون_Executed_Context
            = !(m_taskp && !m_alwaysp && !m_درون_Cont_تخصیص_بده && !m_درون_Initial_Static && !m_درون_BBox
                && !m_taskp->dpiExport());

        إذا (!درون_Executed_Context) أرجع;

        Ast_گره_FTask* ثابت calleep = nodep->taskp();
        إذا (!calleep) أرجع;

        ثابت خودکار& vars = m_capturep->writeSummary(calleep);
        لكل (AstVar* ثابت varp : vars) {
            لكل (صحيح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                Undriven_Var_مدخل* ثابت entryp = getEntryp(varp, usr);
                entryp->driven_از_طریق_Call(nodep);
                إذا (m_alwaysCombp) entryp->driven_Always_Comb_کامل(m_alwaysCombp);
                إذا (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, varp);
            }
        }
    }

    فراغ بازدید(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_taskp);
        m_taskp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstPin* nodep) override {
        VL_RESTORER(m_درون_Inout_Or_Ref_Pin);
        m_درون_Inout_Or_Ref_Pin = nodep->modVarp()->isInoutOrRef();
        iterateChildrenConst(nodep);
    }

    // Until we support tables, primitives will have undriven and unused I/Os
    فراغ بازدید(AstPrimitive*) override {}

    // Coverage artifacts etc shouldn't count as a sink
    فراغ بازدید(Ast_گره_Cover_Decl*) override {}
    فراغ بازدید(AstCoverInc*) override {}
    فراغ بازدید(AstCoverToggle*) override {}
    فراغ بازدید(AstTraceDecl* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    فراغ بازدید(AstTraceInc* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }

    // iterate
    فراغ بازدید(AstConst* nodep) override {}
    فراغ بازدید(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    صریح UndrivenVisitor(AstNetlist* nodep, V3UndrivenCapture* capturep)
        : m_capturep{capturep} {
        iterateConst(nodep);
    }

    ~UndrivenVisitor() override {
        لكل (Undriven_Var_مدخل* ip : m_entryps[1]) ip->reportViolations();
        لكل (صحيح usr = 1; usr < 3; ++usr) {
            لكل (Undriven_Var_مدخل* ip : m_entryps[usr]) حذف ip;
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
