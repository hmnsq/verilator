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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Undriven.h"

#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3UndrivenCapture.h"
#అన్తర్భావ "V3Width.h"

#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Class for every variable we may process

తరగతి Undriven_Var_నమోదు final {
    // MEMBERS
    AstVar* స్థిరరాశి m_varp;  // Variable this tracks
    std::సదిశ<తార్కిక> m_సంపూర్ణ_Flags;  // Used/Driven on whole vector
    std::సదిశ<తార్కిక> m_బిట్టు_Flags;  // Used/Driven on each subbit
    స్థిరరాశి Ast_కణుపు* m_వాడిన_Not_Drivenp = శూన్యసూచిక;  // First read before any write
    స్థిరరాశి AstAlways* m_alwCombp
        = శూన్యసూచిక;  // always_comb of var if driven within always_comb, else nullptr
    స్థిరరాశి AstAlways* m_alwFFp = శూన్యసూచిక;  // always_ff of var if driven within always_ff
    స్థిరరాశి AstAlways* m_alwPlainp = శూన్యసూచిక;  // plain always of var if driven within plain always
    స్థిరరాశి AstClocking* m_clockingp = శూన్యసూచిక;  // clocking block of var if driven as output
    స్థిరరాశి Ast_కణుపు_Var_Ref* m_nodep = శూన్యసూచిక;  // varref if driven, else nullptr
    స్థిరరాశి Ast_కణుపు* m_initStaticp = శూన్యసూచిక;  // varref if in InitialStatic driven
    స్థిరరాశి Ast_కణుపు* m_initialp = శూన్యసూచిక;  // varref if driven in an explicit initial block
    స్థిరరాశి Ast_కణుపు* m_contAssignp = శూన్యసూచిక;  // varref if in continuous assignment driven
    స్థిరరాశి Ast_కణుపు* m_procWritep = శూన్యసూచిక;  // varref if written in process
    తార్కిక m_underGen = అబద్ధం;  // Under a generate
    తార్కిక m_ftaskDriven = అబద్ధం;  // Last driven by function or task

    స్థిరరాశి Ast_కణుపు_FTask_Ref* m_callNodep = శూన్యసూచిక;  // Call node if driven via writeSummary

    గణన : uint8_t {
        FLAG_వాడిన = 0,  // Signal or bit has been read/observed
        FLAG_DRIVEN = 1,  // Signal or bit has been written/driven
        FLAG_DRIVEN_ALWCOMB = 2,  // Whole signal has been driven from always_comb
        FLAG_DRIVEN_ALWFF = 3,  // Whole signal has been driven from always_ff
        FLAG_DRIVEN_ALWPLAIN = 4,  // Whole signal has been driven from a plain always
        FLAG_DRIVEN_CLOCKING = 5,  // Whole signal has been driven as a clocking block output
        FLAGS_చొప్పున_బిట్టు = 6  // Number of flags stored for each tracked bit
    };

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన Undriven_Var_నమోదు(AstVar* varp)
        : m_varp{varp} {  // Construction for when a var is used
        UINFO(9, "create " << varp);
        m_సంపూర్ణ_Flags.resize(FLAGS_చొప్పున_బిట్టు);
        కొరకు (పూర్ణాంకం i = 0; i < FLAGS_చొప్పున_బిట్టు; i++) m_సంపూర్ణ_Flags[i] = అబద్ధం;
        m_బిట్టు_Flags.resize(varp->వెడల్పు() * FLAGS_చొప్పున_బిట్టు);
        కొరకు (పూర్ణాంకం i = 0; i < varp->వెడల్పు() * FLAGS_చొప్పున_బిట్టు; i++) m_బిట్టు_Flags[i] = అబద్ధం;
    }
    ~Undriven_Var_నమోదు() = అప్రమేయ;

అంతరంగ:
    // METHODS
    తార్కిక బిట్టు_అంకె_Ok(పూర్ణాంకం బిట్టు_2) స్థిరరాశి {
        తిరిగి బిట్టు_2 >= 0 && (బిట్టు_2 * FLAGS_చొప్పున_బిట్టు < static_cast<పూర్ణాంకం>(m_బిట్టు_Flags.కొలత()));
    }
    తార్కిక వాడిన_Flag(పూర్ణాంకం బిట్టు_2) స్థిరరాశి {
        తిరిగి m_సంపూర్ణ_Flags[FLAG_వాడిన] || m_బిట్టు_Flags[బిట్టు_2 * FLAGS_చొప్పున_బిట్టు + FLAG_వాడిన];
    }
    తార్కిక drivenFlag(పూర్ణాంకం బిట్టు_2) స్థిరరాశి {
        తిరిగి m_సంపూర్ణ_Flags[FLAG_DRIVEN] || m_బిట్టు_Flags[బిట్టు_2 * FLAGS_చొప్పున_బిట్టు + FLAG_DRIVEN];
    }
    పూర్ణాంకం బిట్టు_లెక్క() స్థిరరాశి { తిరిగి m_బిట్టు_Flags.కొలత() / FLAGS_చొప్పున_బిట్టు; }
    శూన్య రికార్డు_వాడిన_Not_Driven(స్థిరరాశి Ast_కణుపు* nodep) {
        ఒకవేళ (!m_వాడిన_Not_Drivenp) m_వాడిన_Not_Drivenp = nodep;
    }
    గణన బిట్టు_పేర్లు_Which : uint8_t { BN_వాడని, BN_UNDRIVEN, BN_BOTH };
    string బిట్టు_పేర్లు(బిట్టు_పేర్లు_Which which) {
        string బిట్లు;
        తార్కిక prev = అబద్ధం;
        పూర్ణాంకం msb = 0;
        // bit==-1 loops below; we do one extra iteration so end with prev=false
        కొరకు (పూర్ణాంకం బిట్టు_2 = (m_బిట్టు_Flags.కొలత() / FLAGS_చొప్పున_బిట్టు) - 1; బిట్టు_2 >= -1; --బిట్టు_2) {
            ఒకవేళ (బిట్టు_2 >= 0
                && ((which == BN_వాడని && !వాడిన_Flag(బిట్టు_2) && drivenFlag(బిట్టు_2))
                    || (which == BN_UNDRIVEN && వాడిన_Flag(బిట్టు_2) && !drivenFlag(బిట్టు_2))
                    || (which == BN_BOTH && !వాడిన_Flag(బిట్టు_2) && !drivenFlag(బిట్టు_2)))) {
                ఒకవేళ (!prev) {
                    prev = నిజం;
                    msb = బిట్టు_2;
                }
            } లేకపోతే ఒకవేళ (prev) {
                స్థిరరాశి AstBasicDType* స్థిరరాశి bdtypep = m_varp->basicp();
                స్థిరరాశి పూర్ణాంకం lsb = బిట్టు_2 + 1;
                ఒకవేళ (బిట్లు != "") బిట్లు += ",";
                ఒకవేళ (lsb == msb) {
                    బిట్లు += cvt_గమ్యం_Str(lsb + bdtypep->lo());
                } లేకపోతే {
                    ఒకవేళ (bdtypep->ascending()) {
                        బిట్లు
                            += cvt_గమ్యం_Str(lsb + bdtypep->lo()) + ":" + cvt_గమ్యం_Str(msb + bdtypep->lo());
                    } లేకపోతే {
                        బిట్లు
                            += cvt_గమ్యం_Str(msb + bdtypep->lo()) + ":" + cvt_గమ్యం_Str(lsb + bdtypep->lo());
                    }
                }
                prev = అబద్ధం;
            }
        }
        తిరిగి "[" + బిట్లు + "]";
    }

బహిరంగ:
    శూన్య వాడిన_సంపూర్ణ(స్థిరరాశి Ast_కణుపు* nodep) {
        UINFO(9, "set u[*] " << m_varp->name() << " " << nodep);
        ఒకవేళ (!m_సంపూర్ణ_Flags[FLAG_DRIVEN]) {
            కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < బిట్టు_లెక్క(); ++బిట్టు_2) {
                ఒకవేళ (!drivenFlag(బిట్టు_2)) {
                    రికార్డు_వాడిన_Not_Driven(nodep);
                    ఆపు;
                }
            }
        }
        m_సంపూర్ణ_Flags[FLAG_వాడిన] = నిజం;
    }
    శూన్య driven_సంపూర్ణ(స్థిరరాశి Ast_కణుపు* nodep) {
        UINFO(9, "set d[*] " << m_varp->name() << " " << nodep);
        m_సంపూర్ణ_Flags[FLAG_DRIVEN] = నిజం;
    }
    శూన్య driven_సంపూర్ణ(స్థిరరాశి Ast_కణుపు_Var_Ref* nodep, తార్కిక ftaskDef) {
        స్థిరరాశి తార్కిక wasDriven = is_Driven_సంపూర్ణ();
        driven_సంపూర్ణ(nodep);
        // A write in a task/function definition is not an executed process, so it must not
        // displace an already recorded write, nor clear the flag that keeps such a write
        // from being reported as the other write of a MULTIDRIVEN
        ఒకవేళ (ftaskDef && wasDriven) తిరిగి;
        m_ftaskDriven = ftaskDef;
        m_nodep = nodep;
    }
    శూన్య driven_Always_Comb_సంపూర్ణ(స్థిరరాశి AstAlways* alwCombp) {
        m_సంపూర్ణ_Flags[FLAG_DRIVEN_ALWCOMB] = నిజం;
        m_alwCombp = alwCombp;
    }
    శూన్య drivenAlwaysFFWhole(స్థిరరాశి AstAlways* alwFFp, స్థిరరాశి AstVar* varp) {
        m_సంపూర్ణ_Flags[FLAG_DRIVEN_ALWFF] = నిజం;
        m_alwFFp = alwFFp;
    }
    శూన్య driven_Always_Plain_సంపూర్ణ(స్థిరరాశి AstAlways* alwPlainp) {
        m_సంపూర్ణ_Flags[FLAG_DRIVEN_ALWPLAIN] = నిజం;
        m_alwPlainp = alwPlainp;
    }
    శూన్య driven_Clocking_సంపూర్ణ(స్థిరరాశి AstClocking* clockingp) {
        m_సంపూర్ణ_Flags[FLAG_DRIVEN_CLOCKING] = నిజం;
        m_clockingp = clockingp;
    }

    స్థిరరాశి Ast_కణుపు* initStaticp() స్థిరరాశి { తిరిగి m_initStaticp; }
    శూన్య initStaticp(స్థిరరాశి Ast_కణుపు* nodep) { m_initStaticp = nodep; }
    స్థిరరాశి Ast_కణుపు* initialp() స్థిరరాశి { తిరిగి m_initialp; }
    శూన్య initialp(స్థిరరాశి Ast_కణుపు* nodep) { m_initialp = nodep; }
    స్థిరరాశి Ast_కణుపు* contAssignp() స్థిరరాశి { తిరిగి m_contAssignp; }
    శూన్య contAssignp(స్థిరరాశి Ast_కణుపు* nodep) { m_contAssignp = nodep; }
    స్థిరరాశి Ast_కణుపు* procWritep() స్థిరరాశి { తిరిగి m_procWritep; }
    శూన్య procWritep(స్థిరరాశి Ast_కణుపు* nodep) { m_procWritep = nodep; }
    శూన్య underGenerate() { m_underGen = నిజం; }
    తార్కిక isUnderGen() స్థిరరాశి { తిరిగి m_underGen; }
    తార్కిక is_Driven_సంపూర్ణ() స్థిరరాశి { తిరిగి m_సంపూర్ణ_Flags[FLAG_DRIVEN]; }
    తార్కిక is_Driven_Always_Comb_సంపూర్ణ() స్థిరరాశి { తిరిగి m_సంపూర్ణ_Flags[FLAG_DRIVEN_ALWCOMB]; }
    తార్కిక isDrivenAlwaysFFWhole() స్థిరరాశి { తిరిగి m_సంపూర్ణ_Flags[FLAG_DRIVEN_ALWFF]; }
    తార్కిక is_Driven_Always_Plain_సంపూర్ణ() స్థిరరాశి { తిరిగి m_సంపూర్ణ_Flags[FLAG_DRIVEN_ALWPLAIN]; }
    తార్కిక is_Driven_Clocking_సంపూర్ణ() స్థిరరాశి { తిరిగి m_సంపూర్ణ_Flags[FLAG_DRIVEN_CLOCKING]; }
    తార్కిక isFtaskDriven() స్థిరరాశి { తిరిగి m_ftaskDriven; }
    స్థిరరాశి Ast_కణుపు_Var_Ref* getNodep() స్థిరరాశి { తిరిగి m_nodep; }
    స్థిరరాశి AstAlways* getAlwCombp() స్థిరరాశి { తిరిగి m_alwCombp; }
    స్థిరరాశి AstAlways* getAlwFFp() స్థిరరాశి { తిరిగి m_alwFFp; }
    స్థిరరాశి AstAlways* getAlwPlainp() స్థిరరాశి { తిరిగి m_alwPlainp; }
    స్థిరరాశి AstClocking* getClockingp() స్థిరరాశి { తిరిగి m_clockingp; }
    శూన్య వాడిన_బిట్టు_2(పూర్ణాంకం బిట్టు_2, పూర్ణాంకం వెడల్పు, స్థిరరాశి Ast_కణుపు* nodep) {
        UINFO(9, "set u[" << (బిట్టు_2 + వెడల్పు - 1) << ":" << బిట్టు_2 << "] " << m_varp->name());
        కొరకు (పూర్ణాంకం i = 0; i < వెడల్పు; i++) {
            ఒకవేళ (బిట్టు_అంకె_Ok(బిట్టు_2 + i)) {
                ఒకవేళ (!drivenFlag(బిట్టు_2 + i)) రికార్డు_వాడిన_Not_Driven(nodep);
                m_బిట్టు_Flags[(బిట్టు_2 + i) * FLAGS_చొప్పున_బిట్టు + FLAG_వాడిన] = నిజం;
            }
        }
    }
    శూన్య driven_బిట్టు(పూర్ణాంకం బిట్టు_2, పూర్ణాంకం వెడల్పు) {
        UINFO(9, "set d[" << (బిట్టు_2 + వెడల్పు - 1) << ":" << బిట్టు_2 << "] " << m_varp->name());
        కొరకు (పూర్ణాంకం i = 0; i < వెడల్పు; i++) {
            ఒకవేళ (బిట్టు_అంకె_Ok(బిట్టు_2 + i)) m_బిట్టు_Flags[(బిట్టు_2 + i) * FLAGS_చొప్పున_బిట్టు + FLAG_DRIVEN] = నిజం;
        }
    }
    తార్కిక is_వాడిన_Not_Driven_బిట్టు(పూర్ణాంకం బిట్టు_2, పూర్ణాంకం వెడల్పు) స్థిరరాశి {
        కొరకు (పూర్ణాంకం i = 0; i < వెడల్పు; i++) {
            ఒకవేళ (బిట్టు_అంకె_Ok(బిట్టు_2 + i)
                && (m_సంపూర్ణ_Flags[FLAG_వాడిన] || m_బిట్టు_Flags[(బిట్టు_2 + i) * FLAGS_చొప్పున_బిట్టు + FLAG_వాడిన])
                && !(m_సంపూర్ణ_Flags[FLAG_DRIVEN]
                     || m_బిట్టు_Flags[(బిట్టు_2 + i) * FLAGS_చొప్పున_బిట్టు + FLAG_DRIVEN]))
                తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }
    తార్కిక is_వాడిన_Not_Driven_Any() స్థిరరాశి {
        తిరిగి is_వాడిన_Not_Driven_బిట్టు(0, m_బిట్టు_Flags.కొలత() / FLAGS_చొప్పున_బిట్టు);
    }
    స్థిరరాశి Ast_కణుపు* మొదటి_వాడిన_Not_Drivenp() స్థిరరాశి { తిరిగి m_వాడిన_Not_Drivenp; }
    స్థిర తార్కిక వాడని_సరిపోలిక(AstVar* nodep) {
        స్థిరరాశి string regexp = v3Global.opt.వాడని_Regexp();
        ఒకవేళ (regexp == "") తిరిగి అబద్ధం;
        స్థిరరాశి string prettyName = nodep->prettyName();
        తిరిగి VString::wildmatch(prettyName.c_str(), regexp.c_str());
    }
    శూన్య reportViolations() {
        // Combine bits into overall state
        AstVar* స్థిరరాశి nodep = m_varp;

        ఒకవేళ (initStaticp() && procWritep() && nodep->hasUserInit() && !nodep->isClassMember()
            && !nodep->isFuncLocal()) {
            initStaticp()->v3warn(
                PROCASSINIT,
                "Procedural assignment to declaration with initial value: "
                    << nodep->prettyNameQ() << '\n'
                    << initStaticp()->warn_మరిన్ని() << "... Location of variable initialization\n"
                    << initStaticp()->warnContextPrimary() << '\n'
                    << procWritep()->warnOther() << "... Location of variable process write\n"
                    << procWritep()->warn_మరిన్ని()
                    << "... Perhaps should initialize instead using a reset in this process\n"
                    << procWritep()->warnContextSecondary());
        }
        స్థిరరాశి Ast_కణుపు* స్థిరరాశి initp = nodep->hasUserInit() ? initStaticp() : initialp();
        ఒకవేళ (initp && contAssignp() && !nodep->isClassMember() && !nodep->isFuncLocal()) {
            initp->v3warn(E_CONTASSINIT, "Continuous assignment to variable with initial value: "
                                             << nodep->prettyNameQ() << '\n'
                                             << initp->warn_మరిన్ని()
                                             << "... Location of variable initialization\n"
                                             << initp->warnContextPrimary() << '\n'
                                             << contAssignp()->warnOther()
                                             << "... Location of continuous assignment\n"
                                             << contAssignp()->warnContextSecondary());
        }
        ఒకవేళ (nodep->isGenVar()) {  // Genvar
            ఒకవేళ (!nodep->isIfaceRef() && !nodep->is_వాడిన_Param() && !వాడని_సరిపోలిక(nodep)) {
                nodep->v3warn(UNUSEDGENVAR, "Genvar is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_సంకేతం::UNUSEDGENVAR,
                                                 నిజం);  // Warn only once
            }
        } లేకపోతే ఒకవేళ (nodep->isParam()) {  // Parameter
            ఒకవేళ (!nodep->isIfaceRef() && !nodep->is_వాడిన_Param() && !వాడని_సరిపోలిక(nodep)) {
                nodep->v3warn(UNUSEDPARAM, "Parameter is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_సంకేతం::UNUSEDPARAM,
                                                 నిజం);  // Warn only once
            }
        } లేకపోతే {  // Signal
            స్థిరరాశి string varType{nodep->isFuncLocal() ? "Function variable" : "Signal"};
            స్థిరరాశి తార్కిక funcInout = nodep->isFuncLocal() && nodep->isInout();
            తార్కిక allU = నిజం;
            తార్కిక allD = నిజం;
            తార్కిక anyU = m_సంపూర్ణ_Flags[FLAG_వాడిన];
            తార్కిక anyD = m_సంపూర్ణ_Flags[FLAG_DRIVEN];
            తార్కిక anyUnotD = అబద్ధం;
            తార్కిక anyDnotU = అబద్ధం;
            తార్కిక anynotDU = అబద్ధం;
            కొరకు (గుర్తులేని బిట్టు_2 = 0; బిట్టు_2 < m_బిట్టు_Flags.కొలత() / FLAGS_చొప్పున_బిట్టు; బిట్టు_2++) {
                స్థిరరాశి తార్కిక వాడిన = వాడిన_Flag(బిట్టు_2);
                స్థిరరాశి తార్కిక driv = drivenFlag(బిట్టు_2);
                allU &= వాడిన;
                anyU |= వాడిన;
                allD &= driv;
                anyD |= driv;
                anyUnotD |= వాడిన && !driv;
                anyDnotU |= !వాడిన && driv;
                anynotDU |= !వాడిన && !driv;
            }
            ఒకవేళ (funcInout) {
                ఒకవేళ (anyD) allU = నిజం;
                allD = నిజం;
            }
            ఒకవేళ (allU) m_సంపూర్ణ_Flags[FLAG_వాడిన] = నిజం;
            ఒకవేళ (allD) m_సంపూర్ణ_Flags[FLAG_DRIVEN] = నిజం;
            // Test results
            ఒకవేళ (nodep->isIfaceRef()) {
                // For interface top level we don't do any tracking
                // Ideally we'd report unused instance cells, but presumably a signal inside one
                // would get reported as unused
            } లేకపోతే ఒకవేళ (allU && allD) {
                // It's fine
            } లేకపోతే ఒకవేళ (!anyD && !anyU) {
                // UNDRIVEN is considered more serious - as is more likely a bug,
                // thus undriven+unused bits get UNUSED warnings, as they're not as buggy.
                ఒకవేళ (!వాడని_సరిపోలిక(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not driven, nor used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3_Error_సంకేతం::UNUSEDSIGNAL,
                                                     నిజం);  // Warn only once
                }
            } లేకపోతే ఒకవేళ (allD && !anyU) {
                ఒకవేళ (!వాడని_సరిపోలిక(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3_Error_సంకేతం::UNUSEDSIGNAL,
                                                     నిజం);  // Warn only once
                }
            } లేకపోతే ఒకవేళ (!anyD && allU) {
                nodep->v3warn(UNDRIVEN, varType << " is not driven: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_సంకేతం::UNDRIVEN, నిజం);  // Warn only once
            } లేకపోతే ఒకవేళ (!funcInout) {
                // Bits have different dispositions
                స్థిరరాశి std::string var_Type_క్రింది = [&varType]() {
                    std::string str = varType;
                    str[0] = std::tolower(static_cast<గుర్తులేని అక్షరం>(str[0]));
                    తిరిగి str;
                }();
                తార్కిక సమితి_U = అబద్ధం;
                తార్కిక సమితి_D = అబద్ధం;
                ఒకవేళ (anynotDU && !వాడని_సరిపోలిక(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  "Bits of " << var_Type_క్రింది << " are not driven, nor used: "
                                             << nodep->prettyNameQ() << బిట్టు_పేర్లు(BN_BOTH));
                    సమితి_U = నిజం;
                }
                ఒకవేళ (anyDnotU && !వాడని_సరిపోలిక(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL, "Bits of " << var_Type_క్రింది << " are not used: "
                                                           << nodep->prettyNameQ()
                                                           << బిట్టు_పేర్లు(BN_వాడని));
                    సమితి_U = నిజం;
                }
                ఒకవేళ (anyUnotD) {
                    nodep->v3warn(UNDRIVEN, "Bits of " << var_Type_క్రింది << " are not driven: "
                                                       << nodep->prettyNameQ()
                                                       << బిట్టు_పేర్లు(BN_UNDRIVEN));
                    సమితి_D = నిజం;
                }
                ఒకవేళ (సమితి_U) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3_Error_సంకేతం::UNUSEDSIGNAL, నిజం);
                }
                ఒకవేళ (సమితి_D) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3_Error_సంకేతం::UNDRIVEN, నిజం);
                }
            }
        }
    }

    శూన్య driven_ద్వారా_Call(స్థిరరాశి Ast_కణుపు_FTask_Ref* nodep) {
        driven_సంపూర్ణ(nodep);
        ఒకవేళ (!m_callNodep) m_callNodep = nodep;
    }
    స్థిరరాశి Ast_కణుపు_FTask_Ref* callNodep() స్థిరరాశి { తిరిగి m_callNodep; }
};

