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

वर्ग Undriven_Var_प्रविष्टिः final {
    // MEMBERS
    AstVar* ध्रुव m_varp;  // Variable this tracks
    std::सदिशम्<तार्किक> m_सम्पूर्णम्_Flags;  // Used/Driven on whole vector
    std::सदिशम्<तार्किक> m_bitFlags;  // Used/Driven on each subbit
    ध्रुव Ast_ग्रन्थिः* m_प्रयुक्तम्_Not_Drivenp = शून्यसूचकः;  // First read before any write
    ध्रुव AstAlways* m_alwCombp
        = शून्यसूचकः;  // always_comb of var if driven within always_comb, else nullptr
    ध्रुव AstAlways* m_alwFFp = शून्यसूचकः;  // always_ff of var if driven within always_ff
    ध्रुव AstAlways* m_alwPlainp = शून्यसूचकः;  // plain always of var if driven within plain always
    ध्रुव AstClocking* m_clockingp = शून्यसूचकः;  // clocking block of var if driven as output
    ध्रुव Ast_ग्रन्थिः_Var_Ref* m_nodep = शून्यसूचकः;  // varref if driven, else nullptr
    ध्रुव Ast_ग्रन्थिः* m_initStaticp = शून्यसूचकः;  // varref if in InitialStatic driven
    ध्रुव Ast_ग्रन्थिः* m_initialp = शून्यसूचकः;  // varref if driven in an explicit initial block
    ध्रुव Ast_ग्रन्थिः* m_contAssignp = शून्यसूचकः;  // varref if in continuous assignment driven
    ध्रुव Ast_ग्रन्थिः* m_procWritep = शून्यसूचकः;  // varref if written in process
    तार्किक m_underGen = असत्य;  // Under a generate
    तार्किक m_ftaskDriven = असत्य;  // Last driven by function or task

    ध्रुव Ast_ग्रन्थिः_FTask_Ref* m_callNodep = शून्यसूचकः;  // Call node if driven via writeSummary

    गणना : uint8_t {
        FLAG_प्रयुक्तम् = 0,  // Signal or bit has been read/observed
        FLAG_DRIVEN = 1,  // Signal or bit has been written/driven
        FLAG_DRIVEN_ALWCOMB = 2,  // Whole signal has been driven from always_comb
        FLAG_DRIVEN_ALWFF = 3,  // Whole signal has been driven from always_ff
        FLAG_DRIVEN_ALWPLAIN = 4,  // Whole signal has been driven from a plain always
        FLAG_DRIVEN_CLOCKING = 5,  // Whole signal has been driven as a clocking block output
        FLAGS_प्रत्येकम्_BIT = 6  // Number of flags stored for each tracked bit
    };

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट Undriven_Var_प्रविष्टिः(AstVar* varp)
        : m_varp{varp} {  // Construction for when a var is used
        UINFO(9, "create " << varp);
        m_सम्पूर्णम्_Flags.resize(FLAGS_प्रत्येकम्_BIT);
        प्रति (पूर्णाङ्क i = 0; i < FLAGS_प्रत्येकम्_BIT; i++) m_सम्पूर्णम्_Flags[i] = असत्य;
        m_bitFlags.resize(varp->विस्तारः() * FLAGS_प्रत्येकम्_BIT);
        प्रति (पूर्णाङ्क i = 0; i < varp->विस्तारः() * FLAGS_प्रत्येकम्_BIT; i++) m_bitFlags[i] = असत्य;
    }
    ~Undriven_Var_प्रविष्टिः() = स्वभाव;

गुप्त:
    // METHODS
    तार्किक bit_अङ्कः_Ok(पूर्णाङ्क bit) ध्रुव {
        प्रत्यागम bit >= 0 && (bit * FLAGS_प्रत्येकम्_BIT < static_cast<पूर्णाङ्क>(m_bitFlags.आकार()));
    }
    तार्किक प्रयुक्तम्_Flag(पूर्णाङ्क bit) ध्रुव {
        प्रत्यागम m_सम्पूर्णम्_Flags[FLAG_प्रयुक्तम्] || m_bitFlags[bit * FLAGS_प्रत्येकम्_BIT + FLAG_प्रयुक्तम्];
    }
    तार्किक drivenFlag(पूर्णाङ्क bit) ध्रुव {
        प्रत्यागम m_सम्पूर्णम्_Flags[FLAG_DRIVEN] || m_bitFlags[bit * FLAGS_प्रत्येकम्_BIT + FLAG_DRIVEN];
    }
    पूर्णाङ्क bit_गणनम्() ध्रुव { प्रत्यागम m_bitFlags.आकार() / FLAGS_प्रत्येकम्_BIT; }
    शून्य लेखः_प्रयुक्तम्_Not_Driven(ध्रुव Ast_ग्रन्थिः* nodep) {
        यदि (!m_प्रयुक्तम्_Not_Drivenp) m_प्रयुक्तम्_Not_Drivenp = nodep;
    }
    गणना Bit_नामानि_Which : uint8_t { BN_अप्रयुक्तम्, BN_UNDRIVEN, BN_BOTH };
    string bit_नामानि(Bit_नामानि_Which which) {
        string द्वयङ्काः;
        तार्किक पूर्व_2 = असत्य;
        पूर्णाङ्क msb = 0;
        // bit==-1 loops below; we do one extra iteration so end with prev=false
        प्रति (पूर्णाङ्क bit = (m_bitFlags.आकार() / FLAGS_प्रत्येकम्_BIT) - 1; bit >= -1; --bit) {
            यदि (bit >= 0
                && ((which == BN_अप्रयुक्तम् && !प्रयुक्तम्_Flag(bit) && drivenFlag(bit))
                    || (which == BN_UNDRIVEN && प्रयुक्तम्_Flag(bit) && !drivenFlag(bit))
                    || (which == BN_BOTH && !प्रयुक्तम्_Flag(bit) && !drivenFlag(bit)))) {
                यदि (!पूर्व_2) {
                    पूर्व_2 = सत्य;
                    msb = bit;
                }
            } अन्यथा यदि (पूर्व_2) {
                ध्रुव AstBasicDType* ध्रुव bdtypep = m_varp->basicp();
                ध्रुव पूर्णाङ्क lsb = bit + 1;
                यदि (द्वयङ्काः != "") द्वयङ्काः += ",";
                यदि (lsb == msb) {
                    द्वयङ्काः += cvt_गन्तव्यम्_Str(lsb + bdtypep->lo());
                } अन्यथा {
                    यदि (bdtypep->ascending()) {
                        द्वयङ्काः
                            += cvt_गन्तव्यम्_Str(lsb + bdtypep->lo()) + ":" + cvt_गन्तव्यम्_Str(msb + bdtypep->lo());
                    } अन्यथा {
                        द्वयङ्काः
                            += cvt_गन्तव्यम्_Str(msb + bdtypep->lo()) + ":" + cvt_गन्तव्यम्_Str(lsb + bdtypep->lo());
                    }
                }
                पूर्व_2 = असत्य;
            }
        }
        प्रत्यागम "[" + द्वयङ्काः + "]";
    }

सार्वजनिक:
    शून्य प्रयुक्तम्_सम्पूर्णम्(ध्रुव Ast_ग्रन्थिः* nodep) {
        UINFO(9, "set u[*] " << m_varp->name() << " " << nodep);
        यदि (!m_सम्पूर्णम्_Flags[FLAG_DRIVEN]) {
            प्रति (पूर्णाङ्क bit = 0; bit < bit_गणनम्(); ++bit) {
                यदि (!drivenFlag(bit)) {
                    लेखः_प्रयुक्तम्_Not_Driven(nodep);
                    भङ्ग;
                }
            }
        }
        m_सम्पूर्णम्_Flags[FLAG_प्रयुक्तम्] = सत्य;
    }
    शून्य driven_सम्पूर्णम्(ध्रुव Ast_ग्रन्थिः* nodep) {
        UINFO(9, "set d[*] " << m_varp->name() << " " << nodep);
        m_सम्पूर्णम्_Flags[FLAG_DRIVEN] = सत्य;
    }
    शून्य driven_सम्पूर्णम्(ध्रुव Ast_ग्रन्थिः_Var_Ref* nodep, तार्किक ftaskDef) {
        ध्रुव तार्किक wasDriven = is_Driven_सम्पूर्णम्();
        driven_सम्पूर्णम्(nodep);
        // A write in a task/function definition is not an executed process, so it must not
        // displace an already recorded write, nor clear the flag that keeps such a write
        // from being reported as the other write of a MULTIDRIVEN
        यदि (ftaskDef && wasDriven) प्रत्यागम;
        m_ftaskDriven = ftaskDef;
        m_nodep = nodep;
    }
    शून्य driven_Always_Comb_सम्पूर्णम्(ध्रुव AstAlways* alwCombp) {
        m_सम्पूर्णम्_Flags[FLAG_DRIVEN_ALWCOMB] = सत्य;
        m_alwCombp = alwCombp;
    }
    शून्य drivenAlwaysFFWhole(ध्रुव AstAlways* alwFFp, ध्रुव AstVar* varp) {
        m_सम्पूर्णम्_Flags[FLAG_DRIVEN_ALWFF] = सत्य;
        m_alwFFp = alwFFp;
    }
    शून्य driven_Always_Plain_सम्पूर्णम्(ध्रुव AstAlways* alwPlainp) {
        m_सम्पूर्णम्_Flags[FLAG_DRIVEN_ALWPLAIN] = सत्य;
        m_alwPlainp = alwPlainp;
    }
    शून्य driven_Clocking_सम्पूर्णम्(ध्रुव AstClocking* clockingp) {
        m_सम्पूर्णम्_Flags[FLAG_DRIVEN_CLOCKING] = सत्य;
        m_clockingp = clockingp;
    }

    ध्रुव Ast_ग्रन्थिः* initStaticp() ध्रुव { प्रत्यागम m_initStaticp; }
    शून्य initStaticp(ध्रुव Ast_ग्रन्थिः* nodep) { m_initStaticp = nodep; }
    ध्रुव Ast_ग्रन्थिः* initialp() ध्रुव { प्रत्यागम m_initialp; }
    शून्य initialp(ध्रुव Ast_ग्रन्थिः* nodep) { m_initialp = nodep; }
    ध्रुव Ast_ग्रन्थिः* contAssignp() ध्रुव { प्रत्यागम m_contAssignp; }
    शून्य contAssignp(ध्रुव Ast_ग्रन्थिः* nodep) { m_contAssignp = nodep; }
    ध्रुव Ast_ग्रन्थिः* procWritep() ध्रुव { प्रत्यागम m_procWritep; }
    शून्य procWritep(ध्रुव Ast_ग्रन्थिः* nodep) { m_procWritep = nodep; }
    शून्य underGenerate() { m_underGen = सत्य; }
    तार्किक isUnderGen() ध्रुव { प्रत्यागम m_underGen; }
    तार्किक is_Driven_सम्पूर्णम्() ध्रुव { प्रत्यागम m_सम्पूर्णम्_Flags[FLAG_DRIVEN]; }
    तार्किक is_Driven_Always_Comb_सम्पूर्णम्() ध्रुव { प्रत्यागम m_सम्पूर्णम्_Flags[FLAG_DRIVEN_ALWCOMB]; }
    तार्किक isDrivenAlwaysFFWhole() ध्रुव { प्रत्यागम m_सम्पूर्णम्_Flags[FLAG_DRIVEN_ALWFF]; }
    तार्किक is_Driven_Always_Plain_सम्पूर्णम्() ध्रुव { प्रत्यागम m_सम्पूर्णम्_Flags[FLAG_DRIVEN_ALWPLAIN]; }
    तार्किक is_Driven_Clocking_सम्पूर्णम्() ध्रुव { प्रत्यागम m_सम्पूर्णम्_Flags[FLAG_DRIVEN_CLOCKING]; }
    तार्किक isFtaskDriven() ध्रुव { प्रत्यागम m_ftaskDriven; }
    ध्रुव Ast_ग्रन्थिः_Var_Ref* getNodep() ध्रुव { प्रत्यागम m_nodep; }
    ध्रुव AstAlways* getAlwCombp() ध्रुव { प्रत्यागम m_alwCombp; }
    ध्रुव AstAlways* getAlwFFp() ध्रुव { प्रत्यागम m_alwFFp; }
    ध्रुव AstAlways* getAlwPlainp() ध्रुव { प्रत्यागम m_alwPlainp; }
    ध्रुव AstClocking* getClockingp() ध्रुव { प्रत्यागम m_clockingp; }
    शून्य प्रयुक्तम्_Bit(पूर्णाङ्क bit, पूर्णाङ्क विस्तारः, ध्रुव Ast_ग्रन्थिः* nodep) {
        UINFO(9, "set u[" << (bit + विस्तारः - 1) << ":" << bit << "] " << m_varp->name());
        प्रति (पूर्णाङ्क i = 0; i < विस्तारः; i++) {
            यदि (bit_अङ्कः_Ok(bit + i)) {
                यदि (!drivenFlag(bit + i)) लेखः_प्रयुक्तम्_Not_Driven(nodep);
                m_bitFlags[(bit + i) * FLAGS_प्रत्येकम्_BIT + FLAG_प्रयुक्तम्] = सत्य;
            }
        }
    }
    शून्य drivenBit(पूर्णाङ्क bit, पूर्णाङ्क विस्तारः) {
        UINFO(9, "set d[" << (bit + विस्तारः - 1) << ":" << bit << "] " << m_varp->name());
        प्रति (पूर्णाङ्क i = 0; i < विस्तारः; i++) {
            यदि (bit_अङ्कः_Ok(bit + i)) m_bitFlags[(bit + i) * FLAGS_प्रत्येकम्_BIT + FLAG_DRIVEN] = सत्य;
        }
    }
    तार्किक is_प्रयुक्तम्_Not_Driven_Bit(पूर्णाङ्क bit, पूर्णाङ्क विस्तारः) ध्रुव {
        प्रति (पूर्णाङ्क i = 0; i < विस्तारः; i++) {
            यदि (bit_अङ्कः_Ok(bit + i)
                && (m_सम्पूर्णम्_Flags[FLAG_प्रयुक्तम्] || m_bitFlags[(bit + i) * FLAGS_प्रत्येकम्_BIT + FLAG_प्रयुक्तम्])
                && !(m_सम्पूर्णम्_Flags[FLAG_DRIVEN]
                     || m_bitFlags[(bit + i) * FLAGS_प्रत्येकम्_BIT + FLAG_DRIVEN]))
                प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }
    तार्किक is_प्रयुक्तम्_Not_Driven_Any() ध्रुव {
        प्रत्यागम is_प्रयुक्तम्_Not_Driven_Bit(0, m_bitFlags.आकार() / FLAGS_प्रत्येकम्_BIT);
    }
    ध्रुव Ast_ग्रन्थिः* first_प्रयुक्तम्_Not_Drivenp() ध्रुव { प्रत्यागम m_प्रयुक्तम्_Not_Drivenp; }
    स्थिर तार्किक अप्रयुक्तम्_मेलनम्(AstVar* nodep) {
        ध्रुव string regexp = v3Global.opt.अप्रयुक्तम्_Regexp();
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
                    << initStaticp()->warn_अधिकम्() << "... Location of variable initialization\n"
                    << initStaticp()->warnContextPrimary() << '\n'
                    << procWritep()->warnOther() << "... Location of variable process write\n"
                    << procWritep()->warn_अधिकम्()
                    << "... Perhaps should initialize instead using a reset in this process\n"
                    << procWritep()->warnContextSecondary());
        }
        ध्रुव Ast_ग्रन्थिः* ध्रुव initp = nodep->hasUserInit() ? initStaticp() : initialp();
        यदि (initp && contAssignp() && !nodep->isClassMember() && !nodep->isFuncLocal()) {
            initp->v3warn(E_CONTASSINIT, "Continuous assignment to variable with initial value: "
                                             << nodep->prettyNameQ() << '\n'
                                             << initp->warn_अधिकम्()
                                             << "... Location of variable initialization\n"
                                             << initp->warnContextPrimary() << '\n'
                                             << contAssignp()->warnOther()
                                             << "... Location of continuous assignment\n"
                                             << contAssignp()->warnContextSecondary());
        }
        यदि (nodep->isGenVar()) {  // Genvar
            यदि (!nodep->isIfaceRef() && !nodep->is_प्रयुक्तम्_Param() && !अप्रयुक्तम्_मेलनम्(nodep)) {
                nodep->v3warn(UNUSEDGENVAR, "Genvar is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_संहिता::UNUSEDGENVAR,
                                                 सत्य);  // Warn only once
            }
        } अन्यथा यदि (nodep->isParam()) {  // Parameter
            यदि (!nodep->isIfaceRef() && !nodep->is_प्रयुक्तम्_Param() && !अप्रयुक्तम्_मेलनम्(nodep)) {
                nodep->v3warn(UNUSEDPARAM, "Parameter is not used: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_संहिता::UNUSEDPARAM,
                                                 सत्य);  // Warn only once
            }
        } अन्यथा {  // Signal
            ध्रुव string varType{nodep->isFuncLocal() ? "Function variable" : "Signal"};
            ध्रुव तार्किक funcInout = nodep->isFuncLocal() && nodep->isInout();
            तार्किक allU = सत्य;
            तार्किक allD = सत्य;
            तार्किक anyU = m_सम्पूर्णम्_Flags[FLAG_प्रयुक्तम्];
            तार्किक anyD = m_सम्पूर्णम्_Flags[FLAG_DRIVEN];
            तार्किक anyUnotD = असत्य;
            तार्किक anyDnotU = असत्य;
            तार्किक anynotDU = असत्य;
            प्रति (अचिह्नित bit = 0; bit < m_bitFlags.आकार() / FLAGS_प्रत्येकम्_BIT; bit++) {
                ध्रुव तार्किक प्रयुक्तम् = प्रयुक्तम्_Flag(bit);
                ध्रुव तार्किक driv = drivenFlag(bit);
                allU &= प्रयुक्तम्;
                anyU |= प्रयुक्तम्;
                allD &= driv;
                anyD |= driv;
                anyUnotD |= प्रयुक्तम् && !driv;
                anyDnotU |= !प्रयुक्तम् && driv;
                anynotDU |= !प्रयुक्तम् && !driv;
            }
            यदि (funcInout) {
                यदि (anyD) allU = सत्य;
                allD = सत्य;
            }
            यदि (allU) m_सम्पूर्णम्_Flags[FLAG_प्रयुक्तम्] = सत्य;
            यदि (allD) m_सम्पूर्णम्_Flags[FLAG_DRIVEN] = सत्य;
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
                यदि (!अप्रयुक्तम्_मेलनम्(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not driven, nor used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3_Error_संहिता::UNUSEDSIGNAL,
                                                     सत्य);  // Warn only once
                }
            } अन्यथा यदि (allD && !anyU) {
                यदि (!अप्रयुक्तम्_मेलनम्(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  varType << " is not used: " << nodep->prettyNameQ());
                    nodep->fileline()->modifyWarnOff(V3_Error_संहिता::UNUSEDSIGNAL,
                                                     सत्य);  // Warn only once
                }
            } अन्यथा यदि (!anyD && allU) {
                nodep->v3warn(UNDRIVEN, varType << " is not driven: " << nodep->prettyNameQ());
                nodep->fileline()->modifyWarnOff(V3_Error_संहिता::UNDRIVEN, सत्य);  // Warn only once
            } अन्यथा यदि (!funcInout) {
                // Bits have different dispositions
                ध्रुव std::string var_Type_अधस्तनम् = [&varType]() {
                    std::string str = varType;
                    str[0] = std::tolower(static_cast<अचिह्नित वर्ण>(str[0]));
                    प्रत्यागम str;
                }();
                तार्किक समुच्चयः_U = असत्य;
                तार्किक समुच्चयः_D = असत्य;
                यदि (anynotDU && !अप्रयुक्तम्_मेलनम्(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL,
                                  "Bits of " << var_Type_अधस्तनम् << " are not driven, nor used: "
                                             << nodep->prettyNameQ() << bit_नामानि(BN_BOTH));
                    समुच्चयः_U = सत्य;
                }
                यदि (anyDnotU && !अप्रयुक्तम्_मेलनम्(nodep)) {
                    nodep->v3warn(UNUSEDSIGNAL, "Bits of " << var_Type_अधस्तनम् << " are not used: "
                                                           << nodep->prettyNameQ()
                                                           << bit_नामानि(BN_अप्रयुक्तम्));
                    समुच्चयः_U = सत्य;
                }
                यदि (anyUnotD) {
                    nodep->v3warn(UNDRIVEN, "Bits of " << var_Type_अधस्तनम् << " are not driven: "
                                                       << nodep->prettyNameQ()
                                                       << bit_नामानि(BN_UNDRIVEN));
                    समुच्चयः_D = सत्य;
                }
                यदि (समुच्चयः_U) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3_Error_संहिता::UNUSEDSIGNAL, सत्य);
                }
                यदि (समुच्चयः_D) {  // Warn only once
                    nodep->fileline()->modifyWarnOff(V3_Error_संहिता::UNDRIVEN, सत्य);
                }
            }
        }
    }

    शून्य driven_द्वारा_Call(ध्रुव Ast_ग्रन्थिः_FTask_Ref* nodep) {
        driven_सम्पूर्णम्(nodep);
        यदि (!m_callNodep) m_callNodep = nodep;
    }
    ध्रुव Ast_ग्रन्थिः_FTask_Ref* callNodep() ध्रुव { प्रत्यागम m_callNodep; }
};

