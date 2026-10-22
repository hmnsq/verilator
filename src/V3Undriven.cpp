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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Undriven.h"

#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3UndrivenCapture.h"
#അന്തര്ഭാവ "V3Width.h"

#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Class for every variable we may process

വർഗ്ഗം UndrivenVarEntry final {
    // MEMBERS
    AstVar* സ്ഥിരം m_varp;  // Variable this tracks
    std::vector<തർക്കം> m_wholeFlags;  // Used/Driven on whole vector
    std::vector<തർക്കം> m_bitFlags;  // Used/Driven on each subbit
    സ്ഥിരം Ast_കണ്ണി* m_usedNotDrivenp = nullptr;  // First read before any write
    സ്ഥിരം AstAlways* m_alwCombp
        = nullptr;  // always_comb of var if driven within always_comb, else nullptr
    സ്ഥിരം AstAlways* m_alwFFp = nullptr;  // always_ff of var if driven within always_ff
    സ്ഥിരം AstAlways* m_alwPlainp = nullptr;  // plain always of var if driven within plain always
    സ്ഥിരം AstClocking* m_clockingp = nullptr;  // clocking block of var if driven as output
    സ്ഥിരം Ast_കണ്ണി_Var_Ref* m_nodep = nullptr;  // varref if driven, else nullptr
    സ്ഥിരം Ast_കണ്ണി* m_initStaticp = nullptr;  // varref if in InitialStatic driven
    സ്ഥിരം Ast_കണ്ണി* m_initialp = nullptr;  // varref if driven in an explicit initial block
    സ്ഥിരം Ast_കണ്ണി* m_contAssignp = nullptr;  // varref if in continuous assignment driven
    സ്ഥിരം Ast_കണ്ണി* m_procWritep = nullptr;  // varref if written in process
    തർക്കം m_underGen = അസത്യം;  // Under a generate
    തർക്കം m_ftaskDriven = അസത്യം;  // Last driven by function or task

    സ്ഥിരം Ast_കണ്ണി_FTask_Ref* m_callNodep = nullptr;  // Call node if driven via writeSummary

    എണ്ണൽ : uint8_t {
        FLAG_USED = 0,  // Signal or bit has been read/observed
        FLAG_DRIVEN = 1,  // Signal or bit has been written/driven
        FLAG_DRIVEN_ALWCOMB = 2,  // Whole signal has been driven from always_comb
        FLAG_DRIVEN_ALWFF = 3,  // Whole signal has been driven from always_ff
        FLAG_DRIVEN_ALWPLAIN = 4,  // Whole signal has been driven from a plain always
        FLAG_DRIVEN_CLOCKING = 5,  // Whole signal has been driven as a clocking block output
        FLAGS_PER_BIT = 6  // Number of flags stored for each tracked bit
    };

പൊതു:
    // CONSTRUCTORS
    explicit UndrivenVarEntry(AstVar* varp)
        : m_varp{varp} {  // Construction for when a var is used
        UINFO(9, "create " << varp);
        m_wholeFlags.resize(FLAGS_PER_BIT);
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < FLAGS_PER_BIT; i++) m_wholeFlags[i] = അസത്യം;
        m_bitFlags.resize(varp->width() * FLAGS_PER_BIT);
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < varp->width() * FLAGS_PER_BIT; i++) m_bitFlags[i] = അസത്യം;
    }
    ~UndrivenVarEntry() = സ്ഥിരസ്ഥിതി;