//######################################################################
// Undriven state, as a visitor of each AstNode

తరగతి UndrivenVisitor final : బహిరంగ VNVisitorConst {
    // NODE STATE
    // Netlist:
    //  AstVar::user1p          -> UndrivenVar* for usage var, 0=not set yet
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    // Each always:
    //  AstNode::user2p         -> UndrivenVar* for usage var, 0=not set yet
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;

    // STATE
    std::array<std::సదిశ<Undriven_Var_నమోదు*>, 3> m_entryps = {};  // Nodes to delete when finished
    తార్కిక m_లోపల_BBox = అబద్ధం;  // In black box; mark as driven+used
    తార్కిక m_లోపల_Cont_కేటాయించు = అబద్ధం;  // In continuous assignment
    తార్కిక m_లోపల_Initial = అబద్ధం;  // In explicit initial block
    తార్కిక m_లోపల_Initial_అమరిక = అబద్ధం;  // In InitialAutomatic*/InitialStatic* assignment LHS
    తార్కిక m_లోపల_Initial_Static = అబద్ధం;  // In InitialStatic
    తార్కిక m_లోపల_Proc_కేటాయించు = అబద్ధం;  // In procedural assignment
    తార్కిక m_లోపల_FTask_Ref = అబద్ధం;  // In function or task call
    తార్కిక m_లోపల_Inout_Or_Ref_Pin = అబద్ధం;  // Connected to pin that is inout
    తార్కిక m_లోపల_Sel_Lhs = అబద్ధం;  // Iterating the fromp of an AstSel (a partial-bit write target)
    స్థిరరాశి Ast_కణుపు_FTask* m_taskp = శూన్యసూచిక;  // Current task
    స్థిరరాశి AstAlways* m_alwaysp = శూన్యసూచిక;  // Current always of either type
    స్థిరరాశి AstAlways* m_alwaysCombp = శూన్యసూచిక;  // Current always if combo, otherwise nullptr
    స్థిరరాశి AstAlways* m_alwaysFFp = శూన్యసూచిక;  // Current always if ff, otherwise nullptr
    స్థిరరాశి AstAlways* m_alwaysPlainp = శూన్యసూచిక;  // Current always if plain (not comb/ff/latch)
    స్థిరరాశి AstClocking* m_clockingp = శూన్యసూచిక;  // Current clocking block, otherwise nullptr

    V3UndrivenCapture* స్థిరరాశి m_capturep = శూన్యసూచిక;  // Capture object.  'nullptr' if disabled.

    // METHODS

    Undriven_Var_నమోదు* getEntryp(AstVar* nodep, పూర్ణాంకం which_user) {
        ఒకవేళ (!(which_user == 1 ? nodep->user1p() : nodep->user2p())) {
            Undriven_Var_నమోదు* స్థిరరాశి entryp = కొత్త Undriven_Var_నమోదు{nodep};
            // UINFO(9," Associate u="<<which_user<<" "<<cvtToHex(this)<<" "<<nodep->name()<<endl);
            m_entryps[which_user].నెట్టు_వెనుక(entryp);
            ఒకవేళ (which_user == 1) {
                nodep->user1p(entryp);
            } లేకపోతే ఒకవేళ (which_user == 2) {
                nodep->user2p(entryp);
            } లేకపోతే {
                nodep->v3fatalSrc("Bad case");
            }
            తిరిగి entryp;
        } లేకపోతే {
            Undriven_Var_నమోదు* స్థిరరాశి entryp = reinterpret_cast<Undriven_Var_నమోదు*>(
                which_user == 1 ? nodep->user1p() : nodep->user2p());
            తిరిగి entryp;
        }
    }

    శూన్య warn_Alw_Comb_క్రమం(Ast_కణుపు_Var_Ref* nodep, స్థిరరాశి Ast_కణుపు* readp) {
        AstVar* స్థిరరాశి varp = nodep->varp();
        ఒకవేళ (!varp->isParam() && !varp->isGenVar() && !varp->is_వాడిన_Loop_Idx()
            && !varp->ignoreSchedWrite()
            && !m_లోపల_BBox  // We may have falsely considered a SysIgnore as a driver
            && !VN_IS(nodep, VarXRef)  // Xrefs might point at two different instances
            && !varp->fileline()->warnIsOff(
                V3_Error_సంకేతం::ALWCOMBORDER)) {  // Warn only once per variable
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
            varp->fileline()->modifyWarnOff(V3_Error_సంకేతం::ALWCOMBORDER,
                                            నిజం);  // Complain just once for any usage
        }
    }
    // A clocking block 'output' is an additional driver of the signal it names.
    // The conflict is found from whichever driver is reached second.
    తార్కిక clockingDrivesOther(స్థిరరాశి Undriven_Var_నమోదు* entryp, తార్కిక otherWriteIsStaticInit) స్థిరరాశి {
        స్థిరరాశి తార్కిక otherIsExplicitProc
            = entryp->is_Driven_Always_Comb_సంపూర్ణ() || entryp->isDrivenAlwaysFFWhole();
        తిరిగి m_clockingp && !otherIsExplicitProc && !otherWriteIsStaticInit
               && m_clockingp != entryp->getClockingp();
    }
    తార్కిక otherDrivesClocking(స్థిరరాశి Undriven_Var_నమోదు* entryp) స్థిరరాశి {
        తిరిగి !m_clockingp && !m_alwaysCombp && !m_alwaysFFp && !m_లోపల_Initial_Static
               && entryp->is_Driven_Clocking_సంపూర్ణ();
    }
    // A continuous assignment contending with a clocking block 'output' is an
    // unambiguous driver conflict. Multiple clocking blocks may drive the same
    // signal, the last drive wins (IEEE 1800-2023 14.16.2).
    శూన్య warnClockingDriven(Ast_కణుపు_Var_Ref* nodep, స్థిరరాశి Undriven_Var_నమోదు* entryp,
                            స్థిరరాశి Ast_కణుపు* otherWritep, తార్కిక otherWriteIsStaticInit) {
        ఒకవేళ (clockingDrivesOther(entryp, otherWriteIsStaticInit)
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
        ఒకవేళ (otherDrivesClocking(entryp) && m_లోపల_Cont_కేటాయించు) {
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
    శూన్య warnClockingDrivenProc(Ast_కణుపు_Var_Ref* nodep, స్థిరరాశి Undriven_Var_నమోదు* entryp,
                                స్థిరరాశి Ast_కణుపు* otherWritep, తార్కిక otherWriteIsStaticInit) {
        ఒకవేళ (clockingDrivesOther(entryp, otherWriteIsStaticInit) && !entryp->getClockingp()
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
        ఒకవేళ (otherDrivesClocking(entryp) && !m_లోపల_Cont_కేటాయించు) {
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
    శూన్య సందర్శించు(AstVar* nodep) override {
        స్థిరరాశి తార్కిక funcInout = nodep->isFuncLocal() && nodep->isInout();
        కొరకు (పూర్ణాంకం usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            // For assigns and non-combo always, do just usr==1, to look
            // for module-wide undriven etc.
            // For combo always, run both usr==1 for above, and also
            // usr==2 for always-only checks.
            Undriven_Var_నమోదు* స్థిరరాశి entryp = getEntryp(nodep, usr);
            ఒకవేళ ((nodep->isNonOutput() && !funcInout) || nodep->is_Sig_ప్రజా()
                || nodep->hasUserInit() || nodep->isSigUserRWPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->driven_సంపూర్ణ(nodep);
            }
            ఒకవేళ ((nodep->isWritable() && !funcInout) || nodep->is_Sig_ప్రజా()
                || nodep->isSigUserRWPublic() || nodep->is_Sig_User_Rd_ప్రజా()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->వాడిన_సంపూర్ణ(nodep);
            }
            ఒకవేళ (nodep->valuep()) entryp->driven_సంపూర్ణ(nodep->valuep());
        }
        // Discover variables used in bit definitions, etc
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstArraySel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_ముక్క_Sel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstSel* nodep) override {
        Ast_కణుపు_Var_Ref* స్థిరరాశి varrefp = VN_CAST(nodep->fromp(), కణుపు_Var_Ref);
        AstConst* స్థిరరాశి constp = VN_CAST(nodep->lsbp(), Const);
        ఒకవేళ (varrefp && constp && !constp->అంకె().isFourState()) {
            కొరకు (పూర్ణాంకం usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                Undriven_Var_నమోదు* స్థిరరాశి entryp = getEntryp(varrefp->varp(), usr);
                స్థిరరాశి పూర్ణాంకం lsb = constp->గమ్యం_UInt();
                ఒకవేళ (m_లోపల_BBox || varrefp->access().isWriteOrRW()) {
                    // Don't warn if already driven earlier as "a=0; if(a) a=1;" is fine.
                    ఒకవేళ (usr == 2 && m_alwaysCombp
                        && entryp->is_వాడిన_Not_Driven_బిట్టు(lsb, nodep->వెడల్పు())) {
                        UINFO(9, " Select.  Entryp=" << cvt_గమ్యం_Hex(entryp));
                        warn_Alw_Comb_క్రమం(varrefp, entryp->మొదటి_వాడిన_Not_Drivenp());
                    }
                    entryp->driven_బిట్టు(lsb, nodep->వెడల్పు());
                }
                ఒకవేళ (m_లోపల_BBox || !varrefp->access().isWriteOrRW())
                    entryp->వాడిన_బిట్టు_2(lsb, nodep->వెడల్పు(), varrefp);
            }
        } లేకపోతే {
            // skip over static longest static prefix
            iterateConst(nodep->lsbp());
            VL_RESTORER(m_లోపల_Sel_Lhs);
            m_లోపల_Sel_Lhs = !V3_వెడల్పు::selectNonConstantRecurse(nodep->lsbp(), /*inSel=*/నిజం);
            iterateConst(nodep->fromp());
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        // Any variable
        ఒకవేళ (nodep->access().isWriteOrRW()
            && !VN_IS(nodep, VarXRef)) {  // Ignore interface variables and similar ugly items
            ఒకవేళ (m_లోపల_Proc_కేటాయించు && !nodep->varp()->varType().isProcAssignable()
                && !nodep->varp()->isDeclTyped()  //
                && !nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()) {
                nodep->v3warn(PROCASSWIRE, "Procedural assignment to wire, perhaps intended var"
                                               << " (IEEE 1800-2023 6.5): "
                                               << nodep->prettyNameQ());
            } లేకపోతే ఒకవేళ (m_లోపల_Cont_కేటాయించు && !nodep->varp()->varType().isContAssignable()
                       && !nodep->fileline()->language().systemVerilog()) {
                nodep->v3warn(CONTASSREG,
                              "Continuous assignment to reg, perhaps intended wire"
                                  << " (IEEE 1364-2005 6.1; Verilog only, legal in SV): "
                                  << nodep->prettyNameQ());
            }
            ఒకవేళ (m_లోపల_FTask_Ref && nodep->varp()->varType().isNet()) {
                nodep->v3warn(
                    PROCASSWIRE,
                    "Passed wire on output or inout subroutine argument, expected expression that "
                    "is valid on the left hand side of a procedural assignment"
                        << " (IEEE 1800-2023 13.5): " << nodep->prettyNameQ());
            }
        }

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Remember that anything driven here doesn't count toward MULTIDRIVEN.
        తార్కిక ftaskDef = అబద్ధం;
        ఒకవేళ (m_taskp && !m_alwaysp && !m_లోపల_Cont_కేటాయించు && !m_లోపల_Initial_Static && !m_లోపల_BBox
            && !m_taskp->dpiExport()) {
            AstVar* స్థిరరాశి retVarp = VN_CAST(m_taskp->fvarp(), Var);
            ఒకవేళ (!retVarp || nodep->varp() != retVarp) ftaskDef = నిజం;
        }

        కొరకు (పూర్ణాంకం usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            Undriven_Var_నమోదు* స్థిరరాశి entryp = getEntryp(nodep->varp(), usr);
            స్థిరరాశి తార్కిక fdrv = nodep->access().isWriteOrRW()
                              && nodep->varp()->attrFileDescr();  // FD's are also being read from
            ఒకవేళ (m_లోపల_BBox || nodep->access().isWriteOrRW()) {
                ఒకవేళ (usr == 2 && m_alwaysCombp && entryp->is_వాడిన_Not_Driven_Any()) {
                    UINFO(9, " Full bus.  Entryp=" << cvt_గమ్యం_Hex(entryp));
                    warn_Alw_Comb_క్రమం(nodep, entryp->మొదటి_వాడిన_Not_Drivenp());
                }
                స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి otherVarRefp = entryp->getNodep();
                స్థిరరాశి Ast_కణుపు* స్థిరరాశి otherWritep = otherVarRefp
                                                       ? static_cast<స్థిరరాశి Ast_కణుపు*>(otherVarRefp)
                                                       : entryp->callNodep();
                స్థిరరాశి తార్కిక same_File_పంక్తి
                    = otherVarRefp && nodep->fileline() == otherVarRefp->fileline();
                // Preconditions shared by MULTIDRIVEN and MULTIDRIVENPROC.
                స్థిరరాశి తార్కిక multidrivenCommon
                    = entryp->is_Driven_సంపూర్ణ() && !m_లోపల_BBox && !VN_IS(nodep, VarXRef)
                      && !VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType) && !same_File_పంక్తి
                      && !entryp->isUnderGen() && otherWritep && !entryp->isFtaskDriven()
                      && !ftaskDef && !m_లోపల_Sel_Lhs;
                // The two warnings are gated independently on the variable
                // declaration's fileline, as v3warn suppression will check
                // the driving fileline and still warn even if the warning
                // was suppressed with lint_off at the declaration.
                స్థిరరాశి తార్కిక otherWriteIsStaticInit
                    = nodep->varp()->hasUserInit() && otherWritep == entryp->initStaticp();
                ఒకవేళ (multidrivenCommon
                    && !nodep->varp()->fileline()->warnIsOff(V3_Error_సంకేతం::MULTIDRIVEN)) {
                    ఒకవేళ (m_alwaysCombp
                        && (!entryp->is_Driven_Always_Comb_సంపూర్ణ()
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
                    ఒకవేళ (!m_alwaysCombp && entryp->is_Driven_Always_Comb_సంపూర్ణ()) {
                        nodep->v3warn(MULTIDRIVEN, "Variable also written to in always_comb"
                                                       << " (IEEE 1800-2023 9.2.2.2): "
                                                       << nodep->prettyNameQ() << '\n'
                                                       << nodep->warnOther() << '\n'
                                                       << nodep->warnContextPrimary() << '\n'
                                                       << otherWritep->warnOther()
                                                       << "... Location of always_comb write\n"
                                                       << otherWritep->warnContextSecondary());
                    }
                    ఒకవేళ (m_alwaysFFp && !otherWriteIsStaticInit
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
                    ఒకవేళ (!m_alwaysFFp && !m_లోపల_Initial_Static && entryp->isDrivenAlwaysFFWhole()) {
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
                ఒకవేళ (multidrivenCommon && !nodep->varp()->is_వాడిన_Loop_Idx()
                    && !nodep->varp()->fileline()->warnIsOff(V3_Error_సంకేతం::MULTIDRIVENPROC)) {
                    // Two plain always blocks driving the whole signal: legal
                    // SystemVerilog, but a driver conflict for synthesis. The
                    // always_ff/always_comb cases above already cover mixes with
                    // an explicit process, so only warn for plain+plain here.
                    // When the two blocks are clocked differently, the
                    // (on-by-default) MULTIDRIVEN check in V3Delayed already reports
                    // the conflict, so don't also emit MULTIDRIVENPROC for that case.
                    స్థిరరాశి AstAlways* స్థిరరాశి otherAlwaysp = entryp->getAlwPlainp();
                    స్థిరరాశి Ast_Sen_వృక్షం* స్థిరరాశి senp
                        = m_alwaysPlainp ? m_alwaysPlainp->sentreep() : శూన్యసూచిక;
                    స్థిరరాశి Ast_Sen_వృక్షం* స్థిరరాశి otherSenp
                        = otherAlwaysp ? otherAlwaysp->sentreep() : శూన్యసూచిక;
                    స్థిరరాశి తార్కిక differentClocking = senp && otherSenp && senp->hasClocked()
                                                   && otherSenp->hasClocked()
                                                   && !senp->same_వృక్షం(otherSenp);
                    ఒకవేళ (m_alwaysPlainp && entryp->is_Driven_Always_Plain_సంపూర్ణ()
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
                ఒకవేళ (!m_లోపల_Initial_అమరిక || nodep->varp()->hasUserInit()) {
                    // Else don't count default initialization as a driver to a net/variable
                    entryp->driven_సంపూర్ణ(nodep, ftaskDef);
                }
                ఒకవేళ (m_alwaysCombp && entryp->is_Driven_Always_Comb_సంపూర్ణ()
                    && m_alwaysCombp != entryp->getAlwCombp()
                    && m_alwaysCombp->fileline() == entryp->getAlwCombp()->fileline())
                    entryp->underGenerate();
                ఒకవేళ (m_alwaysCombp) entryp->driven_Always_Comb_సంపూర్ణ(m_alwaysCombp);
                ఒకవేళ (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, nodep->varp());
                ఒకవేళ (m_alwaysPlainp) entryp->driven_Always_Plain_సంపూర్ణ(m_alwaysPlainp);
                ఒకవేళ (m_clockingp) entryp->driven_Clocking_సంపూర్ణ(m_clockingp);
            }
            ఒకవేళ (nodep->access().isWriteOrRW() && !VN_IS(nodep, VarXRef)) {
                // Ignoring xrefs as the initial and assignment to track might refer to two
                // different instances.  Ideally all of V3Undriven would move after V3Scope,
                // then could use VarScope tracking instead.
                ఒకవేళ (m_లోపల_Initial_Static && !entryp->initStaticp()) entryp->initStaticp(nodep);
                ఒకవేళ (m_లోపల_Initial && !entryp->initialp()) entryp->initialp(nodep);
                ఒకవేళ (m_లోపల_Cont_కేటాయించు && !entryp->contAssignp()) entryp->contAssignp(nodep);
                ఒకవేళ (m_alwaysp && m_లోపల_Proc_కేటాయించు && !entryp->procWritep())
                    entryp->procWritep(nodep);
            }
            ఒకవేళ ((!m_లోపల_Initial_అమరిక || nodep->varp()->hasUserInit())
                && (m_లోపల_BBox || nodep->access().isReadOrRW()
                    || fdrv
                    // Inouts have only isWrite set, as we don't have more
                    // information and operating on module boundary, treat as
                    // both read and writing
                    || m_లోపల_Inout_Or_Ref_Pin))
                entryp->వాడిన_సంపూర్ణ(nodep);
        }
    }

    // Don't know what black boxed calls do, assume in+out
    శూన్య సందర్శించు(AstSysIgnore* nodep) override {
        VL_RESTORER(m_లోపల_BBox);
        m_లోపల_BBox = నిజం;
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(Ast_కేటాయించు* nodep) override {
        VL_RESTORER(m_లోపల_Proc_కేటాయించు);
        m_లోపల_Proc_కేటాయించు = నిజం;
        {
            VL_RESTORER(m_లోపల_Initial_అమరిక);
            m_లోపల_Initial_అమరిక = అబద్ధం;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_Dly* nodep) override {
        VL_RESTORER(m_లోపల_Proc_కేటాయించు);
        m_లోపల_Proc_కేటాయించు = నిజం;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_Force* nodep) override {
        iterateConst(nodep->rhsp());
        VL_RESTORER(m_లోపల_Initial);
        m_లోపల_Initial = అబద్ధం;
        iterateConst(nodep->lhsp());
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_W* nodep) override {
        VL_RESTORER(m_లోపల_Cont_కేటాయించు);
        m_లోపల_Cont_కేటాయించు = నిజం;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstRelease* nodep) override {
        VL_RESTORER(m_లోపల_Initial);
        m_లోపల_Initial = అబద్ధం;
        iterateConst(nodep->lhsp());
    }
    శూన్య సందర్శించు(AstInitial* nodep) override {
        VL_RESTORER(m_లోపల_Initial);
        m_లోపల_Initial = నిజం;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstInitialAutomatic* nodep) override {
        VL_RESTORER(m_లోపల_Initial_అమరిక);
        m_లోపల_Initial_అమరిక = నిజం;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_లోపల_Initial_అమరిక);
        m_లోపల_Initial_అమరిక = నిజం;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstInitialStatic* nodep) override {
        VL_RESTORER(m_లోపల_Initial_Static);
        m_లోపల_Initial_Static = నిజం;
        VL_RESTORER(m_లోపల_Initial_అమరిక);
        m_లోపల_Initial_అమరిక = నిజం;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_లోపల_Initial_అమరిక);
        m_లోపల_Initial_అమరిక = నిజం;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        VL_RESTORER(m_alwaysCombp);
        VL_RESTORER(m_alwaysFFp);
        VL_RESTORER(m_alwaysPlainp);
        Ast_కణుపు::user2_Clear_వృక్షం();
        m_alwaysp = nodep;
        ఒకవేళ (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            UINFO(9, "   " << nodep);
            m_alwaysCombp = nodep;
        } లేకపోతే {
            m_alwaysCombp = శూన్యసూచిక;
        }
        m_alwaysFFp = nodep->keyword() == VAlwaysKwd::ALWAYS_FF ? nodep : శూన్యసూచిక;
        m_alwaysPlainp = nodep->keyword() == VAlwaysKwd::ALWAYS ? nodep : శూన్యసూచిక;
        iterateChildrenConst(nodep);
        ఒకవేళ (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) UINFO(9, "   Done " << nodep);
    }
    శూన్య సందర్శించు(AstClocking* nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        VL_RESTORER(m_లోపల_FTask_Ref);
        m_లోపల_FTask_Ref = నిజం;

        iterateChildrenConst(nodep);

        ఒకవేళ (!m_capturep) తిరిగి;

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Do not apply writeSummary at calls inside a task definition, or they will look like
        // independent drivers (phantom MULTIDRIVEN).
        స్థిరరాశి తార్కిక లోపల_Executed_Context
            = !(m_taskp && !m_alwaysp && !m_లోపల_Cont_కేటాయించు && !m_లోపల_Initial_Static && !m_లోపల_BBox
                && !m_taskp->dpiExport());

        ఒకవేళ (!లోపల_Executed_Context) తిరిగి;

        Ast_కణుపు_FTask* స్థిరరాశి calleep = nodep->taskp();
        ఒకవేళ (!calleep) తిరిగి;

        స్థిరరాశి స్వయంచాలక& vars = m_capturep->writeSummary(calleep);
        కొరకు (AstVar* స్థిరరాశి varp : vars) {
            కొరకు (పూర్ణాంకం usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                Undriven_Var_నమోదు* స్థిరరాశి entryp = getEntryp(varp, usr);
                entryp->driven_ద్వారా_Call(nodep);
                ఒకవేళ (m_alwaysCombp) entryp->driven_Always_Comb_సంపూర్ణ(m_alwaysCombp);
                ఒకవేళ (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, varp);
            }
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        VL_RESTORER(m_taskp);
        m_taskp = nodep;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstPin* nodep) override {
        VL_RESTORER(m_లోపల_Inout_Or_Ref_Pin);
        m_లోపల_Inout_Or_Ref_Pin = nodep->modVarp()->isInoutOrRef();
        iterateChildrenConst(nodep);
    }

    // Until we support tables, primitives will have undriven and unused I/Os
    శూన్య సందర్శించు(AstPrimitive*) override {}

    // Coverage artifacts etc shouldn't count as a sink
    శూన్య సందర్శించు(Ast_కణుపు_Cover_Decl*) override {}
    శూన్య సందర్శించు(AstCoverInc*) override {}
    శూన్య సందర్శించు(AstCoverToggle*) override {}
    శూన్య సందర్శించు(AstTraceDecl* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    శూన్య సందర్శించు(AstTraceInc* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }

    // iterate
    శూన్య సందర్శించు(AstConst* nodep) override {}
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన UndrivenVisitor(AstNetlist* nodep, V3UndrivenCapture* capturep)
        : m_capturep{capturep} {
        iterateConst(nodep);
    }

    ~UndrivenVisitor() override {
        కొరకు (Undriven_Var_నమోదు* ip : m_entryps[1]) ip->reportViolations();
        కొరకు (పూర్ణాంకం usr = 1; usr < 3; ++usr) {
            కొరకు (Undriven_Var_నమోదు* ip : m_entryps[usr]) తొలగించు ip;
        }
    }
};

//######################################################################
// Undriven class functions

శూన్య V3Undriven::undrivenAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    V3UndrivenCapture capture{nodep};
    { UndrivenVisitor{nodep, &capture}; }

    ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStage("undriven");
}
