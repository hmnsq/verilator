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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Undriven.h"

#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3UndrivenCapture.h"
#अन्तर्भाव "V3Width.h"

#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Class for every variable we may process

वर्ग UndrivenVarEntry final {
    // MEMBERS
    AstVar* ध्रुव m_varp;  // Variable this tracks
    std::vector<तार्किक> m_wholeFlags;  // Used/Driven on whole vector
    std::vector<तार्किक> m_bitFlags;  // Used/Driven on each subbit
    ध्रुव Ast_गाठ* m_usedNotDrivenp = nullptr;  // First read before any write
    ध्रुव AstAlways* m_alwCombp
        = nullptr;  // always_comb of var if driven within always_comb, else nullptr
    ध्रुव AstAlways* m_alwFFp = nullptr;  // always_ff of var if driven within always_ff
    ध्रुव AstAlways* m_alwPlainp = nullptr;  // plain always of var if driven within plain always
    ध्रुव AstClocking* m_clockingp = nullptr;  // clocking block of var if driven as output
    ध्रुव Ast_गाठ_Var_Ref* m_nodep = nullptr;  // varref if driven, else nullptr
    ध्रुव Ast_गाठ* m_initStaticp = nullptr;  // varref if in InitialStatic driven
    ध्रुव Ast_गाठ* m_initialp = nullptr;  // varref if driven in an explicit initial block
    ध्रुव Ast_गाठ* m_contAssignp = nullptr;  // varref if in continuous assignment driven
    ध्रुव Ast_गाठ* m_procWritep = nullptr;  // varref if written in process
    तार्किक m_underGen = असत्य;  // Under a generate
    तार्किक m_ftaskDriven = असत्य;  // Last driven by function or task

    ध्रुव Ast_गाठ_FTask_Ref* m_callNodep = nullptr;  // Call node if driven via writeSummary

    गणना : uint8_t {
        FLAG_USED = 0,  // Signal or bit has been read/observed
        FLAG_DRIVEN = 1,  // Signal or bit has been written/driven
        FLAG_DRIVEN_ALWCOMB = 2,  // Whole signal has been driven from always_comb
        FLAG_DRIVEN_ALWFF = 3,  // Whole signal has been driven from always_ff
        FLAG_DRIVEN_ALWPLAIN = 4,  // Whole signal has been driven from a plain always
        FLAG_DRIVEN_CLOCKING = 5,  // Whole signal has been driven as a clocking block output
        FLAGS_PER_BIT = 6  // Number of flags stored for each tracked bit
    };

सार्वजनिक:
    // CONSTRUCTORS
    explicit UndrivenVarEntry(AstVar* varp)
        : m_varp{varp} {  // Construction for when a var is used
        UINFO(9, "create " << varp);
        m_wholeFlags.resize(FLAGS_PER_BIT);
        प्रति (पूर्णाङ्क i = 0; i < FLAGS_PER_BIT; i++) m_wholeFlags[i] = असत्य;
        m_bitFlags.resize(varp->width() * FLAGS_PER_BIT);
        प्रति (पूर्णाङ्क i = 0; i < varp->width() * FLAGS_PER_BIT; i++) m_bitFlags[i] = असत्य;
    }
    ~UndrivenVarEntry() = स्वभाव;

गुप्त:
    // METHODS
    तार्किक bit_अंक_Ok(पूर्णाङ्क bit) ध्रुव {
        प्रत्यागम bit >= 0 && (bit * FLAGS_PER_BIT < static_cast<पूर्णाङ्क>(m_bitFlags.size()));
    }
    तार्किक usedFlag(पूर्णाङ्क bit) ध्रुव {
        प्रत्यागम m_wholeFlags[FLAG_USED] || m_bitFlags[bit * FLAGS_PER_BIT + FLAG_USED];
    }
    तार्किक drivenFlag(पूर्णाङ्क bit) ध्रुव {
        प्रत्यागम m_wholeFlags[FLAG_DRIVEN] || m_bitFlags[bit * FLAGS_PER_BIT + FLAG_DRIVEN];
    }
    पूर्णाङ्क bit_मोजणी() ध्रुव { प्रत्यागम m_bitFlags.size() / FLAGS_PER_BIT; }
    शून्य recordUsedNotDriven(ध्रुव Ast_गाठ* nodep) {
        यदि (!m_usedNotDrivenp) m_usedNotDrivenp = nodep;
    }
    गणना BitNamesWhich : uint8_t { BN_UNUSED, BN_UNDRIVEN, BN_BOTH };
    string bitNames(BitNamesWhich which) {
        string bits;
        तार्किक prev = असत्य;
        पूर्णाङ्क msb = 0;
        // bit==-1 loops below; we do one extra iteration so end with prev=false
        प्रति (पूर्णाङ्क bit = (m_bitFlags.size() / FLAGS_PER_BIT) - 1; bit >= -1; --bit) {
            यदि (bit >= 0
                && ((which == BN_UNUSED && !usedFlag(bit) && drivenFlag(bit))
                    || (which == BN_UNDRIVEN && usedFlag(bit) && !drivenFlag(bit))
                    || (which == BN_BOTH && !usedFlag(bit) && !drivenFlag(bit)))) {
                यदि (!prev) {
                    prev = सत्य;
                    msb = bit;
                }
            } अन्यथा यदि (prev) {
                ध्रुव AstBasicDType* ध्रुव bdtypep = m_varp->basicp();
                ध्रुव पूर्णाङ्क lsb = bit + 1;
                यदि (bits != "") bits += ",";
                यदि (lsb == msb) {
                    bits += cvtToStr(lsb + bdtypep->lo());
                } अन्यथा {
                    यदि (bdtypep->ascending()) {
                        bits
                            += cvtToStr(lsb + bdtypep->lo()) + ":" + cvtToStr(msb + bdtypep->lo());
                    } अन्यथा {
                        bits
                            += cvtToStr(msb + bdtypep->lo()) + ":" + cvtToStr(lsb + bdtypep->lo());
                    }
                }
                prev = असत्य;
            }
        }
        प्रत्यागम "[" + bits + "]";
    }

सार्वजनिक:
    शून्य usedWhole(ध्रुव Ast_गाठ* nodep) {
        UINFO(9, "set u[*] " << m_varp->name() << " " << nodep);
        यदि (!m_wholeFlags[FLAG_DRIVEN]) {
            प्रति (पूर्णाङ्क bit = 0; bit < bit_मोजणी(); ++bit) {
                यदि (!drivenFlag(bit)) {
                    recordUsedNotDriven(nodep);
                    भङ्ग;
                }
            }
        }
        m_wholeFlags[FLAG_USED] = सत्य;
    }
    शून्य drivenWhole(ध्रुव Ast_गाठ* nodep) {
        UINFO(9, "set d[*] " << m_varp->name() << " " << nodep);
        m_wholeFlags[FLAG_DRIVEN] = सत्य;
    }
    शून्य drivenWhole(ध्रुव Ast_गाठ_Var_Ref* nodep, तार्किक ftaskDef) {
        ध्रुव तार्किक wasDriven = isDrivenWhole();
        drivenWhole(nodep);
        // A write in a task/function definition is not an executed process, so it must not
        // displace an already recorded write, nor clear the flag that keeps such a write
        // from being reported as the other write of a MULTIDRIVEN
        यदि (ftaskDef && wasDriven) प्रत्यागम;
        m_ftaskDriven = ftaskDef;
        m_nodep = nodep;
    }
    शून्य drivenAlwaysCombWhole(ध्रुव AstAlways* alwCombp) {
        m_wholeFlags[FLAG_DRIVEN_ALWCOMB] = सत्य;
        m_alwCombp = alwCombp;
    }
    शून्य drivenAlwaysFFWhole(ध्रुव AstAlways* alwFFp, ध्रुव AstVar* varp) {
        m_wholeFlags[FLAG_DRIVEN_ALWFF] = सत्य;
        m_alwFFp = alwFFp;
    }
    शून्य drivenAlwaysPlainWhole(ध्रुव AstAlways* alwPlainp) {
        m_wholeFlags[FLAG_DRIVEN_ALWPLAIN] = सत्य;
        m_alwPlainp = alwPlainp;
    }
    शून्य drivenClockingWhole(ध्रुव AstClocking* clockingp) {
        m_wholeFlags[FLAG_DRIVEN_CLOCKING] = सत्य;
        m_clockingp = clockingp;
    }

    ध्रुव Ast_गाठ* initStaticp() ध्रुव { प्रत्यागम m_initStaticp; }
    शून्य initStaticp(ध्रुव Ast_गाठ* nodep) { m_initStaticp = nodep; }
    ध्रुव Ast_गाठ* initialp() ध्रुव { प्रत्यागम m_initialp; }
    शून्य initialp(ध्रुव Ast_गाठ* nodep) { m_initialp = nodep; }
    ध्रुव Ast_गाठ* contAssignp() ध्रुव { प्रत्यागम m_contAssignp; }
    शून्य contAssignp(ध्रुव Ast_गाठ* nodep) { m_contAssignp = nodep; }
    ध्रुव Ast_गाठ* procWritep() ध्रुव { प्रत्यागम m_procWritep; }
    शून्य procWritep(ध्रुव Ast_गाठ* nodep) { m_procWritep = nodep; }
    शून्य underGenerate() { m_underGen = सत्य; }
    तार्किक isUnderGen() ध्रुव { प्रत्यागम m_underGen; }
    तार्किक isDrivenWhole() ध्रुव { प्रत्यागम m_wholeFlags[FLAG_DRIVEN]; }
    तार्किक isDrivenAlwaysCombWhole() ध्रुव { प्रत्यागम m_wholeFlags[FLAG_DRIVEN_ALWCOMB]; }
    तार्किक isDrivenAlwaysFFWhole() ध्रुव { प्रत्यागम m_wholeFlags[FLAG_DRIVEN_ALWFF]; }
    तार्किक isDrivenAlwaysPlainWhole() ध्रुव { प्रत्यागम m_wholeFlags[FLAG_DRIVEN_ALWPLAIN]; }
    तार्किक isDrivenClockingWhole() ध्रुव { प्रत्यागम m_wholeFlags[FLAG_DRIVEN_CLOCKING]; }
    तार्किक isFtaskDriven() ध्रुव { प्रत्यागम m_ftaskDriven; }
    ध्रुव Ast_गाठ_Var_Ref* getNodep() ध्रुव { प्रत्यागम m_nodep; }
    ध्रुव AstAlways* getAlwCombp() ध्रुव { प्रत्यागम m_alwCombp; }
    ध्रुव AstAlways* getAlwFFp() ध्रुव { प्रत्यागम m_alwFFp; }
    ध्रुव AstAlways* getAlwPlainp() ध्रुव { प्रत्यागम m_alwPlainp; }
    ध्रुव AstClocking* getClockingp() ध्रुव { प्रत्यागम m_clockingp; }
    शून्य usedBit(पूर्णाङ्क bit, पूर्णाङ्क width, ध्रुव Ast_गाठ* nodep) {
        UINFO(9, "set u[" << (bit + width - 1) << ":" << bit << "] " << m_varp->name());
        प्रति (पूर्णाङ्क i = 0; i < width; i++) {
            यदि (bit_अंक_Ok(bit + i)) {
                यदि (!drivenFlag(bit + i)) recordUsedNotDriven(nodep);
                m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_USED] = सत्य;
            }
        }
    }
    शून्य drivenBit(पूर्णाङ्क bit, पूर्णाङ्क width) {
        UINFO(9, "set d[" << (bit + width - 1) << ":" << bit << "] " << m_varp->name());
        प्रति (पूर्णाङ्क i = 0; i < width; i++) {
            यदि (bit_अंक_Ok(bit + i)) m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_DRIVEN] = सत्य;
        }
    }
    तार्किक isUsedNotDrivenBit(पूर्णाङ्क bit, पूर्णाङ्क width) ध्रुव {
        प्रति (पूर्णाङ्क i = 0; i < width; i++) {
            यदि (bit_अंक_Ok(bit + i)
                && (m_wholeFlags[FLAG_USED] || m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_USED])
                && !(m_wholeFlags[FLAG_DRIVEN]
                     || m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_DRIVEN]))
                प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }
    तार्किक isUsedNotDrivenAny() ध्रुव {
        प्रत्यागम isUsedNotDrivenBit(0, m_bitFlags.size() / FLAGS_PER_BIT);
    }
    ध्रुव Ast_गाठ* firstUsedNotDrivenp() ध्रुव { प्रत्यागम m_usedNotDrivenp; }
    स्थिर तार्किक unusedMatch(AstVar* nodep) {
        ध्रुव string regexp = v3Global.opt.unusedRegexp();
        यदि (regexp == "") प्रत्यागम असत्य;
        ध्रुव string prettyName = nodep->prettyName();
        प्रत्यागम VString::wildmatch(prettyName.c_str(), regexp.c_str());
    }
    शून्य reportViolations() {
        // Combine bits into overall state
        AstVar* ध्रुव nodep = m_varp;

        यदि (initStaticp() && procWritep() && nodep->hasUserInit() && !nodep->isClassMember()
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
        ध्रुव Ast_गाठ* ध्रुव initp = nodep->hasUserInit() ? initStaticp() : initialp();
        यदि (initp && contAssignp() && !nodep->isClassMember() && !nodep->isFuncLocal()) {
            initp->v3warn(E_CONTASSINIT, "Continuous assignment to variable with initial value: "
                                             << nodep->prettyNameQ() << '\n'
                                             << initp->warnMore()
                                             << "... Location of variable initialization\n"
                                             << initp->warnContextPrimary() << '\n'
                                             << contAssignp()->warnOther()
                                             << "... Location of continuous assignment\n"
                                             << contAssignp()->warnContextSecondary());
        }
        यदि (nodep->isGenVar()) {  // Genvar
            यदि (!nodep->isIfaceRef() && !nodep->isUsedParam() && !unusedMatch(nodep)) {
                nodep->v3warn(UNUSEDGENVAR, "Genvar is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDGENVAR,
                                                 सत्य);  // Warn only once
            }
        } अन्यथा यदि (nodep->isParam()) {  // Parameter
            यदि (!nodep->isIfaceRef() && !nodep->isUsedParam() && !unusedMatch(nodep)) {
                nodep->v3warn(UNUSEDPARAM, "Parameter is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDPARAM,
                                                 सत्य);  // Warn only once
            }
        } अन्यथा {  // Signal
            ध्रुव string varType{nodep->isFuncLocal() ? "Function variable" : "Signal"};
            ध्रुव तार्किक funcInout = nodep->isFuncLocal() && nodep->isInout();
            तार्किक allU = सत्य;
            तार्किक allD = सत्य;
            तार्किक anyU = m_wholeFlags[FLAG_USED];
            तार्किक anyD = m_wholeFlags[FLAG_DRIVEN];
            तार्किक anyUnotD = असत्य;
            तार्किक anyDnotU = असत्य;
            तार्किक anynotDU = असत्य;
            प्रति (unsigned bit = 0; bit < m_bitFlags.size() / FLAGS_PER_BIT; bit++) {
                ध्रुव तार्किक used = usedFlag(bit);
                ध्रुव तार्किक driv = drivenFlag(bit);
                allU &= used;
                anyU |= used;
                allD &= driv;
                anyD |= driv;
                anyUnotD |= used && !driv;
                anyDnotU |= !used && driv;
                anynotDU |= !used && !driv;
            }
            यदि (funcInout) {
                यदि (anyD) allU = सत्य;
                allD = सत्य;
            }
            यदि (allU) m_wholeFlags[FLAG_USED] = सत्य;
            यदि (allD) m_wholeFlags[FLAG_DRIVEN] = सत्य;
            // Test results
            यदि (nodep->isIfaceRef()) {
                // For interface top level we don't do any tracking
                // Ideally we'd report unused instance cells, but presumably a signal inside one
                // would get reported as unused
            } अन्यथा यदि (allU && allD) {
                // It's fine
            } अन्यथा यदि (!anyD && !anyU) {
                // UNDRIVEN is considered more serious - as is more likely a bug,
                // thus undriven+unused bits get UNUSED warnings, as they're not as buggy.
                यदि (!unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not driven, nor used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL,
                                                     सत्य);  // Warn only once
                }
            } अन्यथा यदि (allD && !anyU) {
                यदि (!unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL,
                                                     सत्य);  // Warn only once
                }
            } अन्यथा यदि (!anyD && allU) {
                nodep->v3warn(UNDRIVEN, varType << " is not driven: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNDRIVEN, सत्य);  // Warn only once
            } अन्यथा यदि (!funcInout) {
                // Bits have different dispositions
                ध्रुव std::string varTypeLower = [&varType]() {
                    std::string str = varType;
                    str[0] = std::tolower(static_cast<unsigned वर्ण>(str[0]));
                    प्रत्यागम str;
                }();
                तार्किक setU = असत्य;
                तार्किक setD = असत्य;
                यदि (anynotDU && !unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  "Bits of " << varTypeLower << " are not driven, nor used: "
                                             << nodep->prettyNameQ() << bitNames(BN_BOTH));
                    setU = सत्य;
                }
                यदि (anyDnotU && !unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL, "Bits of " << varTypeLower << " are not used: "
                                                           << nodep->prettyNameQ()
                                                           << bitNames(BN_UNUSED));
                    setU = सत्य;
                }
                यदि (anyUnotD) {
                    nodep->v3warn(UNDRIVEN, "Bits of " << varTypeLower << " are not driven: "
                                                       << nodep->prettyNameQ()
                                                       << bitNames(BN_UNDRIVEN));
                    setD = सत्य;
                }
                यदि (setU) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, सत्य);
                }
                यदि (setD) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNDRIVEN, सत्य);
                }
            }
        }
    }

    शून्य drivenViaCall(ध्रुव Ast_गाठ_FTask_Ref* nodep) {
        drivenWhole(nodep);
        यदि (!m_callNodep) m_callNodep = nodep;
    }
    ध्रुव Ast_गाठ_FTask_Ref* callNodep() ध्रुव { प्रत्यागम m_callNodep; }
};

