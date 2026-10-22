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
    std::vector<AstVar*> m_आगत_Vars;  // All the input vars in the AstPrimitive
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
        VL_RESTORER_CLEAR(m_आगत_Vars);
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
                यदि (nodep->is_आगत()) {
                    m_आगत_Vars.थप_back(nodep);
                } अन्यथा {
                    m_outputVars.थप_back(nodep);
                }
                यदि ((m_आगत_Vars.size() == 0) && (m_outputVars.size() == 1)) {
                    m_isFirstOutput = सत्य;
                }
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstUdpTable* nodep) override {
        File_पङ्क्ति* अचर fl = nodep->fileline();
        यदि (m_outputVars.size() != 1) {
            m_outputVars.back()->v3error(
                m_outputVars.size()
                << " output ports for UDP table, there must be one output port");
        }
        यदि (!m_isFirstOutput && m_outputVars.size()) {
            m_आगत_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_oFieldVarp = m_outputVars[0];

        m_alwaysBlockp = नूतन AstAlways{fl, VAlwaysKwd::ALWAYS, nullptr, nullptr};
        fl->warnOff(V3ErrorCode::LATCH, सत्य);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(Ast_Udp_Table_पङ्क्ति* nodep) override {
        File_पङ्क्ति* अचर fl = nodep->fileline();
        यदि (!nodep->udpIsCombo() && !m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        यदि (nodep->udpIsCombo() && m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_गाँठो* iNodep = nodep->iFieldsp();
        Ast_गाँठो* oNodep = nodep->oFieldsp();
        Ast_Sen_रूख* edgetrigp = nullptr;

        AstLogAnd* logandp = नूतन AstLogAnd{fl, नूतन AstConst{fl, AstConst::BitTrue{}},
                                           नूतन AstConst{fl, AstConst::BitTrue{}}};

        लागि (AstVar* अचर varp : m_आगत_Vars) {
            यदि (!iNodep) रोक;
            यदि (Ast_Udp_Table_पङ्क्ति_Val* linevalp = VN_CAST(iNodep, Udp_Table_पङ्क्ति_Val)) {
                string valName = linevalp->name();
                यदि (isEdgeTrig(valName)) {
                    यदि (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    यदि (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(थप_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = नूतन Ast_Sen_रूख{
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
                    यदि (edgetrigp) VL_DO_DANGLING(थप_Deletep(edgetrigp), edgetrigp);
                    यदि (logandp) VL_DO_DANGLING(थप_Deletep(logandp), logandp);
                    फर्काऊ;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        लागि (अचर Ast_गाँठो* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        यदि (inputvars != m_आगत_Vars.size()) {
            nodep->v3error("Incorrect number of input values, expected " << m_आगत_Vars.size()
                                                                         << ", got " << inputvars);
        }

        string अचर oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        यदि (oValName == "-") {
            यदि (edgetrigp) VL_DO_DANGLING(थप_Deletep(edgetrigp), edgetrigp);
            यदि (logandp) VL_DO_DANGLING(थप_Deletep(logandp), logandp);
            फर्काऊ;
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
                यदि (edgetrigp) VL_DO_DANGLING(थप_Deletep(edgetrigp), edgetrigp);
                यदि (logandp) VL_DO_DANGLING(थप_Deletep(logandp), logandp);
                फर्काऊ;
            }
        }

        fl->warnOff(V3ErrorCode::LATCH, सत्य);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_गाँठो_Stmt* अचर assignp
            = newOutputAssign(fl, get_Output_अङ्क(nodep, oValName), edgetrigp != nullptr);
        AstIf* अचर ifp = नूतन AstIf{fl, logandp, assignp};
        यदि (nodep->udpIsCombo()) {
            यदि (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->addStmtsp(ifp);
            यदि (edgetrigp) VL_DO_DANGLING(थप_Deletep(edgetrigp), edgetrigp);
            फर्काऊ;
        }
        यदि (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->addNext(नूतन AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstLogAnd* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    स्थिर तार्किक isEdgeTrig(std::string& valName) {
        यदि (valName == "x" || valName == "X") फर्काऊ असत्य;
        यदि (valName == "*") फर्काऊ सत्य;
        यदि (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            फर्काऊ सत्य;
        }
        यदि (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            फर्काऊ सत्य;
        }
        यदि (valName.size() == 2) {
            यदि (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            अन्यथा यदि (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            अन्यथा यदि (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            फर्काऊ सत्य;
        }
        यदि (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        फर्काऊ असत्य;
    }
    स्थिर तार्किक isCombOutputSig(अचर std::string& valName) {
        फर्काऊ (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    स्थिर तार्किक isSequentOutputSig(अचर std::string& valName) {
        फर्काऊ (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    Ast_गाँठो_Stmt* newOutputAssign(File_पङ्क्ति* fl, अचर V3_सङ्ख्या& out_अङ्क, तार्किक isDelayed) अचर {
        AstVarRef* अचर lhsp = नूतन AstVarRef{fl, m_oFieldVarp, VAccess::WRITE};
        AstConst* अचर rhsp = नूतन AstConst{fl, out_अङ्क};
        यदि (isDelayed) फर्काऊ नूतन AstAssignDly{fl, lhsp, rhsp};
        फर्काऊ नूतन AstAssign{fl, lhsp, rhsp};
    }
    स्थिर V3_सङ्ख्या get_Output_अङ्क(Ast_गाँठो* nodep, अचर std::string& fieldNames) {
        V3_सङ्ख्या output_अङ्क{nodep, 1};
        यदि (fieldNames == "0") {
            output_अङ्क.setBit(0, 0);
        } अन्यथा यदि (fieldNames == "1") {
            output_अङ्क.setBit(0, 1);
        } अन्यथा {
            output_अङ्क.setBit(0, 'x');
        }
        फर्काऊ output_अङ्क;
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = स्वभाव;
};

शून्य V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { अचर UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_रूख("udp", 0, dump_रूख_Either_Level() >= 3);
}
