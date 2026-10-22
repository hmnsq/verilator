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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Undriven.h"

#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3UndrivenCapture.h"
#ଅନ୍ତର୍ଭାଵ "V3Width.h"

#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Class for every variable we may process

ଶ୍ରେଣୀ UndrivenVarEntry final {
    // MEMBERS
    AstVar* ଅଚଳ m_varp;  // Variable this tracks
    std::vector<ତାର୍କିକ> m_wholeFlags;  // Used/Driven on whole vector
    std::vector<ତାର୍କିକ> m_bitFlags;  // Used/Driven on each subbit
    ଅଚଳ Ast_ଗଣ୍ଠି* m_usedNotDrivenp = nullptr;  // First read before any write
    ଅଚଳ AstAlways* m_alwCombp
        = nullptr;  // always_comb of var if driven within always_comb, else nullptr
    ଅଚଳ AstAlways* m_alwFFp = nullptr;  // always_ff of var if driven within always_ff
    ଅଚଳ AstAlways* m_alwPlainp = nullptr;  // plain always of var if driven within plain always
    ଅଚଳ AstClocking* m_clockingp = nullptr;  // clocking block of var if driven as output
    ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* m_nodep = nullptr;  // varref if driven, else nullptr
    ଅଚଳ Ast_ଗଣ୍ଠି* m_initStaticp = nullptr;  // varref if in InitialStatic driven
    ଅଚଳ Ast_ଗଣ୍ଠି* m_initialp = nullptr;  // varref if driven in an explicit initial block
    ଅଚଳ Ast_ଗଣ୍ଠି* m_contAssignp = nullptr;  // varref if in continuous assignment driven
    ଅଚଳ Ast_ଗଣ୍ଠି* m_procWritep = nullptr;  // varref if written in process
    ତାର୍କିକ m_underGen = ମିଥ୍ୟା;  // Under a generate
    ତାର୍କିକ m_ftaskDriven = ମିଥ୍ୟା;  // Last driven by function or task

    ଅଚଳ Ast_ଗଣ୍ଠି_FTask_Ref* m_callNodep = nullptr;  // Call node if driven via writeSummary

    ଗଣନା : uint8_t {
        FLAG_USED = 0,  // Signal or bit has been read/observed
        FLAG_DRIVEN = 1,  // Signal or bit has been written/driven
        FLAG_DRIVEN_ALWCOMB = 2,  // Whole signal has been driven from always_comb
        FLAG_DRIVEN_ALWFF = 3,  // Whole signal has been driven from always_ff
        FLAG_DRIVEN_ALWPLAIN = 4,  // Whole signal has been driven from a plain always
        FLAG_DRIVEN_CLOCKING = 5,  // Whole signal has been driven as a clocking block output
        FLAGS_PER_BIT = 6  // Number of flags stored for each tracked bit
    };

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit UndrivenVarEntry(AstVar* varp)
        : m_varp{varp} {  // Construction for when a var is used
        UINFO(9, "create " << varp);
        m_wholeFlags.resize(FLAGS_PER_BIT);
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < FLAGS_PER_BIT; i++) m_wholeFlags[i] = ମିଥ୍ୟା;
        m_bitFlags.resize(varp->width() * FLAGS_PER_BIT);
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < varp->width() * FLAGS_PER_BIT; i++) m_bitFlags[i] = ମିଥ୍ୟା;
    }
    ~UndrivenVarEntry() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ବ୍ୟକ୍ତିଗତ:
    // METHODS
    ତାର୍କିକ bit_ଅଙ୍କ_Ok(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit) ଅଚଳ {
        ଫେରନ୍ତୁ bit >= 0 && (bit * FLAGS_PER_BIT < static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_bitFlags.size()));
    }
    ତାର୍କିକ usedFlag(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit) ଅଚଳ {
        ଫେରନ୍ତୁ m_wholeFlags[FLAG_USED] || m_bitFlags[bit * FLAGS_PER_BIT + FLAG_USED];
    }
    ତାର୍କିକ drivenFlag(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit) ଅଚଳ {
        ଫେରନ୍ତୁ m_wholeFlags[FLAG_DRIVEN] || m_bitFlags[bit * FLAGS_PER_BIT + FLAG_DRIVEN];
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit_ଗଣନା() ଅଚଳ { ଫେରନ୍ତୁ m_bitFlags.size() / FLAGS_PER_BIT; }
    ଶୂନ୍ୟ recordUsedNotDriven(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (!m_usedNotDrivenp) m_usedNotDrivenp = nodep;
    }
    ଗଣନା BitNamesWhich : uint8_t { BN_UNUSED, BN_UNDRIVEN, BN_BOTH };
    string bitNames(BitNamesWhich which) {
        string bits;
        ତାର୍କିକ prev = ମିଥ୍ୟା;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb = 0;
        // bit==-1 loops below; we do one extra iteration so end with prev=false
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = (m_bitFlags.size() / FLAGS_PER_BIT) - 1; bit >= -1; --bit) {
            ଯଦି (bit >= 0
                && ((which == BN_UNUSED && !usedFlag(bit) && drivenFlag(bit))
                    || (which == BN_UNDRIVEN && usedFlag(bit) && !drivenFlag(bit))
                    || (which == BN_BOTH && !usedFlag(bit) && !drivenFlag(bit)))) {
                ଯଦି (!prev) {
                    prev = ସତ୍ୟ;
                    msb = bit;
                }
            } ଅନ୍ୟଥା ଯଦି (prev) {
                ଅଚଳ AstBasicDType* ଅଚଳ bdtypep = m_varp->basicp();
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = bit + 1;
                ଯଦି (bits != "") bits += ",";
                ଯଦି (lsb == msb) {
                    bits += cvtToStr(lsb + bdtypep->lo());
                } ଅନ୍ୟଥା {
                    ଯଦି (bdtypep->ascending()) {
                        bits
                            += cvtToStr(lsb + bdtypep->lo()) + ":" + cvtToStr(msb + bdtypep->lo());
                    } ଅନ୍ୟଥା {
                        bits
                            += cvtToStr(msb + bdtypep->lo()) + ":" + cvtToStr(lsb + bdtypep->lo());
                    }
                }
                prev = ମିଥ୍ୟା;
            }
        }
        ଫେରନ୍ତୁ "[" + bits + "]";
    }