//######################################################################
// Undriven state, as a visitor of each AstNode

वर्ग UndrivenVisitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    // Netlist:
    //  AstVar::user1p          -> UndrivenVar* for usage var, 0=not set yet
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;
    // Each always:
    //  AstNode::user2p         -> UndrivenVar* for usage var, 0=not set yet
    ध्रुव VNUser2_अन्तर्_Use m_inuser2;

    // STATE
    std::array<std::सदिशम्<Undriven_Var_प्रविष्टिः*>, 3> m_entryps = {};  // Nodes to delete when finished
    तार्किक m_अन्तर्_BBox = असत्य;  // In black box; mark as driven+used
    तार्किक m_अन्तर्_Cont_नियोजय = असत्य;  // In continuous assignment
    तार्किक m_अन्तर्_Initial = असत्य;  // In explicit initial block
    तार्किक m_अन्तर्_Initial_सज्जता = असत्य;  // In InitialAutomatic*/InitialStatic* assignment LHS
    तार्किक m_अन्तर्_Initial_Static = असत्य;  // In InitialStatic
    तार्किक m_अन्तर्_Proc_नियोजय = असत्य;  // In procedural assignment
    तार्किक m_अन्तर्_FTask_Ref = असत्य;  // In function or task call
    तार्किक m_अन्तर्_Inout_Or_Ref_Pin = असत्य;  // Connected to pin that is inout
    तार्किक m_अन्तर्_Sel_Lhs = असत्य;  // Iterating the fromp of an AstSel (a partial-bit write target)
    ध्रुव Ast_ग्रन्थिः_FTask* m_taskp = शून्यसूचकः;  // Current task
    ध्रुव AstAlways* m_alwaysp = शून्यसूचकः;  // Current always of either type
    ध्रुव AstAlways* m_alwaysCombp = शून्यसूचकः;  // Current always if combo, otherwise nullptr
    ध्रुव AstAlways* m_alwaysFFp = शून्यसूचकः;  // Current always if ff, otherwise nullptr
    ध्रुव AstAlways* m_alwaysPlainp = शून्यसूचकः;  // Current always if plain (not comb/ff/latch)
    ध्रुव AstClocking* m_clockingp = शून्यसूचकः;  // Current clocking block, otherwise nullptr

    V3UndrivenCapture* ध्रुव m_capturep = शून्यसूचकः;  // Capture object.  'nullptr' if disabled.

    // METHODS

    Undriven_Var_प्रविष्टिः* getEntryp(AstVar* nodep, पूर्णाङ्क which_user) {
        यदि (!(which_user == 1 ? nodep->user1p() : nodep->user2p())) {
            Undriven_Var_प्रविष्टिः* ध्रुव entryp = नूतन Undriven_Var_प्रविष्टिः{nodep};
            // UINFO(9," Associate u="<<which_user<<" "<<cvtToHex(this)<<" "<<nodep->name()<<endl);
            m_entryps[which_user].प्रक्षिप_पृष्ठभागः(entryp);
            यदि (which_user == 1) {
                nodep->user1p(entryp);
            } अन्यथा यदि (which_user == 2) {
                nodep->user2p(entryp);
            } अन्यथा {
                nodep->v3fatalSrc("Bad case");
            }
            प्रत्यागम entryp;
        } अन्यथा {
            Undriven_Var_प्रविष्टिः* ध्रुव entryp = reinterpret_cast<Undriven_Var_प्रविष्टिः*>(
                which_user == 1 ? nodep->user1p() : nodep->user2p());
            प्रत्यागम entryp;
        }
    }

    शून्य warn_Alw_Comb_क्रमः(Ast_ग्रन्थिः_Var_Ref* nodep, ध्रुव Ast_ग्रन्थिः* readp) {
        AstVar* ध्रुव varp = nodep->varp();
        यदि (!varp->isParam() && !varp->isGenVar() && !varp->is_प्रयुक्तम्_Loop_Idx()
            && !varp->ignoreSchedWrite()
            && !m_अन्तर्_BBox  // We may have falsely considered a SysIgnore as a driver
            && !VN_IS(nodep, VarXRef)  // Xrefs might point at two different instances
            && !varp->fileline()->warnIsOff(
                V3_Error_संहिता::ALWCOMBORDER)) {  // Warn only once per variable
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
            varp->fileline()->modifyWarnOff(V3_Error_संहिता::ALWCOMBORDER,
                                            सत्य);  // Complain just once for any usage
        }
    }
    // A clocking block 'output' is an additional driver of the signal it names.
    // The conflict is found from whichever driver is reached second.
    तार्किक clockingDrivesOther(ध्रुव Undriven_Var_प्रविष्टिः* entryp, तार्किक otherWriteIsStaticInit) ध्रुव {
        ध्रुव तार्किक otherIsExplicitProc
            = entryp->is_Driven_Always_Comb_सम्पूर्णम्() || entryp->isDrivenAlwaysFFWhole();
        प्रत्यागम m_clockingp && !otherIsExplicitProc && !otherWriteIsStaticInit
               && m_clockingp != entryp->getClockingp();
    }
    तार्किक otherDrivesClocking(ध्रुव Undriven_Var_प्रविष्टिः* entryp) ध्रुव {
        प्रत्यागम !m_clockingp && !m_alwaysCombp && !m_alwaysFFp && !m_अन्तर्_Initial_Static
               && entryp->is_Driven_Clocking_सम्पूर्णम्();
    }
    // A continuous assignment contending with a clocking block 'output' is an
    // unambiguous driver conflict. Multiple clocking blocks may drive the same
    // signal, the last drive wins (IEEE 1800-2023 14.16.2).
    शून्य warnClockingDriven(Ast_ग्रन्थिः_Var_Ref* nodep, ध्रुव Undriven_Var_प्रविष्टिः* entryp,
                            ध्रुव Ast_ग्रन्थिः* otherWritep, तार्किक otherWriteIsStaticInit) {
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
        यदि (otherDrivesClocking(entryp) && m_अन्तर्_Cont_नियोजय) {
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
    शून्य warnClockingDrivenProc(Ast_ग्रन्थिः_Var_Ref* nodep, ध्रुव Undriven_Var_प्रविष्टिः* entryp,
                                ध्रुव Ast_ग्रन्थिः* otherWritep, तार्किक otherWriteIsStaticInit) {
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
        यदि (otherDrivesClocking(entryp) && !m_अन्तर्_Cont_नियोजय) {
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
    शून्य दर्शय(AstVar* nodep) override {
        ध्रुव तार्किक funcInout = nodep->isFuncLocal() && nodep->isInout();
        प्रति (पूर्णाङ्क usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            // For assigns and non-combo always, do just usr==1, to look
            // for module-wide undriven etc.
            // For combo always, run both usr==1 for above, and also
            // usr==2 for always-only checks.
            Undriven_Var_प्रविष्टिः* ध्रुव entryp = getEntryp(nodep, usr);
            यदि ((nodep->isNonOutput() && !funcInout) || nodep->is_Sig_लौकिकम्()
                || nodep->hasUserInit() || nodep->isSigUserRWPublic()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->driven_सम्पूर्णम्(nodep);
            }
            यदि ((nodep->isWritable() && !funcInout) || nodep->is_Sig_लौकिकम्()
                || nodep->isSigUserRWPublic() || nodep->is_Sig_User_Rd_लौकिकम्()
                || (m_taskp && (m_taskp->dpiImport() || m_taskp->dpiExport()))) {
                entryp->प्रयुक्तम्_सम्पूर्णम्(nodep);
            }
            यदि (nodep->valuep()) entryp->driven_सम्पूर्णम्(nodep->valuep());
        }
        // Discover variables used in bit definitions, etc
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstArraySel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(Ast_खण्डिका_Sel* nodep) override {
        // Arrays are rarely constant assigned, so for now we punt and do all entries
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstSel* nodep) override {
        Ast_ग्रन्थिः_Var_Ref* ध्रुव varrefp = VN_CAST(nodep->fromp(), ग्रन्थिः_Var_Ref);
        AstConst* ध्रुव constp = VN_CAST(nodep->lsbp(), Const);
        यदि (varrefp && constp && !constp->अङ्कः().isFourState()) {
            प्रति (पूर्णाङ्क usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                Undriven_Var_प्रविष्टिः* ध्रुव entryp = getEntryp(varrefp->varp(), usr);
                ध्रुव पूर्णाङ्क lsb = constp->गन्तव्यम्_UInt();
                यदि (m_अन्तर्_BBox || varrefp->access().isWriteOrRW()) {
                    // Don't warn if already driven earlier as "a=0; if(a) a=1;" is fine.
                    यदि (usr == 2 && m_alwaysCombp
                        && entryp->is_प्रयुक्तम्_Not_Driven_Bit(lsb, nodep->विस्तारः())) {
                        UINFO(9, " Select.  Entryp=" << cvt_गन्तव्यम्_Hex(entryp));
                        warn_Alw_Comb_क्रमः(varrefp, entryp->first_प्रयुक्तम्_Not_Drivenp());
                    }
                    entryp->drivenBit(lsb, nodep->विस्तारः());
                }
                यदि (m_अन्तर्_BBox || !varrefp->access().isWriteOrRW())
                    entryp->प्रयुक्तम्_Bit(lsb, nodep->विस्तारः(), varrefp);
            }
        } अन्यथा {
            // skip over static longest static prefix
            iterateConst(nodep->lsbp());
            VL_RESTORER(m_अन्तर्_Sel_Lhs);
            m_अन्तर्_Sel_Lhs = !V3_विस्तारः::selectNonConstantRecurse(nodep->lsbp(), /*inSel=*/सत्य);
            iterateConst(nodep->fromp());
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        // Any variable
        यदि (nodep->access().isWriteOrRW()
            && !VN_IS(nodep, VarXRef)) {  // Ignore interface variables and similar ugly items
            यदि (m_अन्तर्_Proc_नियोजय && !nodep->varp()->varType().isProcAssignable()
                && !nodep->varp()->isDeclTyped()  //
                && !nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()) {
                nodep->v3warn(PROCASSWIRE, "Procedural assignment to wire, perhaps intended var"
                                               << " (IEEE 1800-2023 6.5): "
                                               << nodep->prettyNameQ());
            } अन्यथा यदि (m_अन्तर्_Cont_नियोजय && !nodep->varp()->varType().isContAssignable()
                       && !nodep->fileline()->language().systemVerilog()) {
                nodep->v3warn(CONTASSREG,
                              "Continuous assignment to reg, perhaps intended wire"
                                  << " (IEEE 1364-2005 6.1; Verilog only, legal in SV): "
                                  << nodep->prettyNameQ());
            }
            यदि (m_अन्तर्_FTask_Ref && nodep->varp()->varType().isNet()) {
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
        यदि (m_taskp && !m_alwaysp && !m_अन्तर्_Cont_नियोजय && !m_अन्तर्_Initial_Static && !m_अन्तर्_BBox
            && !m_taskp->dpiExport()) {
            AstVar* ध्रुव retVarp = VN_CAST(m_taskp->fvarp(), Var);
            यदि (!retVarp || nodep->varp() != retVarp) ftaskDef = सत्य;
        }

        प्रति (पूर्णाङ्क usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
            Undriven_Var_प्रविष्टिः* ध्रुव entryp = getEntryp(nodep->varp(), usr);
            ध्रुव तार्किक fdrv = nodep->access().isWriteOrRW()
                              && nodep->varp()->attrFileDescr();  // FD's are also being read from
            यदि (m_अन्तर्_BBox || nodep->access().isWriteOrRW()) {
                यदि (usr == 2 && m_alwaysCombp && entryp->is_प्रयुक्तम्_Not_Driven_Any()) {
                    UINFO(9, " Full bus.  Entryp=" << cvt_गन्तव्यम्_Hex(entryp));
                    warn_Alw_Comb_क्रमः(nodep, entryp->first_प्रयुक्तम्_Not_Drivenp());
                }
                ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव otherVarRefp = entryp->getNodep();
                ध्रुव Ast_ग्रन्थिः* ध्रुव otherWritep = otherVarRefp
                                                       ? static_cast<ध्रुव Ast_ग्रन्थिः*>(otherVarRefp)
                                                       : entryp->callNodep();
                ध्रुव तार्किक same_File_पङ्क्तिः
                    = otherVarRefp && nodep->fileline() == otherVarRefp->fileline();
                // Preconditions shared by MULTIDRIVEN and MULTIDRIVENPROC.
                ध्रुव तार्किक multidrivenCommon
                    = entryp->is_Driven_सम्पूर्णम्() && !m_अन्तर्_BBox && !VN_IS(nodep, VarXRef)
                      && !VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType) && !same_File_पङ्क्तिः
                      && !entryp->isUnderGen() && otherWritep && !entryp->isFtaskDriven()
                      && !ftaskDef && !m_अन्तर्_Sel_Lhs;
                // The two warnings are gated independently on the variable
                // declaration's fileline, as v3warn suppression will check
                // the driving fileline and still warn even if the warning
                // was suppressed with lint_off at the declaration.
                ध्रुव तार्किक otherWriteIsStaticInit
                    = nodep->varp()->hasUserInit() && otherWritep == entryp->initStaticp();
                यदि (multidrivenCommon
                    && !nodep->varp()->fileline()->warnIsOff(V3_Error_संहिता::MULTIDRIVEN)) {
                    यदि (m_alwaysCombp
                        && (!entryp->is_Driven_Always_Comb_सम्पूर्णम्()
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
                    यदि (!m_alwaysCombp && entryp->is_Driven_Always_Comb_सम्पूर्णम्()) {
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
                    यदि (!m_alwaysFFp && !m_अन्तर्_Initial_Static && entryp->isDrivenAlwaysFFWhole()) {
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
                यदि (multidrivenCommon && !nodep->varp()->is_प्रयुक्तम्_Loop_Idx()
                    && !nodep->varp()->fileline()->warnIsOff(V3_Error_संहिता::MULTIDRIVENPROC)) {
                    // Two plain always blocks driving the whole signal: legal
                    // SystemVerilog, but a driver conflict for synthesis. The
                    // always_ff/always_comb cases above already cover mixes with
                    // an explicit process, so only warn for plain+plain here.
                    // When the two blocks are clocked differently, the
                    // (on-by-default) MULTIDRIVEN check in V3Delayed already reports
                    // the conflict, so don't also emit MULTIDRIVENPROC for that case.
                    ध्रुव AstAlways* ध्रुव otherAlwaysp = entryp->getAlwPlainp();
                    ध्रुव Ast_Sen_वृक्षः* ध्रुव senp
                        = m_alwaysPlainp ? m_alwaysPlainp->sentreep() : शून्यसूचकः;
                    ध्रुव Ast_Sen_वृक्षः* ध्रुव otherSenp
                        = otherAlwaysp ? otherAlwaysp->sentreep() : शून्यसूचकः;
                    ध्रुव तार्किक differentClocking = senp && otherSenp && senp->hasClocked()
                                                   && otherSenp->hasClocked()
                                                   && !senp->same_वृक्षः(otherSenp);
                    यदि (m_alwaysPlainp && entryp->is_Driven_Always_Plain_सम्पूर्णम्()
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
                यदि (!m_अन्तर्_Initial_सज्जता || nodep->varp()->hasUserInit()) {
                    // Else don't count default initialization as a driver to a net/variable
                    entryp->driven_सम्पूर्णम्(nodep, ftaskDef);
                }
                यदि (m_alwaysCombp && entryp->is_Driven_Always_Comb_सम्पूर्णम्()
                    && m_alwaysCombp != entryp->getAlwCombp()
                    && m_alwaysCombp->fileline() == entryp->getAlwCombp()->fileline())
                    entryp->underGenerate();
                यदि (m_alwaysCombp) entryp->driven_Always_Comb_सम्पूर्णम्(m_alwaysCombp);
                यदि (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, nodep->varp());
                यदि (m_alwaysPlainp) entryp->driven_Always_Plain_सम्पूर्णम्(m_alwaysPlainp);
                यदि (m_clockingp) entryp->driven_Clocking_सम्पूर्णम्(m_clockingp);
            }
            यदि (nodep->access().isWriteOrRW() && !VN_IS(nodep, VarXRef)) {
                // Ignoring xrefs as the initial and assignment to track might refer to two
                // different instances.  Ideally all of V3Undriven would move after V3Scope,
                // then could use VarScope tracking instead.
                यदि (m_अन्तर्_Initial_Static && !entryp->initStaticp()) entryp->initStaticp(nodep);
                यदि (m_अन्तर्_Initial && !entryp->initialp()) entryp->initialp(nodep);
                यदि (m_अन्तर्_Cont_नियोजय && !entryp->contAssignp()) entryp->contAssignp(nodep);
                यदि (m_alwaysp && m_अन्तर्_Proc_नियोजय && !entryp->procWritep())
                    entryp->procWritep(nodep);
            }
            यदि ((!m_अन्तर्_Initial_सज्जता || nodep->varp()->hasUserInit())
                && (m_अन्तर्_BBox || nodep->access().isReadOrRW()
                    || fdrv
                    // Inouts have only isWrite set, as we don't have more
                    // information and operating on module boundary, treat as
                    // both read and writing
                    || m_अन्तर्_Inout_Or_Ref_Pin))
                entryp->प्रयुक्तम्_सम्पूर्णम्(nodep);
        }
    }

    // Don't know what black boxed calls do, assume in+out
    शून्य दर्शय(AstSysIgnore* nodep) override {
        VL_RESTORER(m_अन्तर्_BBox);
        m_अन्तर्_BBox = सत्य;
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(Ast_नियोजय* nodep) override {
        VL_RESTORER(m_अन्तर्_Proc_नियोजय);
        m_अन्तर्_Proc_नियोजय = सत्य;
        {
            VL_RESTORER(m_अन्तर्_Initial_सज्जता);
            m_अन्तर्_Initial_सज्जता = असत्य;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }
    शून्य दर्शय(Ast_नियोजय_Dly* nodep) override {
        VL_RESTORER(m_अन्तर्_Proc_नियोजय);
        m_अन्तर्_Proc_नियोजय = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(Ast_नियोजय_Force* nodep) override {
        iterateConst(nodep->rhsp());
        VL_RESTORER(m_अन्तर्_Initial);
        m_अन्तर्_Initial = असत्य;
        iterateConst(nodep->lhsp());
    }
    शून्य दर्शय(Ast_नियोजय_W* nodep) override {
        VL_RESTORER(m_अन्तर्_Cont_नियोजय);
        m_अन्तर्_Cont_नियोजय = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstRelease* nodep) override {
        VL_RESTORER(m_अन्तर्_Initial);
        m_अन्तर्_Initial = असत्य;
        iterateConst(nodep->lhsp());
    }
    शून्य दर्शय(AstInitial* nodep) override {
        VL_RESTORER(m_अन्तर्_Initial);
        m_अन्तर्_Initial = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstInitialAutomatic* nodep) override {
        VL_RESTORER(m_अन्तर्_Initial_सज्जता);
        m_अन्तर्_Initial_सज्जता = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_अन्तर्_Initial_सज्जता);
        m_अन्तर्_Initial_सज्जता = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstInitialStatic* nodep) override {
        VL_RESTORER(m_अन्तर्_Initial_Static);
        m_अन्तर्_Initial_Static = सत्य;
        VL_RESTORER(m_अन्तर्_Initial_सज्जता);
        m_अन्तर्_Initial_सज्जता = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_अन्तर्_Initial_सज्जता);
        m_अन्तर्_Initial_सज्जता = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        VL_RESTORER(m_alwaysCombp);
        VL_RESTORER(m_alwaysFFp);
        VL_RESTORER(m_alwaysPlainp);
        Ast_ग्रन्थिः::user2_Clear_वृक्षः();
        m_alwaysp = nodep;
        यदि (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            UINFO(9, "   " << nodep);
            m_alwaysCombp = nodep;
        } अन्यथा {
            m_alwaysCombp = शून्यसूचकः;
        }
        m_alwaysFFp = nodep->keyword() == VAlwaysKwd::ALWAYS_FF ? nodep : शून्यसूचकः;
        m_alwaysPlainp = nodep->keyword() == VAlwaysKwd::ALWAYS ? nodep : शून्यसूचकः;
        iterateChildrenConst(nodep);
        यदि (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) UINFO(9, "   Done " << nodep);
    }
    शून्य दर्शय(AstClocking* nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        VL_RESTORER(m_अन्तर्_FTask_Ref);
        m_अन्तर्_FTask_Ref = सत्य;

        iterateChildrenConst(nodep);

        यदि (!m_capturep) प्रत्यागम;

        // If writeSummary is enabled, task/function definitions are treated as non-executed.
        // Do not apply writeSummary at calls inside a task definition, or they will look like
        // independent drivers (phantom MULTIDRIVEN).
        ध्रुव तार्किक अन्तर्_Executed_Context
            = !(m_taskp && !m_alwaysp && !m_अन्तर्_Cont_नियोजय && !m_अन्तर्_Initial_Static && !m_अन्तर्_BBox
                && !m_taskp->dpiExport());

        यदि (!अन्तर्_Executed_Context) प्रत्यागम;

        Ast_ग्रन्थिः_FTask* ध्रुव calleep = nodep->taskp();
        यदि (!calleep) प्रत्यागम;

        ध्रुव स्वचालित& vars = m_capturep->writeSummary(calleep);
        प्रति (AstVar* ध्रुव varp : vars) {
            प्रति (पूर्णाङ्क usr = 1; usr < (m_alwaysCombp ? 3 : 2); ++usr) {
                Undriven_Var_प्रविष्टिः* ध्रुव entryp = getEntryp(varp, usr);
                entryp->driven_द्वारा_Call(nodep);
                यदि (m_alwaysCombp) entryp->driven_Always_Comb_सम्पूर्णम्(m_alwaysCombp);
                यदि (m_alwaysFFp) entryp->drivenAlwaysFFWhole(m_alwaysFFp, varp);
            }
        }
    }

    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_taskp);
        m_taskp = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstPin* nodep) override {
        VL_RESTORER(m_अन्तर्_Inout_Or_Ref_Pin);
        m_अन्तर्_Inout_Or_Ref_Pin = nodep->modVarp()->isInoutOrRef();
        iterateChildrenConst(nodep);
    }

    // Until we support tables, primitives will have undriven and unused I/Os
    शून्य दर्शय(AstPrimitive*) override {}

    // Coverage artifacts etc shouldn't count as a sink
    शून्य दर्शय(Ast_ग्रन्थिः_Cover_Decl*) override {}
    शून्य दर्शय(AstCoverInc*) override {}
    शून्य दर्शय(AstCoverToggle*) override {}
    शून्य दर्शय(AstTraceDecl* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    शून्य दर्शय(AstTraceInc* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }

    // iterate
    शून्य दर्शय(AstConst* nodep) override {}
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट UndrivenVisitor(AstNetlist* nodep, V3UndrivenCapture* capturep)
        : m_capturep{capturep} {
        iterateConst(nodep);
    }

    ~UndrivenVisitor() override {
        प्रति (Undriven_Var_प्रविष्टिः* ip : m_entryps[1]) ip->reportViolations();
        प्रति (पूर्णाङ्क usr = 1; usr < 3; ++usr) {
            प्रति (Undriven_Var_प्रविष्टिः* ip : m_entryps[usr]) विलोपय ip;
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
