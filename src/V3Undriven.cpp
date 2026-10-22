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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Undriven.h"

#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3UndrivenCapture.h"
#અન્તર્ભાવ "V3Width.h"

#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Class for every variable we may process

વર્ગ UndrivenVarEntry final {
    // MEMBERS
    AstVar* અચલ m_varp;  // Variable this tracks
    std::vector<તાર્કિક> m_wholeFlags;  // Used/Driven on whole vector
    std::vector<તાર્કિક> m_bitFlags;  // Used/Driven on each subbit
    અચલ Ast_ગાંઠ* m_usedNotDrivenp = nullptr;  // First read before any write
    અચલ AstAlways* m_alwCombp
        = nullptr;  // always_comb of var if driven within always_comb, else nullptr
    અચલ AstAlways* m_alwFFp = nullptr;  // always_ff of var if driven within always_ff
    અચલ AstAlways* m_alwPlainp = nullptr;  // plain always of var if driven within plain always
    અચલ AstClocking* m_clockingp = nullptr;  // clocking block of var if driven as output
    અચલ Ast_ગાંઠ_Var_Ref* m_nodep = nullptr;  // varref if driven, else nullptr
    અચલ Ast_ગાંઠ* m_initStaticp = nullptr;  // varref if in InitialStatic driven
    અચલ Ast_ગાંઠ* m_initialp = nullptr;  // varref if driven in an explicit initial block
    અચલ Ast_ગાંઠ* m_contAssignp = nullptr;  // varref if in continuous assignment driven
    અચલ Ast_ગાંઠ* m_procWritep = nullptr;  // varref if written in process
    તાર્કિક m_underGen = ખોટું;  // Under a generate
    તાર્કિક m_ftaskDriven = ખોટું;  // Last driven by function or task

    અચલ Ast_ગાંઠ_FTask_Ref* m_callNodep = nullptr;  // Call node if driven via writeSummary

    ગણના : uint8_t {
        FLAG_USED = 0,  // Signal or bit has been read/observed
        FLAG_DRIVEN = 1,  // Signal or bit has been written/driven
        FLAG_DRIVEN_ALWCOMB = 2,  // Whole signal has been driven from always_comb
        FLAG_DRIVEN_ALWFF = 3,  // Whole signal has been driven from always_ff
        FLAG_DRIVEN_ALWPLAIN = 4,  // Whole signal has been driven from a plain always
        FLAG_DRIVEN_CLOCKING = 5,  // Whole signal has been driven as a clocking block output
        FLAGS_PER_BIT = 6  // Number of flags stored for each tracked bit
    };

જાહેર:
    // CONSTRUCTORS
    explicit UndrivenVarEntry(AstVar* varp)
        : m_varp{varp} {  // Construction for when a var is used
        UINFO(9, "create " << varp);
        m_wholeFlags.resize(FLAGS_PER_BIT);
        માટે (પૂર્ણાંક i = 0; i < FLAGS_PER_BIT; i++) m_wholeFlags[i] = ખોટું;
        m_bitFlags.resize(varp->width() * FLAGS_PER_BIT);
        માટે (પૂર્ણાંક i = 0; i < varp->width() * FLAGS_PER_BIT; i++) m_bitFlags[i] = ખોટું;
    }
    ~UndrivenVarEntry() = મૂળભૂત;

ખાનગી:
    // METHODS
    તાર્કિક bit_અંક_Ok(પૂર્ણાંક bit) અચલ {
        પાછા bit >= 0 && (bit * FLAGS_PER_BIT < static_cast<પૂર્ણાંક>(m_bitFlags.size()));
    }
    તાર્કિક usedFlag(પૂર્ણાંક bit) અચલ {
        પાછા m_wholeFlags[FLAG_USED] || m_bitFlags[bit * FLAGS_PER_BIT + FLAG_USED];
    }
    તાર્કિક drivenFlag(પૂર્ણાંક bit) અચલ {
        પાછા m_wholeFlags[FLAG_DRIVEN] || m_bitFlags[bit * FLAGS_PER_BIT + FLAG_DRIVEN];
    }
    પૂર્ણાંક bit_ગણતરી() અચલ { પાછા m_bitFlags.size() / FLAGS_PER_BIT; }
    શૂન્ય recordUsedNotDriven(અચલ Ast_ગાંઠ* nodep) {
        જો (!m_usedNotDrivenp) m_usedNotDrivenp = nodep;
    }
    ગણના BitNamesWhich : uint8_t { BN_UNUSED, BN_UNDRIVEN, BN_BOTH };
    string bitNames(BitNamesWhich which) {
        string bits;
        તાર્કિક prev = ખોટું;
        પૂર્ણાંક msb = 0;
        // bit==-1 loops below; we do one extra iteration so end with prev=false
        માટે (પૂર્ણાંક bit = (m_bitFlags.size() / FLAGS_PER_BIT) - 1; bit >= -1; --bit) {
            જો (bit >= 0
                && ((which == BN_UNUSED && !usedFlag(bit) && drivenFlag(bit))
                    || (which == BN_UNDRIVEN && usedFlag(bit) && !drivenFlag(bit))
                    || (which == BN_BOTH && !usedFlag(bit) && !drivenFlag(bit)))) {
                જો (!prev) {
                    prev = સાચું;
                    msb = bit;
                }
            } નહીંતર જો (prev) {
                અચલ AstBasicDType* અચલ bdtypep = m_varp->basicp();
                અચલ પૂર્ણાંક lsb = bit + 1;
                જો (bits != "") bits += ",";
                જો (lsb == msb) {
                    bits += cvtToStr(lsb + bdtypep->lo());
                } નહીંતર {
                    જો (bdtypep->ascending()) {
                        bits
                            += cvtToStr(lsb + bdtypep->lo()) + ":" + cvtToStr(msb + bdtypep->lo());
                    } નહીંતર {
                        bits
                            += cvtToStr(msb + bdtypep->lo()) + ":" + cvtToStr(lsb + bdtypep->lo());
                    }
                }
                prev = ખોટું;
            }
        }
        પાછા "[" + bits + "]";
    }

જાહેર:
    શૂન્ય usedWhole(અચલ Ast_ગાંઠ* nodep) {
        UINFO(9, "set u[*] " << m_varp->name() << " " << nodep);
        જો (!m_wholeFlags[FLAG_DRIVEN]) {
            માટે (પૂર્ણાંક bit = 0; bit < bit_ગણતરી(); ++bit) {
                જો (!drivenFlag(bit)) {
                    recordUsedNotDriven(nodep);
                    તોડો;
                }
            }
        }
        m_wholeFlags[FLAG_USED] = સાચું;
    }
    શૂન્ય drivenWhole(અચલ Ast_ગાંઠ* nodep) {
        UINFO(9, "set d[*] " << m_varp->name() << " " << nodep);
        m_wholeFlags[FLAG_DRIVEN] = સાચું;
    }
    શૂન્ય drivenWhole(અચલ Ast_ગાંઠ_Var_Ref* nodep, તાર્કિક ftaskDef) {
        અચલ તાર્કિક wasDriven = isDrivenWhole();
        drivenWhole(nodep);
        // A write in a task/function definition is not an executed process, so it must not
        // displace an already recorded write, nor clear the flag that keeps such a write
        // from being reported as the other write of a MULTIDRIVEN
        જો (ftaskDef && wasDriven) પાછા;
        m_ftaskDriven = ftaskDef;
        m_nodep = nodep;
    }
    શૂન્ય drivenAlwaysCombWhole(અચલ AstAlways* alwCombp) {
        m_wholeFlags[FLAG_DRIVEN_ALWCOMB] = સાચું;
        m_alwCombp = alwCombp;
    }
    શૂન્ય drivenAlwaysFFWhole(અચલ AstAlways* alwFFp, અચલ AstVar* varp) {
        m_wholeFlags[FLAG_DRIVEN_ALWFF] = સાચું;
        m_alwFFp = alwFFp;
    }
    શૂન્ય drivenAlwaysPlainWhole(અચલ AstAlways* alwPlainp) {
        m_wholeFlags[FLAG_DRIVEN_ALWPLAIN] = સાચું;
        m_alwPlainp = alwPlainp;
    }
    શૂન્ય drivenClockingWhole(અચલ AstClocking* clockingp) {
        m_wholeFlags[FLAG_DRIVEN_CLOCKING] = સાચું;
        m_clockingp = clockingp;
    }

    અચલ Ast_ગાંઠ* initStaticp() અચલ { પાછા m_initStaticp; }
    શૂન્ય initStaticp(અચલ Ast_ગાંઠ* nodep) { m_initStaticp = nodep; }
    અચલ Ast_ગાંઠ* initialp() અચલ { પાછા m_initialp; }
    શૂન્ય initialp(અચલ Ast_ગાંઠ* nodep) { m_initialp = nodep; }
    અચલ Ast_ગાંઠ* contAssignp() અચલ { પાછા m_contAssignp; }
    શૂન્ય contAssignp(અચલ Ast_ગાંઠ* nodep) { m_contAssignp = nodep; }
    અચલ Ast_ગાંઠ* procWritep() અચલ { પાછા m_procWritep; }
    શૂન્ય procWritep(અચલ Ast_ગાંઠ* nodep) { m_procWritep = nodep; }
    શૂન્ય underGenerate() { m_underGen = સાચું; }
    તાર્કિક isUnderGen() અચલ { પાછા m_underGen; }
    તાર્કિક isDrivenWhole() અચલ { પાછા m_wholeFlags[FLAG_DRIVEN]; }
    તાર્કિક isDrivenAlwaysCombWhole() અચલ { પાછા m_wholeFlags[FLAG_DRIVEN_ALWCOMB]; }
    તાર્કિક isDrivenAlwaysFFWhole() અચલ { પાછા m_wholeFlags[FLAG_DRIVEN_ALWFF]; }
    તાર્કિક isDrivenAlwaysPlainWhole() અચલ { પાછા m_wholeFlags[FLAG_DRIVEN_ALWPLAIN]; }
    તાર્કિક isDrivenClockingWhole() અચલ { પાછા m_wholeFlags[FLAG_DRIVEN_CLOCKING]; }
    તાર્કિક isFtaskDriven() અચલ { પાછા m_ftaskDriven; }
    અચલ Ast_ગાંઠ_Var_Ref* getNodep() અચલ { પાછા m_nodep; }
    અચલ AstAlways* getAlwCombp() અચલ { પાછા m_alwCombp; }
    અચલ AstAlways* getAlwFFp() અચલ { પાછા m_alwFFp; }
    અચલ AstAlways* getAlwPlainp() અચલ { પાછા m_alwPlainp; }
    અચલ AstClocking* getClockingp() અચલ { પાછા m_clockingp; }
    શૂન્ય usedBit(પૂર્ણાંક bit, પૂર્ણાંક width, અચલ Ast_ગાંઠ* nodep) {
        UINFO(9, "set u[" << (bit + width - 1) << ":" << bit << "] " << m_varp->name());
        માટે (પૂર્ણાંક i = 0; i < width; i++) {
            જો (bit_અંક_Ok(bit + i)) {
                જો (!drivenFlag(bit + i)) recordUsedNotDriven(nodep);
                m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_USED] = સાચું;
            }
        }
    }
    શૂન્ય drivenBit(પૂર્ણાંક bit, પૂર્ણાંક width) {
        UINFO(9, "set d[" << (bit + width - 1) << ":" << bit << "] " << m_varp->name());
        માટે (પૂર્ણાંક i = 0; i < width; i++) {
            જો (bit_અંક_Ok(bit + i)) m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_DRIVEN] = સાચું;
        }
    }
    તાર્કિક isUsedNotDrivenBit(પૂર્ણાંક bit, પૂર્ણાંક width) અચલ {
        માટે (પૂર્ણાંક i = 0; i < width; i++) {
            જો (bit_અંક_Ok(bit + i)
                && (m_wholeFlags[FLAG_USED] || m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_USED])
                && !(m_wholeFlags[FLAG_DRIVEN]
                     || m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_DRIVEN]))
                પાછા સાચું;
        }
        પાછા ખોટું;
    }
    તાર્કિક isUsedNotDrivenAny() અચલ {
        પાછા isUsedNotDrivenBit(0, m_bitFlags.size() / FLAGS_PER_BIT);
    }
    અચલ Ast_ગાંઠ* firstUsedNotDrivenp() અચલ { પાછા m_usedNotDrivenp; }
    સ્થિર તાર્કિક unusedMatch(AstVar* nodep) {
        અચલ string regexp = v3Global.opt.unusedRegexp();
        જો (regexp == "") પાછા ખોટું;
        અચલ string prettyName = nodep->prettyName();
        પાછા VString::wildmatch(prettyName.c_str(), regexp.c_str());
    }
    શૂન્ય reportViolations() {
        // Combine bits into overall state
        AstVar* અચલ nodep = m_varp;

        જો (initStaticp() && procWritep() && nodep->hasUserInit() && !nodep->isClassMember()
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
        અચલ Ast_ગાંઠ* અચલ initp = nodep->hasUserInit() ? initStaticp() : initialp();
        જો (initp && contAssignp() && !nodep->isClassMember() && !nodep->isFuncLocal()) {
            initp->v3warn(E_CONTASSINIT, "Continuous assignment to variable with initial value: "
                                             << nodep->prettyNameQ() << '\n'
                                             << initp->warnMore()
                                             << "... Location of variable initialization\n"
                                             << initp->warnContextPrimary() << '\n'
                                             << contAssignp()->warnOther()
                                             << "... Location of continuous assignment\n"
                                             << contAssignp()->warnContextSecondary());
        }
        જો (nodep->isGenVar()) {  // Genvar
            જો (!nodep->isIfaceRef() && !nodep->isUsedParam() && !unusedMatch(nodep)) {
                nodep->v3warn(UNUSEDGENVAR, "Genvar is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDGENVAR,
                                                 સાચું);  // Warn only once
            }
        } નહીંતર જો (nodep->isParam()) {  // Parameter
            જો (!nodep->isIfaceRef() && !nodep->isUsedParam() && !unusedMatch(nodep)) {
                nodep->v3warn(UNUSEDPARAM, "Parameter is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDPARAM,
                                                 સાચું);  // Warn only once
            }
        } નહીંતર {  // Signal
            અચલ string varType{nodep->isFuncLocal() ? "Function variable" : "Signal"};
            અચલ તાર્કિક funcInout = nodep->isFuncLocal() && nodep->isInout();
            તાર્કિક allU = સાચું;
            તાર્કિક allD = સાચું;
            તાર્કિક anyU = m_wholeFlags[FLAG_USED];
            તાર્કિક anyD = m_wholeFlags[FLAG_DRIVEN];
            તાર્કિક anyUnotD = ખોટું;
            તાર્કિક anyDnotU = ખોટું;
            તાર્કિક anynotDU = ખોટું;
            માટે (unsigned bit = 0; bit < m_bitFlags.size() / FLAGS_PER_BIT; bit++) {
                અચલ તાર્કિક used = usedFlag(bit);
                અચલ તાર્કિક driv = drivenFlag(bit);
                allU &= used;
                anyU |= used;
                allD &= driv;
                anyD |= driv;
                anyUnotD |= used && !driv;
                anyDnotU |= !used && driv;
                anynotDU |= !used && !driv;
            }
            જો (funcInout) {
                જો (anyD) allU = સાચું;
                allD = સાચું;
            }
            જો (allU) m_wholeFlags[FLAG_USED] = સાચું;
            જો (allD) m_wholeFlags[FLAG_DRIVEN] = સાચું;
            // Test results
            જો (nodep->isIfaceRef()) {
                // For interface top level we don't do any tracking
                // Ideally we'd report unused instance cells, but presumably a signal inside one
                // would get reported as unused
            } નહીંતર જો (allU && allD) {
                // It's fine
            } નહીંતર જો (!anyD && !anyU) {
                // UNDRIVEN is considered more serious - as is more likely a bug,
                // thus undriven+unused bits get UNUSED warnings, as they're not as buggy.
                જો (!unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not driven, nor used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL,
                                                     સાચું);  // Warn only once
                }
            } નહીંતર જો (allD && !anyU) {
                જો (!unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL,
                                                     સાચું);  // Warn only once
                }
            } નહીંતર જો (!anyD && allU) {
                nodep->v3warn(UNDRIVEN, varType << " is not driven: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNDRIVEN, સાચું);  // Warn only once
            } નહીંતર જો (!funcInout) {
                // Bits have different dispositions
                અચલ std::string varTypeLower = [&varType]() {
                    std::string str = varType;
                    str[0] = std::tolower(static_cast<unsigned અક્ષર>(str[0]));
                    પાછા str;
                }();
                તાર્કિક setU = ખોટું;
                તાર્કિક setD = ખોટું;
                જો (anynotDU && !unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  "Bits of " << varTypeLower << " are not driven, nor used: "
                                             << nodep->prettyNameQ() << bitNames(BN_BOTH));
                    setU = સાચું;
                }
                જો (anyDnotU && !unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL, "Bits of " << varTypeLower << " are not used: "
                                                           << nodep->prettyNameQ()
                                                           << bitNames(BN_UNUSED));
                    setU = સાચું;
                }
                જો (anyUnotD) {
                    nodep->v3warn(UNDRIVEN, "Bits of " << varTypeLower << " are not driven: "
                                                       << nodep->prettyNameQ()
                                                       << bitNames(BN_UNDRIVEN));
                    setD = સાચું;
                }
                જો (setU) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, સાચું);
                }
                જો (setD) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNDRIVEN, સાચું);
                }
            }
        }
    }

    શૂન્ય drivenViaCall(અચલ Ast_ગાંઠ_FTask_Ref* nodep) {
        drivenWhole(nodep);
        જો (!m_callNodep) m_callNodep = nodep;
    }
    અચલ Ast_ગાંઠ_FTask_Ref* callNodep() અચલ { પાછા m_callNodep; }
};