ସର୍ବସାଧାରଣ:
    ଶୂନ୍ୟ usedWhole(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        UINFO(9, "set u[*] " << m_varp->name() << " " << nodep);
        ଯଦି (!m_wholeFlags[FLAG_DRIVEN]) {
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < bit_ଗଣନା(); ++bit) {
                ଯଦି (!drivenFlag(bit)) {
                    recordUsedNotDriven(nodep);
                    ଭାଙ୍ଗ;
                }
            }
        }
        m_wholeFlags[FLAG_USED] = ସତ୍ୟ;
    }
    ଶୂନ୍ୟ drivenWhole(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        UINFO(9, "set d[*] " << m_varp->name() << " " << nodep);
        m_wholeFlags[FLAG_DRIVEN] = ସତ୍ୟ;
    }
    ଶୂନ୍ୟ drivenWhole(ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* nodep, ତାର୍କିକ ftaskDef) {
        ଅଚଳ ତାର୍କିକ wasDriven = isDrivenWhole();
        drivenWhole(nodep);
        // A write in a task/function definition is not an executed process, so it must not
        // displace an already recorded write, nor clear the flag that keeps such a write
        // from being reported as the other write of a MULTIDRIVEN
        ଯଦି (ftaskDef && wasDriven) ଫେରନ୍ତୁ;
        m_ftaskDriven = ftaskDef;
        m_nodep = nodep;
    }
    ଶୂନ୍ୟ drivenAlwaysCombWhole(ଅଚଳ AstAlways* alwCombp) {
        m_wholeFlags[FLAG_DRIVEN_ALWCOMB] = ସତ୍ୟ;
        m_alwCombp = alwCombp;
    }
    ଶୂନ୍ୟ drivenAlwaysFFWhole(ଅଚଳ AstAlways* alwFFp, ଅଚଳ AstVar* varp) {
        m_wholeFlags[FLAG_DRIVEN_ALWFF] = ସତ୍ୟ;
        m_alwFFp = alwFFp;
    }
    ଶୂନ୍ୟ drivenAlwaysPlainWhole(ଅଚଳ AstAlways* alwPlainp) {
        m_wholeFlags[FLAG_DRIVEN_ALWPLAIN] = ସତ୍ୟ;
        m_alwPlainp = alwPlainp;
    }
    ଶୂନ୍ୟ drivenClockingWhole(ଅଚଳ AstClocking* clockingp) {
        m_wholeFlags[FLAG_DRIVEN_CLOCKING] = ସତ୍ୟ;
        m_clockingp = clockingp;
    }

    ଅଚଳ Ast_ଗଣ୍ଠି* initStaticp() ଅଚଳ { ଫେରନ୍ତୁ m_initStaticp; }
    ଶୂନ୍ୟ initStaticp(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) { m_initStaticp = nodep; }
    ଅଚଳ Ast_ଗଣ୍ଠି* initialp() ଅଚଳ { ଫେରନ୍ତୁ m_initialp; }
    ଶୂନ୍ୟ initialp(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) { m_initialp = nodep; }
    ଅଚଳ Ast_ଗଣ୍ଠି* contAssignp() ଅଚଳ { ଫେରନ୍ତୁ m_contAssignp; }
    ଶୂନ୍ୟ contAssignp(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) { m_contAssignp = nodep; }
    ଅଚଳ Ast_ଗଣ୍ଠି* procWritep() ଅଚଳ { ଫେରନ୍ତୁ m_procWritep; }
    ଶୂନ୍ୟ procWritep(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) { m_procWritep = nodep; }
    ଶୂନ୍ୟ underGenerate() { m_underGen = ସତ୍ୟ; }
    ତାର୍କିକ isUnderGen() ଅଚଳ { ଫେରନ୍ତୁ m_underGen; }
    ତାର୍କିକ isDrivenWhole() ଅଚଳ { ଫେରନ୍ତୁ m_wholeFlags[FLAG_DRIVEN]; }
    ତାର୍କିକ isDrivenAlwaysCombWhole() ଅଚଳ { ଫେରନ୍ତୁ m_wholeFlags[FLAG_DRIVEN_ALWCOMB]; }
    ତାର୍କିକ isDrivenAlwaysFFWhole() ଅଚଳ { ଫେରନ୍ତୁ m_wholeFlags[FLAG_DRIVEN_ALWFF]; }
    ତାର୍କିକ isDrivenAlwaysPlainWhole() ଅଚଳ { ଫେରନ୍ତୁ m_wholeFlags[FLAG_DRIVEN_ALWPLAIN]; }
    ତାର୍କିକ isDrivenClockingWhole() ଅଚଳ { ଫେରନ୍ତୁ m_wholeFlags[FLAG_DRIVEN_CLOCKING]; }
    ତାର୍କିକ isFtaskDriven() ଅଚଳ { ଫେରନ୍ତୁ m_ftaskDriven; }
    ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* getNodep() ଅଚଳ { ଫେରନ୍ତୁ m_nodep; }
    ଅଚଳ AstAlways* getAlwCombp() ଅଚଳ { ଫେରନ୍ତୁ m_alwCombp; }
    ଅଚଳ AstAlways* getAlwFFp() ଅଚଳ { ଫେରନ୍ତୁ m_alwFFp; }
    ଅଚଳ AstAlways* getAlwPlainp() ଅଚଳ { ଫେରନ୍ତୁ m_alwPlainp; }
    ଅଚଳ AstClocking* getClockingp() ଅଚଳ { ଫେରନ୍ତୁ m_clockingp; }
    ଶୂନ୍ୟ usedBit(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        UINFO(9, "set u[" << (bit + width - 1) << ":" << bit << "] " << m_varp->name());
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < width; i++) {
            ଯଦି (bit_ଅଙ୍କ_Ok(bit + i)) {
                ଯଦି (!drivenFlag(bit + i)) recordUsedNotDriven(nodep);
                m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_USED] = ସତ୍ୟ;
            }
        }
    }
    ଶୂନ୍ୟ drivenBit(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width) {
        UINFO(9, "set d[" << (bit + width - 1) << ":" << bit << "] " << m_varp->name());
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < width; i++) {
            ଯଦି (bit_ଅଙ୍କ_Ok(bit + i)) m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_DRIVEN] = ସତ୍ୟ;
        }
    }
    ତାର୍କିକ isUsedNotDrivenBit(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width) ଅଚଳ {
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < width; i++) {
            ଯଦି (bit_ଅଙ୍କ_Ok(bit + i)
                && (m_wholeFlags[FLAG_USED] || m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_USED])
                && !(m_wholeFlags[FLAG_DRIVEN]
                     || m_bitFlags[(bit + i) * FLAGS_PER_BIT + FLAG_DRIVEN]))
                ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ତାର୍କିକ isUsedNotDrivenAny() ଅଚଳ {
        ଫେରନ୍ତୁ isUsedNotDrivenBit(0, m_bitFlags.size() / FLAGS_PER_BIT);
    }
    ଅଚଳ Ast_ଗଣ୍ଠି* firstUsedNotDrivenp() ଅଚଳ { ଫେରନ୍ତୁ m_usedNotDrivenp; }
    ସ୍ଥିର ତାର୍କିକ unusedMatch(AstVar* nodep) {
        ଅଚଳ string regexp = v3Global.opt.unusedRegexp();
        ଯଦି (regexp == "") ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ string prettyName = nodep->prettyName();
        ଫେରନ୍ତୁ VString::wildmatch(prettyName.c_str(), regexp.c_str());
    }
    ଶୂନ୍ୟ reportViolations() {
        // Combine bits into overall state
        AstVar* ଅଚଳ nodep = m_varp;

        ଯଦି (initStaticp() && procWritep() && nodep->hasUserInit() && !nodep->isClassMember()
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
        ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ initp = nodep->hasUserInit() ? initStaticp() : initialp();
        ଯଦି (initp && contAssignp() && !nodep->isClassMember() && !nodep->isFuncLocal()) {
            initp->v3warn(E_CONTASSINIT, "Continuous assignment to variable with initial value: "
                                             << nodep->prettyNameQ() << '\n'
                                             << initp->warnMore()
                                             << "... Location of variable initialization\n"
                                             << initp->warnContextPrimary() << '\n'
                                             << contAssignp()->warnOther()
                                             << "... Location of continuous assignment\n"
                                             << contAssignp()->warnContextSecondary());
        }
        ଯଦି (nodep->isGenVar()) {  // Genvar
            ଯଦି (!nodep->isIfaceRef() && !nodep->isUsedParam() && !unusedMatch(nodep)) {
                nodep->v3warn(UNUSEDGENVAR, "Genvar is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDGENVAR,
                                                 ସତ୍ୟ);  // Warn only once
            }
        } ଅନ୍ୟଥା ଯଦି (nodep->isParam()) {  // Parameter
            ଯଦି (!nodep->isIfaceRef() && !nodep->isUsedParam() && !unusedMatch(nodep)) {
                nodep->v3warn(UNUSEDPARAM, "Parameter is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDPARAM,
                                                 ସତ୍ୟ);  // Warn only once
            }
        } ଅନ୍ୟଥା {  // Signal
            ଅଚଳ string varType{nodep->isFuncLocal() ? "Function variable" : "Signal"};
            ଅଚଳ ତାର୍କିକ funcInout = nodep->isFuncLocal() && nodep->isInout();
            ତାର୍କିକ allU = ସତ୍ୟ;
            ତାର୍କିକ allD = ସତ୍ୟ;
            ତାର୍କିକ anyU = m_wholeFlags[FLAG_USED];
            ତାର୍କିକ anyD = m_wholeFlags[FLAG_DRIVEN];
            ତାର୍କିକ anyUnotD = ମିଥ୍ୟା;
            ତାର୍କିକ anyDnotU = ମିଥ୍ୟା;
            ତାର୍କିକ anynotDU = ମିଥ୍ୟା;
            ପାଇଁ (unsigned bit = 0; bit < m_bitFlags.size() / FLAGS_PER_BIT; bit++) {
                ଅଚଳ ତାର୍କିକ used = usedFlag(bit);
                ଅଚଳ ତାର୍କିକ driv = drivenFlag(bit);
                allU &= used;
                anyU |= used;
                allD &= driv;
                anyD |= driv;
                anyUnotD |= used && !driv;
                anyDnotU |= !used && driv;
                anynotDU |= !used && !driv;
            }
            ଯଦି (funcInout) {
                ଯଦି (anyD) allU = ସତ୍ୟ;
                allD = ସତ୍ୟ;
            }
            ଯଦି (allU) m_wholeFlags[FLAG_USED] = ସତ୍ୟ;
            ଯଦି (allD) m_wholeFlags[FLAG_DRIVEN] = ସତ୍ୟ;
            // Test results
            ଯଦି (nodep->isIfaceRef()) {
                // For interface top level we don't do any tracking
                // Ideally we'd report unused instance cells, but presumably a signal inside one
                // would get reported as unused
            } ଅନ୍ୟଥା ଯଦି (allU && allD) {
                // It's fine
            } ଅନ୍ୟଥା ଯଦି (!anyD && !anyU) {
                // UNDRIVEN is considered more serious - as is more likely a bug,
                // thus undriven+unused bits get UNUSED warnings, as they're not as buggy.
                ଯଦି (!unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not driven, nor used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL,
                                                     ସତ୍ୟ);  // Warn only once
                }
            } ଅନ୍ୟଥା ଯଦି (allD && !anyU) {
                ଯଦି (!unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL,
                                                     ସତ୍ୟ);  // Warn only once
                }
            } ଅନ୍ୟଥା ଯଦି (!anyD && allU) {
                nodep->v3warn(UNDRIVEN, varType << " is not driven: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3ErrorCode::UNDRIVEN, ସତ୍ୟ);  // Warn only once
            } ଅନ୍ୟଥା ଯଦି (!funcInout) {
                // Bits have different dispositions
                ଅଚଳ std::string varTypeLower = [&varType]() {
                    std::string str = varType;
                    str[0] = std::tolower(static_cast<unsigned ଅକ୍ଷର>(str[0]));
                    ଫେରନ୍ତୁ str;
                }();
                ତାର୍କିକ setU = ମିଥ୍ୟା;
                ତାର୍କିକ setD = ମିଥ୍ୟା;
                ଯଦି (anynotDU && !unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  "Bits of " << varTypeLower << " are not driven, nor used: "
                                             << nodep->prettyNameQ() << bitNames(BN_BOTH));
                    setU = ସତ୍ୟ;
                }
                ଯଦି (anyDnotU && !unusedMatch(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL, "Bits of " << varTypeLower << " are not used: "
                                                           << nodep->prettyNameQ()
                                                           << bitNames(BN_UNUSED));
                    setU = ସତ୍ୟ;
                }
                ଯଦି (anyUnotD) {
                    nodep->v3warn(UNDRIVEN, "Bits of " << varTypeLower << " are not driven: "
                                                       << nodep->prettyNameQ()
                                                       << bitNames(BN_UNDRIVEN));
                    setD = ସତ୍ୟ;
                }
                ଯଦି (setU) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, ସତ୍ୟ);
                }
                ଯଦି (setD) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3ErrorCode::UNDRIVEN, ସତ୍ୟ);
                }
            }
        }
    }

    ଶୂନ୍ୟ drivenViaCall(ଅଚଳ Ast_ଗଣ୍ଠି_FTask_Ref* nodep) {
        drivenWhole(nodep);
        ଯଦି (!m_callNodep) m_callNodep = nodep;
    }
    ଅଚଳ Ast_ଗଣ୍ଠି_FTask_Ref* callNodep() ଅଚଳ { ଫେରନ୍ତୁ m_callNodep; }
};

