// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Implementation of User defined primitives
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// V3Udp's Transformations:
//
// For every table line create an always block containing if statements
// with condition depending on a combination of the input fields:
//
// 0 1 0 on a, b, c turns into !a&b&~c
//
//*************************************************************************

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Udp.h"

#अन्तर्भाव "V3Error.h"

#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

वग्ग UdpVisitor final : सार्वजनिक VNVisitor {
    तार्किक m_inInitial = मिच्छा;  // Is inside of an initial block
    AstVar* m_oFieldVarp = nullptr;  // Output filed var of table line
    std::vector<AstVar*> m_निवेशः_Vars;  // All the input vars in the AstPrimitive
    std::vector<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = nullptr;  // The current primitive
    तार्किक m_isFirstOutput = मिच्छा;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = nullptr;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = nullptr;  // Main Always block in UDP transform

    शून्य visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        सचे (m_primp) m_inInitial = सच्च;
        iterateChildren(nodep);
    }
    शून्य visit(AstAssign* nodep) override {
        सचे (m_inInitial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    शून्य visit(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_isFirstOutput);
        VL_RESTORER_CLEAR(m_निवेशः_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = nullptr;
        m_primp = nodep;
        m_isFirstOutput = मिच्छा;
        iterateChildren(nodep);
    }
    शून्य visit(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        सचे (m_primp) {
            सचे (nodep->isIO()) {
                सचे (nodep->is_निवेशः()) {
                    m_निवेशः_Vars.प्रक्षिप_back(nodep);
                } अञ्ञथा {
                    m_outputVars.प्रक्षिप_back(nodep);
                }
                सचे ((m_निवेशः_Vars.size() == 0) && (m_outputVars.size() == 1)) {
                    m_isFirstOutput = सच्च;
                }
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstUdpTable* nodep) override {
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        सचे (m_outputVars.size() != 1) {
            m_outputVars.back()->v3error(
                m_outputVars.size()
                << " output ports for UDP table, there must be one output port");
        }
        सचे (!m_isFirstOutput && m_outputVars.size()) {
            m_निवेशः_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_oFieldVarp = m_outputVars[0];

        m_alwaysBlockp = नूतन AstAlways{fl, VAlwaysKwd::ALWAYS, nullptr, nullptr};
        fl->warnOff(V3ErrorCode::LATCH, सच्च);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(Ast_Udp_Table_पङ्क्तिः* nodep) override {
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        सचे (!nodep->udpIsCombo() && !m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        सचे (nodep->udpIsCombo() && m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_ग्रन्थिः* iNodep = nodep->iFieldsp();
        Ast_ग्रन्थिः* oNodep = nodep->oFieldsp();
        Ast_Sen_वृक्षः* edgetrigp = nullptr;

        AstLogAnd* logandp = नूतन AstLogAnd{fl, नूतन AstConst{fl, AstConst::BitTrue{}},
                                           नूतन AstConst{fl, AstConst::BitTrue{}}};

        अत्थाय (AstVar* ध्रुव varp : m_निवेशः_Vars) {
            सचे (!iNodep) भङ्ग;
            सचे (Ast_Udp_Table_पङ्क्तिः_Val* linevalp = VN_CAST(iNodep, Udp_Table_पङ्क्तिः_Val)) {
                string valName = linevalp->name();
                सचे (isEdgeTrig(valName)) {
                    सचे (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    सचे (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(प्रक्षिप_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = नूतन Ast_Sen_वृक्षः{
                        fl, नूतन AstSenItem{fl, VEdgeType::ET_BOTHEDGE,
                                           नूतन AstVarRef{fl, varp, VAccess::READ}}};
                }
                सचे (valName == "0" || valName == "f") {
                    logandp = नूतन AstLogAnd{
                        fl, logandp, नूतन AstLogNot{fl, नूतन AstVarRef{fl, varp, VAccess::READ}}};
                } अञ्ञथा सचे (valName == "1" || valName == "r") {
                    logandp = नूतन AstLogAnd{fl, logandp, नूतन AstVarRef{fl, varp, VAccess::READ}};
                } अञ्ञथा सचे (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    सचे (edgetrigp) VL_DO_DANGLING(प्रक्षिप_Deletep(edgetrigp), edgetrigp);
                    सचे (logandp) VL_DO_DANGLING(प्रक्षिप_Deletep(logandp), logandp);
                    पटिनिवत्त;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        अत्थाय (ध्रुव Ast_ग्रन्थिः* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        सचे (inputvars != m_निवेशः_Vars.size()) {
            nodep->v3error("Incorrect number of input values, expected " << m_निवेशः_Vars.size()
                                                                         << ", got " << inputvars);
        }

        string ध्रुव oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        सचे (oValName == "-") {
            सचे (edgetrigp) VL_DO_DANGLING(प्रक्षिप_Deletep(edgetrigp), edgetrigp);
            सचे (logandp) VL_DO_DANGLING(प्रक्षिप_Deletep(logandp), logandp);
            पटिनिवत्त;
        }

        सचे (!nodep->udpIsCombo()) {
            सचे (oNodep->name() == "0") {
                logandp = नूतन AstLogAnd{
                    fl, logandp,
                    नूतन AstLogNot{fl, नूतन AstVarRef{fl, m_oFieldVarp, VAccess::READ}}};
            } अञ्ञथा सचे (oNodep->name() == "1") {
                logandp
                    = नूतन AstLogAnd{fl, logandp, नूतन AstVarRef{fl, m_oFieldVarp, VAccess::READ}};
            } अञ्ञथा सचे (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                सचे (edgetrigp) VL_DO_DANGLING(प्रक्षिप_Deletep(edgetrigp), edgetrigp);
                सचे (logandp) VL_DO_DANGLING(प्रक्षिप_Deletep(logandp), logandp);
                पटिनिवत्त;
            }
        }

        fl->warnOff(V3ErrorCode::LATCH, सच्च);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_ग्रन्थिः_Stmt* ध्रुव assignp
            = newOutputAssign(fl, get_Output_अङ्कः(nodep, oValName), edgetrigp != nullptr);
        AstIf* ध्रुव ifp = नूतन AstIf{fl, logandp, assignp};
        सचे (nodep->udpIsCombo()) {
            सचे (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->addStmtsp(ifp);
            सचे (edgetrigp) VL_DO_DANGLING(प्रक्षिप_Deletep(edgetrigp), edgetrigp);
            पटिनिवत्त;
        }
        सचे (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->addNext(नूतन AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstLogAnd* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    स्थिर तार्किक isEdgeTrig(std::string& valName) {
        सचे (valName == "x" || valName == "X") पटिनिवत्त मिच्छा;
        सचे (valName == "*") पटिनिवत्त सच्च;
        सचे (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            पटिनिवत्त सच्च;
        }
        सचे (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            पटिनिवत्त सच्च;
        }
        सचे (valName.size() == 2) {
            सचे (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            अञ्ञथा सचे (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            अञ्ञथा सचे (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            पटिनिवत्त सच्च;
        }
        सचे (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        पटिनिवत्त मिच्छा;
    }
    स्थिर तार्किक isCombOutputSig(ध्रुव std::string& valName) {
        पटिनिवत्त (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    स्थिर तार्किक isSequentOutputSig(ध्रुव std::string& valName) {
        पटिनिवत्त (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    Ast_ग्रन्थिः_Stmt* newOutputAssign(File_पङ्क्तिः* fl, ध्रुव V3_सङ्ख्या& out_अङ्कः, तार्किक isDelayed) ध्रुव {
        AstVarRef* ध्रुव lhsp = नूतन AstVarRef{fl, m_oFieldVarp, VAccess::WRITE};
        AstConst* ध्रुव rhsp = नूतन AstConst{fl, out_अङ्कः};
        सचे (isDelayed) पटिनिवत्त नूतन AstAssignDly{fl, lhsp, rhsp};
        पटिनिवत्त नूतन AstAssign{fl, lhsp, rhsp};
    }
    स्थिर V3_सङ्ख्या get_Output_अङ्कः(Ast_ग्रन्थिः* nodep, ध्रुव std::string& fieldNames) {
        V3_सङ्ख्या output_अङ्कः{nodep, 1};
        सचे (fieldNames == "0") {
            output_अङ्कः.setBit(0, 0);
        } अञ्ञथा सचे (fieldNames == "1") {
            output_अङ्कः.setBit(0, 1);
        } अञ्ञथा {
            output_अङ्कः.setBit(0, 'x');
        }
        पटिनिवत्त output_अङ्कः;
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = स्वभाव;
};

शून्य V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ध्रुव UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("udp", 0, dump_वृक्षः_Either_Level() >= 3);
}
