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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Undriven.h"

#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3UndrivenCapture.h"
#அந்தர்பாவ "V3Width.h"

#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Class for every variable we may process

வகுப்பு UndrivenVarEntry final {
    // MEMBERS
    AstVar* மாறிலி m_varp;  // Variable this tracks
    std::vector<தருக்க> m_wholeFlags;  // Used/Driven on whole vector
    std::vector<தருக்க> m_bitFlags;  // Used/Driven on each subbit
    மாறிலி Ast_முனை* m_usedNotDrivenp = nullptr;  // First read before any write
    மாறிலி AstAlways* m_alwCombp
        = nullptr;  // always_comb of var if driven within always_comb, else nullptr
    மாறிலி AstAlways* m_alwFFp = nullptr;  // always_ff of var if driven within always_ff
    மாறிலி AstAlways* m_alwPlainp = nullptr;  // plain always of var if driven within plain always
    மாறிலி AstClocking* m_clockingp = nullptr;  // clocking block of var if driven as output
    மாறிலி Ast_முனை_Var_Ref* m_nodep = nullptr;  // varref if driven, else nullptr
    மாறிலி Ast_முனை* m_initStaticp = nullptr;  // varref if in InitialStatic driven
    மாறிலி Ast_முனை* m_initialp = nullptr;  // varref if driven in an explicit initial block
    மாறிலி Ast_முனை* m_contAssignp = nullptr;  // varref if in continuous assignment driven
    மாறிலி Ast_முனை* m_procWritep = nullptr;  // varref if written in process
    தருக்க m_underGen = பொய்;  // Under a generate
    தருக்க m_ftaskDriven = பொய்;  // Last driven by function or task

    மாறிலி Ast_முனை_FTask_Ref* m_callNodep = nullptr;  // Call node if driven via writeSummary

    எண்ணிக்கை : uint8_t {
        FLAG_USED = 0,  // Signal or bit has been read/observed
        FLAG_DRIVEN = 1,  // Signal or bit has been written/driven
        FLAG_DRIVEN_ALWCOMB = 2,  // Whole signal has been driven from always_comb
        FLAG_DRIVEN_ALWFF = 3,  // Whole signal has been driven from always_ff
        FLAG_DRIVEN_ALWPLAIN = 4,  // Whole signal has been driven from a plain always
        FLAG_DRIVEN_CLOCKING = 5,  // Whole signal has been driven as a clocking block output
        FLAGS_PER_BIT = 6  // Number of flags stored for each tracked bit
    };

பொது:
    // CONSTRUCTORS
    explicit UndrivenVarEntry(AstVar* varp)
        : m_varp{varp} {  // Construction for when a var is used
        UINFO(9, "create " << varp);
        m_wholeFlags.resize(FLAGS_PER_BIT);
        க்கு (முழு i = 0; i < FLAGS_PER_BIT; i++) m_wholeFlags[i] = பொய்;
        m_bitFlags.resize(varp->width() * FLAGS_PER_BIT);
        க்கு (முழு i = 0; i < varp->width() * FLAGS_PER_BIT; i++) m_bitFlags[i] = பொய்;
    }
    ~UndrivenVarEntry() = இயல்பு;

தனிப்பட்ட:
    // METHODS
    தருக்க bit_இலக்கம்_Ok(முழு bit) மாறிலி {
        திரும்பு bit >= 0 && (bit * FLAGS_PER_BIT < static_cast<முழு>(m_bitFlags.size()));
    }
    தருக்க usedFlag(முழு bit) மாறிலி {
        திரும்பு m_wholeFlags[FLAG_USED] || m_bitFlags[bit * FLAGS_PER_BIT + FLAG_USED];
    }
    தருக்க drivenFlag(முழு bit) மாறிலி {
        திரும்பு m_wholeFlags[FLAG_DRIVEN] || m_bitFlags[bit * FLAGS_PER_BIT + FLAG_DRIVEN];
    }
    முழு bit_மொத்தம்() மாறிலி { திரும்பு m_bitFlags.size() / FLAGS_PER_BIT; }
    வெறுமை recordUsedNotDriven(மாறிலி Ast_முனை* nodep) {
        எனில் (!m_usedNotDrivenp) m_usedNotDrivenp = nodep;
    }
    எண்ணிக்கை BitNamesWhich : uint8_t { BN_UNUSED, BN_UNDRIVEN, BN_BOTH };
    string bitNames(BitNamesWhich which) {
        string bits;
        தருக்க prev = பொய்;
        முழு msb = 0;
        // bit==-1 loops below; we do one extra iteration so end with prev=false
        க்கு (முழு bit = (m_bitFlags.size() / FLAGS_PER_BIT) - 1; bit >= -1; --bit) {
            எனில் (bit >= 0
                && ((which == BN_UNUSED && !usedFlag(bit) && drivenFlag(bit))
                    || (which == BN_UNDRIVEN && usedFlag(bit) && !drivenFlag(bit))
                    || (which == BN_BOTH && !usedFlag(bit) && !drivenFlag(bit)))) {
                எனில் (!prev) {
                    prev = உண்மை;
                    msb = bit;
                }
            } இல்லையெனில் எனில் (prev) {
                மாறிலி AstBasicDType* மாறிலி bdtypep = m_varp->basicp();
                மாறிலி முழு lsb = bit + 1;
                எனில் (bits != "") bits += ",";
                எனில் (lsb == msb) {
                    bits += cvtToStr(lsb + bdtypep->lo());
                } இல்லையெனில் {
                    எனில் (bdtypep->ascending()) {
                        bits
                            += cvtToStr(lsb + bdtypep->lo()) + ":" + cvtToStr(msb + bdtypep->lo());
                    } இல்லையெனில் {
                        bits
                            += cvtToStr(msb + bdtypep->lo()) + ":" + cvtToStr(lsb + bdtypep->lo());
                    }
                }
                prev = பொய்;
            }
        }
        திரும்பு "[" + bits + "]";
    }

பொது:
    வெறுமை usedWhole(மாறிலி Ast_முனை* nodep) {
        UINFO(9, "set u[*] " << m_varp->name() << " " << nodep);
        எனில் (!m_wholeFlags[FLAG_DRIVEN]) {
            க்கு (முழு bit = 0; bit < bit_மொத்தம்(); ++bit) {
                எனில் (!drivenFlag(bit)) {
                    recordUsedNotDriven(nodep);
                    நிறுத்து;
                }
            }
        }
        m_wholeFlags[FLAG_USED] = உண்மை;
    }
    வெறுமை drivenWhole(மாறிலி Ast_முனை* nodep) {
        UINFO(9, "set d[*] " << m_varp->name() << " " << nodep);
        m_wholeFlags[FLAG_DRIVEN] = உண்மை;
    }
    வெறுமை drivenWhole(மாறிலி Ast_முனை_Var_Ref* nodep, தருக்க ftaskDef) {
        மாறிலி தருக்க wasDriven = isDrivenWhole();
        drivenWhole(nodep);
        // A write in a task/function definition is not an executed process, so it must not
        // displace an already recorded write, nor clear the flag that keeps such a write
        // from being reported as the other write of a MULTIDRIVEN
        எனில் (ftaskDef && wasDriven) திரும்பு;
        m_ftaskDriven = ftaskDef;
        m_nodep = nodep;
    }
    வெறுமை drivenAlwaysCombWhole(மாறிலி AstAlways* alwCombp) {
        m_wholeFlags[FLAG_DRIVEN_ALWCOMB] = உண்மை;
        m_alwCombp = alwCombp;
    }
    வெறுமை drivenAlwaysFFWhole(மாறிலி AstAlways* alwFFp, மாறிலி AstVar* varp) {
        m_wholeFlags[FLAG_DRIVEN_ALWFF] = உண்மை;
        m_alwFFp = alwFFp;
    }
    வெறுமை drivenAlwaysPlainWhole(மாறிலி AstAlways* alwPlainp) {
        m_wholeFlags[FLAG_DRIVEN_ALWPLAIN] = உண்மை;
        m_alwPlainp = alwPlainp;
    }
    வெறுமை drivenClockingWhole(மாறிலி AstClocking* clockingp) {
        m_wholeFlags[FLAG_DRIVEN_CLOCKING] = உண்மை;
        m_clockingp = clockingp;
    }

    மாறிலி Ast_முனை* initStaticp() மாறிலி { திரும்பு m_initStaticp; }
    வெறுமை initStaticp(மாறிலி Ast_முனை* nodep) { m_initStaticp = nodep; }
    மாறிலி Ast_முனை* initialp() மாறிலி { திரும்பு m_initialp; }
    வெறுமை initialp(மாறிலி Ast_முனை* nodep) { m_initialp = nodep; }
    மாறிலி Ast_முனை* contAssignp() மாறிலி { திரும்பு m_contAssignp; }
    வெறுமை contAssignp(மாறிலி Ast_முனை* nodep) { m_contAssignp = nodep; }
    மாறிலி Ast_முனை* procWritep() மாறிலி { திரும்பு m_procWritep; }
    வெறுமை procWritep(மாறிலி Ast_முனை* nodep) { m_procWritep = nodep; }
    வெறுமை underGenerate() { m_underGen = உண்மை; }
    தருக்க isUnderGen() மாறிலி { திரும்பு m_underGen; }
    தருக்க isDrivenWhole() மாறிலி { திரும்பு m_wholeFlags[FLAG_DRIVEN]; }
    தருக்க isDrivenAlwaysCombWhole() மாறிலி { திரும்பு m_wholeFlags[FLAG_DRIVEN_ALWCOMB]; }
    தருக்க isDrivenAlwaysFFWhole() மாறிலி { திரும்பு m_wholeFlags[FLAG_DRIVEN_ALWFF]; }
    தருக்க isDrivenAlwaysPlainWhole() மாறிலி { திரும்பு m_wholeFlags[FLAG_DRIVEN_ALWPLAIN]; }
    தருக்க isDrivenClockingWhole() மாறிலி { திரும்பு m_wholeFlags[FLAG_DRIVEN_CLOCKING]; }
    தருக்க isFtaskDriven() மாறிலி { திரும்பு m_ftaskDriven; }
    மாறிலி Ast_முனை_Var_Ref* getNodep() மாறிலி { திரும்பு m_nodep; }
    மாறிலி AstAlways* getAlwCombp() மாறிலி { திரும்பு m_alwCombp; }
    மாறிலி AstAlways* getAlwFFp() மாறிலி { திரும்பு m_alwFFp; }
    மாறிலி AstAlways* getAlwPlainp() மாறிலி { திரும்பு m_alwPlainp; }
    மாறிலி AstClocking* getClockingp() மாறிலி { திரும்பு m_clockingp; }
    வெறுமை usedBit(முழு bit, முழு width, மாறிலி Ast_முனை* nodep) {
        UINFO(9, "set u[" << (bit + width - 1) << ":" << bit << "] " << m_varp->name());
        க்கு (முழு i = 0; i < width; i++) {
            எனில் (bit_இலக்கம்_Ok(bit + i)) {
                எனில் (!drivenFlag(bit + i)) recordUsedNotDriven(nodep);
                m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_USED] = உண்மை;
            }
        }
    }
    வெறுமை drivenBit(முழு bit, முழு width) {
        UINFO(9, "set d[" << (bit + width - 1) << ":" << bit << "] " << m_varp->name());
        க்கு (முழு i = 0; i < width; i++) {
            எனில் (bit_இலக்கம்_Ok(bit + i)) m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_DRIVEN] = உண்மை;
        }
    }
    தருக்க isUsedNotDrivenBit(முழு bit, முழு width) மாறிலி {
        க்கு (முழு i = 0; i < width; i++) {
            எனில் (bit_இலக்கம்_Ok(bit + i)
                && (m_wholeFlags[FLAG_USED] || m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_USED])
                && !(m_wholeFlags[FLAG_DRIVEN]
                     || m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_DRIVEN]))
                திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }
    தருக்க isUsedNotDrivenAny() மாறிலி {
        திரும்பு isUsedNotDrivenBit(0, m_bitFlags.size() / FLAGS_PER_BIT);
    }
    மாறிலி Ast_முனை* firstUsedNotDrivenp() மாறிலி { திரும்பு m_usedNotDrivenp; }
    நிலையான தருக்க unusedMatch(AstVar* nodep) {
        மாறிலி string regexp = v3Global.opt.unusedRegexp();
        எனில் (regexp == "") திரும்பு பொய்;
        மாறிலி string prettyName = nodep->prettyName();
        திரும்பு VString::wildmatch(prettyName.c_str(), regexp.c_str());
    }
    வெறுமை reportViolations() {
        // Combine bits into overall state
        AstVar* மாறிலி nodep = m_varp;

        எனில் (initStaticp() && procWritep() && nodep->hasUserInit() && !nodep->isClassMember()
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
        மாறிலி Ast_முனை* மாறிலி initp = nodep->hasUserInit() ? initStaticp() : initialp();
        எனில் (initp && contAssignp() && !nodep->isClassMember() && !nodep->isFuncLocal()) {
            initp->v3warn(E_CONTASSINIT, "Continuous assignment to variable with initial value: "
                                             << nodep->prettyNameQ() << '\n'
                                             << initp->warnMore()
                                             << "... Location of variable initialization\n"
                                             << initp->warnContextPrimary() << '\n'
                                             << contAssignp()->warnOther()
                                             << "... Location of continuous assignment\n"
                                             << contAssignp()->warnContextSecondary());
        }
        எனில் (nodep->isGenVar()) {  // Genvar
            எனில் (!nodep->isIfaceRef() && !nodep->isUsedParam() && !unusedMatch(nodep)) {
                nodep->v3warn(UNUSEDGENVAR, "Genvar is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDGENVAR,
                                                 உண்மை);  // Warn only once
            }
        } இல்லையெனில் எனில் (nodep->isParam()) {  // Parameter
            எனில் (!nodep->isIfaceRef() && !nodep->isUsedParam() && !unusedMatch(nodep)) {
                nodep->v3warn(UNUSEDPARAM, "Parameter is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDPARAM,
                                                 உண்மை);  // Warn only once
            }
        } இல்லையெனில் {  // Signal
            மாறிலி string varType{nodep->isFuncLocal() ? "Function variable" : "Signal"};
            மாறிலி தருக்க funcInout = nodep->isFuncLocal() && nodep->isInout();
            தருக்க allU = உண்மை;
            தருக்க allD = உண்மை;
            தருக்க anyU = m_wholeFlags[FLAG_USED];
            தருக்க anyD = m_wholeFlags[FLAG_DRIVEN];
            தருக்க anyUnotD = பொய்;
            தருக்க anyDnotU = பொய்;
            தருக்க anynotDU = பொய்;
            க்கு (unsigned bit = 0; bit < m_bitFlags.size() / FLAGS_PER_BIT; bit++) {
                மாறிலி தருக்க used = usedFlag(bit);
                மாறிலி தருக்க driv = drivenFlag(bit);
                allU &= used;
                anyU |= used;
                allD &= driv;
                anyD |= driv;
                anyUnotD |= used && !driv;
                anyDnotU |= !used && driv;
                anynotDU |= !used && !driv;
            }
            எனில் (funcInout) {
                எனில் (anyD) allU = உண்மை;
                allD = உண்மை;
            }
            எனில் (allU) m_wholeFlags[FLAG_USED] = உண்மை;
            எனில் (allD) m_wholeFlags[FLAG_DRIVEN] = உண்மை;
            // Test results
            எனில் (nodep->isIfaceRef()) {
                // For interface top level we don't do any tracking
                // Ideally we'd report unused instance cells, but presumably a signal inside one
                // would get reported as unused
            } இல்லையெனில் எனில் (allU && allD) {
                // It's fine
            } இல்லையெனில் எனில் (!anyD && !anyU) {
                // UNDRIVEN is considered more serious - as is more likely a bug,
                // thus undriven+unused bits get UNUSED warnings, as they're not as buggy.
                எனில் (!unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not driven, nor used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL,
                                                     உண்மை);  // Warn only once
                }
            } இல்லையெனில் எனில் (allD && !anyU) {
                எனில் (!unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL,
                                                     உண்மை);  // Warn only once
                }
            } இல்லையெனில் எனில் (!anyD && allU) {
                nodep->v3warn(UNDRIVEN, varType << " is not driven: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNDRIVEN, உண்மை);  // Warn only once
            } இல்லையெனில் எனில் (!funcInout) {
                // Bits have different dispositions
                மாறிலி std::string varTypeLower = [&varType]() {
                    std::string str = varType;
                    str[0] = std::tolower(static_cast<unsigned எழுத்து>(str[0]));
                    திரும்பு str;
                }();
                தருக்க setU = பொய்;
                தருக்க setD = பொய்;
                எனில் (anynotDU && !unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  "Bits of " << varTypeLower << " are not driven, nor used: "
                                             << nodep->prettyNameQ() << bitNames(BN_BOTH));
                    setU = உண்மை;
                }
                எனில் (anyDnotU && !unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL, "Bits of " << varTypeLower << " are not used: "
                                                           << nodep->prettyNameQ()
                                                           << bitNames(BN_UNUSED));
                    setU = உண்மை;
                }
                எனில் (anyUnotD) {
                    nodep->v3warn(UNDRIVEN, "Bits of " << varTypeLower << " are not driven: "
                                                       << nodep->prettyNameQ()
                                                       << bitNames(BN_UNDRIVEN));
                    setD = உண்மை;
                }
                எனில் (setU) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, உண்மை);
                }
                எனில் (setD) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNDRIVEN, உண்மை);
                }
            }
        }
    }

    வெறுமை drivenViaCall(மாறிலி Ast_முனை_FTask_Ref* nodep) {
        drivenWhole(nodep);
        எனில் (!m_callNodep) m_callNodep = nodep;
    }
    மாறிலி Ast_முனை_FTask_Ref* callNodep() மாறிலி { திரும்பு m_callNodep; }
};