//######################################################################
// Undriven state, as a visitor of each AstNode

ଶ୍ରେଣୀ UndrivenVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // NODE STATE
    // Netlist:
    //  AstVar::user1p          -> UndrivenVar* for usage var, 0=not set yet
    ଅଚଳ VNUser1InUse m_inuser1;
    // Each always:
    //  AstNode::user2p         -> UndrivenVar* for usage var, 0=not set yet
    ଅଚଳ VNUser2InUse m_inuser2;

    // STATE
    std::array<std::vector<UndrivenVarEntry*>, 3> m_entryps = {};  // Nodes to delete when finished
    ତାର୍କିକ m_inBBox = ମିଥ୍ୟା;  // In black box; mark as driven+used
    ତାର୍କିକ m_inContAssign = ମିଥ୍ୟା;  // In continuous assignment
    ତାର୍କିକ m_inInitial = ମିଥ୍ୟା;  // In explicit initial block
    ତାର୍କିକ m_inInitialSetup = ମିଥ୍ୟା;  // In InitialAutomatic*/InitialStatic* assignment LHS
    ତାର୍କିକ m_inInitialStatic = ମିଥ୍ୟା;  // In InitialStatic
    ତାର୍କିକ m_inProcAssign = ମିଥ୍ୟା;  // In procedural assignment
    ତାର୍କିକ m_inFTaskRef = ମିଥ୍ୟା;  // In function or task call
    ତାର୍କିକ m_inInoutOrRefPin = ମିଥ୍ୟା;  // Connected to pin that is inout
    ତାର୍କିକ m_inSelLhs = ମିଥ୍ୟା;  // Iterating the fromp of an AstSel (a partial-bit write target)
    ଅଚଳ Ast_ଗଣ୍ଠି_FTask* m_taskp = nullptr;  // Current task
    ଅଚଳ AstAlways* m_alwaysp = nullptr;  // Current always of either type
    ଅଚଳ AstAlways* m_alwaysCombp = nullptr;  // Current always if combo, otherwise nullptr
    ଅଚଳ AstAlways* m_alwaysFFp = nullptr;  // Current always if ff, otherwise nullptr
    ଅଚଳ AstAlways* m_alwaysPlainp = nullptr;  // Current always if plain (not comb/ff/latch)
    ଅଚଳ AstClocking* m_clockingp = nullptr;  // Current clocking block, otherwise nullptr

    V3UndrivenCapture* ଅଚଳ m_capturep = nullptr;  // Capture object.  'nullptr' if disabled.

    // METHODS

    UndrivenVarEntry* getEntryp(AstVar* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା which_user) {
        ଯଦି (!(which_user == 1 ? nodep->user1p() : nodep->user2p())) {
            UndrivenVarEntry* ଅଚଳ entryp = ନୂତନ UndrivenVarEntry{nodep};
            // UINFO(9," Associate u="<<which_user<<" "<<cvtToHex(this)<<" "<<nodep->name()<<endl);
            m_entryps[which_user].ଠେଲ_back(entryp);
            ଯଦି (which_user == 1) {
                nodep->user1p(entryp);
            } ଅନ୍ୟଥା ଯଦି (which_user == 2) {
                nodep->user2p(entryp);
            } ଅନ୍ୟଥା {
                nodep->v3fatalSrc("Bad case");
            }
            ଫେରନ୍ତୁ entryp;
        } ଅନ୍ୟଥା {
            UndrivenVarEntry* ଅଚଳ entryp = reinterpret_cast<UndrivenVarEntry*>(
                which_user == 1 ? nodep->user1p() : nodep->user2p());
            ଫେରନ୍ତୁ entryp;
        }
    }

    ଶୂନ୍ୟ warnAlwCombOrder(Ast_ଗଣ୍ଠି_Var_Ref* nodep, ଅଚଳ Ast_ଗଣ୍ଠି* readp) {
        AstVar* ଅଚଳ varp = nodep->varp();
        ଯଦି (!varp->isParam() && !varp->isGenVar() && !varp->isUsedLoopIdx()
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
                                            ସତ୍ୟ);  // Complain just once for any usage
        }
    }
    // A clocking block 'output' is an additional driver of the signal it names.
    // The conflict is found from whichever driver is reached second.
    ତାର୍କିକ clockingDrivesOther(ଅଚଳ UndrivenVarEntry* entryp, ତାର୍କିକ otherWriteIsStaticInit) ଅଚଳ {
        ଅଚଳ ତାର୍କିକ otherIsExplicitProc
            = entryp->isDrivenAlwaysCombWhole() || entryp->isDrivenAlwaysFFWhole();
        ଫେରନ୍ତୁ m_clockingp && !otherIsExplicitProc && !otherWriteIsStaticInit
               && m_clockingp != entryp->getClockingp();
    }
    ତାର୍କିକ otherDrivesClocking(ଅଚଳ UndrivenVarEntry* entryp) ଅଚଳ {
        ଫେରନ୍ତୁ !m_clockingp && !m_alwaysCombp && !m_alwaysFFp && !m_inInitialStatic
               && entryp->isDrivenClockingWhole();
    }
    // A continuous assignment contending with a clocking block 'output' is an
    // unambiguous driver conflict. Multiple clocking blocks may drive the same
    // signal, the last drive wins (IEEE 1800-2023 14.16.2).
    ଶୂନ୍ୟ warnClockingDriven(Ast_ଗଣ୍ଠି_Var_Ref* nodep, ଅଚଳ UndrivenVarEntry* entryp,
                            ଅଚଳ Ast_ଗଣ୍ଠି* otherWritep, ତାର୍କିକ otherWriteIsStaticInit) {
        ଯଦି (clockingDrivesOther(entryp, otherWriteIsStaticInit)
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
        ଯଦି (otherDrivesClocking(entryp) && m_inContAssign) {
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
    ଶୂନ୍ୟ warnClockingDrivenProc(Ast_ଗଣ୍ଠି_Var_Ref* nodep, ଅଚଳ UndrivenVarEntry* entryp,
                                ଅଚଳ Ast_ଗଣ୍ଠି* otherWritep, ତାର୍କିକ otherWriteIsStaticInit) {
        ଯଦି (clockingDrivesOther(entryp, otherWriteIsStaticInit) && !entryp->getClockingp()
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
        ଯଦି (otherDrivesClocking(entryp) && !m_inContAssign) {
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
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        ଅଚଳ ତାର୍କିକ funcInout = nodep->isFuncLocal() && nodep->isInout();
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            // For assigns and non-combo always, do just usr==1, to look
            // for module-wide undriven etc.
            // For combo always, run both usr==1 for above, and also
            // usr==2 for always-only checks.
            UndrivenVarEntry* ଅଚଳ entryp = getEntryp(nodep, usr);
            ଯଦି ((nodep->isNonOutput() && !funcInout) || nodep->isSigPublic()
                || nodep->hasUserInit() || nodep->isSigUserRWPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->drivenWhole(nodep);
            }
            ଯଦି ((nodep->isWritable() && !funcInout) || nodep->isSigPublic()
                || nodep->isSigUserRWPublic() || nodep->isSigUserRdPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->usedWhole(nodep);
            }
            ଯଦି (nodep->valuep()) entryp->drivenWhole(nodep->valuep());
        }
        // Discover variables used in bit definitions, etc
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstArraySel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstSliceSel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstSel* nodep) override {
        Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ varrefp = VN_CAST(nodep->fromp(), ଗଣ୍ଠି_Var_Ref);
        AstConst* ଅଚଳ constp = VN_CAST(nodep->lsbp(), Const);
        ଯଦି (varrefp && constp && !constp->ଅଙ୍କ().isFourState()) {
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                UndrivenVarEntry* ଅଚଳ entryp = getEntryp(varrefp->varp(), usr);
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = constp->toUInt();
                ଯଦି (m_inBBox || varrefp->access().isWriteOrRW()) {
                    // Don't warn if already driven earlier as "a=0; if(a) a=1;" is fine.
                    ଯଦି (usr == 2 && m_alwaysCombp
                        && entryp->isUsedNotDrivenBit(lsb, nodep->width())) {
                        UINFO(9, " Select.  Entryp=" << cvtToHex(entryp));
                        warnAlwCombOrder(varrefp, entryp->firstUsedNotDrivenp());
                    }
                    entryp->drivenBit(lsb, nodep->width());
                }
                ଯଦି (m_inBBox || !varrefp->access().isWriteOrRW())
                    entryp->usedBit(lsb, nodep->width(), varrefp);
            }
        } ଅନ୍ୟଥା {
            // skip over static longest static prefix
            iterateConst(nodep->lsbp());
            VL_RESTORER(m_inSelLhs);
            m_inSelLhs = !V3Width::selectNonConstantRecurse(nodep->lsbp(), /*inSel=*/ସତ୍ୟ);
            iterateConst(nodep->fromp());
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        // Any variable
        ଯଦି (nodep->access().isWriteOrRW()
            && !VN_IS(nodep, VarXRef)) {  // Ignore interface variables and similar ugly items
            ଯଦି (m_inProcAssign && !nodep->varp()->varType().isProcAssignable()
                && !nodep->varp()->isDeclTyped()  //
                && !nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()) {
                nodep->v3warn(PROCASSWIRE, "Procedural assignment to wire, perhaps intended var"
                                               << " (IEEE 1800-2023 6.5): "
                                               << nodep->prettyNameQ());
            } ଅନ୍ୟଥା ଯଦି (m_inContAssign && !nodep->varp()->varType().isContAssignable()
                       && !nodep->fileline()->language().systemVerilog()) {
                nodep->v3warn(CONTASSREG,
                              "Continuous assignment to reg, perhaps intended wire"
                                  << " (IEEE 1364-2005 6.1; Verilog only, legal in SV): "
                                  << nodep->prettyNameQ());
            }
            ଯଦି (m_inFTaskRef && nodep->varp()->varType().isNet()) {
                nodep->v3warn(
                    PROCASSWIRE,
                    "Passed wire on output or inout subroutine argument, expected expression that "
                    "is valid on the left hand side of a procedural assignment"
                        << " (IEEE 1800-2023 13.5): " << nodep->prettyNameQ());
            }
        }

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Remember that anything driven here doesn't count toward MULTIDRIVEN.
        ତାର୍କିକ ftaskDef = ମିଥ୍ୟା;
        ଯଦି (m_taskp && !m_alwaysp && !m_inContAssign && !m_inInitialStatic && !m_inBBox
            && !m_taskp->dpiExport()) {
            AstVar* ଅଚଳ retVarp = VN_CAST(m_taskp->fvarp(), Var);
            ଯଦି (!retVarp || nodep->varp() != retVarp) ftaskDef = ସତ୍ୟ;
        }

        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            UndrivenVarEntry* ଅଚଳ entryp = getEntryp(nodep->varp(), usr);
            ଅଚଳ ତାର୍କିକ fdrv = nodep->access().isWriteOrRW()
                              && nodep->varp()->attrFileDescr();  // FD's are also being read from
            ଯଦି (m_inBBox || nodep->access().isWriteOrRW()) {
                ଯଦି (usr == 2 && m_alwaysCombp && entryp->isUsedNotDrivenAny()) {
                    UINFO(9, " Full bus.  Entryp=" << cvtToHex(entryp));
                    warnAlwCombOrder(nodep, entryp->firstUsedNotDrivenp());
                }
                ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ otherVarRefp = entryp->getNodep();
                ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ otherWritep = otherVarRefp
                                                       ? static_cast<ଅଚଳ Ast_ଗଣ୍ଠି*>(otherVarRefp)
                                                       : entryp->callNodep();
                ଅଚଳ ତାର୍କିକ same_File_ଧାଡ଼ି
                    = otherVarRefp && nodep->fileline() == otherVarRefp->fileline();
                // Preconditions shared by MULTIDRIVEN and MULTIDRIVENPROC.
                ଅଚଳ ତାର୍କିକ multidrivenCommon
                    = entryp->isDrivenWhole() && !m_inBBox && !VN_IS(nodep, VarXRef)
                      && !VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType) && !same_File_ଧାଡ଼ି
                      && !entryp->isUnderGen() && otherWritep && !entryp->isFtaskDriven()
                      && !ftaskDef && !m_inSelLhs;
                // The two warnings are gated independently on the variable
                // declaration's fileline, as v3warn suppression will check
                // the driving fileline and still warn even if the warning
                // was suppressed with lint_off at the declaration.
                ଅଚଳ ତାର୍କିକ otherWriteIsStaticInit
                    = nodep->varp()->hasUserInit() && otherWritep == entryp->initStaticp();
                ଯଦି (multidrivenCommon
                    && !nodep->varp()->fileline()->warnIsOff(V3ErrorCode::MULTIDRIVEN)) {
                    ଯଦି (m_alwaysCombp
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
                    ଯଦି (!m_alwaysCombp && entryp->isDrivenAlwaysCombWhole()) {
                        nodep->v3warn(MULTIDRIVEN, "Variable also written to in always_comb"
                                                       << " (IEEE 1800-2023 9.2.2.2): "
                                                       << nodep->prettyNameQ() << '\n'
                                                       << nodep->warnOther() << '\n'
                                                       << nodep->warnContextPrimary() << '\n'
                                                       << otherWritep->warnOther()
                                                       << "... Location of always_comb write\n"
                                                       << otherWritep->warnContextSecondary());
                    }
                    ଯଦି (m_alwaysFFp && !otherWriteIsStaticInit
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
                    ଯଦି (!m_alwaysFFp && !m_inInitialStatic && entryp->isDrivenAlwaysFFWhole()) {
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
                ଯଦି (multidrivenCommon && !nodep->varp()->isUsedLoopIdx()
                    && !nodep->varp()->fileline()->warnIsOff(V3ErrorCode::MULTIDRIVENPROC)) {
                    // Two plain always blocks driving the whole signal: legal
                    // SystemVerilog, but a driver conflict for synthesis. The
                    // always_ff/always_comb cases above already cover mixes with
                    // an explicit process, so only warn for plain+plain here.
                    // When the two blocks are clocked differently, the
                    // (on-by-default) MULTIDRIVEN check in V3Delayed already reports
                    // the conflict, so don't also emit MULTIDRIVENPROC for that case.
                    ଅଚଳ AstAlways* ଅଚଳ otherAlwaysp = entryp->getAlwPlainp();
                    ଅଚଳ Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senp
                        = m_alwaysPlainp ? m_alwaysPlainp->sentreep() : nullptr;
                    ଅଚଳ Ast_Sen_ବୃକ୍ଷ* ଅଚଳ otherSenp
                        = otherAlwaysp ? otherAlwaysp->sentreep() : nullptr;
                    ଅଚଳ ତାର୍କିକ differentClocking = senp && otherSenp && senp->hasClocked()
                                                   && otherSenp->hasClocked()
                                                   && !senp->same_ବୃକ୍ଷ(otherSenp);
                    ଯଦି (m_alwaysPlainp && entryp->isDrivenAlwaysPlainWhole()
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
                ଯଦି (!m_inInitialSetup || nodep->varp()->hasUserInit()) {
                    // Else don't count default initialization as a driver to a net/variable
                    entryp->drivenWhole(nodep, ftaskDef);
                }
                ଯଦି (m_alwaysCombp && entryp->isDrivenAlwaysCombWhole()
                    && m_alwaysCombp != entryp->getAlwCombp()
                    && m_alwaysCombp->fileline() == entryp->getAlwCombp()->fileline())
                    entryp->underGenerate();
                ଯଦି (m_alwaysCombp) entryp->drivenAlwaysCombWhole(m_alwaysCombp);
                ଯଦି (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, nodep->varp());
                ଯଦି (m_alwaysPlainp) entryp->drivenAlwaysPlainWhole(m_alwaysPlainp);
                ଯଦି (m_clockingp) entryp->drivenClockingWhole(m_clockingp);
            }
            ଯଦି (nodep->access().isWriteOrRW() && !VN_IS(nodep, VarXRef)) {
                // Ignoring xrefs as the initial and assignment to track might refer to two
                // different instances.  Ideally all of V3Undriven would move after V3Scope,
                // then could use VarScope tracking instead.
                ଯଦି (m_inInitialStatic && !entryp->initStaticp()) entryp->initStaticp(nodep);
                ଯଦି (m_inInitial && !entryp->initialp()) entryp->initialp(nodep);
                ଯଦି (m_inContAssign && !entryp->contAssignp()) entryp->contAssignp(nodep);
                ଯଦି (m_alwaysp && m_inProcAssign && !entryp->procWritep())
                    entryp->procWritep(nodep);
            }
            ଯଦି ((!m_inInitialSetup || nodep->varp()->hasUserInit())
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
    ଶୂନ୍ୟ visit(AstSysIgnore* nodep) override {
        VL_RESTORER(m_inBBox);
        m_inBBox = ସତ୍ୟ;
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(AstAssign* nodep) override {
        VL_RESTORER(m_inProcAssign);
        m_inProcAssign = ସତ୍ୟ;
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = ମିଥ୍ୟା;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }
    ଶୂନ୍ୟ visit(AstAssignDly* nodep) override {
        VL_RESTORER(m_inProcAssign);
        m_inProcAssign = ସତ୍ୟ;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstAssignForce* nodep) override {
        iterateConst(nodep->rhsp());
        VL_RESTORER(m_inInitial);
        m_inInitial = ମିଥ୍ୟା;
        iterateConst(nodep->lhsp());
    }
    ଶୂନ୍ୟ visit(AstAssignW* nodep) override {
        VL_RESTORER(m_inContAssign);
        m_inContAssign = ସତ୍ୟ;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstRelease* nodep) override {
        VL_RESTORER(m_inInitial);
        m_inInitial = ମିଥ୍ୟା;
        iterateConst(nodep->lhsp());
    }
    ଶୂନ୍ୟ visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        m_inInitial = ସତ୍ୟ;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstInitialAutomatic* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = ସତ୍ୟ;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = ସତ୍ୟ;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstInitialStatic* nodep) override {
        VL_RESTORER(m_inInitialStatic);
        m_inInitialStatic = ସତ୍ୟ;
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = ସତ୍ୟ;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = ସତ୍ୟ;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        VL_RESTORER(m_alwaysCombp);
        VL_RESTORER(m_alwaysFFp);
        VL_RESTORER(m_alwaysPlainp);
        Ast_ଗଣ୍ଠି::user2_Clear_ବୃକ୍ଷ();
        m_alwaysp = nodep;
        ଯଦି (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            UINFO(9, "   " << nodep);
            m_alwaysCombp = nodep;
        } ଅନ୍ୟଥା {
            m_alwaysCombp = nullptr;
        }
        m_alwaysFFp = nodep->keyword() == VAlwaysKwd::ALWAYS_FF ? nodep : nullptr;
        m_alwaysPlainp = nodep->keyword() == VAlwaysKwd::ALWAYS ? nodep : nullptr;
        iterateChildrenConst(nodep);
        ଯଦି (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) UINFO(9, "   Done " << nodep);
    }
    ଶୂନ୍ୟ visit(AstClocking* nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        VL_RESTORER(m_inFTaskRef);
        m_inFTaskRef = ସତ୍ୟ;

        iterateChildrenConst(nodep);

        ଯଦି (!m_capturep) ଫେରନ୍ତୁ;

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Do not apply writeSummary at calls inside a task definition, or they will look like
        // independent drivers (phantom MULTIDRIVEN).
        ଅଚଳ ତାର୍କିକ inExecutedContext
            = !(m_taskp && !m_alwaysp && !m_inContAssign && !m_inInitialStatic && !m_inBBox
                && !m_taskp->dpiExport());

        ଯଦି (!inExecutedContext) ଫେରନ୍ତୁ;

        Ast_ଗଣ୍ଠି_FTask* ଅଚଳ calleep = nodep->taskp();
        ଯଦି (!calleep) ଫେରନ୍ତୁ;

        ଅଚଳ auto& vars = m_capturep->writeSummary(calleep);
        ପାଇଁ (AstVar* ଅଚଳ varp : vars) {
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                UndrivenVarEntry* ଅଚଳ entryp = getEntryp(varp, usr);
                entryp->drivenViaCall(nodep);
                ଯଦି (m_alwaysCombp) entryp->drivenAlwaysCombWhole(m_alwaysCombp);
                ଯଦି (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, varp);
            }
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        VL_RESTORER(m_taskp);
        m_taskp = nodep;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstPin* nodep) override {
        VL_RESTORER(m_inInoutOrRefPin);
        m_inInoutOrRefPin = nodep->modVarp()->isInoutOrRef();
        iterateChildrenConst(nodep);
    }

    // Until we support tables, primitives will have undriven and unused I/Os
    ଶୂନ୍ୟ visit(AstPrimitive*) override {}

    // Coverage artifacts etc shouldn't count as a sink
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Cover_Decl*) override {}
    ଶୂନ୍ୟ visit(AstCoverInc*) override {}
    ଶୂନ୍ୟ visit(AstCoverToggle*) override {}
    ଶୂନ୍ୟ visit(AstTraceDecl* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    ଶୂନ୍ୟ visit(AstTraceInc* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }

    // iterate
    ଶୂନ୍ୟ visit(AstConst* nodep) override {}
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit UndrivenVisitor(AstNetlist* nodep, V3UndrivenCapture* capturep)
        : m_capturep{capturep} {
        iterateConst(nodep);
    }

    ~UndrivenVisitor() override {
        ପାଇଁ (UndrivenVarEntry* ip : m_entryps[1]) ip->reportViolations();
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା usr = 1; usr < 3; ++usr) {
            ପାଇଁ (UndrivenVarEntry* ip : m_entryps[usr]) delete ip;
        }
    }
};

//######################################################################
// Undriven class functions

ଶୂନ୍ୟ V3Undriven::undrivenAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    V3UndrivenCapture capture{nodep};
    { UndrivenVisitor{nodep, &capture}; }

    ଯଦି (v3Global.opt.stats()) V3Stats::statsStage("undriven");
}