//######################################################################
// Undriven state, as a visitor of each AstNode

વર્ગ UndrivenVisitor final : જાહેર VNVisitorConst {
    // NODE STATE
    // Netlist:
    //  AstVar::user1p          -> UndrivenVar* for usage var, 0=not set yet
    અચલ VNUser1InUse m_inuser1;
    // Each always:
    //  AstNode::user2p         -> UndrivenVar* for usage var, 0=not set yet
    અચલ VNUser2InUse m_inuser2;

    // STATE
    std::array<std::vector<UndrivenVarEntry*>, 3> m_entryps = {};  // Nodes to delete when finished
    તાર્કિક m_inBBox = ખોટું;  // In black box; mark as driven+used
    તાર્કિક m_inContAssign = ખોટું;  // In continuous assignment
    તાર્કિક m_inInitial = ખોટું;  // In explicit initial block
    તાર્કિક m_inInitialSetup = ખોટું;  // In InitialAutomatic*/InitialStatic* assignment LHS
    તાર્કિક m_inInitialStatic = ખોટું;  // In InitialStatic
    તાર્કિક m_inProcAssign = ખોટું;  // In procedural assignment
    તાર્કિક m_inFTaskRef = ખોટું;  // In function or task call
    તાર્કિક m_inInoutOrRefPin = ખોટું;  // Connected to pin that is inout
    તાર્કિક m_inSelLhs = ખોટું;  // Iterating the fromp of an AstSel (a partial-bit write target)
    અચલ Ast_ગાંઠ_FTask* m_taskp = nullptr;  // Current task
    અચલ AstAlways* m_alwaysp = nullptr;  // Current always of either type
    અચલ AstAlways* m_alwaysCombp = nullptr;  // Current always if combo, otherwise nullptr
    અચલ AstAlways* m_alwaysFFp = nullptr;  // Current always if ff, otherwise nullptr
    અચલ AstAlways* m_alwaysPlainp = nullptr;  // Current always if plain (not comb/ff/latch)
    અચલ AstClocking* m_clockingp = nullptr;  // Current clocking block, otherwise nullptr

    V3UndrivenCapture* અચલ m_capturep = nullptr;  // Capture object.  'nullptr' if disabled.

    // METHODS

    UndrivenVarEntry* getEntryp(AstVar* nodep, પૂર્ણાંક which_user) {
        જો (!(which_user == 1 ? nodep->user1p() : nodep->user2p())) {
            UndrivenVarEntry* અચલ entryp = નવું UndrivenVarEntry{nodep};
            // UINFO(9," Associate u="<<which_user<<" "<<cvtToHex(this)<<" "<<nodep->name()<<endl);
            m_entryps[which_user].ધકેલો_back(entryp);
            જો (which_user == 1) {
                nodep->user1p(entryp);
            } નહીંતર જો (which_user == 2) {
                nodep->user2p(entryp);
            } નહીંતર {
                nodep->v3fatalSrc("Bad case");
            }
            પાછા entryp;
        } નહીંતર {
            UndrivenVarEntry* અચલ entryp = reinterpret_cast<UndrivenVarEntry*>(
                which_user == 1 ? nodep->user1p() : nodep->user2p());
            પાછા entryp;
        }
    }

    શૂન્ય warnAlwCombOrder(Ast_ગાંઠ_Var_Ref* nodep, અચલ Ast_ગાંઠ* readp) {
        AstVar* અચલ varp = nodep->varp();
        જો (!varp->isParam() && !varp->isGenVar() && !varp->isUsedLoopIdx()
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
                                            સાચું);  // Complain just once for any usage
        }
    }
    // A clocking block 'output' is an additional driver of the signal it names.
    // The conflict is found from whichever driver is reached second.
    તાર્કિક clockingDrivesOther(અચલ UndrivenVarEntry* entryp, તાર્કિક otherWriteIsStaticInit) અચલ {
        અચલ તાર્કિક otherIsExplicitProc
            = entryp->isDrivenAlwaysCombWhole() || entryp->isDrivenAlwaysFFWhole();
        પાછા m_clockingp && !otherIsExplicitProc && !otherWriteIsStaticInit
               && m_clockingp != entryp->getClockingp();
    }
    તાર્કિક otherDrivesClocking(અચલ UndrivenVarEntry* entryp) અચલ {
        પાછા !m_clockingp && !m_alwaysCombp && !m_alwaysFFp && !m_inInitialStatic
               && entryp->isDrivenClockingWhole();
    }
    // A continuous assignment contending with a clocking block 'output' is an
    // unambiguous driver conflict. Multiple clocking blocks may drive the same
    // signal, the last drive wins (IEEE 1800-2023 14.16.2).
    શૂન્ય warnClockingDriven(Ast_ગાંઠ_Var_Ref* nodep, અચલ UndrivenVarEntry* entryp,
                            અચલ Ast_ગાંઠ* otherWritep, તાર્કિક otherWriteIsStaticInit) {
        જો (clockingDrivesOther(entryp, otherWriteIsStaticInit)
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
        જો (otherDrivesClocking(entryp) && m_inContAssign) {
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
    શૂન્ય warnClockingDrivenProc(Ast_ગાંઠ_Var_Ref* nodep, અચલ UndrivenVarEntry* entryp,
                                અચલ Ast_ગાંઠ* otherWritep, તાર્કિક otherWriteIsStaticInit) {
        જો (clockingDrivesOther(entryp, otherWriteIsStaticInit) && !entryp->getClockingp()
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
        જો (otherDrivesClocking(entryp) && !m_inContAssign) {
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
    શૂન્ય visit(AstVar* nodep) override {
        અચલ તાર્કિક funcInout = nodep->isFuncLocal() && nodep->isInout();
        માટે (પૂર્ણાંક usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            // For assigns and non-combo always, do just usr==1, to look
            // for module-wide undriven etc.
            // For combo always, run both usr==1 for above, and also
            // usr==2 for always-only checks.
            UndrivenVarEntry* અચલ entryp = getEntryp(nodep, usr);
            જો ((nodep->isNonOutput() && !funcInout) || nodep->isSigPublic()
                || nodep->hasUserInit() || nodep->isSigUserRWPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->drivenWhole(nodep);
            }
            જો ((nodep->isWritable() && !funcInout) || nodep->isSigPublic()
                || nodep->isSigUserRWPublic() || nodep->isSigUserRdPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->usedWhole(nodep);
            }
            જો (nodep->valuep()) entryp->drivenWhole(nodep->valuep());
        }
        // Discover variables used in bit definitions, etc
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstArraySel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstSliceSel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstSel* nodep) override {
        Ast_ગાંઠ_Var_Ref* અચલ varrefp = VN_CAST(nodep->fromp(), ગાંઠ_Var_Ref);
        AstConst* અચલ constp = VN_CAST(nodep->lsbp(), Const);
        જો (varrefp && constp && !constp->અંક().isFourState()) {
            માટે (પૂર્ણાંક usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                UndrivenVarEntry* અચલ entryp = getEntryp(varrefp->varp(), usr);
                અચલ પૂર્ણાંક lsb = constp->toUInt();
                જો (m_inBBox || varrefp->access().isWriteOrRW()) {
                    // Don't warn if already driven earlier as "a=0; if(a) a=1;" is fine.
                    જો (usr == 2 && m_alwaysCombp
                        && entryp->isUsedNotDrivenBit(lsb, nodep->width())) {
                        UINFO(9, " Select.  Entryp=" << cvtToHex(entryp));
                        warnAlwCombOrder(varrefp, entryp->firstUsedNotDrivenp());
                    }
                    entryp->drivenBit(lsb, nodep->width());
                }
                જો (m_inBBox || !varrefp->access().isWriteOrRW())
                    entryp->usedBit(lsb, nodep->width(), varrefp);
            }
        } નહીંતર {
            // skip over static longest static prefix
            iterateConst(nodep->lsbp());
            VL_RESTORER(m_inSelLhs);
            m_inSelLhs = !V3Width::selectNonConstantRecurse(nodep->lsbp(), /*inSel=*/સાચું);
            iterateConst(nodep->fromp());
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        // Any variable
        જો (nodep->access().isWriteOrRW()
            && !VN_IS(nodep, VarXRef)) {  // Ignore interface variables and similar ugly items
            જો (m_inProcAssign && !nodep->varp()->varType().isProcAssignable()
                && !nodep->varp()->isDeclTyped()  //
                && !nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()) {
                nodep->v3warn(PROCASSWIRE, "Procedural assignment to wire, perhaps intended var"
                                               << " (IEEE 1800-2023 6.5): "
                                               << nodep->prettyNameQ());
            } નહીંતર જો (m_inContAssign && !nodep->varp()->varType().isContAssignable()
                       && !nodep->fileline()->language().systemVerilog()) {
                nodep->v3warn(CONTASSREG,
                              "Continuous assignment to reg, perhaps intended wire"
                                  << " (IEEE 1364-2005 6.1; Verilog only, legal in SV): "
                                  << nodep->prettyNameQ());
            }
            જો (m_inFTaskRef && nodep->varp()->varType().isNet()) {
                nodep->v3warn(
                    PROCASSWIRE,
                    "Passed wire on output or inout subroutine argument, expected expression that "
                    "is valid on the left hand side of a procedural assignment"
                        << " (IEEE 1800-2023 13.5): " << nodep->prettyNameQ());
            }
        }

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Remember that anything driven here doesn't count toward MULTIDRIVEN.
        તાર્કિક ftaskDef = ખોટું;
        જો (m_taskp && !m_alwaysp && !m_inContAssign && !m_inInitialStatic && !m_inBBox
            && !m_taskp->dpiExport()) {
            AstVar* અચલ retVarp = VN_CAST(m_taskp->fvarp(), Var);
            જો (!retVarp || nodep->varp() != retVarp) ftaskDef = સાચું;
        }

        માટે (પૂર્ણાંક usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            UndrivenVarEntry* અચલ entryp = getEntryp(nodep->varp(), usr);
            અચલ તાર્કિક fdrv = nodep->access().isWriteOrRW()
                              && nodep->varp()->attrFileDescr();  // FD's are also being read from
            જો (m_inBBox || nodep->access().isWriteOrRW()) {
                જો (usr == 2 && m_alwaysCombp && entryp->isUsedNotDrivenAny()) {
                    UINFO(9, " Full bus.  Entryp=" << cvtToHex(entryp));
                    warnAlwCombOrder(nodep, entryp->firstUsedNotDrivenp());
                }
                અચલ Ast_ગાંઠ_Var_Ref* અચલ otherVarRefp = entryp->getNodep();
                અચલ Ast_ગાંઠ* અચલ otherWritep = otherVarRefp
                                                       ? static_cast<અચલ Ast_ગાંઠ*>(otherVarRefp)
                                                       : entryp->callNodep();
                અચલ તાર્કિક same_File_લીટી
                    = otherVarRefp && nodep->fileline() == otherVarRefp->fileline();
                // Preconditions shared by MULTIDRIVEN and MULTIDRIVENPROC.
                અચલ તાર્કિક multidrivenCommon
                    = entryp->isDrivenWhole() && !m_inBBox && !VN_IS(nodep, VarXRef)
                      && !VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType) && !same_File_લીટી
                      && !entryp->isUnderGen() && otherWritep && !entryp->isFtaskDriven()
                      && !ftaskDef && !m_inSelLhs;
                // The two warnings are gated independently on the variable
                // declaration's fileline, as v3warn suppression will check
                // the driving fileline and still warn even if the warning
                // was suppressed with lint_off at the declaration.
                અચલ તાર્કિક otherWriteIsStaticInit
                    = nodep->varp()->hasUserInit() && otherWritep == entryp->initStaticp();
                જો (multidrivenCommon
                    && !nodep->varp()->fileline()->warnIsOff(V3ErrorCode::MULTIDRIVEN)) {
                    જો (m_alwaysCombp
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
                    જો (!m_alwaysCombp && entryp->isDrivenAlwaysCombWhole()) {
                        nodep->v3warn(MULTIDRIVEN, "Variable also written to in always_comb"
                                                       << " (IEEE 1800-2023 9.2.2.2): "
                                                       << nodep->prettyNameQ() << '\n'
                                                       << nodep->warnOther() << '\n'
                                                       << nodep->warnContextPrimary() << '\n'
                                                       << otherWritep->warnOther()
                                                       << "... Location of always_comb write\n"
                                                       << otherWritep->warnContextSecondary());
                    }
                    જો (m_alwaysFFp && !otherWriteIsStaticInit
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
                    જો (!m_alwaysFFp && !m_inInitialStatic && entryp->isDrivenAlwaysFFWhole()) {
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
                જો (multidrivenCommon && !nodep->varp()->isUsedLoopIdx()
                    && !nodep->varp()->fileline()->warnIsOff(V3ErrorCode::MULTIDRIVENPROC)) {
                    // Two plain always blocks driving the whole signal: legal
                    // SystemVerilog, but a driver conflict for synthesis. The
                    // always_ff/always_comb cases above already cover mixes with
                    // an explicit process, so only warn for plain+plain here.
                    // When the two blocks are clocked differently, the
                    // (on-by-default) MULTIDRIVEN check in V3Delayed already reports
                    // the conflict, so don't also emit MULTIDRIVENPROC for that case.
                    અચલ AstAlways* અચલ otherAlwaysp = entryp->getAlwPlainp();
                    અચલ Ast_Sen_વૃક્ષ* અચલ senp
                        = m_alwaysPlainp ? m_alwaysPlainp->sentreep() : nullptr;
                    અચલ Ast_Sen_વૃક્ષ* અચલ otherSenp
                        = otherAlwaysp ? otherAlwaysp->sentreep() : nullptr;
                    અચલ તાર્કિક differentClocking = senp && otherSenp && senp->hasClocked()
                                                   && otherSenp->hasClocked()
                                                   && !senp->same_વૃક્ષ(otherSenp);
                    જો (m_alwaysPlainp && entryp->isDrivenAlwaysPlainWhole()
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
                જો (!m_inInitialSetup || nodep->varp()->hasUserInit()) {
                    // Else don't count default initialization as a driver to a net/variable
                    entryp->drivenWhole(nodep, ftaskDef);
                }
                જો (m_alwaysCombp && entryp->isDrivenAlwaysCombWhole()
                    && m_alwaysCombp != entryp->getAlwCombp()
                    && m_alwaysCombp->fileline() == entryp->getAlwCombp()->fileline())
                    entryp->underGenerate();
                જો (m_alwaysCombp) entryp->drivenAlwaysCombWhole(m_alwaysCombp);
                જો (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, nodep->varp());
                જો (m_alwaysPlainp) entryp->drivenAlwaysPlainWhole(m_alwaysPlainp);
                જો (m_clockingp) entryp->drivenClockingWhole(m_clockingp);
            }
            જો (nodep->access().isWriteOrRW() && !VN_IS(nodep, VarXRef)) {
                // Ignoring xrefs as the initial and assignment to track might refer to two
                // different instances.  Ideally all of V3Undriven would move after V3Scope,
                // then could use VarScope tracking instead.
                જો (m_inInitialStatic && !entryp->initStaticp()) entryp->initStaticp(nodep);
                જો (m_inInitial && !entryp->initialp()) entryp->initialp(nodep);
                જો (m_inContAssign && !entryp->contAssignp()) entryp->contAssignp(nodep);
                જો (m_alwaysp && m_inProcAssign && !entryp->procWritep())
                    entryp->procWritep(nodep);
            }
            જો ((!m_inInitialSetup || nodep->varp()->hasUserInit())
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
    શૂન્ય visit(AstSysIgnore* nodep) override {
        VL_RESTORER(m_inBBox);
        m_inBBox = સાચું;
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(AstAssign* nodep) override {
        VL_RESTORER(m_inProcAssign);
        m_inProcAssign = સાચું;
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = ખોટું;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }
    શૂન્ય visit(AstAssignDly* nodep) override {
        VL_RESTORER(m_inProcAssign);
        m_inProcAssign = સાચું;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstAssignForce* nodep) override {
        iterateConst(nodep->rhsp());
        VL_RESTORER(m_inInitial);
        m_inInitial = ખોટું;
        iterateConst(nodep->lhsp());
    }
    શૂન્ય visit(AstAssignW* nodep) override {
        VL_RESTORER(m_inContAssign);
        m_inContAssign = સાચું;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstRelease* nodep) override {
        VL_RESTORER(m_inInitial);
        m_inInitial = ખોટું;
        iterateConst(nodep->lhsp());
    }
    શૂન્ય visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        m_inInitial = સાચું;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstInitialAutomatic* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = સાચું;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = સાચું;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstInitialStatic* nodep) override {
        VL_RESTORER(m_inInitialStatic);
        m_inInitialStatic = સાચું;
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = સાચું;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = સાચું;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        VL_RESTORER(m_alwaysCombp);
        VL_RESTORER(m_alwaysFFp);
        VL_RESTORER(m_alwaysPlainp);
        Ast_ગાંઠ::user2_Clear_વૃક્ષ();
        m_alwaysp = nodep;
        જો (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            UINFO(9, "   " << nodep);
            m_alwaysCombp = nodep;
        } નહીંતર {
            m_alwaysCombp = nullptr;
        }
        m_alwaysFFp = nodep->keyword() == VAlwaysKwd::ALWAYS_FF ? nodep : nullptr;
        m_alwaysPlainp = nodep->keyword() == VAlwaysKwd::ALWAYS ? nodep : nullptr;
        iterateChildrenConst(nodep);
        જો (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) UINFO(9, "   Done " << nodep);
    }
    શૂન્ય visit(AstClocking* nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        VL_RESTORER(m_inFTaskRef);
        m_inFTaskRef = સાચું;

        iterateChildrenConst(nodep);

        જો (!m_capturep) પાછા;

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Do not apply writeSummary at calls inside a task definition, or they will look like
        // independent drivers (phantom MULTIDRIVEN).
        અચલ તાર્કિક inExecutedContext
            = !(m_taskp && !m_alwaysp && !m_inContAssign && !m_inInitialStatic && !m_inBBox
                && !m_taskp->dpiExport());

        જો (!inExecutedContext) પાછા;

        Ast_ગાંઠ_FTask* અચલ calleep = nodep->taskp();
        જો (!calleep) પાછા;

        અચલ auto& vars = m_capturep->writeSummary(calleep);
        માટે (AstVar* અચલ varp : vars) {
            માટે (પૂર્ણાંક usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                UndrivenVarEntry* અચલ entryp = getEntryp(varp, usr);
                entryp->drivenViaCall(nodep);
                જો (m_alwaysCombp) entryp->drivenAlwaysCombWhole(m_alwaysCombp);
                જો (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, varp);
            }
        }
    }

    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        VL_RESTORER(m_taskp);
        m_taskp = nodep;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstPin* nodep) override {
        VL_RESTORER(m_inInoutOrRefPin);
        m_inInoutOrRefPin = nodep->modVarp()->isInoutOrRef();
        iterateChildrenConst(nodep);
    }

    // Until we support tables, primitives will have undriven and unused I/Os
    શૂન્ય visit(AstPrimitive*) override {}

    // Coverage artifacts etc shouldn't count as a sink
    શૂન્ય visit(Ast_ગાંઠ_Cover_Decl*) override {}
    શૂન્ય visit(AstCoverInc*) override {}
    શૂન્ય visit(AstCoverToggle*) override {}
    શૂન્ય visit(AstTraceDecl* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    શૂન્ય visit(AstTraceInc* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }

    // iterate
    શૂન્ય visit(AstConst* nodep) override {}
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit UndrivenVisitor(AstNetlist* nodep, V3UndrivenCapture* capturep)
        : m_capturep{capturep} {
        iterateConst(nodep);
    }

    ~UndrivenVisitor() override {
        માટે (UndrivenVarEntry* ip : m_entryps[1]) ip->reportViolations();
        માટે (પૂર્ણાંક usr = 1; usr < 3; ++usr) {
            માટે (UndrivenVarEntry* ip : m_entryps[usr]) delete ip;
        }
    }
};

//######################################################################
// Undriven class functions

શૂન્ય V3Undriven::undrivenAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    V3UndrivenCapture capture{nodep};
    { UndrivenVisitor{nodep, &capture}; }

    જો (v3Global.opt.stats()) V3Stats::statsStage("undriven");
}
