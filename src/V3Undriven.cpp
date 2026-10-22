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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Undriven.h"

#شامل "V3Stats.h"
#شامل "V3UndrivenCapture.h"
#شامل "V3Width.h"

#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Class for every variable we may process

جماعت Undriven_Var_اندراج final {
    // MEMBERS
    AstVar* ثابت m_varp;  // Variable this tracks
    std::سمتیہ<منطقی> m_پورا_Flags;  // Used/Driven on whole vector
    std::سمتیہ<منطقی> m_ثنائی_ہندسہ_Flags;  // Used/Driven on each subbit
    ثابت Ast_گرہ* m_مستعمل_Not_Drivenp = خالی_اشاریہ;  // First read before any write
    ثابت AstAlways* m_alwCombp
        = خالی_اشاریہ;  // always_comb of var if driven within always_comb, else nullptr
    ثابت AstAlways* m_alwFFp = خالی_اشاریہ;  // always_ff of var if driven within always_ff
    ثابت AstAlways* m_alwPlainp = خالی_اشاریہ;  // plain always of var if driven within plain always
    ثابت AstClocking* m_clockingp = خالی_اشاریہ;  // clocking block of var if driven as output
    ثابت Ast_گرہ_Var_Ref* m_nodep = خالی_اشاریہ;  // varref if driven, else nullptr
    ثابت Ast_گرہ* m_initStaticp = خالی_اشاریہ;  // varref if in InitialStatic driven
    ثابت Ast_گرہ* m_initialp = خالی_اشاریہ;  // varref if driven in an explicit initial block
    ثابت Ast_گرہ* m_contAssignp = خالی_اشاریہ;  // varref if in continuous assignment driven
    ثابت Ast_گرہ* m_procWritep = خالی_اشاریہ;  // varref if written in process
    منطقی m_underGen = جھوٹ;  // Under a generate
    منطقی m_ftaskDriven = جھوٹ;  // Last driven by function or task

    ثابت Ast_گرہ_FTask_Ref* m_callNodep = خالی_اشاریہ;  // Call node if driven via writeSummary

    شمار : uint8_t {
        FLAG_مستعمل = 0,  // Signal or bit has been read/observed
        FLAG_DRIVEN = 1,  // Signal or bit has been written/driven
        FLAG_DRIVEN_ALWCOMB = 2,  // Whole signal has been driven from always_comb
        FLAG_DRIVEN_ALWFF = 3,  // Whole signal has been driven from always_ff
        FLAG_DRIVEN_ALWPLAIN = 4,  // Whole signal has been driven from a plain always
        FLAG_DRIVEN_CLOCKING = 5,  // Whole signal has been driven as a clocking block output
        FLAGS_فی_ثنائی_ہندسہ = 6  // Number of flags stored for each tracked bit
    };

عوامی:
    // CONSTRUCTORS
    صریح Undriven_Var_اندراج(AstVar* varp)
        : m_varp{varp} {  // Construction for when a var is used
        UINFO(9, "create " << varp);
        m_پورا_Flags.resize(FLAGS_فی_ثنائی_ہندسہ);
        برائے (صحیح i = 0; i < FLAGS_فی_ثنائی_ہندسہ; i++) m_پورا_Flags[i] = جھوٹ;
        m_ثنائی_ہندسہ_Flags.resize(varp->چوڑائی() * FLAGS_فی_ثنائی_ہندسہ);
        برائے (صحیح i = 0; i < varp->چوڑائی() * FLAGS_فی_ثنائی_ہندسہ; i++) m_ثنائی_ہندسہ_Flags[i] = جھوٹ;
    }
    ~Undriven_Var_اندراج() = طےشدہ;

نجی:
    // METHODS
    منطقی ثنائی_ہندسہ_عدد_Ok(صحیح ثنائی_ہندسہ_2) ثابت {
        واپس ثنائی_ہندسہ_2 >= 0 && (ثنائی_ہندسہ_2 * FLAGS_فی_ثنائی_ہندسہ < static_cast<صحیح>(m_ثنائی_ہندسہ_Flags.سائز()));
    }
    منطقی مستعمل_Flag(صحیح ثنائی_ہندسہ_2) ثابت {
        واپس m_پورا_Flags[FLAG_مستعمل] || m_ثنائی_ہندسہ_Flags[ثنائی_ہندسہ_2 * FLAGS_فی_ثنائی_ہندسہ + FLAG_مستعمل];
    }
    منطقی drivenFlag(صحیح ثنائی_ہندسہ_2) ثابت {
        واپس m_پورا_Flags[FLAG_DRIVEN] || m_ثنائی_ہندسہ_Flags[ثنائی_ہندسہ_2 * FLAGS_فی_ثنائی_ہندسہ + FLAG_DRIVEN];
    }
    صحیح ثنائی_ہندسہ_گنتی() ثابت { واپس m_ثنائی_ہندسہ_Flags.سائز() / FLAGS_فی_ثنائی_ہندسہ; }
    خالی ریکارڈ_مستعمل_Not_Driven(ثابت Ast_گرہ* nodep) {
        اگر (!m_مستعمل_Not_Drivenp) m_مستعمل_Not_Drivenp = nodep;
    }
    شمار ثنائی_ہندسہ_نام_Which : uint8_t { BN_غیر_مستعمل, BN_UNDRIVEN, BN_BOTH };
    string ثنائی_ہندسہ_نام(ثنائی_ہندسہ_نام_Which which) {
        string بٹ;
        منطقی پچھلا_2 = جھوٹ;
        صحیح msb = 0;
        // bit==-1 loops below; we do one extra iteration so end with prev=false
        برائے (صحیح ثنائی_ہندسہ_2 = (m_ثنائی_ہندسہ_Flags.سائز() / FLAGS_فی_ثنائی_ہندسہ) - 1; ثنائی_ہندسہ_2 >= -1; --ثنائی_ہندسہ_2) {
            اگر (ثنائی_ہندسہ_2 >= 0
                && ((which == BN_غیر_مستعمل && !مستعمل_Flag(ثنائی_ہندسہ_2) && drivenFlag(ثنائی_ہندسہ_2))
                    || (which == BN_UNDRIVEN && مستعمل_Flag(ثنائی_ہندسہ_2) && !drivenFlag(ثنائی_ہندسہ_2))
                    || (which == BN_BOTH && !مستعمل_Flag(ثنائی_ہندسہ_2) && !drivenFlag(ثنائی_ہندسہ_2)))) {
                اگر (!پچھلا_2) {
                    پچھلا_2 = سچ;
                    msb = ثنائی_ہندسہ_2;
                }
            } ورنہ اگر (پچھلا_2) {
                ثابت AstBasicDType* ثابت bdtypep = m_varp->basicp();
                ثابت صحیح lsb = ثنائی_ہندسہ_2 + 1;
                اگر (بٹ != "") بٹ += ",";
                اگر (lsb == msb) {
                    بٹ += cvt_مقصد_Str(lsb + bdtypep->lo());
                } ورنہ {
                    اگر (bdtypep->ascending()) {
                        بٹ
                            += cvt_مقصد_Str(lsb + bdtypep->lo()) + ":" + cvt_مقصد_Str(msb + bdtypep->lo());
                    } ورنہ {
                        بٹ
                            += cvt_مقصد_Str(msb + bdtypep->lo()) + ":" + cvt_مقصد_Str(lsb + bdtypep->lo());
                    }
                }
                پچھلا_2 = جھوٹ;
            }
        }
        واپس "[" + بٹ + "]";
    }

عوامی:
    خالی مستعمل_پورا(ثابت Ast_گرہ* nodep) {
        UINFO(9, "set u[*] " << m_varp->name() << " " << nodep);
        اگر (!m_پورا_Flags[FLAG_DRIVEN]) {
            برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < ثنائی_ہندسہ_گنتی(); ++ثنائی_ہندسہ_2) {
                اگر (!drivenFlag(ثنائی_ہندسہ_2)) {
                    ریکارڈ_مستعمل_Not_Driven(nodep);
                    توڑو;
                }
            }
        }
        m_پورا_Flags[FLAG_مستعمل] = سچ;
    }
    خالی driven_پورا(ثابت Ast_گرہ* nodep) {
        UINFO(9, "set d[*] " << m_varp->name() << " " << nodep);
        m_پورا_Flags[FLAG_DRIVEN] = سچ;
    }
    خالی driven_پورا(ثابت Ast_گرہ_Var_Ref* nodep, منطقی ftaskDef) {
        ثابت منطقی wasDriven = is_Driven_پورا();
        driven_پورا(nodep);
        // A write in a task/function definition is not an executed process, so it must not
        // displace an already recorded write, nor clear the flag that keeps such a write
        // from being reported as the other write of a MULTIDRIVEN
        اگر (ftaskDef && wasDriven) واپس;
        m_ftaskDriven = ftaskDef;
        m_nodep = nodep;
    }
    خالی driven_Always_Comb_پورا(ثابت AstAlways* alwCombp) {
        m_پورا_Flags[FLAG_DRIVEN_ALWCOMB] = سچ;
        m_alwCombp = alwCombp;
    }
    خالی drivenAlwaysFFWhole(ثابت AstAlways* alwFFp, ثابت AstVar* varp) {
        m_پورا_Flags[FLAG_DRIVEN_ALWFF] = سچ;
        m_alwFFp = alwFFp;
    }
    خالی driven_Always_Plain_پورا(ثابت AstAlways* alwPlainp) {
        m_پورا_Flags[FLAG_DRIVEN_ALWPLAIN] = سچ;
        m_alwPlainp = alwPlainp;
    }
    خالی driven_Clocking_پورا(ثابت AstClocking* clockingp) {
        m_پورا_Flags[FLAG_DRIVEN_CLOCKING] = سچ;
        m_clockingp = clockingp;
    }

    ثابت Ast_گرہ* initStaticp() ثابت { واپس m_initStaticp; }
    خالی initStaticp(ثابت Ast_گرہ* nodep) { m_initStaticp = nodep; }
    ثابت Ast_گرہ* initialp() ثابت { واپس m_initialp; }
    خالی initialp(ثابت Ast_گرہ* nodep) { m_initialp = nodep; }
    ثابت Ast_گرہ* contAssignp() ثابت { واپس m_contAssignp; }
    خالی contAssignp(ثابت Ast_گرہ* nodep) { m_contAssignp = nodep; }
    ثابت Ast_گرہ* procWritep() ثابت { واپس m_procWritep; }
    خالی procWritep(ثابت Ast_گرہ* nodep) { m_procWritep = nodep; }
    خالی underGenerate() { m_underGen = سچ; }
    منطقی isUnderGen() ثابت { واپس m_underGen; }
    منطقی is_Driven_پورا() ثابت { واپس m_پورا_Flags[FLAG_DRIVEN]; }
    منطقی is_Driven_Always_Comb_پورا() ثابت { واپس m_پورا_Flags[FLAG_DRIVEN_ALWCOMB]; }
    منطقی isDrivenAlwaysFFWhole() ثابت { واپس m_پورا_Flags[FLAG_DRIVEN_ALWFF]; }
    منطقی is_Driven_Always_Plain_پورا() ثابت { واپس m_پورا_Flags[FLAG_DRIVEN_ALWPLAIN]; }
    منطقی is_Driven_Clocking_پورا() ثابت { واپس m_پورا_Flags[FLAG_DRIVEN_CLOCKING]; }
    منطقی isFtaskDriven() ثابت { واپس m_ftaskDriven; }
    ثابت Ast_گرہ_Var_Ref* getNodep() ثابت { واپس m_nodep; }
    ثابت AstAlways* getAlwCombp() ثابت { واپس m_alwCombp; }
    ثابت AstAlways* getAlwFFp() ثابت { واپس m_alwFFp; }
    ثابت AstAlways* getAlwPlainp() ثابت { واپس m_alwPlainp; }
    ثابت AstClocking* getClockingp() ثابت { واپس m_clockingp; }
    خالی مستعمل_ثنائی_ہندسہ_2(صحیح ثنائی_ہندسہ_2, صحیح چوڑائی, ثابت Ast_گرہ* nodep) {
        UINFO(9, "set u[" << (ثنائی_ہندسہ_2 + چوڑائی - 1) << ":" << ثنائی_ہندسہ_2 << "] " << m_varp->name());
        برائے (صحیح i = 0; i < چوڑائی; i++) {
            اگر (ثنائی_ہندسہ_عدد_Ok(ثنائی_ہندسہ_2 + i)) {
                اگر (!drivenFlag(ثنائی_ہندسہ_2 + i)) ریکارڈ_مستعمل_Not_Driven(nodep);
                m_ثنائی_ہندسہ_Flags[(ثنائی_ہندسہ_2 + i) * FLAGS_فی_ثنائی_ہندسہ + FLAG_مستعمل] = سچ;
            }
        }
    }
    خالی driven_ثنائی_ہندسہ(صحیح ثنائی_ہندسہ_2, صحیح چوڑائی) {
        UINFO(9, "set d[" << (ثنائی_ہندسہ_2 + چوڑائی - 1) << ":" << ثنائی_ہندسہ_2 << "] " << m_varp->name());
        برائے (صحیح i = 0; i < چوڑائی; i++) {
            اگر (ثنائی_ہندسہ_عدد_Ok(ثنائی_ہندسہ_2 + i)) m_ثنائی_ہندسہ_Flags[(ثنائی_ہندسہ_2 + i) * FLAGS_فی_ثنائی_ہندسہ + FLAG_DRIVEN] = سچ;
        }
    }
    منطقی is_مستعمل_Not_Driven_ثنائی_ہندسہ(صحیح ثنائی_ہندسہ_2, صحیح چوڑائی) ثابت {
        برائے (صحیح i = 0; i < چوڑائی; i++) {
            اگر (ثنائی_ہندسہ_عدد_Ok(ثنائی_ہندسہ_2 + i)
                && (m_پورا_Flags[FLAG_مستعمل] || m_ثنائی_ہندسہ_Flags[(ثنائی_ہندسہ_2 + i) * FLAGS_فی_ثنائی_ہندسہ + FLAG_مستعمل])
                && !(m_پورا_Flags[FLAG_DRIVEN]
                     || m_ثنائی_ہندسہ_Flags[(ثنائی_ہندسہ_2 + i) * FLAGS_فی_ثنائی_ہندسہ + FLAG_DRIVEN]))
                واپس سچ;
        }
        واپس جھوٹ;
    }
    منطقی is_مستعمل_Not_Driven_Any() ثابت {
        واپس is_مستعمل_Not_Driven_ثنائی_ہندسہ(0, m_ثنائی_ہندسہ_Flags.سائز() / FLAGS_فی_ثنائی_ہندسہ);
    }
    ثابت Ast_گرہ* پہلا_مستعمل_Not_Drivenp() ثابت { واپس m_مستعمل_Not_Drivenp; }
    ساکن منطقی غیر_مستعمل_مطابقت(AstVar* nodep) {
        ثابت string regexp = v3Global.opt.غیر_مستعمل_Regexp();
        اگر (regexp == "") واپس جھوٹ;
        ثابت string prettyName = nodep->prettyName();
        واپس VString::wildmatch(prettyName.c_str(), regexp.c_str());
    }
    خالی reportViolations() {
        // Combine bits into overall state
        AstVar* ثابت nodep = m_varp;

        اگر (initStaticp() && procWritep() && nodep->hasUserInit() && !nodep->isClassMember()
            && !nodep->isFuncLocal()) {
            initStaticp()->v3warn(
                PROCASSINIT,
                "Procedural assignment to declaration with initial value: "
                    << nodep->prettyNameQ() << '\n'
                    << initStaticp()->warn_مزید() << "... Location of variable initialization\n"
                    << initStaticp()->warnContextPrimary() << '\n'
                    << procWritep()->warnOther() << "... Location of variable process write\n"
                    << procWritep()->warn_مزید()
                    << "... Perhaps should initialize instead using a reset in this process\n"
                    << procWritep()->warnContextSecondary());
        }
        ثابت Ast_گرہ* ثابت initp = nodep->hasUserInit() ? initStaticp() : initialp();
        اگر (initp && contAssignp() && !nodep->isClassMember() && !nodep->isFuncLocal()) {
            initp->v3warn(E_CONTASSINIT, "Continuous assignment to variable with initial value: "
                                             << nodep->prettyNameQ() << '\n'
                                             << initp->warn_مزید()
                                             << "... Location of variable initialization\n"
                                             << initp->warnContextPrimary() << '\n'
                                             << contAssignp()->warnOther()
                                             << "... Location of continuous assignment\n"
                                             << contAssignp()->warnContextSecondary());
        }
        اگر (nodep->isGenVar()) {  // Genvar
            اگر (!nodep->isIfaceRef() && !nodep->is_مستعمل_Param() && !غیر_مستعمل_مطابقت(nodep)) {
                nodep->v3warn(UNUSEDGENVAR, "Genvar is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_رمز::UNUSEDGENVAR,
                                                 سچ);  // Warn only once
            }
        } ورنہ اگر (nodep->isParam()) {  // Parameter
            اگر (!nodep->isIfaceRef() && !nodep->is_مستعمل_Param() && !غیر_مستعمل_مطابقت(nodep)) {
                nodep->v3warn(UNUSEDPARAM, "Parameter is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_رمز::UNUSEDPARAM,
                                                 سچ);  // Warn only once
            }
        } ورنہ {  // Signal
            ثابت string varType{nodep->isFuncLocal() ? "Function variable" : "Signal"};
            ثابت منطقی funcInout = nodep->isFuncLocal() && nodep->isInout();
            منطقی allU = سچ;
            منطقی allD = سچ;
            منطقی anyU = m_پورا_Flags[FLAG_مستعمل];
            منطقی anyD = m_پورا_Flags[FLAG_DRIVEN];
            منطقی anyUnotD = جھوٹ;
            منطقی anyDnotU = جھوٹ;
            منطقی anynotDU = جھوٹ;
            برائے (غیرعلامتی ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < m_ثنائی_ہندسہ_Flags.سائز() / FLAGS_فی_ثنائی_ہندسہ; ثنائی_ہندسہ_2++) {
                ثابت منطقی مستعمل = مستعمل_Flag(ثنائی_ہندسہ_2);
                ثابت منطقی driv = drivenFlag(ثنائی_ہندسہ_2);
                allU &= مستعمل;
                anyU |= مستعمل;
                allD &= driv;
                anyD |= driv;
                anyUnotD |= مستعمل && !driv;
                anyDnotU |= !مستعمل && driv;
                anynotDU |= !مستعمل && !driv;
            }
            اگر (funcInout) {
                اگر (anyD) allU = سچ;
                allD = سچ;
            }
            اگر (allU) m_پورا_Flags[FLAG_مستعمل] = سچ;
            اگر (allD) m_پورا_Flags[FLAG_DRIVEN] = سچ;
            // Test results
            اگر (nodep->isIfaceRef()) {
                // For interface top level we don't do any tracking
                // Ideally we'd report unused instance cells, but presumably a signal inside one
                // would get reported as unused
            } ورنہ اگر (allU && allD) {
                // It's fine
            } ورنہ اگر (!anyD && !anyU) {
                // UNDRIVEN is considered more serious - as is more likely a bug,
                // thus undriven+unused bits get UNUSED warnings, as they're not as buggy.
                اگر (!غیر_مستعمل_مطابقت(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not driven, nor used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3_Error_رمز::UNUSEDSIGNAL,
                                                     سچ);  // Warn only once
                }
            } ورنہ اگر (allD && !anyU) {
                اگر (!غیر_مستعمل_مطابقت(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3_Error_رمز::UNUSEDSIGNAL,
                                                     سچ);  // Warn only once
                }
            } ورنہ اگر (!anyD && allU) {
                nodep->v3warn(UNDRIVEN, varType << " is not driven: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_رمز::UNDRIVEN, سچ);  // Warn only once
            } ورنہ اگر (!funcInout) {
                // Bits have different dispositions
                ثابت std::string var_Type_زیریں = [&varType]() {
                    std::string str = varType;
                    str[0] = std::tolower(static_cast<غیرعلامتی حرف>(str[0]));
                    واپس str;
                }();
                منطقی سیٹ_U = جھوٹ;
                منطقی سیٹ_D = جھوٹ;
                اگر (anynotDU && !غیر_مستعمل_مطابقت(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  "Bits of " << var_Type_زیریں << " are not driven, nor used: "
                                             << nodep->prettyNameQ() << ثنائی_ہندسہ_نام(BN_BOTH));
                    سیٹ_U = سچ;
                }
                اگر (anyDnotU && !غیر_مستعمل_مطابقت(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL, "Bits of " << var_Type_زیریں << " are not used: "
                                                           << nodep->prettyNameQ()
                                                           << ثنائی_ہندسہ_نام(BN_غیر_مستعمل));
                    سیٹ_U = سچ;
                }
                اگر (anyUnotD) {
                    nodep->v3warn(UNDRIVEN, "Bits of " << var_Type_زیریں << " are not driven: "
                                                       << nodep->prettyNameQ()
                                                       << ثنائی_ہندسہ_نام(BN_UNDRIVEN));
                    سیٹ_D = سچ;
                }
                اگر (سیٹ_U) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3_Error_رمز::UNUSEDSIGNAL, سچ);
                }
                اگر (سیٹ_D) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3_Error_رمز::UNDRIVEN, سچ);
                }
            }
        }
    }

    خالی driven_بذریعہ_Call(ثابت Ast_گرہ_FTask_Ref* nodep) {
        driven_پورا(nodep);
        اگر (!m_callNodep) m_callNodep = nodep;
    }
    ثابت Ast_گرہ_FTask_Ref* callNodep() ثابت { واپس m_callNodep; }
};

