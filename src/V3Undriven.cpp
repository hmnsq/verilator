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

صنف Undriven_Var_اندراج final {
    // MEMBERS
    AstVar* ثابت m_varp;  // Variable this tracks
    std::سمتیہ<منطقي> m_سارا_Flags;  // Used/Driven on whole vector
    std::سمتیہ<منطقي> m_ثنائی_ہندسہ_Flags;  // Used/Driven on each subbit
    ثابت Ast_گرہ* m_ورتیا_Not_Drivenp = خالی_سوچک;  // First read before any write
    ثابت AstAlways* m_alwCombp
        = خالی_سوچک;  // always_comb of var if driven within always_comb, else nullptr
    ثابت AstAlways* m_alwFFp = خالی_سوچک;  // always_ff of var if driven within always_ff
    ثابت AstAlways* m_alwPlainp = خالی_سوچک;  // plain always of var if driven within plain always
    ثابت AstClocking* m_clockingp = خالی_سوچک;  // clocking block of var if driven as output
    ثابت Ast_گرہ_Var_Ref* m_nodep = خالی_سوچک;  // varref if driven, else nullptr
    ثابت Ast_گرہ* m_initStaticp = خالی_سوچک;  // varref if in InitialStatic driven
    ثابت Ast_گرہ* m_initialp = خالی_سوچک;  // varref if driven in an explicit initial block
    ثابت Ast_گرہ* m_contAssignp = خالی_سوچک;  // varref if in continuous assignment driven
    ثابت Ast_گرہ* m_procWritep = خالی_سوچک;  // varref if written in process
    منطقي m_underGen = خطأ;  // Under a generate
    منطقي m_ftaskDriven = خطأ;  // Last driven by function or task

    ثابت Ast_گرہ_FTask_Ref* m_callNodep = خالی_سوچک;  // Call node if driven via writeSummary

    تعداد : uint8_t {
        FLAG_ورتیا = 0,  // Signal or bit has been read/observed
        FLAG_DRIVEN = 1,  // Signal or bit has been written/driven
        FLAG_DRIVEN_ALWCOMB = 2,  // Whole signal has been driven from always_comb
        FLAG_DRIVEN_ALWFF = 3,  // Whole signal has been driven from always_ff
        FLAG_DRIVEN_ALWPLAIN = 4,  // Whole signal has been driven from a plain always
        FLAG_DRIVEN_CLOCKING = 5,  // Whole signal has been driven as a clocking block output
        FLAGS_فی_ثنائی_ہندسہ = 6  // Number of flags stored for each tracked bit
    };

عام:
    // CONSTRUCTORS
    واضح Undriven_Var_اندراج(AstVar* varp)
        : m_varp{varp} {  // Construction for when a var is used
        UINFO(9, "create " << varp);
        m_سارا_Flags.resize(FLAGS_فی_ثنائی_ہندسہ);
        لكل (صحيح i = 0; i < FLAGS_فی_ثنائی_ہندسہ; i++) m_سارا_Flags[i] = خطأ;
        m_ثنائی_ہندسہ_Flags.resize(varp->چوڑائی() * FLAGS_فی_ثنائی_ہندسہ);
        لكل (صحيح i = 0; i < varp->چوڑائی() * FLAGS_فی_ثنائی_ہندسہ; i++) m_ثنائی_ہندسہ_Flags[i] = خطأ;
    }
    ~Undriven_Var_اندراج() = افتراضي;

خاص:
    // METHODS
    منطقي ثنائی_ہندسہ_عدد_Ok(صحيح ثنائی_ہندسہ_2) ثابت {
        أرجع ثنائی_ہندسہ_2 >= 0 && (ثنائی_ہندسہ_2 * FLAGS_فی_ثنائی_ہندسہ < static_cast<صحيح>(m_ثنائی_ہندسہ_Flags.ناپ()));
    }
    منطقي ورتیا_Flag(صحيح ثنائی_ہندسہ_2) ثابت {
        أرجع m_سارا_Flags[FLAG_ورتیا] || m_ثنائی_ہندسہ_Flags[ثنائی_ہندسہ_2 * FLAGS_فی_ثنائی_ہندسہ + FLAG_ورتیا];
    }
    منطقي drivenFlag(صحيح ثنائی_ہندسہ_2) ثابت {
        أرجع m_سارا_Flags[FLAG_DRIVEN] || m_ثنائی_ہندسہ_Flags[ثنائی_ہندسہ_2 * FLAGS_فی_ثنائی_ہندسہ + FLAG_DRIVEN];
    }
    صحيح ثنائی_ہندسہ_گنتی() ثابت { أرجع m_ثنائی_ہندسہ_Flags.ناپ() / FLAGS_فی_ثنائی_ہندسہ; }
    فراغ ریکارڈ_ورتیا_Not_Driven(ثابت Ast_گرہ* nodep) {
        إذا (!m_ورتیا_Not_Drivenp) m_ورتیا_Not_Drivenp = nodep;
    }
    تعداد ثنائی_ہندسہ_ناں_Which : uint8_t { BN_بن_ورتیا, BN_UNDRIVEN, BN_BOTH };
    string ثنائی_ہندسہ_ناں(ثنائی_ہندسہ_ناں_Which which) {
        string بٹ;
        منطقي prev = خطأ;
        صحيح msb = 0;
        // bit==-1 loops below; we do one extra iteration so end with prev=false
        لكل (صحيح ثنائی_ہندسہ_2 = (m_ثنائی_ہندسہ_Flags.ناپ() / FLAGS_فی_ثنائی_ہندسہ) - 1; ثنائی_ہندسہ_2 >= -1; --ثنائی_ہندسہ_2) {
            إذا (ثنائی_ہندسہ_2 >= 0
                && ((which == BN_بن_ورتیا && !ورتیا_Flag(ثنائی_ہندسہ_2) && drivenFlag(ثنائی_ہندسہ_2))
                    || (which == BN_UNDRIVEN && ورتیا_Flag(ثنائی_ہندسہ_2) && !drivenFlag(ثنائی_ہندسہ_2))
                    || (which == BN_BOTH && !ورتیا_Flag(ثنائی_ہندسہ_2) && !drivenFlag(ثنائی_ہندسہ_2)))) {
                إذا (!prev) {
                    prev = سچ;
                    msb = ثنائی_ہندسہ_2;
                }
            } وإلا إذا (prev) {
                ثابت AstBasicDType* ثابت bdtypep = m_varp->basicp();
                ثابت صحيح lsb = ثنائی_ہندسہ_2 + 1;
                إذا (بٹ != "") بٹ += ",";
                إذا (lsb == msb) {
                    بٹ += cvt_منزل_Str(lsb + bdtypep->lo());
                } وإلا {
                    إذا (bdtypep->ascending()) {
                        بٹ
                            += cvt_منزل_Str(lsb + bdtypep->lo()) + ":" + cvt_منزل_Str(msb + bdtypep->lo());
                    } وإلا {
                        بٹ
                            += cvt_منزل_Str(msb + bdtypep->lo()) + ":" + cvt_منزل_Str(lsb + bdtypep->lo());
                    }
                }
                prev = خطأ;
            }
        }
        أرجع "[" + بٹ + "]";
    }

عام:
    فراغ ورتیا_سارا(ثابت Ast_گرہ* nodep) {
        UINFO(9, "set u[*] " << m_varp->name() << " " << nodep);
        إذا (!m_سارا_Flags[FLAG_DRIVEN]) {
            لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < ثنائی_ہندسہ_گنتی(); ++ثنائی_ہندسہ_2) {
                إذا (!drivenFlag(ثنائی_ہندسہ_2)) {
                    ریکارڈ_ورتیا_Not_Driven(nodep);
                    اكسر;
                }
            }
        }
        m_سارا_Flags[FLAG_ورتیا] = سچ;
    }
    فراغ driven_سارا(ثابت Ast_گرہ* nodep) {
        UINFO(9, "set d[*] " << m_varp->name() << " " << nodep);
        m_سارا_Flags[FLAG_DRIVEN] = سچ;
    }
    فراغ driven_سارا(ثابت Ast_گرہ_Var_Ref* nodep, منطقي ftaskDef) {
        ثابت منطقي wasDriven = is_Driven_سارا();
        driven_سارا(nodep);
        // A write in a task/function definition is not an executed process, so it must not
        // displace an already recorded write, nor clear the flag that keeps such a write
        // from being reported as the other write of a MULTIDRIVEN
        إذا (ftaskDef && wasDriven) أرجع;
        m_ftaskDriven = ftaskDef;
        m_nodep = nodep;
    }
    فراغ driven_Always_Comb_سارا(ثابت AstAlways* alwCombp) {
        m_سارا_Flags[FLAG_DRIVEN_ALWCOMB] = سچ;
        m_alwCombp = alwCombp;
    }
    فراغ drivenAlwaysFFWhole(ثابت AstAlways* alwFFp, ثابت AstVar* varp) {
        m_سارا_Flags[FLAG_DRIVEN_ALWFF] = سچ;
        m_alwFFp = alwFFp;
    }
    فراغ driven_Always_Plain_سارا(ثابت AstAlways* alwPlainp) {
        m_سارا_Flags[FLAG_DRIVEN_ALWPLAIN] = سچ;
        m_alwPlainp = alwPlainp;
    }
    فراغ driven_Clocking_سارا(ثابت AstClocking* clockingp) {
        m_سارا_Flags[FLAG_DRIVEN_CLOCKING] = سچ;
        m_clockingp = clockingp;
    }

    ثابت Ast_گرہ* initStaticp() ثابت { أرجع m_initStaticp; }
    فراغ initStaticp(ثابت Ast_گرہ* nodep) { m_initStaticp = nodep; }
    ثابت Ast_گرہ* initialp() ثابت { أرجع m_initialp; }
    فراغ initialp(ثابت Ast_گرہ* nodep) { m_initialp = nodep; }
    ثابت Ast_گرہ* contAssignp() ثابت { أرجع m_contAssignp; }
    فراغ contAssignp(ثابت Ast_گرہ* nodep) { m_contAssignp = nodep; }
    ثابت Ast_گرہ* procWritep() ثابت { أرجع m_procWritep; }
    فراغ procWritep(ثابت Ast_گرہ* nodep) { m_procWritep = nodep; }
    فراغ underGenerate() { m_underGen = سچ; }
    منطقي isUnderGen() ثابت { أرجع m_underGen; }
    منطقي is_Driven_سارا() ثابت { أرجع m_سارا_Flags[FLAG_DRIVEN]; }
    منطقي is_Driven_Always_Comb_سارا() ثابت { أرجع m_سارا_Flags[FLAG_DRIVEN_ALWCOMB]; }
    منطقي isDrivenAlwaysFFWhole() ثابت { أرجع m_سارا_Flags[FLAG_DRIVEN_ALWFF]; }
    منطقي is_Driven_Always_Plain_سارا() ثابت { أرجع m_سارا_Flags[FLAG_DRIVEN_ALWPLAIN]; }
    منطقي is_Driven_Clocking_سارا() ثابت { أرجع m_سارا_Flags[FLAG_DRIVEN_CLOCKING]; }
    منطقي isFtaskDriven() ثابت { أرجع m_ftaskDriven; }
    ثابت Ast_گرہ_Var_Ref* getNodep() ثابت { أرجع m_nodep; }
    ثابت AstAlways* getAlwCombp() ثابت { أرجع m_alwCombp; }
    ثابت AstAlways* getAlwFFp() ثابت { أرجع m_alwFFp; }
    ثابت AstAlways* getAlwPlainp() ثابت { أرجع m_alwPlainp; }
    ثابت AstClocking* getClockingp() ثابت { أرجع m_clockingp; }
    فراغ ورتیا_ثنائی_ہندسہ_2(صحيح ثنائی_ہندسہ_2, صحيح چوڑائی, ثابت Ast_گرہ* nodep) {
        UINFO(9, "set u[" << (ثنائی_ہندسہ_2 + چوڑائی - 1) << ":" << ثنائی_ہندسہ_2 << "] " << m_varp->name());
        لكل (صحيح i = 0; i < چوڑائی; i++) {
            إذا (ثنائی_ہندسہ_عدد_Ok(ثنائی_ہندسہ_2 + i)) {
                إذا (!drivenFlag(ثنائی_ہندسہ_2 + i)) ریکارڈ_ورتیا_Not_Driven(nodep);
                m_ثنائی_ہندسہ_Flags[(ثنائی_ہندسہ_2 + i) * FLAGS_فی_ثنائی_ہندسہ + FLAG_ورتیا] = سچ;
            }
        }
    }
    فراغ driven_ثنائی_ہندسہ(صحيح ثنائی_ہندسہ_2, صحيح چوڑائی) {
        UINFO(9, "set d[" << (ثنائی_ہندسہ_2 + چوڑائی - 1) << ":" << ثنائی_ہندسہ_2 << "] " << m_varp->name());
        لكل (صحيح i = 0; i < چوڑائی; i++) {
            إذا (ثنائی_ہندسہ_عدد_Ok(ثنائی_ہندسہ_2 + i)) m_ثنائی_ہندسہ_Flags[(ثنائی_ہندسہ_2 + i) * FLAGS_فی_ثنائی_ہندسہ + FLAG_DRIVEN] = سچ;
        }
    }
    منطقي is_ورتیا_Not_Driven_ثنائی_ہندسہ(صحيح ثنائی_ہندسہ_2, صحيح چوڑائی) ثابت {
        لكل (صحيح i = 0; i < چوڑائی; i++) {
            إذا (ثنائی_ہندسہ_عدد_Ok(ثنائی_ہندسہ_2 + i)
                && (m_سارا_Flags[FLAG_ورتیا] || m_ثنائی_ہندسہ_Flags[(ثنائی_ہندسہ_2 + i) * FLAGS_فی_ثنائی_ہندسہ + FLAG_ورتیا])
                && !(m_سارا_Flags[FLAG_DRIVEN]
                     || m_ثنائی_ہندسہ_Flags[(ثنائی_ہندسہ_2 + i) * FLAGS_فی_ثنائی_ہندسہ + FLAG_DRIVEN]))
                أرجع سچ;
        }
        أرجع خطأ;
    }
    منطقي is_ورتیا_Not_Driven_Any() ثابت {
        أرجع is_ورتیا_Not_Driven_ثنائی_ہندسہ(0, m_ثنائی_ہندسہ_Flags.ناپ() / FLAGS_فی_ثنائی_ہندسہ);
    }
    ثابت Ast_گرہ* پہلا_ورتیا_Not_Drivenp() ثابت { أرجع m_ورتیا_Not_Drivenp; }
    ساكن منطقي بن_ورتیا_میل(AstVar* nodep) {
        ثابت string regexp = v3Global.opt.بن_ورتیا_Regexp();
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
                    << initStaticp()->warn_ہور() << "... Location of variable initialization\n"
                    << initStaticp()->warnContextPrimary() << '\n'
                    << procWritep()->warnOther() << "... Location of variable process write\n"
                    << procWritep()->warn_ہور()
                    << "... Perhaps should initialize instead using a reset in this process\n"
                    << procWritep()->warnContextSecondary());
        }
        ثابت Ast_گرہ* ثابت initp = nodep->hasUserInit() ? initStaticp() : initialp();
        إذا (initp && contAssignp() && !nodep->isClassMember() && !nodep->isFuncLocal()) {
            initp->v3warn(E_CONTASSINIT, "Continuous assignment to variable with initial value: "
                                             << nodep->prettyNameQ() << '\n'
                                             << initp->warn_ہور()
                                             << "... Location of variable initialization\n"
                                             << initp->warnContextPrimary() << '\n'
                                             << contAssignp()->warnOther()
                                             << "... Location of continuous assignment\n"
                                             << contAssignp()->warnContextSecondary());
        }
        إذا (nodep->isGenVar()) {  // Genvar
            إذا (!nodep->isIfaceRef() && !nodep->is_ورتیا_Param() && !بن_ورتیا_میل(nodep)) {
                nodep->v3warn(UNUSEDGENVAR, "Genvar is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_کوڈ::UNUSEDGENVAR,
                                                 سچ);  // Warn only once
            }
        } وإلا إذا (nodep->isParam()) {  // Parameter
            إذا (!nodep->isIfaceRef() && !nodep->is_ورتیا_Param() && !بن_ورتیا_میل(nodep)) {
                nodep->v3warn(UNUSEDPARAM, "Parameter is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_کوڈ::UNUSEDPARAM,
                                                 سچ);  // Warn only once
            }
        } وإلا {  // Signal
            ثابت string varType{nodep->isFuncLocal() ? "Function variable" : "Signal"};
            ثابت منطقي funcInout = nodep->isFuncLocal() && nodep->isInout();
            منطقي allU = سچ;
            منطقي allD = سچ;
            منطقي anyU = m_سارا_Flags[FLAG_ورتیا];
            منطقي anyD = m_سارا_Flags[FLAG_DRIVEN];
            منطقي anyUnotD = خطأ;
            منطقي anyDnotU = خطأ;
            منطقي anynotDU = خطأ;
            لكل (بنانشانی ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < m_ثنائی_ہندسہ_Flags.ناپ() / FLAGS_فی_ثنائی_ہندسہ; ثنائی_ہندسہ_2++) {
                ثابت منطقي ورتیا = ورتیا_Flag(ثنائی_ہندسہ_2);
                ثابت منطقي driv = drivenFlag(ثنائی_ہندسہ_2);
                allU &= ورتیا;
                anyU |= ورتیا;
                allD &= driv;
                anyD |= driv;
                anyUnotD |= ورتیا && !driv;
                anyDnotU |= !ورتیا && driv;
                anynotDU |= !ورتیا && !driv;
            }
            إذا (funcInout) {
                إذا (anyD) allU = سچ;
                allD = سچ;
            }
            إذا (allU) m_سارا_Flags[FLAG_ورتیا] = سچ;
            إذا (allD) m_سارا_Flags[FLAG_DRIVEN] = سچ;
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
                إذا (!بن_ورتیا_میل(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not driven, nor used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3_Error_کوڈ::UNUSEDSIGNAL,
                                                     سچ);  // Warn only once
                }
            } وإلا إذا (allD && !anyU) {
                إذا (!بن_ورتیا_میل(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3_Error_کوڈ::UNUSEDSIGNAL,
                                                     سچ);  // Warn only once
                }
            } وإلا إذا (!anyD && allU) {
                nodep->v3warn(UNDRIVEN, varType << " is not driven: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_کوڈ::UNDRIVEN, سچ);  // Warn only once
            } وإلا إذا (!funcInout) {
                // Bits have different dispositions
                ثابت std::string var_Type_ہیٹھلا = [&varType]() {
                    std::string str = varType;
                    str[0] = std::tolower(static_cast<بنانشانی حرف>(str[0]));
                    أرجع str;
                }();
                منطقي سیٹ_U = خطأ;
                منطقي سیٹ_D = خطأ;
                إذا (anynotDU && !بن_ورتیا_میل(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  "Bits of " << var_Type_ہیٹھلا << " are not driven, nor used: "
                                             << nodep->prettyNameQ() << ثنائی_ہندسہ_ناں(BN_BOTH));
                    سیٹ_U = سچ;
                }
                إذا (anyDnotU && !بن_ورتیا_میل(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL, "Bits of " << var_Type_ہیٹھلا << " are not used: "
                                                           << nodep->prettyNameQ()
                                                           << ثنائی_ہندسہ_ناں(BN_بن_ورتیا));
                    سیٹ_U = سچ;
                }
                إذا (anyUnotD) {
                    nodep->v3warn(UNDRIVEN, "Bits of " << var_Type_ہیٹھلا << " are not driven: "
                                                       << nodep->prettyNameQ()
                                                       << ثنائی_ہندسہ_ناں(BN_UNDRIVEN));
                    سیٹ_D = سچ;
                }
                إذا (سیٹ_U) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3_Error_کوڈ::UNUSEDSIGNAL, سچ);
                }
                إذا (سیٹ_D) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3_Error_کوڈ::UNDRIVEN, سچ);
                }
            }
        }
    }

    فراغ driven_راہیں_Call(ثابت Ast_گرہ_FTask_Ref* nodep) {
        driven_سارا(nodep);
        إذا (!m_callNodep) m_callNodep = nodep;
    }
    ثابت Ast_گرہ_FTask_Ref* callNodep() ثابت { أرجع m_callNodep; }
};

