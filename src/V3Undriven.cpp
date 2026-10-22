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

صنف UndrivenVarEntry final {
    // MEMBERS
    AstVar* ثابت m_varp;  // Variable this tracks
    std::vector<منطقي> m_wholeFlags;  // Used/Driven on whole vector
    std::vector<منطقي> m_bitFlags;  // Used/Driven on each subbit
    ثابت Ast_گرہ* m_usedNotDrivenp = nullptr;  // First read before any write
    ثابت AstAlways* m_alwCombp
        = nullptr;  // always_comb of var if driven within always_comb, else nullptr
    ثابت AstAlways* m_alwFFp = nullptr;  // always_ff of var if driven within always_ff
    ثابت AstAlways* m_alwPlainp = nullptr;  // plain always of var if driven within plain always
    ثابت AstClocking* m_clockingp = nullptr;  // clocking block of var if driven as output
    ثابت Ast_گرہ_Var_Ref* m_nodep = nullptr;  // varref if driven, else nullptr
    ثابت Ast_گرہ* m_initStaticp = nullptr;  // varref if in InitialStatic driven
    ثابت Ast_گرہ* m_initialp = nullptr;  // varref if driven in an explicit initial block
    ثابت Ast_گرہ* m_contAssignp = nullptr;  // varref if in continuous assignment driven
    ثابت Ast_گرہ* m_procWritep = nullptr;  // varref if written in process
    منطقي m_underGen = خطأ;  // Under a generate
    منطقي m_ftaskDriven = خطأ;  // Last driven by function or task

    ثابت Ast_گرہ_FTask_Ref* m_callNodep = nullptr;  // Call node if driven via writeSummary

    تعداد : uint8_t {
        FLAG_USED = 0,  // Signal or bit has been read/observed
        FLAG_DRIVEN = 1,  // Signal or bit has been written/driven
        FLAG_DRIVEN_ALWCOMB = 2,  // Whole signal has been driven from always_comb
        FLAG_DRIVEN_ALWFF = 3,  // Whole signal has been driven from always_ff
        FLAG_DRIVEN_ALWPLAIN = 4,  // Whole signal has been driven from a plain always
        FLAG_DRIVEN_CLOCKING = 5,  // Whole signal has been driven as a clocking block output
        FLAGS_PER_BIT = 6  // Number of flags stored for each tracked bit
    };

عام:
    // CONSTRUCTORS
    explicit UndrivenVarEntry(AstVar* varp)
        : m_varp{varp} {  // Construction for when a var is used
        UINFO(9, "create " << varp);
        m_wholeFlags.resize(FLAGS_PER_BIT);
        لكل (صحيح i = 0; i < FLAGS_PER_BIT; i++) m_wholeFlags[i] = خطأ;
        m_bitFlags.resize(varp->width() * FLAGS_PER_BIT);
        لكل (صحيح i = 0; i < varp->width() * FLAGS_PER_BIT; i++) m_bitFlags[i] = خطأ;
    }
    ~UndrivenVarEntry() = افتراضي;

خاص:
    // METHODS
    منطقي bit_عدد_Ok(صحيح bit) ثابت {
        أرجع bit >= 0 && (bit * FLAGS_PER_BIT < static_cast<صحيح>(m_bitFlags.size()));
    }
    منطقي usedFlag(صحيح bit) ثابت {
        أرجع m_wholeFlags[FLAG_USED] || m_bitFlags[bit * FLAGS_PER_BIT + FLAG_USED];
    }
    منطقي drivenFlag(صحيح bit) ثابت {
        أرجع m_wholeFlags[FLAG_DRIVEN] || m_bitFlags[bit * FLAGS_PER_BIT + FLAG_DRIVEN];
    }
    صحيح bit_گنتی() ثابت { أرجع m_bitFlags.size() / FLAGS_PER_BIT; }
    فراغ recordUsedNotDriven(ثابت Ast_گرہ* nodep) {
        إذا (!m_usedNotDrivenp) m_usedNotDrivenp = nodep;
    }
    تعداد BitNamesWhich : uint8_t { BN_UNUSED, BN_UNDRIVEN, BN_BOTH };
    string bitNames(BitNamesWhich which) {
        string bits;
        منطقي prev = خطأ;
        صحيح msb = 0;
        // bit==-1 loops below; we do one extra iteration so end with prev=false
        لكل (صحيح bit = (m_bitFlags.size() / FLAGS_PER_BIT) - 1; bit >= -1; --bit) {
            إذا (bit >= 0
                && ((which == BN_UNUSED && !usedFlag(bit) && drivenFlag(bit))
                    || (which == BN_UNDRIVEN && usedFlag(bit) && !drivenFlag(bit))
                    || (which == BN_BOTH && !usedFlag(bit) && !drivenFlag(bit)))) {
                إذا (!prev) {
                    prev = true;
                    msb = bit;
                }
            } وإلا إذا (prev) {
                ثابت AstBasicDType* ثابت bdtypep = m_varp->basicp();
                ثابت صحيح lsb = bit + 1;
                إذا (bits != "") bits += ",";
                إذا (lsb == msb) {
                    bits += cvtToStr(lsb + bdtypep->lo());
                } وإلا {
                    إذا (bdtypep->ascending()) {
                        bits
                            += cvtToStr(lsb + bdtypep->lo()) + ":" + cvtToStr(msb + bdtypep->lo());
                    } وإلا {
                        bits
                            += cvtToStr(msb + bdtypep->lo()) + ":" + cvtToStr(lsb + bdtypep->lo());
                    }
                }
                prev = خطأ;
            }
        }
        أرجع "[" + bits + "]";
    }

عام:
    فراغ usedWhole(ثابت Ast_گرہ* nodep) {
        UINFO(9, "set u[*] " << m_varp->name() << " " << nodep);
        إذا (!m_wholeFlags[FLAG_DRIVEN]) {
            لكل (صحيح bit = 0; bit < bit_گنتی(); ++bit) {
                إذا (!drivenFlag(bit)) {
                    recordUsedNotDriven(nodep);
                    اكسر;
                }
            }
        }
        m_wholeFlags[FLAG_USED] = true;
    }
    فراغ drivenWhole(ثابت Ast_گرہ* nodep) {
        UINFO(9, "set d[*] " << m_varp->name() << " " << nodep);
        m_wholeFlags[FLAG_DRIVEN] = true;
    }
    فراغ drivenWhole(ثابت Ast_گرہ_Var_Ref* nodep, منطقي ftaskDef) {
        ثابت منطقي wasDriven = isDrivenWhole();
        drivenWhole(nodep);
        // A write in a task/function definition is not an executed process, so it must not
        // displace an already recorded write, nor clear the flag that keeps such a write
        // from being reported as the other write of a MULTIDRIVEN
        إذا (ftaskDef && wasDriven) أرجع;
        m_ftaskDriven = ftaskDef;
        m_nodep = nodep;
    }
    فراغ drivenAlwaysCombWhole(ثابت AstAlways* alwCombp) {
        m_wholeFlags[FLAG_DRIVEN_ALWCOMB] = true;
        m_alwCombp = alwCombp;
    }
    فراغ drivenAlwaysFFWhole(ثابت AstAlways* alwFFp, ثابت AstVar* varp) {
        m_wholeFlags[FLAG_DRIVEN_ALWFF] = true;
        m_alwFFp = alwFFp;
    }
    فراغ drivenAlwaysPlainWhole(ثابت AstAlways* alwPlainp) {
        m_wholeFlags[FLAG_DRIVEN_ALWPLAIN] = true;
        m_alwPlainp = alwPlainp;
    }
    فراغ drivenClockingWhole(ثابت AstClocking* clockingp) {
        m_wholeFlags[FLAG_DRIVEN_CLOCKING] = true;
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
    فراغ underGenerate() { m_underGen = true; }
    منطقي isUnderGen() ثابت { أرجع m_underGen; }
    منطقي isDrivenWhole() ثابت { أرجع m_wholeFlags[FLAG_DRIVEN]; }
    منطقي isDrivenAlwaysCombWhole() ثابت { أرجع m_wholeFlags[FLAG_DRIVEN_ALWCOMB]; }
    منطقي isDrivenAlwaysFFWhole() ثابت { أرجع m_wholeFlags[FLAG_DRIVEN_ALWFF]; }
    منطقي isDrivenAlwaysPlainWhole() ثابت { أرجع m_wholeFlags[FLAG_DRIVEN_ALWPLAIN]; }
    منطقي isDrivenClockingWhole() ثابت { أرجع m_wholeFlags[FLAG_DRIVEN_CLOCKING]; }
    منطقي isFtaskDriven() ثابت { أرجع m_ftaskDriven; }
    ثابت Ast_گرہ_Var_Ref* getNodep() ثابت { أرجع m_nodep; }
    ثابت AstAlways* getAlwCombp() ثابت { أرجع m_alwCombp; }
    ثابت AstAlways* getAlwFFp() ثابت { أرجع m_alwFFp; }
    ثابت AstAlways* getAlwPlainp() ثابت { أرجع m_alwPlainp; }
    ثابت AstClocking* getClockingp() ثابت { أرجع m_clockingp; }
    فراغ usedBit(صحيح bit, صحيح width, ثابت Ast_گرہ* nodep) {
        UINFO(9, "set u[" << (bit + width - 1) << ":" << bit << "] " << m_varp->name());
        لكل (صحيح i = 0; i < width; i++) {
            إذا (bit_عدد_Ok(bit + i)) {
                إذا (!drivenFlag(bit + i)) recordUsedNotDriven(nodep);
                m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_USED] = true;
            }
        }
    }
    فراغ drivenBit(صحيح bit, صحيح width) {
        UINFO(9, "set d[" << (bit + width - 1) << ":" << bit << "] " << m_varp->name());
        لكل (صحيح i = 0; i < width; i++) {
            إذا (bit_عدد_Ok(bit + i)) m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_DRIVEN] = true;
        }
    }
    منطقي isUsedNotDrivenBit(صحيح bit, صحيح width) ثابت {
        لكل (صحيح i = 0; i < width; i++) {
            إذا (bit_عدد_Ok(bit + i)
                && (m_wholeFlags[FLAG_USED] || m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_USED])
                && !(m_wholeFlags[FLAG_DRIVEN]
                     || m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_DRIVEN]))
                أرجع true;
        }
        أرجع خطأ;
    }
    منطقي isUsedNotDrivenAny() ثابت {
        أرجع isUsedNotDrivenBit(0, m_bitFlags.size() / FLAGS_PER_BIT);
    }
    ثابت Ast_گرہ* firstUsedNotDrivenp() ثابت { أرجع m_usedNotDrivenp; }
    ساكن منطقي unusedMatch(AstVar* nodep) {
        ثابت string regexp = v3Global.opt.unusedRegexp();
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
                    << initStaticp()->warnMore() << "... Location of variable initialization\n"
                    << initStaticp()->warnContextPrimary() << '\n'
                    << procWritep()->warnOther() << "... Location of variable process write\n"
                    << procWritep()->warnMore()
                    << "... Perhaps should initialize instead using a reset in this process\n"
                    << procWritep()->warnContextSecondary());
        }
        ثابت Ast_گرہ* ثابت initp = nodep->hasUserInit() ? initStaticp() : initialp();
        إذا (initp && contAssignp() && !nodep->isClassMember() && !nodep->isFuncLocal()) {
            initp->v3warn(E_CONTASSINIT, "Continuous assignment to variable with initial value: "
                                             << nodep->prettyNameQ() << '\n'
                                             << initp->warnMore()
                                             << "... Location of variable initialization\n"
                                             << initp->warnContextPrimary() << '\n'
                                             << contAssignp()->warnOther()
                                             << "... Location of continuous assignment\n"
                                             << contAssignp()->warnContextSecondary());
        }
        إذا (nodep->isGenVar()) {  // Genvar
            إذا (!nodep->isIfaceRef() && !nodep->isUsedParam() && !unusedMatch(nodep)) {
                nodep->v3warn(UNUSEDGENVAR, "Genvar is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDGENVAR,
                                                 true);  // Warn only once
            }
        } وإلا إذا (nodep->isParam()) {  // Parameter
            إذا (!nodep->isIfaceRef() && !nodep->isUsedParam() && !unusedMatch(nodep)) {
                nodep->v3warn(UNUSEDPARAM, "Parameter is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDPARAM,
                                                 true);  // Warn only once
            }
        } وإلا {  // Signal
            ثابت string varType{nodep->isFuncLocal() ? "Function variable" : "Signal"};
            ثابت منطقي funcInout = nodep->isFuncLocal() && nodep->isInout();
            منطقي allU = true;
            منطقي allD = true;
            منطقي anyU = m_wholeFlags[FLAG_USED];
            منطقي anyD = m_wholeFlags[FLAG_DRIVEN];
            منطقي anyUnotD = خطأ;
            منطقي anyDnotU = خطأ;
            منطقي anynotDU = خطأ;
            لكل (unsigned bit = 0; bit < m_bitFlags.size() / FLAGS_PER_BIT; bit++) {
                ثابت منطقي used = usedFlag(bit);
                ثابت منطقي driv = drivenFlag(bit);
                allU &= used;
                anyU |= used;
                allD &= driv;
                anyD |= driv;
                anyUnotD |= used && !driv;
                anyDnotU |= !used && driv;
                anynotDU |= !used && !driv;
            }
            إذا (funcInout) {
                إذا (anyD) allU = true;
                allD = true;
            }
            إذا (allU) m_wholeFlags[FLAG_USED] = true;
            إذا (allD) m_wholeFlags[FLAG_DRIVEN] = true;
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
                إذا (!unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not driven, nor used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL,
                                                     true);  // Warn only once
                }
            } وإلا إذا (allD && !anyU) {
                إذا (!unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL,
                                                     true);  // Warn only once
                }
            } وإلا إذا (!anyD && allU) {
                nodep->v3warn(UNDRIVEN, varType << " is not driven: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNDRIVEN, true);  // Warn only once
            } وإلا إذا (!funcInout) {
                // Bits have different dispositions
                ثابت std::string varTypeLower = [&varType]() {
                    std::string str = varType;
                    str[0] = std::tolower(static_cast<unsigned حرف>(str[0]));
                    أرجع str;
                }();
                منطقي setU = خطأ;
                منطقي setD = خطأ;
                إذا (anynotDU && !unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  "Bits of " << varTypeLower << " are not driven, nor used: "
                                             << nodep->prettyNameQ() << bitNames(BN_BOTH));
                    setU = true;
                }
                إذا (anyDnotU && !unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL, "Bits of " << varTypeLower << " are not used: "
                                                           << nodep->prettyNameQ()
                                                           << bitNames(BN_UNUSED));
                    setU = true;
                }
                إذا (anyUnotD) {
                    nodep->v3warn(UNDRIVEN, "Bits of " << varTypeLower << " are not driven: "
                                                       << nodep->prettyNameQ()
                                                       << bitNames(BN_UNDRIVEN));
                    setD = true;
                }
                إذا (setU) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, true);
                }
                إذا (setD) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNDRIVEN, true);
                }
            }
        }
    }

    فراغ drivenViaCall(ثابت Ast_گرہ_FTask_Ref* nodep) {
        drivenWhole(nodep);
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
    ثابت VNUser1InUse m_inuser1;
    // Each always:
    //  AstNode::user2p         -> UndrivenVar* for usage var, 0=not set yet
    ثابت VNUser2InUse m_inuser2;

    // STATE
    std::array<std::vector<UndrivenVarEntry*>, 3> m_entryps = {};  // Nodes to delete when finished
    منطقي m_inBBox = خطأ;  // In black box; mark as driven+used
    منطقي m_inContAssign = خطأ;  // In continuous assignment
    منطقي m_inInitial = خطأ;  // In explicit initial block
    منطقي m_inInitialSetup = خطأ;  // In InitialAutomatic*/InitialStatic* assignment LHS
    منطقي m_inInitialStatic = خطأ;  // In InitialStatic
    منطقي m_inProcAssign = خطأ;  // In procedural assignment
    منطقي m_inFTaskRef = خطأ;  // In function or task call
    منطقي m_inInoutOrRefPin = خطأ;  // Connected to pin that is inout
    منطقي m_inSelLhs = خطأ;  // Iterating the fromp of an AstSel (a partial-bit write target)
    ثابت Ast_گرہ_FTask* m_taskp = nullptr;  // Current task
    ثابت AstAlways* m_alwaysp = nullptr;  // Current always of either type
    ثابت AstAlways* m_alwaysCombp = nullptr;  // Current always if combo, otherwise nullptr
    ثابت AstAlways* m_alwaysFFp = nullptr;  // Current always if ff, otherwise nullptr
    ثابت AstAlways* m_alwaysPlainp = nullptr;  // Current always if plain (not comb/ff/latch)
    ثابت AstClocking* m_clockingp = nullptr;  // Current clocking block, otherwise nullptr

    V3UndrivenCapture* ثابت m_capturep = nullptr;  // Capture object.  'nullptr' if disabled.

    // METHODS

    UndrivenVarEntry* getEntryp(AstVar* nodep, صحيح which_user) {
        إذا (!(which_user == 1 ? nodep->user1p() : nodep->user2p())) {
            UndrivenVarEntry* ثابت entryp = جديد UndrivenVarEntry{nodep};
            // UINFO(9," Associate u="<<which_user<<" "<<cvtToHex(this)<<" "<<nodep->name()<<endl);
            m_entryps[which_user].ڈالو_back(entryp);
            إذا (which_user == 1) {
                nodep->user1p(entryp);
            } وإلا إذا (which_user == 2) {
                nodep->user2p(entryp);
            } وإلا {
                nodep->v3fatalSrc("Bad case");
            }
            أرجع entryp;
        } وإلا {
            UndrivenVarEntry* ثابت entryp = reinterpret_cast<UndrivenVarEntry*>(
                which_user == 1 ? nodep->user1p() : nodep->user2p());
            أرجع entryp;
        }
    }

    فراغ warnAlwCombOrder(Ast_گرہ_Var_Ref* nodep, ثابت Ast_گرہ* readp) {
        AstVar* ثابت varp = nodep->varp();
        إذا (!varp->isParam() && !varp->isGenVar() && !varp->isUsedLoopIdx()
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
    منطقي clockingDrivesOther(ثابت UndrivenVarEntry* entryp, منطقي otherWriteIsStaticInit) ثابت {
        ثابت منطقي otherIsExplicitProc
            = entryp->isDrivenAlwaysCombWhole() || entryp->isDrivenAlwaysFFWhole();
        أرجع m_clockingp && !otherIsExplicitProc && !otherWriteIsStaticInit
               && m_clockingp != entryp->getClockingp();
    }
    منطقي otherDrivesClocking(ثابت UndrivenVarEntry* entryp) ثابت {
        أرجع !m_clockingp && !m_alwaysCombp && !m_alwaysFFp && !m_inInitialStatic
               && entryp->isDrivenClockingWhole();
    }
    // A continuous assignment contending with a clocking block 'output' is an
    // unambiguous driver conflict. Multiple clocking blocks may drive the same
    // signal, the last drive wins (IEEE 1800-2023 14.16.2).
    فراغ warnClockingDriven(Ast_گرہ_Var_Ref* nodep, ثابت UndrivenVarEntry* entryp,
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
        إذا (otherDrivesClocking(entryp) && m_inContAssign) {
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
    فراغ warnClockingDrivenProc(Ast_گرہ_Var_Ref* nodep, ثابت UndrivenVarEntry* entryp,
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
        إذا (otherDrivesClocking(entryp) && !m_inContAssign) {
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
    فراغ visit(AstVar* nodep) override {
        ثابت منطقي funcInout = nodep->isFuncLocal() && nodep->isInout();
        لكل (صحيح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            // For assigns and non-combo always, do just usr==1, to look
            // for module-wide undriven etc.
            // For combo always, run both usr==1 for above, and also
            // usr==2 for always-only checks.
            UndrivenVarEntry* ثابت entryp = getEntryp(nodep, usr);
            إذا ((nodep->isNonOutput() && !funcInout) || nodep->isSigPublic()
                || nodep->hasUserInit() || nodep->isSigUserRWPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->drivenWhole(nodep);
            }
            إذا ((nodep->isWritable() && !funcInout) || nodep->isSigPublic()
                || nodep->isSigUserRWPublic() || nodep->isSigUserRdPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->usedWhole(nodep);
            }
            إذا (nodep->valuep()) entryp->drivenWhole(nodep->valuep());
        }
        // Discover variables used in bit definitions, etc
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstArraySel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstSliceSel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstSel* nodep) override {
        Ast_گرہ_Var_Ref* ثابت varrefp = VN_CAST(nodep->fromp(), گرہ_Var_Ref);
        AstConst* ثابت constp = VN_CAST(nodep->lsbp(), Const);
        إذا (varrefp && constp && !constp->عدد().isFourState()) {
            لكل (صحيح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                UndrivenVarEntry* ثابت entryp = getEntryp(varrefp->varp(), usr);
                ثابت صحيح lsb = constp->toUInt();
                إذا (m_inBBox || varrefp->access().isWriteOrRW()) {
                    // Don't warn if already driven earlier as "a=0; if(a) a=1;" is fine.
                    إذا (usr == 2 && m_alwaysCombp
                        && entryp->isUsedNotDrivenBit(lsb, nodep->width())) {
                        UINFO(9, " Select.  Entryp=" << cvtToHex(entryp));
                        warnAlwCombOrder(varrefp, entryp->firstUsedNotDrivenp());
                    }
                    entryp->drivenBit(lsb, nodep->width());
                }
                إذا (m_inBBox || !varrefp->access().isWriteOrRW())
                    entryp->usedBit(lsb, nodep->width(), varrefp);
            }
        } وإلا {
            // skip over static longest static prefix
            iterateConst(nodep->lsbp());
            VL_RESTORER(m_inSelLhs);
            m_inSelLhs = !V3Width::selectNonConstantRecurse(nodep->lsbp(), /*inSel=*/true);
            iterateConst(nodep->fromp());
        }
    }
    فراغ visit(Ast_گرہ_Var_Ref* nodep) override {
        // Any variable
        إذا (nodep->access().isWriteOrRW()
            && !VN_IS(nodep, VarXRef)) {  // Ignore interface variables and similar ugly items
            إذا (m_inProcAssign && !nodep->varp()->varType().isProcAssignable()
                && !nodep->varp()->isDeclTyped()  //
                && !nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()) {
                nodep->v3warn(PROCASSWIRE, "Procedural assignment to wire, perhaps intended var"
                                               << " (IEEE 1800-2023 6.5): "
                                               << nodep->prettyNameQ());
            } وإلا إذا (m_inContAssign && !nodep->varp()->varType().isContAssignable()
                       && !nodep->fileline()->language().systemVerilog()) {
                nodep->v3warn(CONTASSREG,
                              "Continuous assignment to reg, perhaps intended wire"
                                  << " (IEEE 1364-2005 6.1; Verilog only, legal in SV): "
                                  << nodep->prettyNameQ());
            }
            إذا (m_inFTaskRef && nodep->varp()->varType().isNet()) {
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
        إذا (m_taskp && !m_alwaysp && !m_inContAssign && !m_inInitialStatic && !m_inBBox
            && !m_taskp->dpiExport()) {
            AstVar* ثابت retVarp = VN_CAST(m_taskp->fvarp(), Var);
            إذا (!retVarp || nodep->varp() != retVarp) ftaskDef = true;
        }

        لكل (صحيح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            UndrivenVarEntry* ثابت entryp = getEntryp(nodep->varp(), usr);
            ثابت منطقي fdrv = nodep->access().isWriteOrRW()
                              && nodep->varp()->attrFileDescr();  // FD's are also being read from
            إذا (m_inBBox || nodep->access().isWriteOrRW()) {
                إذا (usr == 2 && m_alwaysCombp && entryp->isUsedNotDrivenAny()) {
                    UINFO(9, " Full bus.  Entryp=" << cvtToHex(entryp));
                    warnAlwCombOrder(nodep, entryp->firstUsedNotDrivenp());
                }
                ثابت Ast_گرہ_Var_Ref* ثابت otherVarRefp = entryp->getNodep();
                ثابت Ast_گرہ* ثابت otherWritep = otherVarRefp
                                                       ? static_cast<ثابت Ast_گرہ*>(otherVarRefp)
                                                       : entryp->callNodep();
                ثابت منطقي same_File_سطر
                    = otherVarRefp && nodep->fileline() == otherVarRefp->fileline();
                // Preconditions shared by MULTIDRIVEN and MULTIDRIVENPROC.
                ثابت منطقي multidrivenCommon
                    = entryp->isDrivenWhole() && !m_inBBox && !VN_IS(nodep, VarXRef)
                      && !VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType) && !same_File_سطر
                      && !entryp->isUnderGen() && otherWritep && !entryp->isFtaskDriven()
                      && !ftaskDef && !m_inSelLhs;
                // The two warnings are gated independently on the variable
                // declaration's fileline, as v3warn suppression will check
                // the driving fileline and still warn even if the warning
                // was suppressed with lint_off at the declaration.
                ثابت منطقي otherWriteIsStaticInit
                    = nodep->varp()->hasUserInit() && otherWritep == entryp->initStaticp();
                إذا (multidrivenCommon
                    && !nodep->varp()->fileline()->warnIsOff(V3ErrorCode::MULTIDRIVEN)) {
                    إذا (m_alwaysCombp
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
                    إذا (!m_alwaysCombp && entryp->isDrivenAlwaysCombWhole()) {
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
                    إذا (!m_alwaysFFp && !m_inInitialStatic && entryp->isDrivenAlwaysFFWhole()) {
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
                إذا (multidrivenCommon && !nodep->varp()->isUsedLoopIdx()
                    && !nodep->varp()->fileline()->warnIsOff(V3ErrorCode::MULTIDRIVENPROC)) {
                    // Two plain always blocks driving the whole signal: legal
                    // SystemVerilog, but a driver conflict for synthesis. The
                    // always_ff/always_comb cases above already cover mixes with
                    // an explicit process, so only warn for plain+plain here.
                    // When the two blocks are clocked differently, the
                    // (on-by-default) MULTIDRIVEN check in V3Delayed already reports
                    // the conflict, so don't also emit MULTIDRIVENPROC for that case.
                    ثابت AstAlways* ثابت otherAlwaysp = entryp->getAlwPlainp();
                    ثابت Ast_Sen_درخت* ثابت senp
                        = m_alwaysPlainp ? m_alwaysPlainp->sentreep() : nullptr;
                    ثابت Ast_Sen_درخت* ثابت otherSenp
                        = otherAlwaysp ? otherAlwaysp->sentreep() : nullptr;
                    ثابت منطقي differentClocking = senp && otherSenp && senp->hasClocked()
                                                   && otherSenp->hasClocked()
                                                   && !senp->same_درخت(otherSenp);
                    إذا (m_alwaysPlainp && entryp->isDrivenAlwaysPlainWhole()
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
                إذا (!m_inInitialSetup || nodep->varp()->hasUserInit()) {
                    // Else don't count default initialization as a driver to a net/variable
                    entryp->drivenWhole(nodep, ftaskDef);
                }
                إذا (m_alwaysCombp && entryp->isDrivenAlwaysCombWhole()
                    && m_alwaysCombp != entryp->getAlwCombp()
                    && m_alwaysCombp->fileline() == entryp->getAlwCombp()->fileline())
                    entryp->underGenerate();
                إذا (m_alwaysCombp) entryp->drivenAlwaysCombWhole(m_alwaysCombp);
                إذا (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, nodep->varp());
                إذا (m_alwaysPlainp) entryp->drivenAlwaysPlainWhole(m_alwaysPlainp);
                إذا (m_clockingp) entryp->drivenClockingWhole(m_clockingp);
            }
            إذا (nodep->access().isWriteOrRW() && !VN_IS(nodep, VarXRef)) {
                // Ignoring xrefs as the initial and assignment to track might refer to two
                // different instances.  Ideally all of V3Undriven would move after V3Scope,
                // then could use VarScope tracking instead.
                إذا (m_inInitialStatic && !entryp->initStaticp()) entryp->initStaticp(nodep);
                إذا (m_inInitial && !entryp->initialp()) entryp->initialp(nodep);
                إذا (m_inContAssign && !entryp->contAssignp()) entryp->contAssignp(nodep);
                إذا (m_alwaysp && m_inProcAssign && !entryp->procWritep())
                    entryp->procWritep(nodep);
            }
            إذا ((!m_inInitialSetup || nodep->varp()->hasUserInit())
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
    فراغ visit(AstSysIgnore* nodep) override {
        VL_RESTORER(m_inBBox);
        m_inBBox = true;
        iterateChildrenConst(nodep);
    }

    فراغ visit(AstAssign* nodep) override {
        VL_RESTORER(m_inProcAssign);
        m_inProcAssign = true;
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = خطأ;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }
    فراغ visit(AstAssignDly* nodep) override {
        VL_RESTORER(m_inProcAssign);
        m_inProcAssign = true;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstAssignForce* nodep) override {
        iterateConst(nodep->rhsp());
        VL_RESTORER(m_inInitial);
        m_inInitial = خطأ;
        iterateConst(nodep->lhsp());
    }
    فراغ visit(AstAssignW* nodep) override {
        VL_RESTORER(m_inContAssign);
        m_inContAssign = true;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstRelease* nodep) override {
        VL_RESTORER(m_inInitial);
        m_inInitial = خطأ;
        iterateConst(nodep->lhsp());
    }
    فراغ visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        m_inInitial = true;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstInitialAutomatic* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = true;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = true;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstInitialStatic* nodep) override {
        VL_RESTORER(m_inInitialStatic);
        m_inInitialStatic = true;
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = true;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = true;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstAlways* nodep) override {
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
            m_alwaysCombp = nullptr;
        }
        m_alwaysFFp = nodep->keyword() == VAlwaysKwd::ALWAYS_FF ? nodep : nullptr;
        m_alwaysPlainp = nodep->keyword() == VAlwaysKwd::ALWAYS ? nodep : nullptr;
        iterateChildrenConst(nodep);
        إذا (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) UINFO(9, "   Done " << nodep);
    }
    فراغ visit(AstClocking* nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterateChildrenConst(nodep);
    }

    فراغ visit(Ast_گرہ_FTask_Ref* nodep) override {
        VL_RESTORER(m_inFTaskRef);
        m_inFTaskRef = true;

        iterateChildrenConst(nodep);

        إذا (!m_capturep) أرجع;

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Do not apply writeSummary at calls inside a task definition, or they will look like
        // independent drivers (phantom MULTIDRIVEN).
        ثابت منطقي inExecutedContext
            = !(m_taskp && !m_alwaysp && !m_inContAssign && !m_inInitialStatic && !m_inBBox
                && !m_taskp->dpiExport());

        إذا (!inExecutedContext) أرجع;

        Ast_گرہ_FTask* ثابت calleep = nodep->taskp();
        إذا (!calleep) أرجع;

        ثابت auto& vars = m_capturep->writeSummary(calleep);
        لكل (AstVar* ثابت varp : vars) {
            لكل (صحيح usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                UndrivenVarEntry* ثابت entryp = getEntryp(varp, usr);
                entryp->drivenViaCall(nodep);
                إذا (m_alwaysCombp) entryp->drivenAlwaysCombWhole(m_alwaysCombp);
                إذا (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, varp);
            }
        }
    }

    فراغ visit(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_taskp);
        m_taskp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstPin* nodep) override {
        VL_RESTORER(m_inInoutOrRefPin);
        m_inInoutOrRefPin = nodep->modVarp()->isInoutOrRef();
        iterateChildrenConst(nodep);
    }

    // Until we support tables, primitives will have undriven and unused I/Os
    فراغ visit(AstPrimitive*) override {}

    // Coverage artifacts etc shouldn't count as a sink
    فراغ visit(Ast_گرہ_Cover_Decl*) override {}
    فراغ visit(AstCoverInc*) override {}
    فراغ visit(AstCoverToggle*) override {}
    فراغ visit(AstTraceDecl* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    فراغ visit(AstTraceInc* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }

    // iterate
    فراغ visit(AstConst* nodep) override {}
    فراغ visit(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    explicit UndrivenVisitor(AstNetlist* nodep, V3UndrivenCapture* capturep)
        : m_capturep{capturep} {
        iterateConst(nodep);
    }

    ~UndrivenVisitor() override {
        لكل (UndrivenVarEntry* ip : m_entryps[1]) ip->reportViolations();
        لكل (صحيح usr = 1; usr < 3; ++usr) {
            لكل (UndrivenVarEntry* ip : m_entryps[usr]) delete ip;
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