//######################################################################
// Undriven state, as a visitor of each AstNode

جماعت UndrivenVisitor final : عوامی VNVisitorConst {
    // NODE STATE
    // Netlist:
    //  AstVar::user1p          -> UndrivenVar* for usage var, 0=not set yet
    ثابت VNUser1_اندر_Use m_inuser1;
    // Each always:
    //  AstNode::user2p         -> UndrivenVar* for usage var, 0=not set yet
    ثابت VNUser2_اندر_Use m_inuser2;

    // STATE
    std::array<std::سمتیہ<Undriven_Var_اندراج*>, 3> m_entryps = {};  // Nodes to delete when finished
    منطقی m_اندر_BBox = جھوٹ;  // In black box; mark as driven+used
    منطقی m_اندر_Cont_تفویض = جھوٹ;  // In continuous assignment
    منطقی m_اندر_Initial = جھوٹ;  // In explicit initial block
    منطقی m_اندر_Initial_تیاری = جھوٹ;  // In InitialAutomatic*/InitialStatic* assignment LHS
    منطقی m_اندر_Initial_Static = جھوٹ;  // In InitialStatic
    منطقی m_اندر_Proc_تفویض = جھوٹ;  // In procedural assignment
    منطقی m_اندر_FTask_Ref = جھوٹ;  // In function or task call
    منطقی m_اندر_Inout_Or_Ref_Pin = جھوٹ;  // Connected to pin that is inout
    منطقی m_اندر_Sel_Lhs = جھوٹ;  // Iterating the fromp of an AstSel (a partial-bit write target)
    ثابت Ast_گرہ_FTask* m_taskp = خالی_اشاریہ;  // Current task
    ثابت AstAlways* m_alwaysp = خالی_اشاریہ;  // Current always of either type
    ثابت AstAlways* m_alwaysCombp = خالی_اشاریہ;  // Current always if combo, otherwise nullptr
    ثابت AstAlways* m_alwaysFFp = خالی_اشاریہ;  // Current always if ff, otherwise nullptr
    ثابت AstAlways* m_alwaysPlainp = خالی_اشاریہ;  // Current always if plain (not comb/ff/latch)
    ثابت AstClocking* m_clockingp = خالی_اشاریہ;  // Current clocking block, otherwise nullptr

    V3UndrivenCapture* ثابت m_capturep = خالی_اشاریہ;  // Capture object.  'nullptr' if disabled.

    // METHODS

    Undriven_Var_اندراج* getEntryp(AstVar* nodep, صحیح which_user) {
        اگر (!(which_user == 1 ? nodep->user1p() : nodep->user2p())) {
            Undriven_Var_اندراج* ثابت entryp = نیا Undriven_Var_اندراج{nodep};
            // UINFO(9," Associate u="<<which_user<<" "<<cvtToHex(this)<<" "<<nodep->name()<<endl);
            m_entryps[which_user].ڈالو_عقب(entryp);
            اگر (which_user == 1) {
                nodep->user1p(entryp);
            } ورنہ اگر (which_user == 2) {
                nodep->user2p(entryp);
            } ورنہ {
                nodep->v3fatalSrc("Bad case");
            }
            واپس entryp;
        } ورنہ {
            Undriven_Var_اندراج* ثابت entryp = reinterpret_cast<Undriven_Var_اندراج*>(
                which_user == 1 ? nodep->user1p() : nodep->user2p());
            واپس entryp;
        }
    }

    خالی warn_Alw_Comb_ترتیب(Ast_گرہ_Var_Ref* nodep, ثابت Ast_گرہ* readp) {
        AstVar* ثابت varp = nodep->varp();
        اگر (!varp->isParam() && !varp->isGenVar() && !varp->is_مستعمل_Loop_Idx()
            && !varp->ignoreSchedWrite()
            && !m_اندر_BBox  // We may have falsely considered a SysIgnore as a driver
            && !VN_IS(nodep, VarXRef)  // Xrefs might point at two different instances
            && !varp->fileline()->warnIsOff(
                V3_Error_رمز::ALWCOMBORDER)) {  // Warn only once per variable
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
            varp->fileline()->modifyWarnOff(V3_Error_رمز::ALWCOMBORDER,
                                            سچ);  // Complain just once for any usage
        }
    }
    // A clocking block 'output' is an additional driver of the signal it names.
    // The conflict is found from whichever driver is reached second.
    منطقی clockingDrivesOther(ثابت Undriven_Var_اندراج* entryp, منطقی otherWriteIsStaticInit) ثابت {
        ثابت منطقی otherIsExplicitProc
            = entryp->is_Driven_Always_Comb_پورا() || entryp->isDrivenAlwaysFFWhole();
        واپس m_clockingp && !otherIsExplicitProc && !otherWriteIsStaticInit
               && m_clockingp != entryp->getClockingp();
    }
    منطقی otherDrivesClocking(ثابت Undriven_Var_اندراج* entryp) ثابت {
        واپس !m_clockingp && !m_alwaysCombp && !m_alwaysFFp && !m_اندر_Initial_Static
               && entryp->is_Driven_Clocking_پورا();
    }
    // A continuous assignment contending with a clocking block 'output' is an
    // unambiguous driver conflict. Multiple clocking blocks may drive the same
    // signal, the last drive wins (IEEE 1800-2023 14.16.2).
    خالی warnClockingDriven(Ast_گرہ_Var_Ref* nodep, ثابت Undriven_Var_اندراج* entryp,
                            ثابت Ast_گرہ* otherWritep, منطقی otherWriteIsStaticInit) {
        اگر (clockingDrivesOther(entryp, otherWriteIsStaticInit)
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
        اگر (otherDrivesClocking(entryp) && m_اندر_Cont_تفویض) {
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
    خالی warnClockingDrivenProc(Ast_گرہ_Var_Ref* nodep, ثابت Undriven_Var_اندراج* entryp,
                                ثابت Ast_گرہ* otherWritep, منطقی otherWriteIsStaticInit) {
        اگر (clockingDrivesOther(entryp, otherWriteIsStaticInit) && !entryp->getClockingp()
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
        اگر (otherDrivesClocking(entryp) && !m_اندر_Cont_تفویض) {
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
    خالی دورہ(AstVar* nodep) override {
        ثابت منطقی funcInout = nodep->isFuncLocal() && nodep->isInout();
        برائے (صحیح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            // For assigns and non-combo always, do just usr==1, to look
            // for module-wide undriven etc.
            // For combo always, run both usr==1 for above, and also
            // usr==2 for always-only checks.
            Undriven_Var_اندراج* ثابت entryp = getEntryp(nodep, usr);
            اگر ((nodep->isNonOutput() && !funcInout) || nodep->is_Sig_عوامی()
                || nodep->hasUserInit() || nodep->isSigUserRWPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->driven_پورا(nodep);
            }
            اگر ((nodep->isWritable() && !funcInout) || nodep->is_Sig_عوامی()
                || nodep->isSigUserRWPublic() || nodep->is_Sig_User_Rd_عوامی()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->مستعمل_پورا(nodep);
            }
            اگر (nodep->valuep()) entryp->driven_پورا(nodep->valuep());
        }
        // Discover variables used in bit definitions, etc
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstArraySel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_قاش_Sel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstSel* nodep) override {
        Ast_گرہ_Var_Ref* ثابت varrefp = VN_CAST(nodep->fromp(), گرہ_Var_Ref);
        AstConst* ثابت constp = VN_CAST(nodep->lsbp(), Const);
        اگر (varrefp && constp && !constp->عدد().isFourState()) {
            برائے (صحیح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                Undriven_Var_اندراج* ثابت entryp = getEntryp(varrefp->varp(), usr);
                ثابت صحیح lsb = constp->مقصد_UInt();
                اگر (m_اندر_BBox || varrefp->access().isWriteOrRW()) {
                    // Don't warn if already driven earlier as "a=0; if(a) a=1;" is fine.
                    اگر (usr == 2 && m_alwaysCombp
                        && entryp->is_مستعمل_Not_Driven_ثنائی_ہندسہ(lsb, nodep->چوڑائی())) {
                        UINFO(9, " Select.  Entryp=" << cvt_مقصد_Hex(entryp));
                        warn_Alw_Comb_ترتیب(varrefp, entryp->پہلا_مستعمل_Not_Drivenp());
                    }
                    entryp->driven_ثنائی_ہندسہ(lsb, nodep->چوڑائی());
                }
                اگر (m_اندر_BBox || !varrefp->access().isWriteOrRW())
                    entryp->مستعمل_ثنائی_ہندسہ_2(lsb, nodep->چوڑائی(), varrefp);
            }
        } ورنہ {
            // skip over static longest static prefix
            iterateConst(nodep->lsbp());
            VL_RESTORER(m_اندر_Sel_Lhs);
            m_اندر_Sel_Lhs = !V3_چوڑائی::selectNonConstantRecurse(nodep->lsbp(), /*inSel=*/سچ);
            iterateConst(nodep->fromp());
        }
    }
    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
        // Any variable
        اگر (nodep->access().isWriteOrRW()
            && !VN_IS(nodep, VarXRef)) {  // Ignore interface variables and similar ugly items
            اگر (m_اندر_Proc_تفویض && !nodep->varp()->varType().isProcAssignable()
                && !nodep->varp()->isDeclTyped()  //
                && !nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()) {
                nodep->v3warn(PROCASSWIRE, "Procedural assignment to wire, perhaps intended var"
                                               << " (IEEE 1800-2023 6.5): "
                                               << nodep->prettyNameQ());
            } ورنہ اگر (m_اندر_Cont_تفویض && !nodep->varp()->varType().isContAssignable()
                       && !nodep->fileline()->language().systemVerilog()) {
                nodep->v3warn(CONTASSREG,
                              "Continuous assignment to reg, perhaps intended wire"
                                  << " (IEEE 1364-2005 6.1; Verilog only, legal in SV): "
                                  << nodep->prettyNameQ());
            }
            اگر (m_اندر_FTask_Ref && nodep->varp()->varType().isNet()) {
                nodep->v3warn(
                    PROCASSWIRE,
                    "Passed wire on output or inout subroutine argument, expected expression that "
                    "is valid on the left hand side of a procedural assignment"
                        << " (IEEE 1800-2023 13.5): " << nodep->prettyNameQ());
            }
        }

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Remember that anything driven here doesn't count toward MULTIDRIVEN.
        منطقی ftaskDef = جھوٹ;
        اگر (m_taskp && !m_alwaysp && !m_اندر_Cont_تفویض && !m_اندر_Initial_Static && !m_اندر_BBox
            && !m_taskp->dpiExport()) {
            AstVar* ثابت retVarp = VN_CAST(m_taskp->fvarp(), Var);
            اگر (!retVarp || nodep->varp() != retVarp) ftaskDef = سچ;
        }

        برائے (صحیح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            Undriven_Var_اندراج* ثابت entryp = getEntryp(nodep->varp(), usr);
            ثابت منطقی fdrv = nodep->access().isWriteOrRW()
                              && nodep->varp()->attrFileDescr();  // FD's are also being read from
            اگر (m_اندر_BBox || nodep->access().isWriteOrRW()) {
                اگر (usr == 2 && m_alwaysCombp && entryp->is_مستعمل_Not_Driven_Any()) {
                    UINFO(9, " Full bus.  Entryp=" << cvt_مقصد_Hex(entryp));
                    warn_Alw_Comb_ترتیب(nodep, entryp->پہلا_مستعمل_Not_Drivenp());
                }
                ثابت Ast_گرہ_Var_Ref* ثابت otherVarRefp = entryp->getNodep();
                ثابت Ast_گرہ* ثابت otherWritep = otherVarRefp
                                                       ? static_cast<ثابت Ast_گرہ*>(otherVarRefp)
                                                       : entryp->callNodep();
                ثابت منطقی same_File_سطر
                    = otherVarRefp && nodep->fileline() == otherVarRefp->fileline();
                // Preconditions shared by MULTIDRIVEN and MULTIDRIVENPROC.
                ثابت منطقی multidrivenCommon
                    = entryp->is_Driven_پورا() && !m_اندر_BBox && !VN_IS(nodep, VarXRef)
                      && !VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType) && !same_File_سطر
                      && !entryp->isUnderGen() && otherWritep && !entryp->isFtaskDriven()
                      && !ftaskDef && !m_اندر_Sel_Lhs;
                // The two warnings are gated independently on the variable
                // declaration's fileline, as v3warn suppression will check
                // the driving fileline and still warn even if the warning
                // was suppressed with lint_off at the declaration.
                ثابت منطقی otherWriteIsStaticInit
                    = nodep->varp()->hasUserInit() && otherWritep == entryp->initStaticp();
                اگر (multidrivenCommon
                    && !nodep->varp()->fileline()->warnIsOff(V3_Error_رمز::MULTIDRIVEN)) {
                    اگر (m_alwaysCombp
                        && (!entryp->is_Driven_Always_Comb_پورا()
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
                    اگر (!m_alwaysCombp && entryp->is_Driven_Always_Comb_پورا()) {
                        nodep->v3warn(MULTIDRIVEN, "Variable also written to in always_comb"
                                                       << " (IEEE 1800-2023 9.2.2.2): "
                                                       << nodep->prettyNameQ() << '\n'
                                                       << nodep->warnOther() << '\n'
                                                       << nodep->warnContextPrimary() << '\n'
                                                       << otherWritep->warnOther()
                                                       << "... Location of always_comb write\n"
                                                       << otherWritep->warnContextSecondary());
                    }
                    اگر (m_alwaysFFp && !otherWriteIsStaticInit
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
                    اگر (!m_alwaysFFp && !m_اندر_Initial_Static && entryp->isDrivenAlwaysFFWhole()) {
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
                اگر (multidrivenCommon && !nodep->varp()->is_مستعمل_Loop_Idx()
                    && !nodep->varp()->fileline()->warnIsOff(V3_Error_رمز::MULTIDRIVENPROC)) {
                    // Two plain always blocks driving the whole signal: legal
                    // SystemVerilog, but a driver conflict for synthesis. The
                    // always_ff/always_comb cases above already cover mixes with
                    // an explicit process, so only warn for plain+plain here.
                    // When the two blocks are clocked differently, the
                    // (on-by-default) MULTIDRIVEN check in V3Delayed already reports
                    // the conflict, so don't also emit MULTIDRIVENPROC for that case.
                    ثابت AstAlways* ثابت otherAlwaysp = entryp->getAlwPlainp();
                    ثابت Ast_Sen_درخت* ثابت senp
                        = m_alwaysPlainp ? m_alwaysPlainp->sentreep() : خالی_اشاریہ;
                    ثابت Ast_Sen_درخت* ثابت otherSenp
                        = otherAlwaysp ? otherAlwaysp->sentreep() : خالی_اشاریہ;
                    ثابت منطقی differentClocking = senp && otherSenp && senp->hasClocked()
                                                   && otherSenp->hasClocked()
                                                   && !senp->same_درخت(otherSenp);
                    اگر (m_alwaysPlainp && entryp->is_Driven_Always_Plain_پورا()
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
                اگر (!m_اندر_Initial_تیاری || nodep->varp()->hasUserInit()) {
                    // Else don't count default initialization as a driver to a net/variable
                    entryp->driven_پورا(nodep, ftaskDef);
                }
                اگر (m_alwaysCombp && entryp->is_Driven_Always_Comb_پورا()
                    && m_alwaysCombp != entryp->getAlwCombp()
                    && m_alwaysCombp->fileline() == entryp->getAlwCombp()->fileline())
                    entryp->underGenerate();
                اگر (m_alwaysCombp) entryp->driven_Always_Comb_پورا(m_alwaysCombp);
                اگر (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, nodep->varp());
                اگر (m_alwaysPlainp) entryp->driven_Always_Plain_پورا(m_alwaysPlainp);
                اگر (m_clockingp) entryp->driven_Clocking_پورا(m_clockingp);
            }
            اگر (nodep->access().isWriteOrRW() && !VN_IS(nodep, VarXRef)) {
                // Ignoring xrefs as the initial and assignment to track might refer to two
                // different instances.  Ideally all of V3Undriven would move after V3Scope,
                // then could use VarScope tracking instead.
                اگر (m_اندر_Initial_Static && !entryp->initStaticp()) entryp->initStaticp(nodep);
                اگر (m_اندر_Initial && !entryp->initialp()) entryp->initialp(nodep);
                اگر (m_اندر_Cont_تفویض && !entryp->contAssignp()) entryp->contAssignp(nodep);
                اگر (m_alwaysp && m_اندر_Proc_تفویض && !entryp->procWritep())
                    entryp->procWritep(nodep);
            }
            اگر ((!m_اندر_Initial_تیاری || nodep->varp()->hasUserInit())
                && (m_اندر_BBox || nodep->access().isReadOrRW()
                    || fdrv
                    // Inouts have only isWrite set, as we don't have more
                    // information and operating on module boundary, treat as
                    // both read and writing
                    || m_اندر_Inout_Or_Ref_Pin))
                entryp->مستعمل_پورا(nodep);
        }
    }

    // Don't know what black boxed calls do, assume in+out
    خالی دورہ(AstSysIgnore* nodep) override {
        VL_RESTORER(m_اندر_BBox);
        m_اندر_BBox = سچ;
        iterateChildrenConst(nodep);
    }

    خالی دورہ(Ast_تفویض* nodep) override {
        VL_RESTORER(m_اندر_Proc_تفویض);
        m_اندر_Proc_تفویض = سچ;
        {
            VL_RESTORER(m_اندر_Initial_تیاری);
            m_اندر_Initial_تیاری = جھوٹ;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }
    خالی دورہ(Ast_تفویض_Dly* nodep) override {
        VL_RESTORER(m_اندر_Proc_تفویض);
        m_اندر_Proc_تفویض = سچ;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_تفویض_Force* nodep) override {
        iterateConst(nodep->rhsp());
        VL_RESTORER(m_اندر_Initial);
        m_اندر_Initial = جھوٹ;
        iterateConst(nodep->lhsp());
    }
    خالی دورہ(Ast_تفویض_W* nodep) override {
        VL_RESTORER(m_اندر_Cont_تفویض);
        m_اندر_Cont_تفویض = سچ;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstRelease* nodep) override {
        VL_RESTORER(m_اندر_Initial);
        m_اندر_Initial = جھوٹ;
        iterateConst(nodep->lhsp());
    }
    خالی دورہ(AstInitial* nodep) override {
        VL_RESTORER(m_اندر_Initial);
        m_اندر_Initial = سچ;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstInitialAutomatic* nodep) override {
        VL_RESTORER(m_اندر_Initial_تیاری);
        m_اندر_Initial_تیاری = سچ;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_اندر_Initial_تیاری);
        m_اندر_Initial_تیاری = سچ;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstInitialStatic* nodep) override {
        VL_RESTORER(m_اندر_Initial_Static);
        m_اندر_Initial_Static = سچ;
        VL_RESTORER(m_اندر_Initial_تیاری);
        m_اندر_Initial_تیاری = سچ;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_اندر_Initial_تیاری);
        m_اندر_Initial_تیاری = سچ;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        VL_RESTORER(m_alwaysCombp);
        VL_RESTORER(m_alwaysFFp);
        VL_RESTORER(m_alwaysPlainp);
        Ast_گرہ::user2_Clear_درخت();
        m_alwaysp = nodep;
        اگر (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            UINFO(9, "   " << nodep);
            m_alwaysCombp = nodep;
        } ورنہ {
            m_alwaysCombp = خالی_اشاریہ;
        }
        m_alwaysFFp = nodep->keyword() == VAlwaysKwd::ALWAYS_FF ? nodep : خالی_اشاریہ;
        m_alwaysPlainp = nodep->keyword() == VAlwaysKwd::ALWAYS ? nodep : خالی_اشاریہ;
        iterateChildrenConst(nodep);
        اگر (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) UINFO(9, "   Done " << nodep);
    }
    خالی دورہ(AstClocking* nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterateChildrenConst(nodep);
    }

    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        VL_RESTORER(m_اندر_FTask_Ref);
        m_اندر_FTask_Ref = سچ;

        iterateChildrenConst(nodep);

        اگر (!m_capturep) واپس;

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Do not apply writeSummary at calls inside a task definition, or they will look like
        // independent drivers (phantom MULTIDRIVEN).
        ثابت منطقی اندر_Executed_Context
            = !(m_taskp && !m_alwaysp && !m_اندر_Cont_تفویض && !m_اندر_Initial_Static && !m_اندر_BBox
                && !m_taskp->dpiExport());

        اگر (!اندر_Executed_Context) واپس;

        Ast_گرہ_FTask* ثابت calleep = nodep->taskp();
        اگر (!calleep) واپس;

        ثابت خودکار& vars = m_capturep->writeSummary(calleep);
        برائے (AstVar* ثابت varp : vars) {
            برائے (صحیح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                Undriven_Var_اندراج* ثابت entryp = getEntryp(varp, usr);
                entryp->driven_بذریعہ_Call(nodep);
                اگر (m_alwaysCombp) entryp->driven_Always_Comb_پورا(m_alwaysCombp);
                اگر (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, varp);
            }
        }
    }

    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_taskp);
        m_taskp = nodep;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstPin* nodep) override {
        VL_RESTORER(m_اندر_Inout_Or_Ref_Pin);
        m_اندر_Inout_Or_Ref_Pin = nodep->modVarp()->isInoutOrRef();
        iterateChildrenConst(nodep);
    }

    // Until we support tables, primitives will have undriven and unused I/Os
    خالی دورہ(AstPrimitive*) override {}

    // Coverage artifacts etc shouldn't count as a sink
    خالی دورہ(Ast_گرہ_Cover_Decl*) override {}
    خالی دورہ(AstCoverInc*) override {}
    خالی دورہ(AstCoverToggle*) override {}
    خالی دورہ(AstTraceDecl* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    خالی دورہ(AstTraceInc* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }

    // iterate
    خالی دورہ(AstConst* nodep) override {}
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح UndrivenVisitor(AstNetlist* nodep, V3UndrivenCapture* capturep)
        : m_capturep{capturep} {
        iterateConst(nodep);
    }

    ~UndrivenVisitor() override {
        برائے (Undriven_Var_اندراج* ip : m_entryps[1]) ip->reportViolations();
        برائے (صحیح usr = 1; usr < 3; ++usr) {
            برائے (Undriven_Var_اندراج* ip : m_entryps[usr]) حذف ip;
        }
    }
};

//######################################################################
// Undriven class functions

خالی V3Undriven::undrivenAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    V3UndrivenCapture capture{nodep};
    { UndrivenVisitor{nodep, &capture}; }

    اگر (v3Global.opt.stats()) V3Stats::statsStage("undriven");
}