//######################################################################
// Undriven state, as a visitor of each AstNode

صنف UndrivenVisitor final : عام VNVisitorConst {
    // NODE STATE
    // Netlist:
    //  AstVar::user1p          -> UndrivenVar* for usage var, 0=not set yet
    ثابت VNUser1_وچ_Use m_inuser1;
    // Each always:
    //  AstNode::user2p         -> UndrivenVar* for usage var, 0=not set yet
    ثابت VNUser2_وچ_Use m_inuser2;

    // STATE
    std::array<std::سمتیہ<Undriven_Var_اندراج*>, 3> m_entryps = {};  // Nodes to delete when finished
    منطقي m_وچ_BBox = خطأ;  // In black box; mark as driven+used
    منطقي m_وچ_Cont_سونپو = خطأ;  // In continuous assignment
    منطقي m_وچ_Initial = خطأ;  // In explicit initial block
    منطقي m_وچ_Initial_تیاری = خطأ;  // In InitialAutomatic*/InitialStatic* assignment LHS
    منطقي m_وچ_Initial_Static = خطأ;  // In InitialStatic
    منطقي m_وچ_Proc_سونپو = خطأ;  // In procedural assignment
    منطقي m_وچ_FTask_Ref = خطأ;  // In function or task call
    منطقي m_وچ_Inout_Or_Ref_Pin = خطأ;  // Connected to pin that is inout
    منطقي m_وچ_Sel_Lhs = خطأ;  // Iterating the fromp of an AstSel (a partial-bit write target)
    ثابت Ast_گرہ_FTask* m_taskp = خالی_سوچک;  // Current task
    ثابت AstAlways* m_alwaysp = خالی_سوچک;  // Current always of either type
    ثابت AstAlways* m_alwaysCombp = خالی_سوچک;  // Current always if combo, otherwise nullptr
    ثابت AstAlways* m_alwaysFFp = خالی_سوچک;  // Current always if ff, otherwise nullptr
    ثابت AstAlways* m_alwaysPlainp = خالی_سوچک;  // Current always if plain (not comb/ff/latch)
    ثابت AstClocking* m_clockingp = خالی_سوچک;  // Current clocking block, otherwise nullptr

    V3UndrivenCapture* ثابت m_capturep = خالی_سوچک;  // Capture object.  'nullptr' if disabled.

    // METHODS

    Undriven_Var_اندراج* getEntryp(AstVar* nodep, صحيح which_user) {
        إذا (!(which_user == 1 ? nodep->user1p() : nodep->user2p())) {
            Undriven_Var_اندراج* ثابت entryp = جديد Undriven_Var_اندراج{nodep};
            // UINFO(9," Associate u="<<which_user<<" "<<cvtToHex(this)<<" "<<nodep->name()<<endl);
            m_entryps[which_user].ڈالو_پٹھ(entryp);
            إذا (which_user == 1) {
                nodep->user1p(entryp);
            } وإلا إذا (which_user == 2) {
                nodep->user2p(entryp);
            } وإلا {
                nodep->v3fatalSrc("Bad case");
            }
            أرجع entryp;
        } وإلا {
            Undriven_Var_اندراج* ثابت entryp = reinterpret_cast<Undriven_Var_اندراج*>(
                which_user == 1 ? nodep->user1p() : nodep->user2p());
            أرجع entryp;
        }
    }

    فراغ warn_Alw_Comb_ترتیب(Ast_گرہ_Var_Ref* nodep, ثابت Ast_گرہ* readp) {
        AstVar* ثابت varp = nodep->varp();
        إذا (!varp->isParam() && !varp->isGenVar() && !varp->is_ورتیا_Loop_Idx()
            && !varp->ignoreSchedWrite()
            && !m_وچ_BBox  // We may have falsely considered a SysIgnore as a driver
            && !VN_IS(nodep, VarXRef)  // Xrefs might point at two different instances
            && !varp->fileline()->warnIsOff(
                V3_Error_کوڈ::ALWCOMBORDER)) {  // Warn only once per variable
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
            varp->fileline()->modifyWarnOff(V3_Error_کوڈ::ALWCOMBORDER,
                                            سچ);  // Complain just once for any usage
        }
    }
    // A clocking block 'output' is an additional driver of the signal it names.
    // The conflict is found from whichever driver is reached second.
    منطقي clockingDrivesOther(ثابت Undriven_Var_اندراج* entryp, منطقي otherWriteIsStaticInit) ثابت {
        ثابت منطقي otherIsExplicitProc
            = entryp->is_Driven_Always_Comb_سارا() || entryp->isDrivenAlwaysFFWhole();
        أرجع m_clockingp && !otherIsExplicitProc && !otherWriteIsStaticInit
               && m_clockingp != entryp->getClockingp();
    }
    منطقي otherDrivesClocking(ثابت Undriven_Var_اندراج* entryp) ثابت {
        أرجع !m_clockingp && !m_alwaysCombp && !m_alwaysFFp && !m_وچ_Initial_Static
               && entryp->is_Driven_Clocking_سارا();
    }
    // A continuous assignment contending with a clocking block 'output' is an
    // unambiguous driver conflict. Multiple clocking blocks may drive the same
    // signal, the last drive wins (IEEE 1800-2023 14.16.2).
    فراغ warnClockingDriven(Ast_گرہ_Var_Ref* nodep, ثابت Undriven_Var_اندراج* entryp,
                            ثابت Ast_گرہ* otherWritep, منطقي otherWriteIsStaticInit) {
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
        إذا (otherDrivesClocking(entryp) && m_وچ_Cont_سونپو) {
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
    فراغ warnClockingDrivenProc(Ast_گرہ_Var_Ref* nodep, ثابت Undriven_Var_اندراج* entryp,
                                ثابت Ast_گرہ* otherWritep, منطقي otherWriteIsStaticInit) {
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
        إذا (otherDrivesClocking(entryp) && !m_وچ_Cont_سونپو) {
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
    فراغ ویکھو(AstVar* nodep) override {
        ثابت منطقي funcInout = nodep->isFuncLocal() && nodep->isInout();
        لكل (صحيح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            // For assigns and non-combo always, do just usr==1, to look
            // for module-wide undriven etc.
            // For combo always, run both usr==1 for above, and also
            // usr==2 for always-only checks.
            Undriven_Var_اندراج* ثابت entryp = getEntryp(nodep, usr);
            إذا ((nodep->isNonOutput() && !funcInout) || nodep->is_Sig_عوامی()
                || nodep->hasUserInit() || nodep->isSigUserRWPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->driven_سارا(nodep);
            }
            إذا ((nodep->isWritable() && !funcInout) || nodep->is_Sig_عوامی()
                || nodep->isSigUserRWPublic() || nodep->is_Sig_User_Rd_عوامی()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->ورتیا_سارا(nodep);
            }
            إذا (nodep->valuep()) entryp->driven_سارا(nodep->valuep());
        }
        // Discover variables used in bit definitions, etc
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstArraySel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(Ast_ٹکڑا_Sel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstSel* nodep) override {
        Ast_گرہ_Var_Ref* ثابت varrefp = VN_CAST(nodep->fromp(), گرہ_Var_Ref);
        AstConst* ثابت constp = VN_CAST(nodep->lsbp(), Const);
        إذا (varrefp && constp && !constp->عدد().isFourState()) {
            لكل (صحيح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                Undriven_Var_اندراج* ثابت entryp = getEntryp(varrefp->varp(), usr);
                ثابت صحيح lsb = constp->منزل_UInt();
                إذا (m_وچ_BBox || varrefp->access().isWriteOrRW()) {
                    // Don't warn if already driven earlier as "a=0; if(a) a=1;" is fine.
                    إذا (usr == 2 && m_alwaysCombp
                        && entryp->is_ورتیا_Not_Driven_ثنائی_ہندسہ(lsb, nodep->چوڑائی())) {
                        UINFO(9, " Select.  Entryp=" << cvt_منزل_Hex(entryp));
                        warn_Alw_Comb_ترتیب(varrefp, entryp->پہلا_ورتیا_Not_Drivenp());
                    }
                    entryp->driven_ثنائی_ہندسہ(lsb, nodep->چوڑائی());
                }
                إذا (m_وچ_BBox || !varrefp->access().isWriteOrRW())
                    entryp->ورتیا_ثنائی_ہندسہ_2(lsb, nodep->چوڑائی(), varrefp);
            }
        } وإلا {
            // skip over static longest static prefix
            iterateConst(nodep->lsbp());
            VL_RESTORER(m_وچ_Sel_Lhs);
            m_وچ_Sel_Lhs = !V3_چوڑائی::selectNonConstantRecurse(nodep->lsbp(), /*inSel=*/سچ);
            iterateConst(nodep->fromp());
        }
    }
    فراغ ویکھو(Ast_گرہ_Var_Ref* nodep) override {
        // Any variable
        إذا (nodep->access().isWriteOrRW()
            && !VN_IS(nodep, VarXRef)) {  // Ignore interface variables and similar ugly items
            إذا (m_وچ_Proc_سونپو && !nodep->varp()->varType().isProcAssignable()
                && !nodep->varp()->isDeclTyped()  //
                && !nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()) {
                nodep->v3warn(PROCASSWIRE, "Procedural assignment to wire, perhaps intended var"
                                               << " (IEEE 1800-2023 6.5): "
                                               << nodep->prettyNameQ());
            } وإلا إذا (m_وچ_Cont_سونپو && !nodep->varp()->varType().isContAssignable()
                       && !nodep->fileline()->language().systemVerilog()) {
                nodep->v3warn(CONTASSREG,
                              "Continuous assignment to reg, perhaps intended wire"
                                  << " (IEEE 1364-2005 6.1; Verilog only, legal in SV): "
                                  << nodep->prettyNameQ());
            }
            إذا (m_وچ_FTask_Ref && nodep->varp()->varType().isNet()) {
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
        إذا (m_taskp && !m_alwaysp && !m_وچ_Cont_سونپو && !m_وچ_Initial_Static && !m_وچ_BBox
            && !m_taskp->dpiExport()) {
            AstVar* ثابت retVarp = VN_CAST(m_taskp->fvarp(), Var);
            إذا (!retVarp || nodep->varp() != retVarp) ftaskDef = سچ;
        }

        لكل (صحيح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            Undriven_Var_اندراج* ثابت entryp = getEntryp(nodep->varp(), usr);
            ثابت منطقي fdrv = nodep->access().isWriteOrRW()
                              && nodep->varp()->attrFileDescr();  // FD's are also being read from
            إذا (m_وچ_BBox || nodep->access().isWriteOrRW()) {
                إذا (usr == 2 && m_alwaysCombp && entryp->is_ورتیا_Not_Driven_Any()) {
                    UINFO(9, " Full bus.  Entryp=" << cvt_منزل_Hex(entryp));
                    warn_Alw_Comb_ترتیب(nodep, entryp->پہلا_ورتیا_Not_Drivenp());
                }
                ثابت Ast_گرہ_Var_Ref* ثابت otherVarRefp = entryp->getNodep();
                ثابت Ast_گرہ* ثابت otherWritep = otherVarRefp
                                                       ? static_cast<ثابت Ast_گرہ*>(otherVarRefp)
                                                       : entryp->callNodep();
                ثابت منطقي same_File_سطر
                    = otherVarRefp && nodep->fileline() == otherVarRefp->fileline();
                // Preconditions shared by MULTIDRIVEN and MULTIDRIVENPROC.
                ثابت منطقي multidrivenCommon
                    = entryp->is_Driven_سارا() && !m_وچ_BBox && !VN_IS(nodep, VarXRef)
                      && !VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType) && !same_File_سطر
                      && !entryp->isUnderGen() && otherWritep && !entryp->isFtaskDriven()
                      && !ftaskDef && !m_وچ_Sel_Lhs;
                // The two warnings are gated independently on the variable
                // declaration's fileline, as v3warn suppression will check
                // the driving fileline and still warn even if the warning
                // was suppressed with lint_off at the declaration.
                ثابت منطقي otherWriteIsStaticInit
                    = nodep->varp()->hasUserInit() && otherWritep == entryp->initStaticp();
                إذا (multidrivenCommon
                    && !nodep->varp()->fileline()->warnIsOff(V3_Error_کوڈ::MULTIDRIVEN)) {
                    إذا (m_alwaysCombp
                        && (!entryp->is_Driven_Always_Comb_سارا()
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
                    إذا (!m_alwaysCombp && entryp->is_Driven_Always_Comb_سارا()) {
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
                    إذا (!m_alwaysFFp && !m_وچ_Initial_Static && entryp->isDrivenAlwaysFFWhole()) {
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
                إذا (multidrivenCommon && !nodep->varp()->is_ورتیا_Loop_Idx()
                    && !nodep->varp()->fileline()->warnIsOff(V3_Error_کوڈ::MULTIDRIVENPROC)) {
                    // Two plain always blocks driving the whole signal: legal
                    // SystemVerilog, but a driver conflict for synthesis. The
                    // always_ff/always_comb cases above already cover mixes with
                    // an explicit process, so only warn for plain+plain here.
                    // When the two blocks are clocked differently, the
                    // (on-by-default) MULTIDRIVEN check in V3Delayed already reports
                    // the conflict, so don't also emit MULTIDRIVENPROC for that case.
                    ثابت AstAlways* ثابت otherAlwaysp = entryp->getAlwPlainp();
                    ثابت Ast_Sen_درخت* ثابت senp
                        = m_alwaysPlainp ? m_alwaysPlainp->sentreep() : خالی_سوچک;
                    ثابت Ast_Sen_درخت* ثابت otherSenp
                        = otherAlwaysp ? otherAlwaysp->sentreep() : خالی_سوچک;
                    ثابت منطقي differentClocking = senp && otherSenp && senp->hasClocked()
                                                   && otherSenp->hasClocked()
                                                   && !senp->same_درخت(otherSenp);
                    إذا (m_alwaysPlainp && entryp->is_Driven_Always_Plain_سارا()
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
                إذا (!m_وچ_Initial_تیاری || nodep->varp()->hasUserInit()) {
                    // Else don't count default initialization as a driver to a net/variable
                    entryp->driven_سارا(nodep, ftaskDef);
                }
                إذا (m_alwaysCombp && entryp->is_Driven_Always_Comb_سارا()
                    && m_alwaysCombp != entryp->getAlwCombp()
                    && m_alwaysCombp->fileline() == entryp->getAlwCombp()->fileline())
                    entryp->underGenerate();
                إذا (m_alwaysCombp) entryp->driven_Always_Comb_سارا(m_alwaysCombp);
                إذا (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, nodep->varp());
                إذا (m_alwaysPlainp) entryp->driven_Always_Plain_سارا(m_alwaysPlainp);
                إذا (m_clockingp) entryp->driven_Clocking_سارا(m_clockingp);
            }
            إذا (nodep->access().isWriteOrRW() && !VN_IS(nodep, VarXRef)) {
                // Ignoring xrefs as the initial and assignment to track might refer to two
                // different instances.  Ideally all of V3Undriven would move after V3Scope,
                // then could use VarScope tracking instead.
                إذا (m_وچ_Initial_Static && !entryp->initStaticp()) entryp->initStaticp(nodep);
                إذا (m_وچ_Initial && !entryp->initialp()) entryp->initialp(nodep);
                إذا (m_وچ_Cont_سونپو && !entryp->contAssignp()) entryp->contAssignp(nodep);
                إذا (m_alwaysp && m_وچ_Proc_سونپو && !entryp->procWritep())
                    entryp->procWritep(nodep);
            }
            إذا ((!m_وچ_Initial_تیاری || nodep->varp()->hasUserInit())
                && (m_وچ_BBox || nodep->access().isReadOrRW()
                    || fdrv
                    // Inouts have only isWrite set, as we don't have more
                    // information and operating on module boundary, treat as
                    // both read and writing
                    || m_وچ_Inout_Or_Ref_Pin))
                entryp->ورتیا_سارا(nodep);
        }
    }

    // Don't know what black boxed calls do, assume in+out
    فراغ ویکھو(AstSysIgnore* nodep) override {
        VL_RESTORER(m_وچ_BBox);
        m_وچ_BBox = سچ;
        iterateChildrenConst(nodep);
    }

    فراغ ویکھو(Ast_سونپو* nodep) override {
        VL_RESTORER(m_وچ_Proc_سونپو);
        m_وچ_Proc_سونپو = سچ;
        {
            VL_RESTORER(m_وچ_Initial_تیاری);
            m_وچ_Initial_تیاری = خطأ;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }
    فراغ ویکھو(Ast_سونپو_Dly* nodep) override {
        VL_RESTORER(m_وچ_Proc_سونپو);
        m_وچ_Proc_سونپو = سچ;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(Ast_سونپو_Force* nodep) override {
        iterateConst(nodep->rhsp());
        VL_RESTORER(m_وچ_Initial);
        m_وچ_Initial = خطأ;
        iterateConst(nodep->lhsp());
    }
    فراغ ویکھو(Ast_سونپو_W* nodep) override {
        VL_RESTORER(m_وچ_Cont_سونپو);
        m_وچ_Cont_سونپو = سچ;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstRelease* nodep) override {
        VL_RESTORER(m_وچ_Initial);
        m_وچ_Initial = خطأ;
        iterateConst(nodep->lhsp());
    }
    فراغ ویکھو(AstInitial* nodep) override {
        VL_RESTORER(m_وچ_Initial);
        m_وچ_Initial = سچ;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstInitialAutomatic* nodep) override {
        VL_RESTORER(m_وچ_Initial_تیاری);
        m_وچ_Initial_تیاری = سچ;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_وچ_Initial_تیاری);
        m_وچ_Initial_تیاری = سچ;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstInitialStatic* nodep) override {
        VL_RESTORER(m_وچ_Initial_Static);
        m_وچ_Initial_Static = سچ;
        VL_RESTORER(m_وچ_Initial_تیاری);
        m_وچ_Initial_تیاری = سچ;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_وچ_Initial_تیاری);
        m_وچ_Initial_تیاری = سچ;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        VL_RESTORER(m_alwaysCombp);
        VL_RESTORER(m_alwaysFFp);
        VL_RESTORER(m_alwaysPlainp);
        Ast_گرہ::user2_Clear_درخت();
        m_alwaysp = nodep;
        إذا (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            UINFO(9, "   " << nodep);
            m_alwaysCombp = nodep;
        } وإلا {
            m_alwaysCombp = خالی_سوچک;
        }
        m_alwaysFFp = nodep->keyword() == VAlwaysKwd::ALWAYS_FF ? nodep : خالی_سوچک;
        m_alwaysPlainp = nodep->keyword() == VAlwaysKwd::ALWAYS ? nodep : خالی_سوچک;
        iterateChildrenConst(nodep);
        إذا (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) UINFO(9, "   Done " << nodep);
    }
    فراغ ویکھو(AstClocking* nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterateChildrenConst(nodep);
    }

    فراغ ویکھو(Ast_گرہ_FTask_Ref* nodep) override {
        VL_RESTORER(m_وچ_FTask_Ref);
        m_وچ_FTask_Ref = سچ;

        iterateChildrenConst(nodep);

        إذا (!m_capturep) أرجع;

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Do not apply writeSummary at calls inside a task definition, or they will look like
        // independent drivers (phantom MULTIDRIVEN).
        ثابت منطقي وچ_Executed_Context
            = !(m_taskp && !m_alwaysp && !m_وچ_Cont_سونپو && !m_وچ_Initial_Static && !m_وچ_BBox
                && !m_taskp->dpiExport());

        إذا (!وچ_Executed_Context) أرجع;

        Ast_گرہ_FTask* ثابت calleep = nodep->taskp();
        إذا (!calleep) أرجع;

        ثابت خودکار& vars = m_capturep->writeSummary(calleep);
        لكل (AstVar* ثابت varp : vars) {
            لكل (صحيح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                Undriven_Var_اندراج* ثابت entryp = getEntryp(varp, usr);
                entryp->driven_راہیں_Call(nodep);
                إذا (m_alwaysCombp) entryp->driven_Always_Comb_سارا(m_alwaysCombp);
                إذا (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, varp);
            }
        }
    }

    فراغ ویکھو(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_taskp);
        m_taskp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstPin* nodep) override {
        VL_RESTORER(m_وچ_Inout_Or_Ref_Pin);
        m_وچ_Inout_Or_Ref_Pin = nodep->modVarp()->isInoutOrRef();
        iterateChildrenConst(nodep);
    }

    // Until we support tables, primitives will have undriven and unused I/Os
    فراغ ویکھو(AstPrimitive*) override {}

    // Coverage artifacts etc shouldn't count as a sink
    فراغ ویکھو(Ast_گرہ_Cover_Decl*) override {}
    فراغ ویکھو(AstCoverInc*) override {}
    فراغ ویکھو(AstCoverToggle*) override {}
    فراغ ویکھو(AstTraceDecl* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    فراغ ویکھو(AstTraceInc* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }

    // iterate
    فراغ ویکھو(AstConst* nodep) override {}
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    واضح UndrivenVisitor(AstNetlist* nodep, V3UndrivenCapture* capturep)
        : m_capturep{capturep} {
        iterateConst(nodep);
    }

    ~UndrivenVisitor() override {
        لكل (Undriven_Var_اندراج* ip : m_entryps[1]) ip->reportViolations();
        لكل (صحيح usr = 1; usr < 3; ++usr) {
            لكل (Undriven_Var_اندراج* ip : m_entryps[usr]) مٹاؤ ip;
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
