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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Udp.h"

#समावेश "V3Error.h"

#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

श्रेणी UdpVisitor final : खुला VNVisitor {
    bool m_inInitial = false;  // Is inside of an initial block
    AstVar* m_oFieldVarp = nullptr;  // Output filed var of table line
    std::vector<AstVar*> m_निवेश_Vars;  // All the input vars in the AstPrimitive
    std::vector<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = nullptr;  // The current primitive
    bool m_isFirstOutput = false;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = nullptr;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = nullptr;  // Main Always block in UDP transform

    व्योम visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        अगर (m_primp) m_inInitial = true;
        iterateChildren(nodep);
    }
    व्योम visit(AstAssign* nodep) override {
        अगर (m_inInitial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    व्योम visit(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_isFirstOutput);
        VL_RESTORER_CLEAR(m_निवेश_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = nullptr;
        m_primp = nodep;
        m_isFirstOutput = false;
        iterateChildren(nodep);
    }
    व्योम visit(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        अगर (m_primp) {
            अगर (nodep->isIO()) {
                अगर (nodep->is_निवेश()) {
                    m_निवेश_Vars.धकेलो_back(nodep);
                } अन्यथा {
                    m_outputVars.धकेलो_back(nodep);
                }
                अगर ((m_निवेश_Vars.आकार() == 0) && (m_outputVars.आकार() == 1)) {
                    m_isFirstOutput = true;
                }
            }
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstUdpTable* nodep) override {
        File_पंक्ति* स्थिर fl = nodep->fileline();
        अगर (m_outputVars.आकार() != 1) {
            m_outputVars.back()->v3error(
                m_outputVars.आकार()
                << " output ports for UDP table, there must be one output port");
        }
        अगर (!m_isFirstOutput && m_outputVars.आकार()) {
            m_निवेश_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_oFieldVarp = m_outputVars[0];

        m_alwaysBlockp = नया AstAlways{fl, VAlwaysKwd::ALWAYS, nullptr, nullptr};
        fl->warnOff(V3ErrorCode::LATCH, true);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(Ast_Udp_Table_पंक्ति* nodep) override {
        File_पंक्ति* स्थिर fl = nodep->fileline();
        अगर (!nodep->udpIsCombo() && !m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        अगर (nodep->udpIsCombo() && m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_गाँठ* iNodep = nodep->iFieldsp();
        Ast_गाँठ* oNodep = nodep->oFieldsp();
        Ast_Sen_वृक्ष* edgetrigp = nullptr;

        AstLogAnd* logandp = नया AstLogAnd{fl, नया AstConst{fl, AstConst::BitTrue{}},
                                           नया AstConst{fl, AstConst::BitTrue{}}};

        क्रम (AstVar* स्थिर varp : m_निवेश_Vars) {
            अगर (!iNodep) अवरोध;
            अगर (Ast_Udp_Table_पंक्ति_Val* linevalp = VN_CAST(iNodep, Udp_Table_पंक्ति_Val)) {
                string valName = linevalp->name();
                अगर (isEdgeTrig(valName)) {
                    अगर (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    अगर (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(धकेलो_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = नया Ast_Sen_वृक्ष{
                        fl, नया AstSenItem{fl, VEdgeType::ET_BOTHEDGE,
                                           नया AstVarRef{fl, varp, VAccess::READ}}};
                }
                अगर (valName == "0" || valName == "f") {
                    logandp = नया AstLogAnd{
                        fl, logandp, नया AstLogNot{fl, नया AstVarRef{fl, varp, VAccess::READ}}};
                } अन्यथा अगर (valName == "1" || valName == "r") {
                    logandp = नया AstLogAnd{fl, logandp, नया AstVarRef{fl, varp, VAccess::READ}};
                } अन्यथा अगर (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    अगर (edgetrigp) VL_DO_DANGLING(धकेलो_Deletep(edgetrigp), edgetrigp);
                    अगर (logandp) VL_DO_DANGLING(धकेलो_Deletep(logandp), logandp);
                    वापस;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        क्रम (स्थिर Ast_गाँठ* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        अगर (inputvars != m_निवेश_Vars.आकार()) {
            nodep->v3error("Incorrect number of input values, expected " << m_निवेश_Vars.आकार()
                                                                         << ", got " << inputvars);
        }

        string स्थिर oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        अगर (oValName == "-") {
            अगर (edgetrigp) VL_DO_DANGLING(धकेलो_Deletep(edgetrigp), edgetrigp);
            अगर (logandp) VL_DO_DANGLING(धकेलो_Deletep(logandp), logandp);
            वापस;
        }

        अगर (!nodep->udpIsCombo()) {
            अगर (oNodep->name() == "0") {
                logandp = नया AstLogAnd{
                    fl, logandp,
                    नया AstLogNot{fl, नया AstVarRef{fl, m_oFieldVarp, VAccess::READ}}};
            } अन्यथा अगर (oNodep->name() == "1") {
                logandp
                    = नया AstLogAnd{fl, logandp, नया AstVarRef{fl, m_oFieldVarp, VAccess::READ}};
            } अन्यथा अगर (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                अगर (edgetrigp) VL_DO_DANGLING(धकेलो_Deletep(edgetrigp), edgetrigp);
                अगर (logandp) VL_DO_DANGLING(धकेलो_Deletep(logandp), logandp);
                वापस;
            }
        }

        fl->warnOff(V3ErrorCode::LATCH, true);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_गाँठ_Stmt* स्थिर assignp
            = newOutputAssign(fl, get_Output_अंक(nodep, oValName), edgetrigp != nullptr);
        AstIf* स्थिर ifp = नया AstIf{fl, logandp, assignp};
        अगर (nodep->udpIsCombo()) {
            अगर (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->addStmtsp(ifp);
            अगर (edgetrigp) VL_DO_DANGLING(धकेलो_Deletep(edgetrigp), edgetrigp);
            वापस;
        }
        अगर (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->add_अग्र(नया AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }
    व्योम visit(AstLogAnd* nodep) override { iterateChildren(nodep); }
    व्योम visit(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    अटल bool isEdgeTrig(std::string& valName) {
        अगर (valName == "x" || valName == "X") वापस false;
        अगर (valName == "*") वापस true;
        अगर (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            वापस true;
        }
        अगर (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            वापस true;
        }
        अगर (valName.आकार() == 2) {
            अगर (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            अन्यथा अगर (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            अन्यथा अगर (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            वापस true;
        }
        अगर (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        वापस false;
    }
    अटल bool isCombOutputSig(स्थिर std::string& valName) {
        वापस (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    अटल bool isSequentOutputSig(स्थिर std::string& valName) {
        वापस (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    Ast_गाँठ_Stmt* newOutputAssign(File_पंक्ति* fl, स्थिर V3_संख्या& out_अंक, bool isDelayed) स्थिर {
        AstVarRef* स्थिर lhsp = नया AstVarRef{fl, m_oFieldVarp, VAccess::WRITE};
        AstConst* स्थिर rhsp = नया AstConst{fl, out_अंक};
        अगर (isDelayed) वापस नया AstAssignDly{fl, lhsp, rhsp};
        वापस नया AstAssign{fl, lhsp, rhsp};
    }
    अटल V3_संख्या get_Output_अंक(Ast_गाँठ* nodep, स्थिर std::string& fieldNames) {
        V3_संख्या output_अंक{nodep, 1};
        अगर (fieldNames == "0") {
            output_अंक.setBit(0, 0);
        } अन्यथा अगर (fieldNames == "1") {
            output_अंक.setBit(0, 1);
        } अन्यथा {
            output_अंक.setBit(0, 'x');
        }
        वापस output_अंक;
    }

खुला:
    // CONSTRUCTORS
    explicit UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = शेष;
};

व्योम V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { स्थिर UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("udp", 0, dump_वृक्ष_Either_Level() >= 3);
}