//######################################################################
// Undriven state, as a visitor of each AstNode

वर्ग UndrivenVisitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    // Netlist:
    //  AstVar::user1p          -> UndrivenVar* for usage var, 0=not set yet
    ध्रुव VNUser1InUse m_inuser1;
    // Each always:
    //  AstNode::user2p         -> UndrivenVar* for usage var, 0=not set yet
    ध्रुव VNUser2InUse m_inuser2;

    // STATE
    std::array<std::vector<UndrivenVarEntry*>, 3> m_entryps = {};  // Nodes to delete when finished
    तार्किक m_inBBox = असत्य;  // In black box; mark as driven+used
    तार्किक m_inContAssign = असत्य;  // In continuous assignment
    तार्किक m_inInitial = असत्य;  // In explicit initial block
    तार्किक m_inInitialSetup = असत्य;  // In InitialAutomatic*/InitialStatic* assignment LHS
    तार्किक m_inInitialStatic = असत्य;  // In InitialStatic
    तार्किक m_inProcAssign = असत्य;  // In procedural assignment
    तार्किक m_inFTaskRef = असत्य;  // In function or task call
    तार्किक m_inInoutOrRefPin = असत्य;  // Connected to pin that is inout
    तार्किक m_inSelLhs = असत्य;  // Iterating the fromp of an AstSel (a partial-bit write target)
    ध्रुव Ast_गाठ_FTask* m_taskp = nullptr;  // Current task
    ध्रुव AstAlways* m_alwaysp = nullptr;  // Current always of either type
    ध्रुव AstAlways* m_alwaysCombp = nullptr;  // Current always if combo, otherwise nullptr
    ध्रुव AstAlways* m_alwaysFFp = nullptr;  // Current always if ff, otherwise nullptr
    ध्रुव AstAlways* m_alwaysPlainp = nullptr;  // Current always if plain (not comb/ff/latch)
    ध्रुव AstClocking* m_clockingp = nullptr;  // Current clocking block, otherwise nullptr

    V3UndrivenCapture* ध्रुव m_capturep = nullptr;  // Capture object.  'nullptr' if disabled.

    // METHODS

    UndrivenVarEntry* getEntryp(AstVar* nodep, पूर्णाङ्क which_user) {
        यदि (!(which_user == 1 ? nodep->user1p() : nodep->user2p())) {
            UndrivenVarEntry* ध्रुव entryp = नूतन UndrivenVarEntry{nodep};
            // UINFO(9," Associate u="<<which_user<<" "<<cvtToHex(this)<<" "<<nodep->name()<<endl);
            m_entryps[which_user].ढकला_back(entryp);
            यदि (which_user == 1) {
                nodep->user1p(entryp);
            } अन्यथा यदि (which_user == 2) {
                nodep->user2p(entryp);
            } अन्यथा {
                nodep->v3fatalSrc("Bad case");
            }
            प्रत्यागम entryp;
        } अन्यथा {
            UndrivenVarEntry* ध्रुव entryp = reinterpret_cast<UndrivenVarEntry*>(
                which_user == 1 ? nodep->user1p() : nodep->user2p());
            प्रत्यागम entryp;
        }
    }

    शून्य warnAlwCombOrder(Ast_गाठ_Var_Ref* nodep, ध्रुव Ast_गाठ* readp) {
        AstVar* ध्रुव varp = nodep->varp();
        यदि (!varp->isParam() && !varp->isGenVar() && !varp->isUsedLoopIdx()
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
                                            सत्य);  // Complain just once for any usage
        }
    }
    // A clocking block 'output' is an additional driver of the signal it names.
    // The conflict is found from whichever driver is reached second.
    तार्किक clockingDrivesOther(ध्रुव UndrivenVarEntry* entryp, तार्किक otherWriteIsStaticInit) ध्रुव {
        ध्रुव तार्किक otherIsExplicitProc
            = entryp->isDrivenAlwaysCombWhole() || entryp->isDrivenAlwaysFFWhole();
        प्रत्यागम m_clockingp && !otherIsExplicitProc && !otherWriteIsStaticInit
               && m_clockingp != entryp->getClockingp();
    }
    तार्किक otherDrivesClocking(ध्रुव UndrivenVarEntry* entryp) ध्रुव {
        प्रत्यागम !m_clockingp && !m_alwaysCombp && !m_alwaysFFp && !m_inInitialStatic
               && entryp->isDrivenClockingWhole();
    }
    // A continuous assignment contending with a clocking block 'output' is an
    // unambiguous driver conflict. Multiple clocking blocks may drive the same
    // signal, the last drive wins (IEEE 1800-2023 14.16.2).
    शून्य warnClockingDriven(Ast_गाठ_Var_Ref* nodep, ध्रुव UndrivenVarEntry* entryp,
                            ध्रुव Ast_गाठ* otherWritep, तार्किक otherWriteIsStaticInit) {
        यदि (clockingDrivesOther(entryp, otherWriteIsStaticInit)
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
        यदि (otherDrivesClocking(entryp) && m_inContAssign) {
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
    शून्य warnClockingDrivenProc(Ast_गाठ_Var_Ref* nodep, ध्रुव UndrivenVarEntry* entryp,
                                ध्रुव Ast_गाठ* otherWritep, तार्किक otherWriteIsStaticInit) {
        यदि (clockingDrivesOther(entryp, otherWriteIsStaticInit) && !entryp->getClockingp()
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
        यदि (otherDrivesClocking(entryp) && !m_inContAssign) {
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
    शून्य visit(AstVar* nodep) override {
        ध्रुव तार्किक funcInout = nodep->isFuncLocal() && nodep->isInout();
        प्रति (पूर्णाङ्क usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            // For assigns and non-combo always, do just usr==1, to look
            // for module-wide undriven etc.
            // For combo always, run both usr==1 for above, and also
            // usr==2 for always-only checks.
            UndrivenVarEntry* ध्रुव entryp = getEntryp(nodep, usr);
            यदि ((nodep->isNonOutput() && !funcInout) || nodep->isSigPublic()
                || nodep->hasUserInit() || nodep->isSigUserRWPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->drivenWhole(nodep);
            }
            यदि ((nodep->isWritable() && !funcInout) || nodep->isSigPublic()
                || nodep->isSigUserRWPublic() || nodep->isSigUserRdPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->usedWhole(nodep);
            }
            यदि (nodep->valuep()) entryp->drivenWhole(nodep->valuep());
        }
        // Discover variables used in bit definitions, etc
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstArraySel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstSliceSel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstSel* nodep) override {
        Ast_गाठ_Var_Ref* ध्रुव varrefp = VN_CAST(nodep->fromp(), गाठ_Var_Ref);
        AstConst* ध्रुव constp = VN_CAST(nodep->lsbp(), Const);
        यदि (varrefp && constp && !constp->अंक().isFourState()) {
            प्रति (पूर्णाङ्क usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                UndrivenVarEntry* ध्रुव entryp = getEntryp(varrefp->varp(), usr);
                ध्रुव पूर्णाङ्क lsb = constp->toUInt();
                यदि (m_inBBox || varrefp->access().isWriteOrRW()) {
                    // Don't warn if already driven earlier as "a=0; if(a) a=1;" is fine.
                    यदि (usr == 2 && m_alwaysCombp
                        && entryp->isUsedNotDrivenBit(lsb, nodep->width())) {
                        UINFO(9, " Select.  Entryp=" << cvtToHex(entryp));
                        warnAlwCombOrder(varrefp, entryp->firstUsedNotDrivenp());
                    }
                    entryp->drivenBit(lsb, nodep->width());
                }
                यदि (m_inBBox || !varrefp->access().isWriteOrRW())
                    entryp->usedBit(lsb, nodep->width(), varrefp);
            }
        } अन्यथा {
            // skip over static longest static prefix
            iterateConst(nodep->lsbp());
            VL_RESTORER(m_inSelLhs);
            m_inSelLhs = !V3Width::selectNonConstantRecurse(nodep->lsbp(), /*inSel=*/सत्य);
            iterateConst(nodep->fromp());
        }
    }
    शून्य visit(Ast_गाठ_Var_Ref* nodep) override {
        // Any variable
        यदि (nodep->access().isWriteOrRW()
            && !VN_IS(nodep, VarXRef)) {  // Ignore interface variables and similar ugly items
            यदि (m_inProcAssign && !nodep->varp()->varType().isProcAssignable()
                && !nodep->varp()->isDeclTyped()  //
                && !nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()) {
                nodep->v3warn(PROCASSWIRE, "Procedural assignment to wire, perhaps intended var"
                                               << " (IEEE 1800-2023 6.5): "
                                               << nodep->prettyNameQ());
            } अन्यथा यदि (m_inContAssign && !nodep->varp()->varType().isContAssignable()
                       && !nodep->fileline()->language().systemVerilog()) {
                nodep->v3warn(CONTASSREG,
                              "Continuous assignment to reg, perhaps intended wire"
                                  << " (IEEE 1364-2005 6.1; Verilog only, legal in SV): "
                                  << nodep->prettyNameQ());
            }
            यदि (m_inFTaskRef && nodep->varp()->varType().isNet()) {
                nodep->v3warn(
                    PROCASSWIRE,
                    "Passed wire on output or inout subroutine argument, expected expression that "
                    "is valid on the left hand side of a procedural assignment"
                        << " (IEEE 1800-2023 13.5): " << nodep->prettyNameQ());
            }
        }

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Remember that anything driven here doesn't count toward MULTIDRIVEN.
        तार्किक ftaskDef = असत्य;
        यदि (m_taskp && !m_alwaysp && !m_inContAssign && !m_inInitialStatic && !m_inBBox
            && !m_taskp->dpiExport()) {
            AstVar* ध्रुव retVarp = VN_CAST(m_taskp->fvarp(), Var);
            यदि (!retVarp || nodep->varp() != retVarp) ftaskDef = सत्य;
        }

        प्रति (पूर्णाङ्क usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            UndrivenVarEntry* ध्रुव entryp = getEntryp(nodep->varp(), usr);
            ध्रुव तार्किक fdrv = nodep->access().isWriteOrRW()
                              && nodep->varp()->attrFileDescr();  // FD's are also being read from
            यदि (m_inBBox || nodep->access().isWriteOrRW()) {
                यदि (usr == 2 && m_alwaysCombp && entryp->isUsedNotDrivenAny()) {
                    UINFO(9, " Full bus.  Entryp=" << cvtToHex(entryp));
                    warnAlwCombOrder(nodep, entryp->firstUsedNotDrivenp());
                }
                ध्रुव Ast_गाठ_Var_Ref* ध्रुव otherVarRefp = entryp->getNodep();
                ध्रुव Ast_गाठ* ध्रुव otherWritep = otherVarRefp
                                                       ? static_cast<ध्रुव Ast_गाठ*>(otherVarRefp)
                                                       : entryp->callNodep();
                ध्रुव तार्किक same_File_ओळ
                    = otherVarRefp && nodep->fileline() == otherVarRefp->fileline();
                // Preconditions shared by MULTIDRIVEN and MULTIDRIVENPROC.
                ध्रुव तार्किक multidrivenCommon
                    = entryp->isDrivenWhole() && !m_inBBox && !VN_IS(nodep, VarXRef)
                      && !VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType) && !same_File_ओळ
                      && !entryp->isUnderGen() && otherWritep && !entryp->isFtaskDriven()
                      && !ftaskDef && !m_inSelLhs;
                // The two warnings are gated independently on the variable
                // declaration's fileline, as v3warn suppression will check
                // the driving fileline and still warn even if the warning
                // was suppressed with lint_off at the declaration.
                ध्रुव तार्किक otherWriteIsStaticInit
                    = nodep->varp()->hasUserInit() && otherWritep == entryp->initStaticp();
                यदि (multidrivenCommon
                    && !nodep->varp()->fileline()->warnIsOff(V3ErrorCode::MULTIDRIVEN)) {
                    यदि (m_alwaysCombp
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
                    यदि (!m_alwaysCombp && entryp->isDrivenAlwaysCombWhole()) {
                        nodep->v3warn(MULTIDRIVEN, "Variable also written to in always_comb"
                                                       << " (IEEE 1800-2023 9.2.2.2): "
                                                       << nodep->prettyNameQ() << '\n'
                                                       << nodep->warnOther() << '\n'
                                                       << nodep->warnContextPrimary() << '\n'
                                                       << otherWritep->warnOther()
                                                       << "... Location of always_comb write\n"
                                                       << otherWritep->warnContextSecondary());
                    }
                    यदि (m_alwaysFFp && !otherWriteIsStaticInit
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
                    यदि (!m_alwaysFFp && !m_inInitialStatic && entryp->isDrivenAlwaysFFWhole()) {
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
                यदि (multidrivenCommon && !nodep->varp()->isUsedLoopIdx()
                    && !nodep->varp()->fileline()->warnIsOff(V3ErrorCode::MULTIDRIVENPROC)) {
                    // Two plain always blocks driving the whole signal: legal
                    // SystemVerilog, but a driver conflict for synthesis. The
                    // always_ff/always_comb cases above already cover mixes with
                    // an explicit process, so only warn for plain+plain here.
                    // When the two blocks are clocked differently, the
                    // (on-by-default) MULTIDRIVEN check in V3Delayed already reports
                    // the conflict, so don't also emit MULTIDRIVENPROC for that case.
                    ध्रुव AstAlways* ध्रुव otherAlwaysp = entryp->getAlwPlainp();
                    ध्रुव Ast_Sen_वृक्ष* ध्रुव senp
                        = m_alwaysPlainp ? m_alwaysPlainp->sentreep() : nullptr;
                    ध्रुव Ast_Sen_वृक्ष* ध्रुव otherSenp
                        = otherAlwaysp ? otherAlwaysp->sentreep() : nullptr;
                    ध्रुव तार्किक differentClocking = senp && otherSenp && senp->hasClocked()
                                                   && otherSenp->hasClocked()
                                                   && !senp->same_वृक्ष(otherSenp);
                    यदि (m_alwaysPlainp && entryp->isDrivenAlwaysPlainWhole()
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
                यदि (!m_inInitialSetup || nodep->varp()->hasUserInit()) {
                    // Else don't count default initialization as a driver to a net/variable
                    entryp->drivenWhole(nodep, ftaskDef);
                }
                यदि (m_alwaysCombp && entryp->isDrivenAlwaysCombWhole()
                    && m_alwaysCombp != entryp->getAlwCombp()
                    && m_alwaysCombp->fileline() == entryp->getAlwCombp()->fileline())
                    entryp->underGenerate();
                यदि (m_alwaysCombp) entryp->drivenAlwaysCombWhole(m_alwaysCombp);
                यदि (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, nodep->varp());
                यदि (m_alwaysPlainp) entryp->drivenAlwaysPlainWhole(m_alwaysPlainp);
                यदि (m_clockingp) entryp->drivenClockingWhole(m_clockingp);
            }
            यदि (nodep->access().isWriteOrRW() && !VN_IS(nodep, VarXRef)) {
                // Ignoring xrefs as the initial and assignment to track might refer to two
                // different instances.  Ideally all of V3Undriven would move after V3Scope,
                // then could use VarScope tracking instead.
                यदि (m_inInitialStatic && !entryp->initStaticp()) entryp->initStaticp(nodep);
                यदि (m_inInitial && !entryp->initialp()) entryp->initialp(nodep);
                यदि (m_inContAssign && !entryp->contAssignp()) entryp->contAssignp(nodep);
                यदि (m_alwaysp && m_inProcAssign && !entryp->procWritep())
                    entryp->procWritep(nodep);
            }
            यदि ((!m_inInitialSetup || nodep->varp()->hasUserInit())
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
    शून्य visit(AstSysIgnore* nodep) override {
        VL_RESTORER(m_inBBox);
        m_inBBox = सत्य;
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstAssign* nodep) override {
        VL_RESTORER(m_inProcAssign);
        m_inProcAssign = सत्य;
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = असत्य;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }
    शून्य visit(AstAssignDly* nodep) override {
        VL_RESTORER(m_inProcAssign);
        m_inProcAssign = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstAssignForce* nodep) override {
        iterateConst(nodep->rhsp());
        VL_RESTORER(m_inInitial);
        m_inInitial = असत्य;
        iterateConst(nodep->lhsp());
    }
    शून्य visit(AstAssignW* nodep) override {
        VL_RESTORER(m_inContAssign);
        m_inContAssign = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstRelease* nodep) override {
        VL_RESTORER(m_inInitial);
        m_inInitial = असत्य;
        iterateConst(nodep->lhsp());
    }
    शून्य visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        m_inInitial = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstInitialAutomatic* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstInitialStatic* nodep) override {
        VL_RESTORER(m_inInitialStatic);
        m_inInitialStatic = सत्य;
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        VL_RESTORER(m_alwaysCombp);
        VL_RESTORER(m_alwaysFFp);
        VL_RESTORER(m_alwaysPlainp);
        Ast_गाठ::user2_Clear_वृक्ष();
        m_alwaysp = nodep;
        यदि (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            UINFO(9, "   " << nodep);
            m_alwaysCombp = nodep;
        } अन्यथा {
            m_alwaysCombp = nullptr;
        }
        m_alwaysFFp = nodep->keyword() == VAlwaysKwd::ALWAYS_FF ? nodep : nullptr;
        m_alwaysPlainp = nodep->keyword() == VAlwaysKwd::ALWAYS ? nodep : nullptr;
        iterateChildrenConst(nodep);
        यदि (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) UINFO(9, "   Done " << nodep);
    }
    शून्य visit(AstClocking* nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_गाठ_FTask_Ref* nodep) override {
        VL_RESTORER(m_inFTaskRef);
        m_inFTaskRef = सत्य;

        iterateChildrenConst(nodep);

        यदि (!m_capturep) प्रत्यागम;

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Do not apply writeSummary at calls inside a task definition, or they will look like
        // independent drivers (phantom MULTIDRIVEN).
        ध्रुव तार्किक inExecutedContext
            = !(m_taskp && !m_alwaysp && !m_inContAssign && !m_inInitialStatic && !m_inBBox
                && !m_taskp->dpiExport());

        यदि (!inExecutedContext) प्रत्यागम;

        Ast_गाठ_FTask* ध्रुव calleep = nodep->taskp();
        यदि (!calleep) प्रत्यागम;

        ध्रुव auto& vars = m_capturep->writeSummary(calleep);
        प्रति (AstVar* ध्रुव varp : vars) {
            प्रति (पूर्णाङ्क usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                UndrivenVarEntry* ध्रुव entryp = getEntryp(varp, usr);
                entryp->drivenViaCall(nodep);
                यदि (m_alwaysCombp) entryp->drivenAlwaysCombWhole(m_alwaysCombp);
                यदि (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, varp);
            }
        }
    }

    शून्य visit(Ast_गाठ_FTask* nodep) override {
        VL_RESTORER(m_taskp);
        m_taskp = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstPin* nodep) override {
        VL_RESTORER(m_inInoutOrRefPin);
        m_inInoutOrRefPin = nodep->modVarp()->isInoutOrRef();
        iterateChildrenConst(nodep);
    }

    // Until we support tables, primitives will have undriven and unused I/Os
    शून्य visit(AstPrimitive*) override {}

    // Coverage artifacts etc shouldn't count as a sink
    शून्य visit(Ast_गाठ_Cover_Decl*) override {}
    शून्य visit(AstCoverInc*) override {}
    शून्य visit(AstCoverToggle*) override {}
    शून्य visit(AstTraceDecl* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    शून्य visit(AstTraceInc* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }

    // iterate
    शून्य visit(AstConst* nodep) override {}
    शून्य visit(Ast_गाठ* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit UndrivenVisitor(AstNetlist* nodep, V3UndrivenCapture* capturep)
        : m_capturep{capturep} {
        iterateConst(nodep);
    }

    ~UndrivenVisitor() override {
        प्रति (UndrivenVarEntry* ip : m_entryps[1]) ip->reportViolations();
        प्रति (पूर्णाङ्क usr = 1; usr < 3; ++usr) {
            प्रति (UndrivenVarEntry* ip : m_entryps[usr]) delete ip;
        }
    }
};

//######################################################################
// Undriven class functions

शून्य V3Undriven::undrivenAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    V3UndrivenCapture capture{nodep};
    { UndrivenVisitor{nodep, &capture}; }

    यदि (v3Global.opt.stats()) V3Stats::statsStage("undriven");
}
