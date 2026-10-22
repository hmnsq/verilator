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
    तार्किक m_inInitial = असत्य;  // Is inside of an initial block
    AstVar* m_oFieldVarp = nullptr;  // Output filed var of table line
    std::vector<AstVar*> m_आदान_Vars;  // All the input vars in the AstPrimitive
    std::vector<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = nullptr;  // The current primitive
    तार्किक m_isFirstOutput = असत्य;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = nullptr;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = nullptr;  // Main Always block in UDP transform

    शून्य visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        यदि (m_primp) m_inInitial = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(AstAssign* nodep) override {
        यदि (m_inInitial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    शून्य visit(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_isFirstOutput);
        VL_RESTORER_CLEAR(m_आदान_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = nullptr;
        m_primp = nodep;
        m_isFirstOutput = असत्य;
        iterateChildren(nodep);
    }
    शून्य visit(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        यदि (m_primp) {
            यदि (nodep->isIO()) {
                यदि (nodep->is_आदान()) {
                    m_आदान_Vars.ढकला_back(nodep);
                } अन्यथा {
                    m_outputVars.ढकला_back(nodep);
                }
                यदि ((m_आदान_Vars.size() == 0) && (m_outputVars.size() == 1)) {
                    m_isFirstOutput = सत्य;
                }
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstUdpTable* nodep) override {
        File_ओळ* ध्रुव fl = nodep->fileline();
        यदि (m_outputVars.size() != 1) {
            m_outputVars.back()->v3error(
                m_outputVars.size()
                << " output ports for UDP table, there must be one output port");
        }
        यदि (!m_isFirstOutput && m_outputVars.size()) {
            m_आदान_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_oFieldVarp = m_outputVars[0];

        m_alwaysBlockp = नूतन AstAlways{fl, VAlwaysKwd::ALWAYS, nullptr, nullptr};
        fl->warnOff(V3ErrorCode::LATCH, सत्य);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(Ast_Udp_Table_ओळ* nodep) override {
        File_ओळ* ध्रुव fl = nodep->fileline();
        यदि (!nodep->udpIsCombo() && !m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        यदि (nodep->udpIsCombo() && m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_गाठ* iNodep = nodep->iFieldsp();
        Ast_गाठ* oNodep = nodep->oFieldsp();
        Ast_Sen_वृक्ष* edgetrigp = nullptr;

        AstLogAnd* logandp = नूतन AstLogAnd{fl, नूतन AstConst{fl, AstConst::BitTrue{}},
                                           नूतन AstConst{fl, AstConst::BitTrue{}}};

        प्रति (AstVar* ध्रुव varp : m_आदान_Vars) {
            यदि (!iNodep) भङ्ग;
            यदि (Ast_Udp_Table_ओळ_Val* linevalp = VN_CAST(iNodep, Udp_Table_ओळ_Val)) {
                string valName = linevalp->name();
                यदि (isEdgeTrig(valName)) {
                    यदि (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    यदि (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(ढकला_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = नूतन Ast_Sen_वृक्ष{
                        fl, नूतन AstSenItem{fl, VEdgeType::ET_BOTHEDGE,
                                           नूतन AstVarRef{fl, varp, VAccess::READ}}};
                }
                यदि (valName == "0" || valName == "f") {
                    logandp = नूतन AstLogAnd{
                        fl, logandp, नूतन AstLogNot{fl, नूतन AstVarRef{fl, varp, VAccess::READ}}};
                } अन्यथा यदि (valName == "1" || valName == "r") {
                    logandp = नूतन AstLogAnd{fl, logandp, नूतन AstVarRef{fl, varp, VAccess::READ}};
                } अन्यथा यदि (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    यदि (edgetrigp) VL_DO_DANGLING(ढकला_Deletep(edgetrigp), edgetrigp);
                    यदि (logandp) VL_DO_DANGLING(ढकला_Deletep(logandp), logandp);
                    प्रत्यागम;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        प्रति (ध्रुव Ast_गाठ* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        यदि (inputvars != m_आदान_Vars.size()) {
            nodep->v3error("Incorrect number of input values, expected " << m_आदान_Vars.size()
                                                                         << ", got " << inputvars);
        }

        string ध्रुव oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        यदि (oValName == "-") {
            यदि (edgetrigp) VL_DO_DANGLING(ढकला_Deletep(edgetrigp), edgetrigp);
            यदि (logandp) VL_DO_DANGLING(ढकला_Deletep(logandp), logandp);
            प्रत्यागम;
        }

        यदि (!nodep->udpIsCombo()) {
            यदि (oNodep->name() == "0") {
                logandp = नूतन AstLogAnd{
                    fl, logandp,
                    नूतन AstLogNot{fl, नूतन AstVarRef{fl, m_oFieldVarp, VAccess::READ}}};
            } अन्यथा यदि (oNodep->name() == "1") {
                logandp
                    = नूतन AstLogAnd{fl, logandp, नूतन AstVarRef{fl, m_oFieldVarp, VAccess::READ}};
            } अन्यथा यदि (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                यदि (edgetrigp) VL_DO_DANGLING(ढकला_Deletep(edgetrigp), edgetrigp);
                यदि (logandp) VL_DO_DANGLING(ढकला_Deletep(logandp), logandp);
                प्रत्यागम;
            }
        }

        fl->warnOff(V3ErrorCode::LATCH, सत्य);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_गाठ_Stmt* ध्रुव assignp
            = newOutputAssign(fl, get_Output_अंक(nodep, oValName), edgetrigp != nullptr);
        AstIf* ध्रुव ifp = नूतन AstIf{fl, logandp, assignp};
        यदि (nodep->udpIsCombo()) {
            यदि (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->addStmtsp(ifp);
            यदि (edgetrigp) VL_DO_DANGLING(ढकला_Deletep(edgetrigp), edgetrigp);
            प्रत्यागम;
        }
        यदि (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->addNext(नूतन AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstLogAnd* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    स्थिर तार्किक isEdgeTrig(std::string& valName) {
        यदि (valName == "x" || valName == "X") प्रत्यागम असत्य;
        यदि (valName == "*") प्रत्यागम सत्य;
        यदि (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            प्रत्यागम सत्य;
        }
        यदि (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            प्रत्यागम सत्य;
        }
        यदि (valName.size() == 2) {
            यदि (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            अन्यथा यदि (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            अन्यथा यदि (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            प्रत्यागम सत्य;
        }
        यदि (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        प्रत्यागम असत्य;
    }
    स्थिर तार्किक isCombOutputSig(ध्रुव std::string& valName) {
        प्रत्यागम (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    स्थिर तार्किक isSequentOutputSig(ध्रुव std::string& valName) {
        प्रत्यागम (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    Ast_गाठ_Stmt* newOutputAssign(File_ओळ* fl, ध्रुव V3_संख्या& out_अंक, तार्किक isDelayed) ध्रुव {
        AstVarRef* ध्रुव lhsp = नूतन AstVarRef{fl, m_oFieldVarp, VAccess::WRITE};
        AstConst* ध्रुव rhsp = नूतन AstConst{fl, out_अंक};
        यदि (isDelayed) प्रत्यागम नूतन AstAssignDly{fl, lhsp, rhsp};
        प्रत्यागम नूतन AstAssign{fl, lhsp, rhsp};
    }
    स्थिर V3_संख्या get_Output_अंक(Ast_गाठ* nodep, ध्रुव std::string& fieldNames) {
        V3_संख्या output_अंक{nodep, 1};
        यदि (fieldNames == "0") {
            output_अंक.setBit(0, 0);
        } अन्यथा यदि (fieldNames == "1") {
            output_अंक.setBit(0, 1);
        } अन्यथा {
            output_अंक.setBit(0, 'x');
        }
        प्रत्यागम output_अंक;
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = स्वभाव;
};

शून्य V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ध्रुव UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("udp", 0, dump_वृक्ष_Either_Level() >= 3);
}
