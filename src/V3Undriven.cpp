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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Undriven.h"

#समावेश "V3Stats.h"
#समावेश "V3UndrivenCapture.h"
#समावेश "V3Width.h"

#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Class for every variable we may process

श्रेणी UndrivenVarEntry final {
    // MEMBERS
    AstVar* स्थिर m_varp;  // Variable this tracks
    std::vector<bool> m_wholeFlags;  // Used/Driven on whole vector
    std::vector<bool> m_bitFlags;  // Used/Driven on each subbit
    स्थिर Ast_गाँठ* m_usedNotDrivenp = nullptr;  // First read before any write
    स्थिर AstAlways* m_alwCombp
        = nullptr;  // always_comb of var if driven within always_comb, else nullptr
    स्थिर AstAlways* m_alwFFp = nullptr;  // always_ff of var if driven within always_ff
    स्थिर AstAlways* m_alwPlainp = nullptr;  // plain always of var if driven within plain always
    स्थिर AstClocking* m_clockingp = nullptr;  // clocking block of var if driven as output
    स्थिर Ast_गाँठ_Var_Ref* m_nodep = nullptr;  // varref if driven, else nullptr
    स्थिर Ast_गाँठ* m_initStaticp = nullptr;  // varref if in InitialStatic driven
    स्थिर Ast_गाँठ* m_initialp = nullptr;  // varref if driven in an explicit initial block
    स्थिर Ast_गाँठ* m_contAssignp = nullptr;  // varref if in continuous assignment driven
    स्थिर Ast_गाँठ* m_procWritep = nullptr;  // varref if written in process
    bool m_underGen = false;  // Under a generate
    bool m_ftaskDriven = false;  // Last driven by function or task

    स्थिर Ast_गाँठ_FTask_Ref* m_callNodep = nullptr;  // Call node if driven via writeSummary

    क्रमागत : uint8_t {
        FLAG_USED = 0,  // Signal or bit has been read/observed
        FLAG_DRIVEN = 1,  // Signal or bit has been written/driven
        FLAG_DRIVEN_ALWCOMB = 2,  // Whole signal has been driven from always_comb
        FLAG_DRIVEN_ALWFF = 3,  // Whole signal has been driven from always_ff
        FLAG_DRIVEN_ALWPLAIN = 4,  // Whole signal has been driven from a plain always
        FLAG_DRIVEN_CLOCKING = 5,  // Whole signal has been driven as a clocking block output
        FLAGS_PER_BIT = 6  // Number of flags stored for each tracked bit
    };

खुला:
    // CONSTRUCTORS
    explicit UndrivenVarEntry(AstVar* varp)
        : m_varp{varp} {  // Construction for when a var is used
        UINFO(9, "create " << varp);
        m_wholeFlags.resize(FLAGS_PER_BIT);
        क्रम (पूर्णांक i = 0; i < FLAGS_PER_BIT; i++) m_wholeFlags[i] = false;
        m_bitFlags.resize(varp->width() * FLAGS_PER_BIT);
        क्रम (पूर्णांक i = 0; i < varp->width() * FLAGS_PER_BIT; i++) m_bitFlags[i] = false;
    }
    ~UndrivenVarEntry() = शेष;

निजी:
    // METHODS
    bool bit_अंक_Ok(पूर्णांक bit) स्थिर {
        वापस bit >= 0 && (bit * FLAGS_PER_BIT < static_cast<पूर्णांक>(m_bitFlags.आकार()));
    }
    bool usedFlag(पूर्णांक bit) स्थिर {
        वापस m_wholeFlags[FLAG_USED] || m_bitFlags[bit * FLAGS_PER_BIT + FLAG_USED];
    }
    bool drivenFlag(पूर्णांक bit) स्थिर {
        वापस m_wholeFlags[FLAG_DRIVEN] || m_bitFlags[bit * FLAGS_PER_BIT + FLAG_DRIVEN];
    }
    पूर्णांक bit_गिनती() स्थिर { वापस m_bitFlags.आकार() / FLAGS_PER_BIT; }
    व्योम recordUsedNotDriven(स्थिर Ast_गाँठ* nodep) {
        अगर (!m_usedNotDrivenp) m_usedNotDrivenp = nodep;
    }
    क्रमागत BitNamesWhich : uint8_t { BN_UNUSED, BN_UNDRIVEN, BN_BOTH };
    string bitNames(BitNamesWhich which) {
        string bits;
        bool पूर्व_2 = false;
        पूर्णांक msb = 0;
        // bit==-1 loops below; we do one extra iteration so end with prev=false
        क्रम (पूर्णांक bit = (m_bitFlags.आकार() / FLAGS_PER_BIT) - 1; bit >= -1; --bit) {
            अगर (bit >= 0
                && ((which == BN_UNUSED && !usedFlag(bit) && drivenFlag(bit))
                    || (which == BN_UNDRIVEN && usedFlag(bit) && !drivenFlag(bit))
                    || (which == BN_BOTH && !usedFlag(bit) && !drivenFlag(bit)))) {
                अगर (!पूर्व_2) {
                    पूर्व_2 = true;
                    msb = bit;
                }
            } अन्यथा अगर (पूर्व_2) {
                स्थिर AstBasicDType* स्थिर bdtypep = m_varp->basicp();
                स्थिर पूर्णांक lsb = bit + 1;
                अगर (bits != "") bits += ",";
                अगर (lsb == msb) {
                    bits += cvtToStr(lsb + bdtypep->lo());
                } अन्यथा {
                    अगर (bdtypep->ascending()) {
                        bits
                            += cvtToStr(lsb + bdtypep->lo()) + ":" + cvtToStr(msb + bdtypep->lo());
                    } अन्यथा {
                        bits
                            += cvtToStr(msb + bdtypep->lo()) + ":" + cvtToStr(lsb + bdtypep->lo());
                    }
                }
                पूर्व_2 = false;
            }
        }
        वापस "[" + bits + "]";
    }

खुला:
    व्योम usedWhole(स्थिर Ast_गाँठ* nodep) {
        UINFO(9, "set u[*] " << m_varp->name() << " " << nodep);
        अगर (!m_wholeFlags[FLAG_DRIVEN]) {
            क्रम (पूर्णांक bit = 0; bit < bit_गिनती(); ++bit) {
                अगर (!drivenFlag(bit)) {
                    recordUsedNotDriven(nodep);
                    अवरोध;
                }
            }
        }
        m_wholeFlags[FLAG_USED] = true;
    }
    व्योम drivenWhole(स्थिर Ast_गाँठ* nodep) {
        UINFO(9, "set d[*] " << m_varp->name() << " " << nodep);
        m_wholeFlags[FLAG_DRIVEN] = true;
    }
    व्योम drivenWhole(स्थिर Ast_गाँठ_Var_Ref* nodep, bool ftaskDef) {
        स्थिर bool wasDriven = isDrivenWhole();
        drivenWhole(nodep);
        // A write in a task/function definition is not an executed process, so it must not
        // displace an already recorded write, nor clear the flag that keeps such a write
        // from being reported as the other write of a MULTIDRIVEN
        अगर (ftaskDef && wasDriven) वापस;
        m_ftaskDriven = ftaskDef;
        m_nodep = nodep;
    }
    व्योम drivenAlwaysCombWhole(स्थिर AstAlways* alwCombp) {
        m_wholeFlags[FLAG_DRIVEN_ALWCOMB] = true;
        m_alwCombp = alwCombp;
    }
    व्योम drivenAlwaysFFWhole(स्थिर AstAlways* alwFFp, स्थिर AstVar* varp) {
        m_wholeFlags[FLAG_DRIVEN_ALWFF] = true;
        m_alwFFp = alwFFp;
    }
    व्योम drivenAlwaysPlainWhole(स्थिर AstAlways* alwPlainp) {
        m_wholeFlags[FLAG_DRIVEN_ALWPLAIN] = true;
        m_alwPlainp = alwPlainp;
    }
    व्योम drivenClockingWhole(स्थिर AstClocking* clockingp) {
        m_wholeFlags[FLAG_DRIVEN_CLOCKING] = true;
        m_clockingp = clockingp;
    }

    स्थिर Ast_गाँठ* initStaticp() स्थिर { वापस m_initStaticp; }
    व्योम initStaticp(स्थिर Ast_गाँठ* nodep) { m_initStaticp = nodep; }
    स्थिर Ast_गाँठ* initialp() स्थिर { वापस m_initialp; }
    व्योम initialp(स्थिर Ast_गाँठ* nodep) { m_initialp = nodep; }
    स्थिर Ast_गाँठ* contAssignp() स्थिर { वापस m_contAssignp; }
    व्योम contAssignp(स्थिर Ast_गाँठ* nodep) { m_contAssignp = nodep; }
    स्थिर Ast_गाँठ* procWritep() स्थिर { वापस m_procWritep; }
    व्योम procWritep(स्थिर Ast_गाँठ* nodep) { m_procWritep = nodep; }
    व्योम underGenerate() { m_underGen = true; }
    bool isUnderGen() स्थिर { वापस m_underGen; }
    bool isDrivenWhole() स्थिर { वापस m_wholeFlags[FLAG_DRIVEN]; }
    bool isDrivenAlwaysCombWhole() स्थिर { वापस m_wholeFlags[FLAG_DRIVEN_ALWCOMB]; }
    bool isDrivenAlwaysFFWhole() स्थिर { वापस m_wholeFlags[FLAG_DRIVEN_ALWFF]; }
    bool isDrivenAlwaysPlainWhole() स्थिर { वापस m_wholeFlags[FLAG_DRIVEN_ALWPLAIN]; }
    bool isDrivenClockingWhole() स्थिर { वापस m_wholeFlags[FLAG_DRIVEN_CLOCKING]; }
    bool isFtaskDriven() स्थिर { वापस m_ftaskDriven; }
    स्थिर Ast_गाँठ_Var_Ref* getNodep() स्थिर { वापस m_nodep; }
    स्थिर AstAlways* getAlwCombp() स्थिर { वापस m_alwCombp; }
    स्थिर AstAlways* getAlwFFp() स्थिर { वापस m_alwFFp; }
    स्थिर AstAlways* getAlwPlainp() स्थिर { वापस m_alwPlainp; }
    स्थिर AstClocking* getClockingp() स्थिर { वापस m_clockingp; }
    व्योम usedBit(पूर्णांक bit, पूर्णांक width, स्थिर Ast_गाँठ* nodep) {
        UINFO(9, "set u[" << (bit + width - 1) << ":" << bit << "] " << m_varp->name());
        क्रम (पूर्णांक i = 0; i < width; i++) {
            अगर (bit_अंक_Ok(bit + i)) {
                अगर (!drivenFlag(bit + i)) recordUsedNotDriven(nodep);
                m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_USED] = true;
            }
        }
    }
    व्योम drivenBit(पूर्णांक bit, पूर्णांक width) {
        UINFO(9, "set d[" << (bit + width - 1) << ":" << bit << "] " << m_varp->name());
        क्रम (पूर्णांक i = 0; i < width; i++) {
            अगर (bit_अंक_Ok(bit + i)) m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_DRIVEN] = true;
        }
    }
    bool isUsedNotDrivenBit(पूर्णांक bit, पूर्णांक width) स्थिर {
        क्रम (पूर्णांक i = 0; i < width; i++) {
            अगर (bit_अंक_Ok(bit + i)
                && (m_wholeFlags[FLAG_USED] || m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_USED])
                && !(m_wholeFlags[FLAG_DRIVEN]
                     || m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_DRIVEN]))
                वापस true;
        }
        वापस false;
    }
    bool isUsedNotDrivenAny() स्थिर {
        वापस isUsedNotDrivenBit(0, m_bitFlags.आकार() / FLAGS_PER_BIT);
    }
    स्थिर Ast_गाँठ* firstUsedNotDrivenp() स्थिर { वापस m_usedNotDrivenp; }
    अटल bool unusedMatch(AstVar* nodep) {
        स्थिर string regexp = v3Global.opt.unusedRegexp();
        अगर (regexp == "") वापस false;
        स्थिर string prettyName = nodep->prettyName();
        वापस VString::wildmatch(prettyName.c_str(), regexp.c_str());
    }
    व्योम reportViolations() {
        // Combine bits into overall state
        AstVar* स्थिर nodep = m_varp;

        अगर (initStaticp() && procWritep() && nodep->hasUserInit() && !nodep->isClassMember()
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
        स्थिर Ast_गाँठ* स्थिर initp = nodep->hasUserInit() ? initStaticp() : initialp();
        अगर (initp && contAssignp() && !nodep->isClassMember() && !nodep->isFuncLocal()) {
            initp->v3warn(E_CONTASSINIT, "Continuous assignment to variable with initial value: "
                                             << nodep->prettyNameQ() << '\n'
                                             << initp->warnMore()
                                             << "... Location of variable initialization\n"
                                             << initp->warnContextPrimary() << '\n'
                                             << contAssignp()->warnOther()
                                             << "... Location of continuous assignment\n"
                                             << contAssignp()->warnContextSecondary());
        }
        अगर (nodep->isGenVar()) {  // Genvar
            अगर (!nodep->isIfaceRef() && !nodep->isUsedParam() && !unusedMatch(nodep)) {
                nodep->v3warn(UNUSEDGENVAR, "Genvar is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDGENVAR,
                                                 true);  // Warn only once
            }
        } अन्यथा अगर (nodep->isParam()) {  // Parameter
            अगर (!nodep->isIfaceRef() && !nodep->isUsedParam() && !unusedMatch(nodep)) {
                nodep->v3warn(UNUSEDPARAM, "Parameter is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDPARAM,
                                                 true);  // Warn only once
            }
        } अन्यथा {  // Signal
            स्थिर string varType{nodep->isFuncLocal() ? "Function variable" : "Signal"};
            स्थिर bool funcInout = nodep->isFuncLocal() && nodep->isInout();
            bool allU = true;
            bool allD = true;
            bool anyU = m_wholeFlags[FLAG_USED];
            bool anyD = m_wholeFlags[FLAG_DRIVEN];
            bool anyUnotD = false;
            bool anyDnotU = false;
            bool anynotDU = false;
            क्रम (अचिन्हित bit = 0; bit < m_bitFlags.आकार() / FLAGS_PER_BIT; bit++) {
                स्थिर bool used = usedFlag(bit);
                स्थिर bool driv = drivenFlag(bit);
                allU &= used;
                anyU |= used;
                allD &= driv;
                anyD |= driv;
                anyUnotD |= used && !driv;
                anyDnotU |= !used && driv;
                anynotDU |= !used && !driv;
            }
            अगर (funcInout) {
                अगर (anyD) allU = true;
                allD = true;
            }
            अगर (allU) m_wholeFlags[FLAG_USED] = true;
            अगर (allD) m_wholeFlags[FLAG_DRIVEN] = true;
            // Test results
            अगर (nodep->isIfaceRef()) {
                // For interface top level we don't do any tracking
                // Ideally we'd report unused instance cells, but presumably a signal inside one
                // would get reported as unused
            } अन्यथा अगर (allU && allD) {
                // It's fine
            } अन्यथा अगर (!anyD && !anyU) {
                // UNDRIVEN is considered more serious - as is more likely a bug,
                // thus undriven+unused bits get UNUSED warnings, as they're not as buggy.
                अगर (!unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not driven, nor used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL,
                                                     true);  // Warn only once
                }
            } अन्यथा अगर (allD && !anyU) {
                अगर (!unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL,
                                                     true);  // Warn only once
                }
            } अन्यथा अगर (!anyD && allU) {
                nodep->v3warn(UNDRIVEN, varType << " is not driven: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNDRIVEN, true);  // Warn only once
            } अन्यथा अगर (!funcInout) {
                // Bits have different dispositions
                स्थिर std::string varTypeLower = [&varType]() {
                    std::string str = varType;
                    str[0] = std::छोटे(static_cast<अचिन्हित अक्षर>(str[0]));
                    वापस str;
                }();
                bool setU = false;
                bool setD = false;
                अगर (anynotDU && !unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  "Bits of " << varTypeLower << " are not driven, nor used: "
                                             << nodep->prettyNameQ() << bitNames(BN_BOTH));
                    setU = true;
                }
                अगर (anyDnotU && !unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL, "Bits of " << varTypeLower << " are not used: "
                                                           << nodep->prettyNameQ()
                                                           << bitNames(BN_UNUSED));
                    setU = true;
                }
                अगर (anyUnotD) {
                    nodep->v3warn(UNDRIVEN, "Bits of " << varTypeLower << " are not driven: "
                                                       << nodep->prettyNameQ()
                                                       << bitNames(BN_UNDRIVEN));
                    setD = true;
                }
                अगर (setU) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, true);
                }
                अगर (setD) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNDRIVEN, true);
                }
            }
        }
    }

    व्योम drivenViaCall(स्थिर Ast_गाँठ_FTask_Ref* nodep) {
        drivenWhole(nodep);
        अगर (!m_callNodep) m_callNodep = nodep;
    }
    स्थिर Ast_गाँठ_FTask_Ref* callNodep() स्थिर { वापस m_callNodep; }
};