സ്വകാര്യ:
    // METHODS
    തർക്കം bit_അക്കം_Ok(പൂർണ്ണസംഖ്യ bit) സ്ഥിരം {
        തിരികെ bit >= 0 && (bit * FLAGS_PER_BIT < static_cast<പൂർണ്ണസംഖ്യ>(m_bitFlags.size()));
    }
    തർക്കം usedFlag(പൂർണ്ണസംഖ്യ bit) സ്ഥിരം {
        തിരികെ m_wholeFlags[FLAG_USED] || m_bitFlags[bit * FLAGS_PER_BIT + FLAG_USED];
    }
    തർക്കം drivenFlag(പൂർണ്ണസംഖ്യ bit) സ്ഥിരം {
        തിരികെ m_wholeFlags[FLAG_DRIVEN] || m_bitFlags[bit * FLAGS_PER_BIT + FLAG_DRIVEN];
    }
    പൂർണ്ണസംഖ്യ bit_എണ്ണം() സ്ഥിരം { തിരികെ m_bitFlags.size() / FLAGS_PER_BIT; }
    ശൂന്യം recordUsedNotDriven(സ്ഥിരം Ast_കണ്ണി* nodep) {
        എങ്കിൽ (!m_usedNotDrivenp) m_usedNotDrivenp = nodep;
    }
    എണ്ണൽ BitNamesWhich : uint8_t { BN_UNUSED, BN_UNDRIVEN, BN_BOTH };
    string bitNames(BitNamesWhich which) {
        string bits;
        തർക്കം prev = അസത്യം;
        പൂർണ്ണസംഖ്യ msb = 0;
        // bit==-1 loops below; we do one extra iteration so end with prev=false
        വേണ്ടി (പൂർണ്ണസംഖ്യ bit = (m_bitFlags.size() / FLAGS_PER_BIT) - 1; bit >= -1; --bit) {
            എങ്കിൽ (bit >= 0
                && ((which == BN_UNUSED && !usedFlag(bit) && drivenFlag(bit))
                    || (which == BN_UNDRIVEN && usedFlag(bit) && !drivenFlag(bit))
                    || (which == BN_BOTH && !usedFlag(bit) && !drivenFlag(bit)))) {
                എങ്കിൽ (!prev) {
                    prev = സത്യം;
                    msb = bit;
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (prev) {
                സ്ഥിരം AstBasicDType* സ്ഥിരം bdtypep = m_varp->basicp();
                സ്ഥിരം പൂർണ്ണസംഖ്യ lsb = bit + 1;
                എങ്കിൽ (bits != "") bits += ",";
                എങ്കിൽ (lsb == msb) {
                    bits += cvtToStr(lsb + bdtypep->lo());
                } അല്ലെങ്കിൽ {
                    എങ്കിൽ (bdtypep->ascending()) {
                        bits
                            += cvtToStr(lsb + bdtypep->lo()) + ":" + cvtToStr(msb + bdtypep->lo());
                    } അല്ലെങ്കിൽ {
                        bits
                            += cvtToStr(msb + bdtypep->lo()) + ":" + cvtToStr(lsb + bdtypep->lo());
                    }
                }
                prev = അസത്യം;
            }
        }
        തിരികെ "[" + bits + "]";
    }

പൊതു:
    ശൂന്യം usedWhole(സ്ഥിരം Ast_കണ്ണി* nodep) {
        UINFO(9, "set u[*] " << m_varp->name() << " " << nodep);
        എങ്കിൽ (!m_wholeFlags[FLAG_DRIVEN]) {
            വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < bit_എണ്ണം(); ++bit) {
                എങ്കിൽ (!drivenFlag(bit)) {
                    recordUsedNotDriven(nodep);
                    നിർത്തുക;
                }
            }
        }
        m_wholeFlags[FLAG_USED] = സത്യം;
    }
    ശൂന്യം drivenWhole(സ്ഥിരം Ast_കണ്ണി* nodep) {
        UINFO(9, "set d[*] " << m_varp->name() << " " << nodep);
        m_wholeFlags[FLAG_DRIVEN] = സത്യം;
    }
    ശൂന്യം drivenWhole(സ്ഥിരം Ast_കണ്ണി_Var_Ref* nodep, തർക്കം ftaskDef) {
        സ്ഥിരം തർക്കം wasDriven = isDrivenWhole();
        drivenWhole(nodep);
        // A write in a task/function definition is not an executed process, so it must not
        // displace an already recorded write, nor clear the flag that keeps such a write
        // from being reported as the other write of a MULTIDRIVEN
        എങ്കിൽ (ftaskDef && wasDriven) തിരികെ;
        m_ftaskDriven = ftaskDef;
        m_nodep = nodep;
    }
    ശൂന്യം drivenAlwaysCombWhole(സ്ഥിരം AstAlways* alwCombp) {
        m_wholeFlags[FLAG_DRIVEN_ALWCOMB] = സത്യം;
        m_alwCombp = alwCombp;
    }
    ശൂന്യം drivenAlwaysFFWhole(സ്ഥിരം AstAlways* alwFFp, സ്ഥിരം AstVar* varp) {
        m_wholeFlags[FLAG_DRIVEN_ALWFF] = സത്യം;
        m_alwFFp = alwFFp;
    }
    ശൂന്യം drivenAlwaysPlainWhole(സ്ഥിരം AstAlways* alwPlainp) {
        m_wholeFlags[FLAG_DRIVEN_ALWPLAIN] = സത്യം;
        m_alwPlainp = alwPlainp;
    }
    ശൂന്യം drivenClockingWhole(സ്ഥിരം AstClocking* clockingp) {
        m_wholeFlags[FLAG_DRIVEN_CLOCKING] = സത്യം;
        m_clockingp = clockingp;
    }

    സ്ഥിരം Ast_കണ്ണി* initStaticp() സ്ഥിരം { തിരികെ m_initStaticp; }
    ശൂന്യം initStaticp(സ്ഥിരം Ast_കണ്ണി* nodep) { m_initStaticp = nodep; }
    സ്ഥിരം Ast_കണ്ണി* initialp() സ്ഥിരം { തിരികെ m_initialp; }
    ശൂന്യം initialp(സ്ഥിരം Ast_കണ്ണി* nodep) { m_initialp = nodep; }
    സ്ഥിരം Ast_കണ്ണി* contAssignp() സ്ഥിരം { തിരികെ m_contAssignp; }
    ശൂന്യം contAssignp(സ്ഥിരം Ast_കണ്ണി* nodep) { m_contAssignp = nodep; }
    സ്ഥിരം Ast_കണ്ണി* procWritep() സ്ഥിരം { തിരികെ m_procWritep; }
    ശൂന്യം procWritep(സ്ഥിരം Ast_കണ്ണി* nodep) { m_procWritep = nodep; }
    ശൂന്യം underGenerate() { m_underGen = സത്യം; }
    തർക്കം isUnderGen() സ്ഥിരം { തിരികെ m_underGen; }
    തർക്കം isDrivenWhole() സ്ഥിരം { തിരികെ m_wholeFlags[FLAG_DRIVEN]; }
    തർക്കം isDrivenAlwaysCombWhole() സ്ഥിരം { തിരികെ m_wholeFlags[FLAG_DRIVEN_ALWCOMB]; }
    തർക്കം isDrivenAlwaysFFWhole() സ്ഥിരം { തിരികെ m_wholeFlags[FLAG_DRIVEN_ALWFF]; }
    തർക്കം isDrivenAlwaysPlainWhole() സ്ഥിരം { തിരികെ m_wholeFlags[FLAG_DRIVEN_ALWPLAIN]; }
    തർക്കം isDrivenClockingWhole() സ്ഥിരം { തിരികെ m_wholeFlags[FLAG_DRIVEN_CLOCKING]; }
    തർക്കം isFtaskDriven() സ്ഥിരം { തിരികെ m_ftaskDriven; }
    സ്ഥിരം Ast_കണ്ണി_Var_Ref* getNodep() സ്ഥിരം { തിരികെ m_nodep; }
    സ്ഥിരം AstAlways* getAlwCombp() സ്ഥിരം { തിരികെ m_alwCombp; }
    സ്ഥിരം AstAlways* getAlwFFp() സ്ഥിരം { തിരികെ m_alwFFp; }
    സ്ഥിരം AstAlways* getAlwPlainp() സ്ഥിരം { തിരികെ m_alwPlainp; }
    സ്ഥിരം AstClocking* getClockingp() സ്ഥിരം { തിരികെ m_clockingp; }
    ശൂന്യം usedBit(പൂർണ്ണസംഖ്യ bit, പൂർണ്ണസംഖ്യ width, സ്ഥിരം Ast_കണ്ണി* nodep) {
        UINFO(9, "set u[" << (bit + width - 1) << ":" << bit << "] " << m_varp->name());
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < width; i++) {
            എങ്കിൽ (bit_അക്കം_Ok(bit + i)) {
                എങ്കിൽ (!drivenFlag(bit + i)) recordUsedNotDriven(nodep);
                m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_USED] = സത്യം;
            }
        }
    }
    ശൂന്യം drivenBit(പൂർണ്ണസംഖ്യ bit, പൂർണ്ണസംഖ്യ width) {
        UINFO(9, "set d[" << (bit + width - 1) << ":" << bit << "] " << m_varp->name());
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < width; i++) {
            എങ്കിൽ (bit_അക്കം_Ok(bit + i)) m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_DRIVEN] = സത്യം;
        }
    }
    തർക്കം isUsedNotDrivenBit(പൂർണ്ണസംഖ്യ bit, പൂർണ്ണസംഖ്യ width) സ്ഥിരം {
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < width; i++) {
            എങ്കിൽ (bit_അക്കം_Ok(bit + i)
                && (m_wholeFlags[FLAG_USED] || m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_USED])
                && !(m_wholeFlags[FLAG_DRIVEN]
                     || m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_DRIVEN]))
                തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }
    തർക്കം isUsedNotDrivenAny() സ്ഥിരം {
        തിരികെ isUsedNotDrivenBit(0, m_bitFlags.size() / FLAGS_PER_BIT);
    }
    സ്ഥിരം Ast_കണ്ണി* firstUsedNotDrivenp() സ്ഥിരം { തിരികെ m_usedNotDrivenp; }
    സ്ഥിതം തർക്കം unusedMatch(AstVar* nodep) {
        സ്ഥിരം string regexp = v3Global.opt.unusedRegexp();
        എങ്കിൽ (regexp == "") തിരികെ അസത്യം;
        സ്ഥിരം string prettyName = nodep->prettyName();
        തിരികെ VString::wildmatch(prettyName.c_str(), regexp.c_str());
    }
    ശൂന്യം reportViolations() {
        // Combine bits into overall state
        AstVar* സ്ഥിരം nodep = m_varp;

        എങ്കിൽ (initStaticp() && procWritep() && nodep->hasUserInit() && !nodep->isClassMember()
            && !nodep->isFuncLocal()) {
            initStaticp()->v3warn(
                PROCASSINIT,
                "Procedural assignment to declaration with initial value: "
                    << nodep->prettyNameQ() << '\n'
                    << initStaticp()->warnMore() << "... Location of variable initialization\n"
                    << initStaticp()->warnContextPrimary() << '\n'
                    << procWritep()->warnOther() << "... Location of variable process write\n"
                    << procWritep()->warnMore()
                    << "... Perhaps should initialize instead using a reset in this process\n"
                    << procWritep()->warnContextSecondary());
        }
        സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം initp = nodep->hasUserInit() ? initStaticp() : initialp();
        എങ്കിൽ (initp && contAssignp() && !nodep->isClassMember() && !nodep->isFuncLocal()) {
            initp->v3warn(E_CONTASSINIT, "Continuous assignment to variable with initial value: "
                                             << nodep->prettyNameQ() << '\n'
                                             << initp->warnMore()
                                             << "... Location of variable initialization\n"
                                             << initp->warnContextPrimary() << '\n'
                                             << contAssignp()->warnOther()
                                             << "... Location of continuous assignment\n"
                                             << contAssignp()->warnContextSecondary());
        }
        എങ്കിൽ (nodep->isGenVar()) {  // Genvar
            എങ്കിൽ (!nodep->isIfaceRef() && !nodep->isUsedParam() && !unusedMatch(nodep)) {
                nodep->v3warn(UNUSEDGENVAR, "Genvar is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDGENVAR,
                                                 സത്യം);  // Warn only once
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isParam()) {  // Parameter
            എങ്കിൽ (!nodep->isIfaceRef() && !nodep->isUsedParam() && !unusedMatch(nodep)) {
                nodep->v3warn(UNUSEDPARAM, "Parameter is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDPARAM,
                                                 സത്യം);  // Warn only once
            }
        } അല്ലെങ്കിൽ {  // Signal
            സ്ഥിരം string varType{nodep->isFuncLocal() ? "Function variable" : "Signal"};
            സ്ഥിരം തർക്കം funcInout = nodep->isFuncLocal() && nodep->isInout();
            തർക്കം allU = സത്യം;
            തർക്കം allD = സത്യം;
            തർക്കം anyU = m_wholeFlags[FLAG_USED];
            തർക്കം anyD = m_wholeFlags[FLAG_DRIVEN];
            തർക്കം anyUnotD = അസത്യം;
            തർക്കം anyDnotU = അസത്യം;
            തർക്കം anynotDU = അസത്യം;
            വേണ്ടി (unsigned bit = 0; bit < m_bitFlags.size() / FLAGS_PER_BIT; bit++) {
                സ്ഥിരം തർക്കം used = usedFlag(bit);
                സ്ഥിരം തർക്കം driv = drivenFlag(bit);
                allU &= used;
                anyU |= used;
                allD &= driv;
                anyD |= driv;
                anyUnotD |= used && !driv;
                anyDnotU |= !used && driv;
                anynotDU |= !used && !driv;
            }
            എങ്കിൽ (funcInout) {
                എങ്കിൽ (anyD) allU = സത്യം;
                allD = സത്യം;
            }
            എങ്കിൽ (allU) m_wholeFlags[FLAG_USED] = സത്യം;
            എങ്കിൽ (allD) m_wholeFlags[FLAG_DRIVEN] = സത്യം;
            // Test results
            എങ്കിൽ (nodep->isIfaceRef()) {
                // For interface top level we don't do any tracking
                // Ideally we'd report unused instance cells, but presumably a signal inside one
                // would get reported as unused
            } അല്ലെങ്കിൽ എങ്കിൽ (allU && allD) {
                // It's fine
            } അല്ലെങ്കിൽ എങ്കിൽ (!anyD && !anyU) {
                // UNDRIVEN is considered more serious - as is more likely a bug,
                // thus undriven+unused bits get UNUSED warnings, as they're not as buggy.
                എങ്കിൽ (!unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not driven, nor used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL,
                                                     സത്യം);  // Warn only once
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (allD && !anyU) {
                എങ്കിൽ (!unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL,
                                                     സത്യം);  // Warn only once
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (!anyD && allU) {
                nodep->v3warn(UNDRIVEN, varType << " is not driven: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNDRIVEN, സത്യം);  // Warn only once
            } അല്ലെങ്കിൽ എങ്കിൽ (!funcInout) {
                // Bits have different dispositions
                സ്ഥിരം std::string varTypeLower = [&varType]() {
                    std::string str = varType;
                    str[0] = std::tolower(static_cast<unsigned അക്ഷരം>(str[0]));
                    തിരികെ str;
                }();
                തർക്കം setU = അസത്യം;
                തർക്കം setD = അസത്യം;
                എങ്കിൽ (anynotDU && !unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  "Bits of " << varTypeLower << " are not driven, nor used: "
                                             << nodep->prettyNameQ() << bitNames(BN_BOTH));
                    setU = സത്യം;
                }
                എങ്കിൽ (anyDnotU && !unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL, "Bits of " << varTypeLower << " are not used: "
                                                           << nodep->prettyNameQ()
                                                           << bitNames(BN_UNUSED));
                    setU = സത്യം;
                }
                എങ്കിൽ (anyUnotD) {
                    nodep->v3warn(UNDRIVEN, "Bits of " << varTypeLower << " are not driven: "
                                                       << nodep->prettyNameQ()
                                                       << bitNames(BN_UNDRIVEN));
                    setD = സത്യം;
                }
                എങ്കിൽ (setU) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, സത്യം);
                }
                എങ്കിൽ (setD) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNDRIVEN, സത്യം);
                }
            }
        }
    }

    ശൂന്യം drivenViaCall(സ്ഥിരം Ast_കണ്ണി_FTask_Ref* nodep) {
        drivenWhole(nodep);
        എങ്കിൽ (!m_callNodep) m_callNodep = nodep;
    }
    സ്ഥിരം Ast_കണ്ണി_FTask_Ref* callNodep() സ്ഥിരം { തിരികെ m_callNodep; }
};

