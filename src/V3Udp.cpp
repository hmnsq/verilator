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

वर्ग UdpVisitor final : सार्वजनिक VNVisitor {
    तार्किक m_inInitial = मिच्छ;  // Is inside of an initial block
    AstVar* m_oFieldVarp = nullptr;  // Output filed var of table line
    std::vector<AstVar*> m_निवेशः_Vars;  // All the input vars in the AstPrimitive
    std::vector<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = nullptr;  // The current primitive
    तार्किक m_isFirstOutput = मिच्छ;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = nullptr;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = nullptr;  // Main Always block in UDP transform

    शून्य visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        जइ (m_primp) m_inInitial = सच्च;
        iterateChildren(nodep);
    }
    शून्य visit(AstAssign* nodep) override {
        जइ (m_inInitial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
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
        m_isFirstOutput = मिच्छ;
        iterateChildren(nodep);
    }
    शून्य visit(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        जइ (m_primp) {
            जइ (nodep->isIO()) {
                जइ (nodep->is_निवेशः()) {
                    m_निवेशः_Vars.प्रक्षिप_back(nodep);
                } अण्णहा {
                    m_outputVars.प्रक्षिप_back(nodep);
                }
                जइ ((m_निवेशः_Vars.size() == 0) && (m_outputVars.size() == 1)) {
                    m_isFirstOutput = सच्च;
                }
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstUdpTable* nodep) override {
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        जइ (m_outputVars.size() != 1) {
            m_outputVars.back()->v3error(
                m_outputVars.size()
                << " output ports for UDP table, there must be one output port");
        }
        जइ (!m_isFirstOutput && m_outputVars.size()) {
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
        जइ (!nodep->udpIsCombo() && !m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        जइ (nodep->udpIsCombo() && m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_ग्रन्थिः* iNodep = nodep->iFieldsp();
        Ast_ग्रन्थिः* oNodep = nodep->oFieldsp();
        Ast_Sen_वृक्षः* edgetrigp = nullptr;

        AstLogAnd* logandp = नूतन AstLogAnd{fl, नूतन AstConst{fl, AstConst::BitTrue{}},
                                           नूतन AstConst{fl, AstConst::BitTrue{}}};

        प्रति (AstVar* ध्रुव varp : m_निवेशः_Vars) {
            जइ (!iNodep) भङ्ग;
            जइ (Ast_Udp_Table_पङ्क्तिः_Val* linevalp = VN_CAST(iNodep, Udp_Table_पङ्क्तिः_Val)) {
                string valName = linevalp->name();
                जइ (isEdgeTrig(valName)) {
                    जइ (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    जइ (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(प्रक्षिप_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = नूतन Ast_Sen_वृक्षः{
                        fl, नूतन AstSenItem{fl, VEdgeType::ET_BOTHEDGE,
                                           नूतन AstVarRef{fl, varp, VAccess::READ}}};
                }
                जइ (valName == "0" || valName == "f") {
                    logandp = नूतन AstLogAnd{
                        fl, logandp, नूतन AstLogNot{fl, नूतन AstVarRef{fl, varp, VAccess::READ}}};
                } अण्णहा जइ (valName == "1" || valName == "r") {
                    logandp = नूतन AstLogAnd{fl, logandp, नूतन AstVarRef{fl, varp, VAccess::READ}};
                } अण्णहा जइ (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    जइ (edgetrigp) VL_DO_DANGLING(प्रक्षिप_Deletep(edgetrigp), edgetrigp);
                    जइ (logandp) VL_DO_DANGLING(प्रक्षिप_Deletep(logandp), logandp);
                    प्रत्यागम;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        प्रति (ध्रुव Ast_ग्रन्थिः* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        जइ (inputvars != m_निवेशः_Vars.size()) {
            nodep->v3error("Incorrect number of input values, expected " << m_निवेशः_Vars.size()
                                                                         << ", got " << inputvars);
        }

        string ध्रुव oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        जइ (oValName == "-") {
            जइ (edgetrigp) VL_DO_DANGLING(प्रक्षिप_Deletep(edgetrigp), edgetrigp);
            जइ (logandp) VL_DO_DANGLING(प्रक्षिप_Deletep(logandp), logandp);
            प्रत्यागम;
        }

        जइ (!nodep->udpIsCombo()) {
            जइ (oNodep->name() == "0") {
                logandp = नूतन AstLogAnd{
                    fl, logandp,
                    नूतन AstLogNot{fl, नूतन AstVarRef{fl, m_oFieldVarp, VAccess::READ}}};
            } अण्णहा जइ (oNodep->name() == "1") {
                logandp
                    = नूतन AstLogAnd{fl, logandp, नूतन AstVarRef{fl, m_oFieldVarp, VAccess::READ}};
            } अण्णहा जइ (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                जइ (edgetrigp) VL_DO_DANGLING(प्रक्षिप_Deletep(edgetrigp), edgetrigp);
                जइ (logandp) VL_DO_DANGLING(प्रक्षिप_Deletep(logandp), logandp);
                प्रत्यागम;
            }
        }

        fl->warnOff(V3ErrorCode::LATCH, सच्च);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_ग्रन्थिः_Stmt* ध्रुव assignp
            = newOutputAssign(fl, get_Output_अङ्कः(nodep, oValName), edgetrigp != nullptr);
        AstIf* ध्रुव ifp = नूतन AstIf{fl, logandp, assignp};
        जइ (nodep->udpIsCombo()) {
            जइ (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->addStmtsp(ifp);
            जइ (edgetrigp) VL_DO_DANGLING(प्रक्षिप_Deletep(edgetrigp), edgetrigp);
            प्रत्यागम;
        }
        जइ (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->addNext(नूतन AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstLogAnd* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    स्थिर तार्किक isEdgeTrig(std::string& valName) {
        जइ (valName == "x" || valName == "X") प्रत्यागम मिच्छ;
        जइ (valName == "*") प्रत्यागम सच्च;
        जइ (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            प्रत्यागम सच्च;
        }
        जइ (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            प्रत्यागम सच्च;
        }
        जइ (valName.size() == 2) {
            जइ (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            अण्णहा जइ (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            अण्णहा जइ (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            प्रत्यागम सच्च;
        }
        जइ (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        प्रत्यागम मिच्छ;
    }
    स्थिर तार्किक isCombOutputSig(ध्रुव std::string& valName) {
        प्रत्यागम (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    स्थिर तार्किक isSequentOutputSig(ध्रुव std::string& valName) {
        प्रत्यागम (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    Ast_ग्रन्थिः_Stmt* newOutputAssign(File_पङ्क्तिः* fl, ध्रुव V3_सङ्ख्या& out_अङ्कः, तार्किक isDelayed) ध्रुव {
        AstVarRef* ध्रुव lhsp = नूतन AstVarRef{fl, m_oFieldVarp, VAccess::WRITE};
        AstConst* ध्रुव rhsp = नूतन AstConst{fl, out_अङ्कः};
        जइ (isDelayed) प्रत्यागम नूतन AstAssignDly{fl, lhsp, rhsp};
        प्रत्यागम नूतन AstAssign{fl, lhsp, rhsp};
    }
    स्थिर V3_सङ्ख्या get_Output_अङ्कः(Ast_ग्रन्थिः* nodep, ध्रुव std::string& fieldNames) {
        V3_सङ्ख्या output_अङ्कः{nodep, 1};
        जइ (fieldNames == "0") {
            output_अङ्कः.setBit(0, 0);
        } अण्णहा जइ (fieldNames == "1") {
            output_अङ्कः.setBit(0, 1);
        } अण्णहा {
            output_अङ्कः.setBit(0, 'x');
        }
        प्रत्यागम output_अङ्कः;
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