//######################################################################
// Undriven state, as a visitor of each AstNode

श्रेणी UndrivenVisitor final : खुला VNVisitorConst {
    // NODE STATE
    // Netlist:
    //  AstVar::user1p          -> UndrivenVar* for usage var, 0=not set yet
    स्थिर VNUser1InUse m_inuser1;
    // Each always:
    //  AstNode::user2p         -> UndrivenVar* for usage var, 0=not set yet
    स्थिर VNUser2InUse m_inuser2;

    // STATE
    std::array<std::vector<UndrivenVarEntry*>, 3> m_entryps = {};  // Nodes to delete when finished
    bool m_inBBox = false;  // In black box; mark as driven+used
    bool m_inContAssign = false;  // In continuous assignment
    bool m_inInitial = false;  // In explicit initial block
    bool m_inInitialSetup = false;  // In InitialAutomatic*/InitialStatic* assignment LHS
    bool m_inInitialStatic = false;  // In InitialStatic
    bool m_inProcAssign = false;  // In procedural assignment
    bool m_inFTaskRef = false;  // In function or task call
    bool m_inInoutOrRefPin = false;  // Connected to pin that is inout
    bool m_inSelLhs = false;  // Iterating the fromp of an AstSel (a partial-bit write target)
    स्थिर Ast_गाँठ_FTask* m_taskp = nullptr;  // Current task
    स्थिर AstAlways* m_alwaysp = nullptr;  // Current always of either type
    स्थिर AstAlways* m_alwaysCombp = nullptr;  // Current always if combo, otherwise nullptr
    स्थिर AstAlways* m_alwaysFFp = nullptr;  // Current always if ff, otherwise nullptr
    स्थिर AstAlways* m_alwaysPlainp = nullptr;  // Current always if plain (not comb/ff/latch)
    स्थिर AstClocking* m_clockingp = nullptr;  // Current clocking block, otherwise nullptr

    V3UndrivenCapture* स्थिर m_capturep = nullptr;  // Capture object.  'nullptr' if disabled.

    // METHODS

    UndrivenVarEntry* getEntryp(AstVar* nodep, पूर्णांक which_user) {
        अगर (!(which_user == 1 ? nodep->user1p() : nodep->user2p())) {
            UndrivenVarEntry* स्थिर entryp = नया UndrivenVarEntry{nodep};
            // UINFO(9," Associate u="<<which_user<<" "<<cvtToHex(this)<<" "<<nodep->name()<<endl);
            m_entryps[which_user].धकेलो_back(entryp);
            अगर (which_user == 1) {
                nodep->user1p(entryp);
            } अन्यथा अगर (which_user == 2) {
                nodep->user2p(entryp);
            } अन्यथा {
                nodep->v3fatalSrc("Bad case");
            }
            वापस entryp;
        } अन्यथा {
            UndrivenVarEntry* स्थिर entryp = reinterpret_cast<UndrivenVarEntry*>(
                which_user == 1 ? nodep->user1p() : nodep->user2p());
            वापस entryp;
        }
    }

    व्योम warnAlwCombOrder(Ast_गाँठ_Var_Ref* nodep, स्थिर Ast_गाँठ* readp) {
        AstVar* स्थिर varp = nodep->varp();
        अगर (!varp->isParam() && !varp->isGenVar() && !varp->isUsedLoopIdx()
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
                                            true);  // Complain just once for any usage
        }
    }
    // A clocking block 'output' is an additional driver of the signal it names.
    // The conflict is found from whichever driver is reached second.
    bool clockingDrivesOther(स्थिर UndrivenVarEntry* entryp, bool otherWriteIsStaticInit) स्थिर {
        स्थिर bool otherIsExplicitProc
            = entryp->isDrivenAlwaysCombWhole() || entryp->isDrivenAlwaysFFWhole();
        वापस m_clockingp && !otherIsExplicitProc && !otherWriteIsStaticInit
               && m_clockingp != entryp->getClockingp();
    }
    bool otherDrivesClocking(स्थिर UndrivenVarEntry* entryp) स्थिर {
        वापस !m_clockingp && !m_alwaysCombp && !m_alwaysFFp && !m_inInitialStatic
               && entryp->isDrivenClockingWhole();
    }
    // A continuous assignment contending with a clocking block 'output' is an
    // unambiguous driver conflict. Multiple clocking blocks may drive the same
    // signal, the last drive wins (IEEE 1800-2023 14.16.2).
    व्योम warnClockingDriven(Ast_गाँठ_Var_Ref* nodep, स्थिर UndrivenVarEntry* entryp,
                            स्थिर Ast_गाँठ* otherWritep, bool otherWriteIsStaticInit) {
        अगर (clockingDrivesOther(entryp, otherWriteIsStaticInit)
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
        अगर (otherDrivesClocking(entryp) && m_inContAssign) {
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
    व्योम warnClockingDrivenProc(Ast_गाँठ_Var_Ref* nodep, स्थिर UndrivenVarEntry* entryp,
                                स्थिर Ast_गाँठ* otherWritep, bool otherWriteIsStaticInit) {
        अगर (clockingDrivesOther(entryp, otherWriteIsStaticInit) && !entryp->getClockingp()
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
        अगर (otherDrivesClocking(entryp) && !m_inContAssign) {
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
    व्योम visit(AstVar* nodep) override {
        स्थिर bool funcInout = nodep->isFuncLocal() && nodep->isInout();
        क्रम (पूर्णांक usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            // For assigns and non-combo always, do just usr==1, to look
            // for module-wide undriven etc.
            // For combo always, run both usr==1 for above, and also
            // usr==2 for always-only checks.
            UndrivenVarEntry* स्थिर entryp = getEntryp(nodep, usr);
            अगर ((nodep->isNonOutput() && !funcInout) || nodep->isSigPublic()
                || nodep->hasUserInit() || nodep->isSigUserRWPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->drivenWhole(nodep);
            }
            अगर ((nodep->isWritable() && !funcInout) || nodep->isSigPublic()
                || nodep->isSigUserRWPublic() || nodep->isSigUserRdPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->usedWhole(nodep);
            }
            अगर (nodep->valuep()) entryp->drivenWhole(nodep->valuep());
        }
        // Discover variables used in bit definitions, etc
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstArraySel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstSliceSel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstSel* nodep) override {
        Ast_गाँठ_Var_Ref* स्थिर varrefp = VN_CAST(nodep->fromp(), गाँठ_Var_Ref);
        AstConst* स्थिर constp = VN_CAST(nodep->lsbp(), Const);
        अगर (varrefp && constp && !constp->अंक().isFourState()) {
            क्रम (पूर्णांक usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                UndrivenVarEntry* स्थिर entryp = getEntryp(varrefp->varp(), usr);
                स्थिर पूर्णांक lsb = constp->toUInt();
                अगर (m_inBBox || varrefp->access().isWriteOrRW()) {
                    // Don't warn if already driven earlier as "a=0; if(a) a=1;" is fine.
                    अगर (usr == 2 && m_alwaysCombp
                        && entryp->isUsedNotDrivenBit(lsb, nodep->width())) {
                        UINFO(9, " Select.  Entryp=" << cvtToHex(entryp));
                        warnAlwCombOrder(varrefp, entryp->firstUsedNotDrivenp());
                    }
                    entryp->drivenBit(lsb, nodep->width());
                }
                अगर (m_inBBox || !varrefp->access().isWriteOrRW())
                    entryp->usedBit(lsb, nodep->width(), varrefp);
            }
        } अन्यथा {
            // skip over static longest static prefix
            iterateConst(nodep->lsbp());
            VL_RESTORER(m_inSelLhs);
            m_inSelLhs = !V3Width::selectNonConstantRecurse(nodep->lsbp(), /*inSel=*/true);
            iterateConst(nodep->fromp());
        }
    }
    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        // Any variable
        अगर (nodep->access().isWriteOrRW()
            && !VN_IS(nodep, VarXRef)) {  // Ignore interface variables and similar ugly items
            अगर (m_inProcAssign && !nodep->varp()->varType().isProcAssignable()
                && !nodep->varp()->isDeclTyped()  //
                && !nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()) {
                nodep->v3warn(PROCASSWIRE, "Procedural assignment to wire, perhaps intended var"
                                               << " (IEEE 1800-2023 6.5): "
                                               << nodep->prettyNameQ());
            } अन्यथा अगर (m_inContAssign && !nodep->varp()->varType().isContAssignable()
                       && !nodep->fileline()->language().systemVerilog()) {
                nodep->v3warn(CONTASSREG,
                              "Continuous assignment to reg, perhaps intended wire"
                                  << " (IEEE 1364-2005 6.1; Verilog only, legal in SV): "
                                  << nodep->prettyNameQ());
            }
            अगर (m_inFTaskRef && nodep->varp()->varType().isNet()) {
                nodep->v3warn(
                    PROCASSWIRE,
                    "Passed wire on output or inout subroutine argument, expected expression that "
                    "is valid on the left hand side of a procedural assignment"
                        << " (IEEE 1800-2023 13.5): " << nodep->prettyNameQ());
            }
        }

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Remember that anything driven here doesn't count toward MULTIDRIVEN.
        bool ftaskDef = false;
        अगर (m_taskp && !m_alwaysp && !m_inContAssign && !m_inInitialStatic && !m_inBBox
            && !m_taskp->dpiExport()) {
            AstVar* स्थिर retVarp = VN_CAST(m_taskp->fvarp(), Var);
            अगर (!retVarp || nodep->varp() != retVarp) ftaskDef = true;
        }

        क्रम (पूर्णांक usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            UndrivenVarEntry* स्थिर entryp = getEntryp(nodep->varp(), usr);
            स्थिर bool fdrv = nodep->access().isWriteOrRW()
                              && nodep->varp()->attrFileDescr();  // FD's are also being read from
            अगर (m_inBBox || nodep->access().isWriteOrRW()) {
                अगर (usr == 2 && m_alwaysCombp && entryp->isUsedNotDrivenAny()) {
                    UINFO(9, " Full bus.  Entryp=" << cvtToHex(entryp));
                    warnAlwCombOrder(nodep, entryp->firstUsedNotDrivenp());
                }
                स्थिर Ast_गाँठ_Var_Ref* स्थिर otherVarRefp = entryp->getNodep();
                स्थिर Ast_गाँठ* स्थिर otherWritep = otherVarRefp
                                                       ? static_cast<स्थिर Ast_गाँठ*>(otherVarRefp)
                                                       : entryp->callNodep();
                स्थिर bool same_File_पंक्ति
                    = otherVarRefp && nodep->fileline() == otherVarRefp->fileline();
                // Preconditions shared by MULTIDRIVEN and MULTIDRIVENPROC.
                स्थिर bool multidrivenCommon
                    = entryp->isDrivenWhole() && !m_inBBox && !VN_IS(nodep, VarXRef)
                      && !VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType) && !same_File_पंक्ति
                      && !entryp->isUnderGen() && otherWritep && !entryp->isFtaskDriven()
                      && !ftaskDef && !m_inSelLhs;
                // The two warnings are gated independently on the variable
                // declaration's fileline, as v3warn suppression will check
                // the driving fileline and still warn even if the warning
                // was suppressed with lint_off at the declaration.
                स्थिर bool otherWriteIsStaticInit
                    = nodep->varp()->hasUserInit() && otherWritep == entryp->initStaticp();
                अगर (multidrivenCommon
                    && !nodep->varp()->fileline()->warnIsOff(V3ErrorCode::MULTIDRIVEN)) {
                    अगर (m_alwaysCombp
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
                    अगर (!m_alwaysCombp && entryp->isDrivenAlwaysCombWhole()) {
                        nodep->v3warn(MULTIDRIVEN, "Variable also written to in always_comb"
                                                       << " (IEEE 1800-2023 9.2.2.2): "
                                                       << nodep->prettyNameQ() << '\n'
                                                       << nodep->warnOther() << '\n'
                                                       << nodep->warnContextPrimary() << '\n'
                                                       << otherWritep->warnOther()
                                                       << "... Location of always_comb write\n"
                                                       << otherWritep->warnContextSecondary());
                    }
                    अगर (m_alwaysFFp && !otherWriteIsStaticInit
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
                    अगर (!m_alwaysFFp && !m_inInitialStatic && entryp->isDrivenAlwaysFFWhole()) {
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
                अगर (multidrivenCommon && !nodep->varp()->isUsedLoopIdx()
                    && !nodep->varp()->fileline()->warnIsOff(V3ErrorCode::MULTIDRIVENPROC)) {
                    // Two plain always blocks driving the whole signal: legal
                    // SystemVerilog, but a driver conflict for synthesis. The
                    // always_ff/always_comb cases above already cover mixes with
                    // an explicit process, so only warn for plain+plain here.
                    // When the two blocks are clocked differently, the
                    // (on-by-default) MULTIDRIVEN check in V3Delayed already reports
                    // the conflict, so don't also emit MULTIDRIVENPROC for that case.
                    स्थिर AstAlways* स्थिर otherAlwaysp = entryp->getAlwPlainp();
                    स्थिर Ast_Sen_वृक्ष* स्थिर senp
                        = m_alwaysPlainp ? m_alwaysPlainp->sentreep() : nullptr;
                    स्थिर Ast_Sen_वृक्ष* स्थिर otherSenp
                        = otherAlwaysp ? otherAlwaysp->sentreep() : nullptr;
                    स्थिर bool differentClocking = senp && otherSenp && senp->hasClocked()
                                                   && otherSenp->hasClocked()
                                                   && !senp->same_वृक्ष(otherSenp);
                    अगर (m_alwaysPlainp && entryp->isDrivenAlwaysPlainWhole()
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
                अगर (!m_inInitialSetup || nodep->varp()->hasUserInit()) {
                    // Else don't count default initialization as a driver to a net/variable
                    entryp->drivenWhole(nodep, ftaskDef);
                }
                अगर (m_alwaysCombp && entryp->isDrivenAlwaysCombWhole()
                    && m_alwaysCombp != entryp->getAlwCombp()
                    && m_alwaysCombp->fileline() == entryp->getAlwCombp()->fileline())
                    entryp->underGenerate();
                अगर (m_alwaysCombp) entryp->drivenAlwaysCombWhole(m_alwaysCombp);
                अगर (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, nodep->varp());
                अगर (m_alwaysPlainp) entryp->drivenAlwaysPlainWhole(m_alwaysPlainp);
                अगर (m_clockingp) entryp->drivenClockingWhole(m_clockingp);
            }
            अगर (nodep->access().isWriteOrRW() && !VN_IS(nodep, VarXRef)) {
                // Ignoring xrefs as the initial and assignment to track might refer to two
                // different instances.  Ideally all of V3Undriven would move after V3Scope,
                // then could use VarScope tracking instead.
                अगर (m_inInitialStatic && !entryp->initStaticp()) entryp->initStaticp(nodep);
                अगर (m_inInitial && !entryp->initialp()) entryp->initialp(nodep);
                अगर (m_inContAssign && !entryp->contAssignp()) entryp->contAssignp(nodep);
                अगर (m_alwaysp && m_inProcAssign && !entryp->procWritep())
                    entryp->procWritep(nodep);
            }
            अगर ((!m_inInitialSetup || nodep->varp()->hasUserInit())
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
    व्योम visit(AstSysIgnore* nodep) override {
        VL_RESTORER(m_inBBox);
        m_inBBox = true;
        iterateChildrenConst(nodep);
    }

    व्योम visit(AstAssign* nodep) override {
        VL_RESTORER(m_inProcAssign);
        m_inProcAssign = true;
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = false;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }
    व्योम visit(AstAssignDly* nodep) override {
        VL_RESTORER(m_inProcAssign);
        m_inProcAssign = true;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstAssignForce* nodep) override {
        iterateConst(nodep->rhsp());
        VL_RESTORER(m_inInitial);
        m_inInitial = false;
        iterateConst(nodep->lhsp());
    }
    व्योम visit(AstAssignW* nodep) override {
        VL_RESTORER(m_inContAssign);
        m_inContAssign = true;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstRelease* nodep) override {
        VL_RESTORER(m_inInitial);
        m_inInitial = false;
        iterateConst(nodep->lhsp());
    }
    व्योम visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        m_inInitial = true;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstInitialAutomatic* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = true;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = true;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstInitialStatic* nodep) override {
        VL_RESTORER(m_inInitialStatic);
        m_inInitialStatic = true;
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = true;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = true;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        VL_RESTORER(m_alwaysCombp);
        VL_RESTORER(m_alwaysFFp);
        VL_RESTORER(m_alwaysPlainp);
        Ast_गाँठ::user2_Clear_वृक्ष();
        m_alwaysp = nodep;
        अगर (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            UINFO(9, "   " << nodep);
            m_alwaysCombp = nodep;
        } अन्यथा {
            m_alwaysCombp = nullptr;
        }
        m_alwaysFFp = nodep->keyword() == VAlwaysKwd::ALWAYS_FF ? nodep : nullptr;
        m_alwaysPlainp = nodep->keyword() == VAlwaysKwd::ALWAYS ? nodep : nullptr;
        iterateChildrenConst(nodep);
        अगर (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) UINFO(9, "   Done " << nodep);
    }
    व्योम visit(AstClocking* nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterateChildrenConst(nodep);
    }

    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        VL_RESTORER(m_inFTaskRef);
        m_inFTaskRef = true;

        iterateChildrenConst(nodep);

        अगर (!m_capturep) वापस;

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Do not apply writeSummary at calls inside a task definition, or they will look like
        // independent drivers (phantom MULTIDRIVEN).
        स्थिर bool inExecutedContext
            = !(m_taskp && !m_alwaysp && !m_inContAssign && !m_inInitialStatic && !m_inBBox
                && !m_taskp->dpiExport());

        अगर (!inExecutedContext) वापस;

        Ast_गाँठ_FTask* स्थिर calleep = nodep->taskp();
        अगर (!calleep) वापस;

        स्थिर स्वतः& vars = m_capturep->writeSummary(calleep);
        क्रम (AstVar* स्थिर varp : vars) {
            क्रम (पूर्णांक usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                UndrivenVarEntry* स्थिर entryp = getEntryp(varp, usr);
                entryp->drivenViaCall(nodep);
                अगर (m_alwaysCombp) entryp->drivenAlwaysCombWhole(m_alwaysCombp);
                अगर (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, varp);
            }
        }
    }

    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        VL_RESTORER(m_taskp);
        m_taskp = nodep;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstPin* nodep) override {
        VL_RESTORER(m_inInoutOrRefPin);
        m_inInoutOrRefPin = nodep->modVarp()->isInoutOrRef();
        iterateChildrenConst(nodep);
    }

    // Until we support tables, primitives will have undriven and unused I/Os
    व्योम visit(AstPrimitive*) override {}

    // Coverage artifacts etc shouldn't count as a sink
    व्योम visit(Ast_गाँठ_Cover_Decl*) override {}
    व्योम visit(AstCoverInc*) override {}
    व्योम visit(AstCoverToggle*) override {}
    व्योम visit(AstTraceDecl* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    व्योम visit(AstTraceInc* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }

    // iterate
    व्योम visit(AstConst* nodep) override {}
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

खुला:
    // CONSTRUCTORS
    explicit UndrivenVisitor(AstNetlist* nodep, V3UndrivenCapture* capturep)
        : m_capturep{capturep} {
        iterateConst(nodep);
    }

    ~UndrivenVisitor() override {
        क्रम (UndrivenVarEntry* ip : m_entryps[1]) ip->reportViolations();
        क्रम (पूर्णांक usr = 1; usr < 3; ++usr) {
            क्रम (UndrivenVarEntry* ip : m_entryps[usr]) मिटाओ ip;
        }
    }
};

//######################################################################
// Undriven class functions

व्योम V3Undriven::undrivenAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    V3UndrivenCapture capture{nodep};
    { UndrivenVisitor{nodep, &capture}; }

    अगर (v3Global.opt.stats()) V3Stats::statsStage("undriven");
}