//######################################################################
// Undriven state, as a visitor of each AstNode

வகுப்பு UndrivenVisitor final : பொது VNVisitorConst {
    // NODE STATE
    // Netlist:
    //  AstVar::user1p          -> UndrivenVar* for usage var, 0=not set yet
    மாறிலி VNUser1InUse m_inuser1;
    // Each always:
    //  AstNode::user2p         -> UndrivenVar* for usage var, 0=not set yet
    மாறிலி VNUser2InUse m_inuser2;

    // STATE
    std::array<std::vector<UndrivenVarEntry*>, 3> m_entryps = {};  // Nodes to delete when finished
    தருக்க m_inBBox = பொய்;  // In black box; mark as driven+used
    தருக்க m_inContAssign = பொய்;  // In continuous assignment
    தருக்க m_inInitial = பொய்;  // In explicit initial block
    தருக்க m_inInitialSetup = பொய்;  // In InitialAutomatic*/InitialStatic* assignment LHS
    தருக்க m_inInitialStatic = பொய்;  // In InitialStatic
    தருக்க m_inProcAssign = பொய்;  // In procedural assignment
    தருக்க m_inFTaskRef = பொய்;  // In function or task call
    தருக்க m_inInoutOrRefPin = பொய்;  // Connected to pin that is inout
    தருக்க m_inSelLhs = பொய்;  // Iterating the fromp of an AstSel (a partial-bit write target)
    மாறிலி Ast_முனை_FTask* m_taskp = nullptr;  // Current task
    மாறிலி AstAlways* m_alwaysp = nullptr;  // Current always of either type
    மாறிலி AstAlways* m_alwaysCombp = nullptr;  // Current always if combo, otherwise nullptr
    மாறிலி AstAlways* m_alwaysFFp = nullptr;  // Current always if ff, otherwise nullptr
    மாறிலி AstAlways* m_alwaysPlainp = nullptr;  // Current always if plain (not comb/ff/latch)
    மாறிலி AstClocking* m_clockingp = nullptr;  // Current clocking block, otherwise nullptr

    V3UndrivenCapture* மாறிலி m_capturep = nullptr;  // Capture object.  'nullptr' if disabled.

    // METHODS

    UndrivenVarEntry* getEntryp(AstVar* nodep, முழு which_user) {
        எனில் (!(which_user == 1 ? nodep->user1p() : nodep->user2p())) {
            UndrivenVarEntry* மாறிலி entryp = புதிய UndrivenVarEntry{nodep};
            // UINFO(9," Associate u="<<which_user<<" "<<cvtToHex(this)<<" "<<nodep->name()<<endl);
            m_entryps[which_user].தள்ளு_back(entryp);
            எனில் (which_user == 1) {
                nodep->user1p(entryp);
            } இல்லையெனில் எனில் (which_user == 2) {
                nodep->user2p(entryp);
            } இல்லையெனில் {
                nodep->v3fatalSrc("Bad case");
            }
            திரும்பு entryp;
        } இல்லையெனில் {
            UndrivenVarEntry* மாறிலி entryp = reinterpret_cast<UndrivenVarEntry*>(
                which_user == 1 ? nodep->user1p() : nodep->user2p());
            திரும்பு entryp;
        }
    }

    வெறுமை warnAlwCombOrder(Ast_முனை_Var_Ref* nodep, மாறிலி Ast_முனை* readp) {
        AstVar* மாறிலி varp = nodep->varp();
        எனில் (!varp->isParam() && !varp->isGenVar() && !varp->isUsedLoopIdx()
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
                                            உண்மை);  // Complain just once for any usage
        }
    }
    // A clocking block 'output' is an additional driver of the signal it names.
    // The conflict is found from whichever driver is reached second.
    தருக்க clockingDrivesOther(மாறிலி UndrivenVarEntry* entryp, தருக்க otherWriteIsStaticInit) மாறிலி {
        மாறிலி தருக்க otherIsExplicitProc
            = entryp->isDrivenAlwaysCombWhole() || entryp->isDrivenAlwaysFFWhole();
        திரும்பு m_clockingp && !otherIsExplicitProc && !otherWriteIsStaticInit
               && m_clockingp != entryp->getClockingp();
    }
    தருக்க otherDrivesClocking(மாறிலி UndrivenVarEntry* entryp) மாறிலி {
        திரும்பு !m_clockingp && !m_alwaysCombp && !m_alwaysFFp && !m_inInitialStatic
               && entryp->isDrivenClockingWhole();
    }
    // A continuous assignment contending with a clocking block 'output' is an
    // unambiguous driver conflict. Multiple clocking blocks may drive the same
    // signal, the last drive wins (IEEE 1800-2023 14.16.2).
    வெறுமை warnClockingDriven(Ast_முனை_Var_Ref* nodep, மாறிலி UndrivenVarEntry* entryp,
                            மாறிலி Ast_முனை* otherWritep, தருக்க otherWriteIsStaticInit) {
        எனில் (clockingDrivesOther(entryp, otherWriteIsStaticInit)
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
        எனில் (otherDrivesClocking(entryp) && m_inContAssign) {
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
    வெறுமை warnClockingDrivenProc(Ast_முனை_Var_Ref* nodep, மாறிலி UndrivenVarEntry* entryp,
                                மாறிலி Ast_முனை* otherWritep, தருக்க otherWriteIsStaticInit) {
        எனில் (clockingDrivesOther(entryp, otherWriteIsStaticInit) && !entryp->getClockingp()
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
        எனில் (otherDrivesClocking(entryp) && !m_inContAssign) {
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
    வெறுமை visit(AstVar* nodep) override {
        மாறிலி தருக்க funcInout = nodep->isFuncLocal() && nodep->isInout();
        க்கு (முழு usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            // For assigns and non-combo always, do just usr==1, to look
            // for module-wide undriven etc.
            // For combo always, run both usr==1 for above, and also
            // usr==2 for always-only checks.
            UndrivenVarEntry* மாறிலி entryp = getEntryp(nodep, usr);
            எனில் ((nodep->isNonOutput() && !funcInout) || nodep->isSigPublic()
                || nodep->hasUserInit() || nodep->isSigUserRWPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->drivenWhole(nodep);
            }
            எனில் ((nodep->isWritable() && !funcInout) || nodep->isSigPublic()
                || nodep->isSigUserRWPublic() || nodep->isSigUserRdPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->usedWhole(nodep);
            }
            எனில் (nodep->valuep()) entryp->drivenWhole(nodep->valuep());
        }
        // Discover variables used in bit definitions, etc
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstArraySel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstSliceSel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstSel* nodep) override {
        Ast_முனை_Var_Ref* மாறிலி varrefp = VN_CAST(nodep->fromp(), முனை_Var_Ref);
        AstConst* மாறிலி constp = VN_CAST(nodep->lsbp(), Const);
        எனில் (varrefp && constp && !constp->இலக்கம்().isFourState()) {
            க்கு (முழு usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                UndrivenVarEntry* மாறிலி entryp = getEntryp(varrefp->varp(), usr);
                மாறிலி முழு lsb = constp->toUInt();
                எனில் (m_inBBox || varrefp->access().isWriteOrRW()) {
                    // Don't warn if already driven earlier as "a=0; if(a) a=1;" is fine.
                    எனில் (usr == 2 && m_alwaysCombp
                        && entryp->isUsedNotDrivenBit(lsb, nodep->width())) {
                        UINFO(9, " Select.  Entryp=" << cvtToHex(entryp));
                        warnAlwCombOrder(varrefp, entryp->firstUsedNotDrivenp());
                    }
                    entryp->drivenBit(lsb, nodep->width());
                }
                எனில் (m_inBBox || !varrefp->access().isWriteOrRW())
                    entryp->usedBit(lsb, nodep->width(), varrefp);
            }
        } இல்லையெனில் {
            // skip over static longest static prefix
            iterateConst(nodep->lsbp());
            VL_RESTORER(m_inSelLhs);
            m_inSelLhs = !V3Width::selectNonConstantRecurse(nodep->lsbp(), /*inSel=*/உண்மை);
            iterateConst(nodep->fromp());
        }
    }
    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        // Any variable
        எனில் (nodep->access().isWriteOrRW()
            && !VN_IS(nodep, VarXRef)) {  // Ignore interface variables and similar ugly items
            எனில் (m_inProcAssign && !nodep->varp()->varType().isProcAssignable()
                && !nodep->varp()->isDeclTyped()  //
                && !nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()) {
                nodep->v3warn(PROCASSWIRE, "Procedural assignment to wire, perhaps intended var"
                                               << " (IEEE 1800-2023 6.5): "
                                               << nodep->prettyNameQ());
            } இல்லையெனில் எனில் (m_inContAssign && !nodep->varp()->varType().isContAssignable()
                       && !nodep->fileline()->language().systemVerilog()) {
                nodep->v3warn(CONTASSREG,
                              "Continuous assignment to reg, perhaps intended wire"
                                  << " (IEEE 1364-2005 6.1; Verilog only, legal in SV): "
                                  << nodep->prettyNameQ());
            }
            எனில் (m_inFTaskRef && nodep->varp()->varType().isNet()) {
                nodep->v3warn(
                    PROCASSWIRE,
                    "Passed wire on output or inout subroutine argument, expected expression that "
                    "is valid on the left hand side of a procedural assignment"
                        << " (IEEE 1800-2023 13.5): " << nodep->prettyNameQ());
            }
        }

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Remember that anything driven here doesn't count toward MULTIDRIVEN.
        தருக்க ftaskDef = பொய்;
        எனில் (m_taskp && !m_alwaysp && !m_inContAssign && !m_inInitialStatic && !m_inBBox
            && !m_taskp->dpiExport()) {
            AstVar* மாறிலி retVarp = VN_CAST(m_taskp->fvarp(), Var);
            எனில் (!retVarp || nodep->varp() != retVarp) ftaskDef = உண்மை;
        }

        க்கு (முழு usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            UndrivenVarEntry* மாறிலி entryp = getEntryp(nodep->varp(), usr);
            மாறிலி தருக்க fdrv = nodep->access().isWriteOrRW()
                              && nodep->varp()->attrFileDescr();  // FD's are also being read from
            எனில் (m_inBBox || nodep->access().isWriteOrRW()) {
                எனில் (usr == 2 && m_alwaysCombp && entryp->isUsedNotDrivenAny()) {
                    UINFO(9, " Full bus.  Entryp=" << cvtToHex(entryp));
                    warnAlwCombOrder(nodep, entryp->firstUsedNotDrivenp());
                }
                மாறிலி Ast_முனை_Var_Ref* மாறிலி otherVarRefp = entryp->getNodep();
                மாறிலி Ast_முனை* மாறிலி otherWritep = otherVarRefp
                                                       ? static_cast<மாறிலி Ast_முனை*>(otherVarRefp)
                                                       : entryp->callNodep();
                மாறிலி தருக்க same_File_வரி
                    = otherVarRefp && nodep->fileline() == otherVarRefp->fileline();
                // Preconditions shared by MULTIDRIVEN and MULTIDRIVENPROC.
                மாறிலி தருக்க multidrivenCommon
                    = entryp->isDrivenWhole() && !m_inBBox && !VN_IS(nodep, VarXRef)
                      && !VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType) && !same_File_வரி
                      && !entryp->isUnderGen() && otherWritep && !entryp->isFtaskDriven()
                      && !ftaskDef && !m_inSelLhs;
                // The two warnings are gated independently on the variable
                // declaration's fileline, as v3warn suppression will check
                // the driving fileline and still warn even if the warning
                // was suppressed with lint_off at the declaration.
                மாறிலி தருக்க otherWriteIsStaticInit
                    = nodep->varp()->hasUserInit() && otherWritep == entryp->initStaticp();
                எனில் (multidrivenCommon
                    && !nodep->varp()->fileline()->warnIsOff(V3ErrorCode::MULTIDRIVEN)) {
                    எனில் (m_alwaysCombp
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
                    எனில் (!m_alwaysCombp && entryp->isDrivenAlwaysCombWhole()) {
                        nodep->v3warn(MULTIDRIVEN, "Variable also written to in always_comb"
                                                       << " (IEEE 1800-2023 9.2.2.2): "
                                                       << nodep->prettyNameQ() << '\n'
                                                       << nodep->warnOther() << '\n'
                                                       << nodep->warnContextPrimary() << '\n'
                                                       << otherWritep->warnOther()
                                                       << "... Location of always_comb write\n"
                                                       << otherWritep->warnContextSecondary());
                    }
                    எனில் (m_alwaysFFp && !otherWriteIsStaticInit
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
                    எனில் (!m_alwaysFFp && !m_inInitialStatic && entryp->isDrivenAlwaysFFWhole()) {
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
                எனில் (multidrivenCommon && !nodep->varp()->isUsedLoopIdx()
                    && !nodep->varp()->fileline()->warnIsOff(V3ErrorCode::MULTIDRIVENPROC)) {
                    // Two plain always blocks driving the whole signal: legal
                    // SystemVerilog, but a driver conflict for synthesis. The
                    // always_ff/always_comb cases above already cover mixes with
                    // an explicit process, so only warn for plain+plain here.
                    // When the two blocks are clocked differently, the
                    // (on-by-default) MULTIDRIVEN check in V3Delayed already reports
                    // the conflict, so don't also emit MULTIDRIVENPROC for that case.
                    மாறிலி AstAlways* மாறிலி otherAlwaysp = entryp->getAlwPlainp();
                    மாறிலி Ast_Sen_மரம்* மாறிலி senp
                        = m_alwaysPlainp ? m_alwaysPlainp->sentreep() : nullptr;
                    மாறிலி Ast_Sen_மரம்* மாறிலி otherSenp
                        = otherAlwaysp ? otherAlwaysp->sentreep() : nullptr;
                    மாறிலி தருக்க differentClocking = senp && otherSenp && senp->hasClocked()
                                                   && otherSenp->hasClocked()
                                                   && !senp->same_மரம்(otherSenp);
                    எனில் (m_alwaysPlainp && entryp->isDrivenAlwaysPlainWhole()
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
                எனில் (!m_inInitialSetup || nodep->varp()->hasUserInit()) {
                    // Else don't count default initialization as a driver to a net/variable
                    entryp->drivenWhole(nodep, ftaskDef);
                }
                எனில் (m_alwaysCombp && entryp->isDrivenAlwaysCombWhole()
                    && m_alwaysCombp != entryp->getAlwCombp()
                    && m_alwaysCombp->fileline() == entryp->getAlwCombp()->fileline())
                    entryp->underGenerate();
                எனில் (m_alwaysCombp) entryp->drivenAlwaysCombWhole(m_alwaysCombp);
                எனில் (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, nodep->varp());
                எனில் (m_alwaysPlainp) entryp->drivenAlwaysPlainWhole(m_alwaysPlainp);
                எனில் (m_clockingp) entryp->drivenClockingWhole(m_clockingp);
            }
            எனில் (nodep->access().isWriteOrRW() && !VN_IS(nodep, VarXRef)) {
                // Ignoring xrefs as the initial and assignment to track might refer to two
                // different instances.  Ideally all of V3Undriven would move after V3Scope,
                // then could use VarScope tracking instead.
                எனில் (m_inInitialStatic && !entryp->initStaticp()) entryp->initStaticp(nodep);
                எனில் (m_inInitial && !entryp->initialp()) entryp->initialp(nodep);
                எனில் (m_inContAssign && !entryp->contAssignp()) entryp->contAssignp(nodep);
                எனில் (m_alwaysp && m_inProcAssign && !entryp->procWritep())
                    entryp->procWritep(nodep);
            }
            எனில் ((!m_inInitialSetup || nodep->varp()->hasUserInit())
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
    வெறுமை visit(AstSysIgnore* nodep) override {
        VL_RESTORER(m_inBBox);
        m_inBBox = உண்மை;
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(AstAssign* nodep) override {
        VL_RESTORER(m_inProcAssign);
        m_inProcAssign = உண்மை;
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = பொய்;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }
    வெறுமை visit(AstAssignDly* nodep) override {
        VL_RESTORER(m_inProcAssign);
        m_inProcAssign = உண்மை;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstAssignForce* nodep) override {
        iterateConst(nodep->rhsp());
        VL_RESTORER(m_inInitial);
        m_inInitial = பொய்;
        iterateConst(nodep->lhsp());
    }
    வெறுமை visit(AstAssignW* nodep) override {
        VL_RESTORER(m_inContAssign);
        m_inContAssign = உண்மை;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstRelease* nodep) override {
        VL_RESTORER(m_inInitial);
        m_inInitial = பொய்;
        iterateConst(nodep->lhsp());
    }
    வெறுமை visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        m_inInitial = உண்மை;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstInitialAutomatic* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = உண்மை;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = உண்மை;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstInitialStatic* nodep) override {
        VL_RESTORER(m_inInitialStatic);
        m_inInitialStatic = உண்மை;
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = உண்மை;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = உண்மை;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        VL_RESTORER(m_alwaysCombp);
        VL_RESTORER(m_alwaysFFp);
        VL_RESTORER(m_alwaysPlainp);
        Ast_முனை::user2_Clear_மரம்();
        m_alwaysp = nodep;
        எனில் (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            UINFO(9, "   " << nodep);
            m_alwaysCombp = nodep;
        } இல்லையெனில் {
            m_alwaysCombp = nullptr;
        }
        m_alwaysFFp = nodep->keyword() == VAlwaysKwd::ALWAYS_FF ? nodep : nullptr;
        m_alwaysPlainp = nodep->keyword() == VAlwaysKwd::ALWAYS ? nodep : nullptr;
        iterateChildrenConst(nodep);
        எனில் (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) UINFO(9, "   Done " << nodep);
    }
    வெறுமை visit(AstClocking* nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        VL_RESTORER(m_inFTaskRef);
        m_inFTaskRef = உண்மை;

        iterateChildrenConst(nodep);

        எனில் (!m_capturep) திரும்பு;

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Do not apply writeSummary at calls inside a task definition, or they will look like
        // independent drivers (phantom MULTIDRIVEN).
        மாறிலி தருக்க inExecutedContext
            = !(m_taskp && !m_alwaysp && !m_inContAssign && !m_inInitialStatic && !m_inBBox
                && !m_taskp->dpiExport());

        எனில் (!inExecutedContext) திரும்பு;

        Ast_முனை_FTask* மாறிலி calleep = nodep->taskp();
        எனில் (!calleep) திரும்பு;

        மாறிலி auto& vars = m_capturep->writeSummary(calleep);
        க்கு (AstVar* மாறிலி varp : vars) {
            க்கு (முழு usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                UndrivenVarEntry* மாறிலி entryp = getEntryp(varp, usr);
                entryp->drivenViaCall(nodep);
                எனில் (m_alwaysCombp) entryp->drivenAlwaysCombWhole(m_alwaysCombp);
                எனில் (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, varp);
            }
        }
    }

    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        VL_RESTORER(m_taskp);
        m_taskp = nodep;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstPin* nodep) override {
        VL_RESTORER(m_inInoutOrRefPin);
        m_inInoutOrRefPin = nodep->modVarp()->isInoutOrRef();
        iterateChildrenConst(nodep);
    }

    // Until we support tables, primitives will have undriven and unused I/Os
    வெறுமை visit(AstPrimitive*) override {}

    // Coverage artifacts etc shouldn't count as a sink
    வெறுமை visit(Ast_முனை_Cover_Decl*) override {}
    வெறுமை visit(AstCoverInc*) override {}
    வெறுமை visit(AstCoverToggle*) override {}
    வெறுமை visit(AstTraceDecl* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    வெறுமை visit(AstTraceInc* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }

    // iterate
    வெறுமை visit(AstConst* nodep) override {}
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

பொது:
    // CONSTRUCTORS
    explicit UndrivenVisitor(AstNetlist* nodep, V3UndrivenCapture* capturep)
        : m_capturep{capturep} {
        iterateConst(nodep);
    }

    ~UndrivenVisitor() override {
        க்கு (UndrivenVarEntry* ip : m_entryps[1]) ip->reportViolations();
        க்கு (முழு usr = 1; usr < 3; ++usr) {
            க்கு (UndrivenVarEntry* ip : m_entryps[usr]) delete ip;
        }
    }
};

//######################################################################
// Undriven class functions

வெறுமை V3Undriven::undrivenAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    V3UndrivenCapture capture{nodep};
    { UndrivenVisitor{nodep, &capture}; }

    எனில் (v3Global.opt.stats()) V3Stats::statsStage("undriven");
}
