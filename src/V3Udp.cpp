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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Udp.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"

#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ UdpVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    ತಾರ್ಕಿಕ m_inInitial = ಅಸತ್ಯ;  // Is inside of an initial block
    AstVar* m_oFieldVarp = nullptr;  // Output filed var of table line
    std::vector<AstVar*> m_ಒಳಹರಿವು_Vars;  // All the input vars in the AstPrimitive
    std::vector<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = nullptr;  // The current primitive
    ತಾರ್ಕಿಕ m_isFirstOutput = ಅಸತ್ಯ;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = nullptr;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = nullptr;  // Main Always block in UDP transform

    ಶೂನ್ಯ visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        ಯದಿ (m_primp) m_inInitial = ಸತ್ಯ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstAssign* nodep) override {
        ಯದಿ (m_inInitial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_isFirstOutput);
        VL_RESTORER_CLEAR(m_ಒಳಹರಿವು_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = nullptr;
        m_primp = nodep;
        m_isFirstOutput = ಅಸತ್ಯ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        ಯದಿ (m_primp) {
            ಯದಿ (nodep->isIO()) {
                ಯದಿ (nodep->is_ಒಳಹರಿವು()) {
                    m_ಒಳಹರಿವು_Vars.ತಳ್ಳು_back(nodep);
                } ಅನ್ಯಥಾ {
                    m_outputVars.ತಳ್ಳು_back(nodep);
                }
                ಯದಿ ((m_ಒಳಹರಿವು_Vars.size() == 0) && (m_outputVars.size() == 1)) {
                    m_isFirstOutput = ಸತ್ಯ;
                }
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstUdpTable* nodep) override {
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        ಯದಿ (m_outputVars.size() != 1) {
            m_outputVars.back()->v3error(
                m_outputVars.size()
                << " output ports for UDP table, there must be one output port");
        }
        ಯದಿ (!m_isFirstOutput && m_outputVars.size()) {
            m_ಒಳಹರಿವು_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_oFieldVarp = m_outputVars[0];

        m_alwaysBlockp = ನೂತನ AstAlways{fl, VAlwaysKwd::ALWAYS, nullptr, nullptr};
        fl->warnOff(V3ErrorCode::LATCH, ಸತ್ಯ);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(Ast_Udp_Table_ಸಾಲು* nodep) override {
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        ಯದಿ (!nodep->udpIsCombo() && !m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        ಯದಿ (nodep->udpIsCombo() && m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_ಗಂಟು* iNodep = nodep->iFieldsp();
        Ast_ಗಂಟು* oNodep = nodep->oFieldsp();
        Ast_Sen_ವೃಕ್ಷ* edgetrigp = nullptr;

        AstLogAnd* logandp = ನೂತನ AstLogAnd{fl, ನೂತನ AstConst{fl, AstConst::BitTrue{}},
                                           ನೂತನ AstConst{fl, AstConst::BitTrue{}}};

        ಪ್ರತಿ (AstVar* ಧ್ರುವ varp : m_ಒಳಹರಿವು_Vars) {
            ಯದಿ (!iNodep) ಭಙ್ಗ;
            ಯದಿ (Ast_Udp_Table_ಸಾಲು_Val* linevalp = VN_CAST(iNodep, Udp_Table_ಸಾಲು_Val)) {
                string valName = linevalp->name();
                ಯದಿ (isEdgeTrig(valName)) {
                    ಯದಿ (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    ಯದಿ (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = ನೂತನ Ast_Sen_ವೃಕ್ಷ{
                        fl, ನೂತನ AstSenItem{fl, VEdgeType::ET_BOTHEDGE,
                                           ನೂತನ AstVarRef{fl, varp, VAccess::READ}}};
                }
                ಯದಿ (valName == "0" || valName == "f") {
                    logandp = ನೂತನ AstLogAnd{
                        fl, logandp, ನೂತನ AstLogNot{fl, ನೂತನ AstVarRef{fl, varp, VAccess::READ}}};
                } ಅನ್ಯಥಾ ಯದಿ (valName == "1" || valName == "r") {
                    logandp = ನೂತನ AstLogAnd{fl, logandp, ನೂತನ AstVarRef{fl, varp, VAccess::READ}};
                } ಅನ್ಯಥಾ ಯದಿ (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    ಯದಿ (edgetrigp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(edgetrigp), edgetrigp);
                    ಯದಿ (logandp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(logandp), logandp);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        ಯದಿ (inputvars != m_ಒಳಹರಿವು_Vars.size()) {
            nodep->v3error("Incorrect number of input values, expected " << m_ಒಳಹರಿವು_Vars.size()
                                                                         << ", got " << inputvars);
        }

        string ಧ್ರುವ oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        ಯದಿ (oValName == "-") {
            ಯದಿ (edgetrigp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(edgetrigp), edgetrigp);
            ಯದಿ (logandp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(logandp), logandp);
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (!nodep->udpIsCombo()) {
            ಯದಿ (oNodep->name() == "0") {
                logandp = ನೂತನ AstLogAnd{
                    fl, logandp,
                    ನೂತನ AstLogNot{fl, ನೂತನ AstVarRef{fl, m_oFieldVarp, VAccess::READ}}};
            } ಅನ್ಯಥಾ ಯದಿ (oNodep->name() == "1") {
                logandp
                    = ನೂತನ AstLogAnd{fl, logandp, ನೂತನ AstVarRef{fl, m_oFieldVarp, VAccess::READ}};
            } ಅನ್ಯಥಾ ಯದಿ (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                ಯದಿ (edgetrigp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(edgetrigp), edgetrigp);
                ಯದಿ (logandp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(logandp), logandp);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        fl->warnOff(V3ErrorCode::LATCH, ಸತ್ಯ);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_ಗಂಟು_Stmt* ಧ್ರುವ assignp
            = newOutputAssign(fl, get_Output_ಅಂಕೆ(nodep, oValName), edgetrigp != nullptr);
        AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{fl, logandp, assignp};
        ಯದಿ (nodep->udpIsCombo()) {
            ಯದಿ (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->addStmtsp(ifp);
            ಯದಿ (edgetrigp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(edgetrigp), edgetrigp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->addNext(ನೂತನ AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ visit(AstLogAnd* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ visit(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isEdgeTrig(std::string& valName) {
        ಯದಿ (valName == "x" || valName == "X") ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (valName == "*") ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (valName.size() == 2) {
            ಯದಿ (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            ಅನ್ಯಥಾ ಯದಿ (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            ಅನ್ಯಥಾ ಯದಿ (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isCombOutputSig(ಧ್ರುವ std::string& valName) {
        ಪ್ರತ್ಯಾಗಮ (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSequentOutputSig(ಧ್ರುವ std::string& valName) {
        ಪ್ರತ್ಯಾಗಮ (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    Ast_ಗಂಟು_Stmt* newOutputAssign(File_ಸಾಲು* fl, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& out_ಅಂಕೆ, ತಾರ್ಕಿಕ isDelayed) ಧ್ರುವ {
        AstVarRef* ಧ್ರುವ lhsp = ನೂತನ AstVarRef{fl, m_oFieldVarp, VAccess::WRITE};
        AstConst* ಧ್ರುವ rhsp = ನೂತನ AstConst{fl, out_ಅಂಕೆ};
        ಯದಿ (isDelayed) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssignDly{fl, lhsp, rhsp};
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssign{fl, lhsp, rhsp};
    }
    ಸ್ಥಿರ V3_ಸಂಖ್ಯೆ get_Output_ಅಂಕೆ(Ast_ಗಂಟು* nodep, ಧ್ರುವ std::string& fieldNames) {
        V3_ಸಂಖ್ಯೆ output_ಅಂಕೆ{nodep, 1};
        ಯದಿ (fieldNames == "0") {
            output_ಅಂಕೆ.setBit(0, 0);
        } ಅನ್ಯಥಾ ಯದಿ (fieldNames == "1") {
            output_ಅಂಕೆ.setBit(0, 1);
        } ಅನ್ಯಥಾ {
            output_ಅಂಕೆ.setBit(0, 'x');
        }
        ಪ್ರತ್ಯಾಗಮ output_ಅಂಕೆ;
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = ಸ್ವಭಾವ;
};

ಶೂನ್ಯ V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ಧ್ರುವ UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("udp", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