//######################################################################
// Undriven state, as a visitor of each AstNode

വർഗ്ഗം UndrivenVisitor final : പൊതു VNVisitorConst {
    // NODE STATE
    // Netlist:
    //  AstVar::user1p          -> UndrivenVar* for usage var, 0=not set yet
    സ്ഥിരം VNUser1InUse m_inuser1;
    // Each always:
    //  AstNode::user2p         -> UndrivenVar* for usage var, 0=not set yet
    സ്ഥിരം VNUser2InUse m_inuser2;

    // STATE
    std::array<std::vector<UndrivenVarEntry*>, 3> m_entryps = {};  // Nodes to delete when finished
    തർക്കം m_inBBox = അസത്യം;  // In black box; mark as driven+used
    തർക്കം m_inContAssign = അസത്യം;  // In continuous assignment
    തർക്കം m_inInitial = അസത്യം;  // In explicit initial block
    തർക്കം m_inInitialSetup = അസത്യം;  // In InitialAutomatic*/InitialStatic* assignment LHS
    തർക്കം m_inInitialStatic = അസത്യം;  // In InitialStatic
    തർക്കം m_inProcAssign = അസത്യം;  // In procedural assignment
    തർക്കം m_inFTaskRef = അസത്യം;  // In function or task call
    തർക്കം m_inInoutOrRefPin = അസത്യം;  // Connected to pin that is inout
    തർക്കം m_inSelLhs = അസത്യം;  // Iterating the fromp of an AstSel (a partial-bit write target)
    സ്ഥിരം Ast_കണ്ണി_FTask* m_taskp = nullptr;  // Current task
    സ്ഥിരം AstAlways* m_alwaysp = nullptr;  // Current always of either type
    സ്ഥിരം AstAlways* m_alwaysCombp = nullptr;  // Current always if combo, otherwise nullptr
    സ്ഥിരം AstAlways* m_alwaysFFp = nullptr;  // Current always if ff, otherwise nullptr
    സ്ഥിരം AstAlways* m_alwaysPlainp = nullptr;  // Current always if plain (not comb/ff/latch)
    സ്ഥിരം AstClocking* m_clockingp = nullptr;  // Current clocking block, otherwise nullptr

    V3UndrivenCapture* സ്ഥിരം m_capturep = nullptr;  // Capture object.  'nullptr' if disabled.

    // METHODS

    UndrivenVarEntry* getEntryp(AstVar* nodep, പൂർണ്ണസംഖ്യ which_user) {
        എങ്കിൽ (!(which_user == 1 ? nodep->user1p() : nodep->user2p())) {
            UndrivenVarEntry* സ്ഥിരം entryp = പുതിയ UndrivenVarEntry{nodep};
            // UINFO(9," Associate u="<<which_user<<" "<<cvtToHex(this)<<" "<<nodep->name()<<endl);
            m_entryps[which_user].തള്ളുക_back(entryp);
            എങ്കിൽ (which_user == 1) {
                nodep->user1p(entryp);
            } അല്ലെങ്കിൽ എങ്കിൽ (which_user == 2) {
                nodep->user2p(entryp);
            } അല്ലെങ്കിൽ {
                nodep->v3fatalSrc("Bad case");
            }
            തിരികെ entryp;
        } അല്ലെങ്കിൽ {
            UndrivenVarEntry* സ്ഥിരം entryp = reinterpret_cast<UndrivenVarEntry*>(
                which_user == 1 ? nodep->user1p() : nodep->user2p());
            തിരികെ entryp;
        }
    }

    ശൂന്യം warnAlwCombOrder(Ast_കണ്ണി_Var_Ref* nodep, സ്ഥിരം Ast_കണ്ണി* readp) {
        AstVar* സ്ഥിരം varp = nodep->varp();
        എങ്കിൽ (!varp->isParam() && !varp->isGenVar() && !varp->isUsedLoopIdx()
            && !varp->ignoreSchedWrite()
            && !m_inBBox  // We may have falsely considered a SysIgnore as a driver
            && !VN_IS(nodep, VarXRef)  // Xrefs might point at two different instances
            && !varp->fileline()->warnIsOff(
                V3ErrorCode::ALWCOMBORDER)) {  // Warn only once per variable
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
            varp->fileline()->modifyWarnOff(V3ErrorCode::ALWCOMBORDER,
                                            സത്യം);  // Complain just once for any usage
        }
    }
    // A clocking block 'output' is an additional driver of the signal it names.
    // The conflict is found from whichever driver is reached second.
    തർക്കം clockingDrivesOther(സ്ഥിരം UndrivenVarEntry* entryp, തർക്കം otherWriteIsStaticInit) സ്ഥിരം {
        സ്ഥിരം തർക്കം otherIsExplicitProc
            = entryp->isDrivenAlwaysCombWhole() || entryp->isDrivenAlwaysFFWhole();
        തിരികെ m_clockingp && !otherIsExplicitProc && !otherWriteIsStaticInit
               && m_clockingp != entryp->getClockingp();
    }
    തർക്കം otherDrivesClocking(സ്ഥിരം UndrivenVarEntry* entryp) സ്ഥിരം {
        തിരികെ !m_clockingp && !m_alwaysCombp && !m_alwaysFFp && !m_inInitialStatic
               && entryp->isDrivenClockingWhole();
    }
    // A continuous assignment contending with a clocking block 'output' is an
    // unambiguous driver conflict. Multiple clocking blocks may drive the same
    // signal, the last drive wins (IEEE 1800-2023 14.16.2).
    ശൂന്യം warnClockingDriven(Ast_കണ്ണി_Var_Ref* nodep, സ്ഥിരം UndrivenVarEntry* entryp,
                            സ്ഥിരം Ast_കണ്ണി* otherWritep, തർക്കം otherWriteIsStaticInit) {
        എങ്കിൽ (clockingDrivesOther(entryp, otherWriteIsStaticInit)
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
        എങ്കിൽ (otherDrivesClocking(entryp) && m_inContAssign) {
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
    ശൂന്യം warnClockingDrivenProc(Ast_കണ്ണി_Var_Ref* nodep, സ്ഥിരം UndrivenVarEntry* entryp,
                                സ്ഥിരം Ast_കണ്ണി* otherWritep, തർക്കം otherWriteIsStaticInit) {
        എങ്കിൽ (clockingDrivesOther(entryp, otherWriteIsStaticInit) && !entryp->getClockingp()
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
        എങ്കിൽ (otherDrivesClocking(entryp) && !m_inContAssign) {
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
    ശൂന്യം visit(AstVar* nodep) override {
        സ്ഥിരം തർക്കം funcInout = nodep->isFuncLocal() && nodep->isInout();
        വേണ്ടി (പൂർണ്ണസംഖ്യ usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            // For assigns and non-combo always, do just usr==1, to look
            // for module-wide undriven etc.
            // For combo always, run both usr==1 for above, and also
            // usr==2 for always-only checks.
            UndrivenVarEntry* സ്ഥിരം entryp = getEntryp(nodep, usr);
            എങ്കിൽ ((nodep->isNonOutput() && !funcInout) || nodep->isSigPublic()
                || nodep->hasUserInit() || nodep->isSigUserRWPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->drivenWhole(nodep);
            }
            എങ്കിൽ ((nodep->isWritable() && !funcInout) || nodep->isSigPublic()
                || nodep->isSigUserRWPublic() || nodep->isSigUserRdPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->usedWhole(nodep);
            }
            എങ്കിൽ (nodep->valuep()) entryp->drivenWhole(nodep->valuep());
        }
        // Discover variables used in bit definitions, etc
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstArraySel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstSliceSel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstSel* nodep) override {
        Ast_കണ്ണി_Var_Ref* സ്ഥിരം varrefp = VN_CAST(nodep->fromp(), കണ്ണി_Var_Ref);
        AstConst* സ്ഥിരം constp = VN_CAST(nodep->lsbp(), Const);
        എങ്കിൽ (varrefp && constp && !constp->അക്കം().isFourState()) {
            വേണ്ടി (പൂർണ്ണസംഖ്യ usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                UndrivenVarEntry* സ്ഥിരം entryp = getEntryp(varrefp->varp(), usr);
                സ്ഥിരം പൂർണ്ണസംഖ്യ lsb = constp->toUInt();
                എങ്കിൽ (m_inBBox || varrefp->access().isWriteOrRW()) {
                    // Don't warn if already driven earlier as "a=0; if(a) a=1;" is fine.
                    എങ്കിൽ (usr == 2 && m_alwaysCombp
                        && entryp->isUsedNotDrivenBit(lsb, nodep->width())) {
                        UINFO(9, " Select.  Entryp=" << cvtToHex(entryp));
                        warnAlwCombOrder(varrefp, entryp->firstUsedNotDrivenp());
                    }
                    entryp->drivenBit(lsb, nodep->width());
                }
                എങ്കിൽ (m_inBBox || !varrefp->access().isWriteOrRW())
                    entryp->usedBit(lsb, nodep->width(), varrefp);
            }
        } അല്ലെങ്കിൽ {
            // skip over static longest static prefix
            iterateConst(nodep->lsbp());
            VL_RESTORER(m_inSelLhs);
            m_inSelLhs = !V3Width::selectNonConstantRecurse(nodep->lsbp(), /*inSel=*/സത്യം);
            iterateConst(nodep->fromp());
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        // Any variable
        എങ്കിൽ (nodep->access().isWriteOrRW()
            && !VN_IS(nodep, VarXRef)) {  // Ignore interface variables and similar ugly items
            എങ്കിൽ (m_inProcAssign && !nodep->varp()->varType().isProcAssignable()
                && !nodep->varp()->isDeclTyped()  //
                && !nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()) {
                nodep->v3warn(PROCASSWIRE, "Procedural assignment to wire, perhaps intended var"
                                               << " (IEEE 1800-2023 6.5): "
                                               << nodep->prettyNameQ());
            } അല്ലെങ്കിൽ എങ്കിൽ (m_inContAssign && !nodep->varp()->varType().isContAssignable()
                       && !nodep->fileline()->language().systemVerilog()) {
                nodep->v3warn(CONTASSREG,
                              "Continuous assignment to reg, perhaps intended wire"
                                  << " (IEEE 1364-2005 6.1; Verilog only, legal in SV): "
                                  << nodep->prettyNameQ());
            }
            എങ്കിൽ (m_inFTaskRef && nodep->varp()->varType().isNet()) {
                nodep->v3warn(
                    PROCASSWIRE,
                    "Passed wire on output or inout subroutine argument, expected expression that "
                    "is valid on the left hand side of a procedural assignment"
                        << " (IEEE 1800-2023 13.5): " << nodep->prettyNameQ());
            }
        }

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Remember that anything driven here doesn't count toward MULTIDRIVEN.
        തർക്കം ftaskDef = അസത്യം;
        എങ്കിൽ (m_taskp && !m_alwaysp && !m_inContAssign && !m_inInitialStatic && !m_inBBox
            && !m_taskp->dpiExport()) {
            AstVar* സ്ഥിരം retVarp = VN_CAST(m_taskp->fvarp(), Var);
            എങ്കിൽ (!retVarp || nodep->varp() != retVarp) ftaskDef = സത്യം;
        }

        വേണ്ടി (പൂർണ്ണസംഖ്യ usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            UndrivenVarEntry* സ്ഥിരം entryp = getEntryp(nodep->varp(), usr);
            സ്ഥിരം തർക്കം fdrv = nodep->access().isWriteOrRW()
                              && nodep->varp()->attrFileDescr();  // FD's are also being read from
            എങ്കിൽ (m_inBBox || nodep->access().isWriteOrRW()) {
                എങ്കിൽ (usr == 2 && m_alwaysCombp && entryp->isUsedNotDrivenAny()) {
                    UINFO(9, " Full bus.  Entryp=" << cvtToHex(entryp));
                    warnAlwCombOrder(nodep, entryp->firstUsedNotDrivenp());
                }
                സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം otherVarRefp = entryp->getNodep();
                സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം otherWritep = otherVarRefp
                                                       ? static_cast<സ്ഥിരം Ast_കണ്ണി*>(otherVarRefp)
                                                       : entryp->callNodep();
                സ്ഥിരം തർക്കം same_File_വരി
                    = otherVarRefp && nodep->fileline() == otherVarRefp->fileline();
                // Preconditions shared by MULTIDRIVEN and MULTIDRIVENPROC.
                സ്ഥിരം തർക്കം multidrivenCommon
                    = entryp->isDrivenWhole() && !m_inBBox && !VN_IS(nodep, VarXRef)
                      && !VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType) && !same_File_വരി
                      && !entryp->isUnderGen() && otherWritep && !entryp->isFtaskDriven()
                      && !ftaskDef && !m_inSelLhs;
                // The two warnings are gated independently on the variable
                // declaration's fileline, as v3warn suppression will check
                // the driving fileline and still warn even if the warning
                // was suppressed with lint_off at the declaration.
                സ്ഥിരം തർക്കം otherWriteIsStaticInit
                    = nodep->varp()->hasUserInit() && otherWritep == entryp->initStaticp();
                എങ്കിൽ (multidrivenCommon
                    && !nodep->varp()->fileline()->warnIsOff(V3ErrorCode::MULTIDRIVEN)) {
                    എങ്കിൽ (m_alwaysCombp
                        && (!entryp->isDrivenAlwaysCombWhole()
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
                    എങ്കിൽ (!m_alwaysCombp && entryp->isDrivenAlwaysCombWhole()) {
                        nodep->v3warn(MULTIDRIVEN, "Variable also written to in always_comb"
                                                       << " (IEEE 1800-2023 9.2.2.2): "
                                                       << nodep->prettyNameQ() << '\n'
                                                       << nodep->warnOther() << '\n'
                                                       << nodep->warnContextPrimary() << '\n'
                                                       << otherWritep->warnOther()
                                                       << "... Location of always_comb write\n"
                                                       << otherWritep->warnContextSecondary());
                    }
                    എങ്കിൽ (m_alwaysFFp && !otherWriteIsStaticInit
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
                    എങ്കിൽ (!m_alwaysFFp && !m_inInitialStatic && entryp->isDrivenAlwaysFFWhole()) {
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
                എങ്കിൽ (multidrivenCommon && !nodep->varp()->isUsedLoopIdx()
                    && !nodep->varp()->fileline()->warnIsOff(V3ErrorCode::MULTIDRIVENPROC)) {
                    // Two plain always blocks driving the whole signal: legal
                    // SystemVerilog, but a driver conflict for synthesis. The
                    // always_ff/always_comb cases above already cover mixes with
                    // an explicit process, so only warn for plain+plain here.
                    // When the two blocks are clocked differently, the
                    // (on-by-default) MULTIDRIVEN check in V3Delayed already reports
                    // the conflict, so don't also emit MULTIDRIVENPROC for that case.
                    സ്ഥിരം AstAlways* സ്ഥിരം otherAlwaysp = entryp->getAlwPlainp();
                    സ്ഥിരം Ast_Sen_വൃക്ഷം* സ്ഥിരം senp
                        = m_alwaysPlainp ? m_alwaysPlainp->sentreep() : nullptr;
                    സ്ഥിരം Ast_Sen_വൃക്ഷം* സ്ഥിരം otherSenp
                        = otherAlwaysp ? otherAlwaysp->sentreep() : nullptr;
                    സ്ഥിരം തർക്കം differentClocking = senp && otherSenp && senp->hasClocked()
                                                   && otherSenp->hasClocked()
                                                   && !senp->same_വൃക്ഷം(otherSenp);
                    എങ്കിൽ (m_alwaysPlainp && entryp->isDrivenAlwaysPlainWhole()
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
                എങ്കിൽ (!m_inInitialSetup || nodep->varp()->hasUserInit()) {
                    // Else don't count default initialization as a driver to a net/variable
                    entryp->drivenWhole(nodep, ftaskDef);
                }
                എങ്കിൽ (m_alwaysCombp && entryp->isDrivenAlwaysCombWhole()
                    && m_alwaysCombp != entryp->getAlwCombp()
                    && m_alwaysCombp->fileline() == entryp->getAlwCombp()->fileline())
                    entryp->underGenerate();
                എങ്കിൽ (m_alwaysCombp) entryp->drivenAlwaysCombWhole(m_alwaysCombp);
                എങ്കിൽ (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, nodep->varp());
                എങ്കിൽ (m_alwaysPlainp) entryp->drivenAlwaysPlainWhole(m_alwaysPlainp);
                എങ്കിൽ (m_clockingp) entryp->drivenClockingWhole(m_clockingp);
            }
            എങ്കിൽ (nodep->access().isWriteOrRW() && !VN_IS(nodep, VarXRef)) {
                // Ignoring xrefs as the initial and assignment to track might refer to two
                // different instances.  Ideally all of V3Undriven would move after V3Scope,
                // then could use VarScope tracking instead.
                എങ്കിൽ (m_inInitialStatic && !entryp->initStaticp()) entryp->initStaticp(nodep);
                എങ്കിൽ (m_inInitial && !entryp->initialp()) entryp->initialp(nodep);
                എങ്കിൽ (m_inContAssign && !entryp->contAssignp()) entryp->contAssignp(nodep);
                എങ്കിൽ (m_alwaysp && m_inProcAssign && !entryp->procWritep())
                    entryp->procWritep(nodep);
            }
            എങ്കിൽ ((!m_inInitialSetup || nodep->varp()->hasUserInit())
                && (m_inBBox || nodep->access().isReadOrRW()
                    || fdrv
                    // Inouts have only isWrite set, as we don't have more
                    // information and operating on module boundary, treat as
                    // both read and writing
                    || m_inInoutOrRefPin))
                entryp->usedWhole(nodep);
        }
    }

    // Don't know what black boxed calls do, assume in+out
    ശൂന്യം visit(AstSysIgnore* nodep) override {
        VL_RESTORER(m_inBBox);
        m_inBBox = സത്യം;
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(AstAssign* nodep) override {
        VL_RESTORER(m_inProcAssign);
        m_inProcAssign = സത്യം;
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = അസത്യം;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }
    ശൂന്യം visit(AstAssignDly* nodep) override {
        VL_RESTORER(m_inProcAssign);
        m_inProcAssign = സത്യം;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstAssignForce* nodep) override {
        iterateConst(nodep->rhsp());
        VL_RESTORER(m_inInitial);
        m_inInitial = അസത്യം;
        iterateConst(nodep->lhsp());
    }
    ശൂന്യം visit(AstAssignW* nodep) override {
        VL_RESTORER(m_inContAssign);
        m_inContAssign = സത്യം;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstRelease* nodep) override {
        VL_RESTORER(m_inInitial);
        m_inInitial = അസത്യം;
        iterateConst(nodep->lhsp());
    }
    ശൂന്യം visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        m_inInitial = സത്യം;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstInitialAutomatic* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = സത്യം;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = സത്യം;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstInitialStatic* nodep) override {
        VL_RESTORER(m_inInitialStatic);
        m_inInitialStatic = സത്യം;
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = സത്യം;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = സത്യം;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        VL_RESTORER(m_alwaysCombp);
        VL_RESTORER(m_alwaysFFp);
        VL_RESTORER(m_alwaysPlainp);
        Ast_കണ്ണി::user2_Clear_വൃക്ഷം();
        m_alwaysp = nodep;
        എങ്കിൽ (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            UINFO(9, "   " << nodep);
            m_alwaysCombp = nodep;
        } അല്ലെങ്കിൽ {
            m_alwaysCombp = nullptr;
        }
        m_alwaysFFp = nodep->keyword() == VAlwaysKwd::ALWAYS_FF ? nodep : nullptr;
        m_alwaysPlainp = nodep->keyword() == VAlwaysKwd::ALWAYS ? nodep : nullptr;
        iterateChildrenConst(nodep);
        എങ്കിൽ (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) UINFO(9, "   Done " << nodep);
    }
    ശൂന്യം visit(AstClocking* nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        VL_RESTORER(m_inFTaskRef);
        m_inFTaskRef = സത്യം;

        iterateChildrenConst(nodep);

        എങ്കിൽ (!m_capturep) തിരികെ;

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Do not apply writeSummary at calls inside a task definition, or they will look like
        // independent drivers (phantom MULTIDRIVEN).
        സ്ഥിരം തർക്കം inExecutedContext
            = !(m_taskp && !m_alwaysp && !m_inContAssign && !m_inInitialStatic && !m_inBBox
                && !m_taskp->dpiExport());

        എങ്കിൽ (!inExecutedContext) തിരികെ;

        Ast_കണ്ണി_FTask* സ്ഥിരം calleep = nodep->taskp();
        എങ്കിൽ (!calleep) തിരികെ;

        സ്ഥിരം auto& vars = m_capturep->writeSummary(calleep);
        വേണ്ടി (AstVar* സ്ഥിരം varp : vars) {
            വേണ്ടി (പൂർണ്ണസംഖ്യ usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                UndrivenVarEntry* സ്ഥിരം entryp = getEntryp(varp, usr);
                entryp->drivenViaCall(nodep);
                എങ്കിൽ (m_alwaysCombp) entryp->drivenAlwaysCombWhole(m_alwaysCombp);
                എങ്കിൽ (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, varp);
            }
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        VL_RESTORER(m_taskp);
        m_taskp = nodep;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstPin* nodep) override {
        VL_RESTORER(m_inInoutOrRefPin);
        m_inInoutOrRefPin = nodep->modVarp()->isInoutOrRef();
        iterateChildrenConst(nodep);
    }

    // Until we support tables, primitives will have undriven and unused I/Os
    ശൂന്യം visit(AstPrimitive*) override {}

    // Coverage artifacts etc shouldn't count as a sink
    ശൂന്യം visit(Ast_കണ്ണി_Cover_Decl*) override {}
    ശൂന്യം visit(AstCoverInc*) override {}
    ശൂന്യം visit(AstCoverToggle*) override {}
    ശൂന്യം visit(AstTraceDecl* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    ശൂന്യം visit(AstTraceInc* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }

    // iterate
    ശൂന്യം visit(AstConst* nodep) override {}
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit UndrivenVisitor(AstNetlist* nodep, V3UndrivenCapture* capturep)
        : m_capturep{capturep} {
        iterateConst(nodep);
    }

    ~UndrivenVisitor() override {
        വേണ്ടി (UndrivenVarEntry* ip : m_entryps[1]) ip->reportViolations();
        വേണ്ടി (പൂർണ്ണസംഖ്യ usr = 1; usr < 3; ++usr) {
            വേണ്ടി (UndrivenVarEntry* ip : m_entryps[usr]) delete ip;
        }
    }
};

//######################################################################
// Undriven class functions

ശൂന്യം V3Undriven::undrivenAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    V3UndrivenCapture capture{nodep};
    { UndrivenVisitor{nodep, &capture}; }

    എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStage("undriven");
}
