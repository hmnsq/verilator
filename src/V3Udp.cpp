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
    ತಾರ್ಕಿಕ m_ಒಳ_Initial = ಸುಳ್ಳು;  // Is inside of an initial block
    AstVar* m_o_ಕ್ಷೇತ್ರ_Varp = ಶೂನ್ಯಸೂಚಕ;  // Output filed var of table line
    std::ಸದಿಶ<AstVar*> m_ಒಳಹರಿವು_Vars;  // All the input vars in the AstPrimitive
    std::ಸದಿಶ<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = ಶೂನ್ಯಸೂಚಕ;  // The current primitive
    ತಾರ್ಕಿಕ m_is_ಮೊದಲ_Output = ಸುಳ್ಳು;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = ಶೂನ್ಯಸೂಚಕ;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = ಶೂನ್ಯಸೂಚಕ;  // Main Always block in UDP transform

    ಶೂನ್ಯ ಭೇಟಿ(AstInitial* nodep) override {
        VL_RESTORER(m_ಒಳ_Initial);
        ಆದರೆ (m_primp) m_ಒಳ_Initial = ನಿಜ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (m_ಒಳ_Initial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_is_ಮೊದಲ_Output);
        VL_RESTORER_CLEAR(m_ಒಳಹರಿವು_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = ಶೂನ್ಯಸೂಚಕ;
        m_primp = nodep;
        m_is_ಮೊದಲ_Output = ಸುಳ್ಳು;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        ಆದರೆ (m_primp) {
            ಆದರೆ (nodep->isIO()) {
                ಆದರೆ (nodep->is_ಒಳಹರಿವು()) {
                    m_ಒಳಹರಿವು_Vars.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    m_outputVars.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
                }
                ಆದರೆ ((m_ಒಳಹರಿವು_Vars.ಗಾತ್ರ_2() == 0) && (m_outputVars.ಗಾತ್ರ_2() == 1)) {
                    m_is_ಮೊದಲ_Output = ನಿಜ;
                }
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Udp_ಕೋಷ್ಟಕ* nodep) override {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        ಆದರೆ (m_outputVars.ಗಾತ್ರ_2() != 1) {
            m_outputVars.ಹಿಂಭಾಗ()->v3error(
                m_outputVars.ಗಾತ್ರ_2()
                << " output ports for UDP table, there must be one output port");
        }
        ಆದರೆ (!m_is_ಮೊದಲ_Output && m_outputVars.ಗಾತ್ರ_2()) {
            m_ಒಳಹರಿವು_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_o_ಕ್ಷೇತ್ರ_Varp = m_outputVars[0];

        m_alwaysBlockp = ಹೊಸ AstAlways{fl, VAlwaysKwd::ALWAYS, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        fl->warnOff(V3_Error_ಸಂಕೇತ::LATCH, ನಿಜ);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Udp_ಕೋಷ್ಟಕ_ಸಾಲು* nodep) override {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        ಆದರೆ (!nodep->udpIsCombo() && !m_o_ಕ್ಷೇತ್ರ_Varp->is_ಬಿಟ್_Logic()) {
            m_o_ಕ್ಷೇತ್ರ_Varp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        ಆದರೆ (nodep->udpIsCombo() && m_o_ಕ್ಷೇತ್ರ_Varp->is_ಬಿಟ್_Logic()) {
            m_o_ಕ್ಷೇತ್ರ_Varp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_ಗಂಟು* iNodep = nodep->iFieldsp();
        Ast_ಗಂಟು* oNodep = nodep->oFieldsp();
        Ast_Sen_ವೃಕ್ಷ* edgetrigp = ಶೂನ್ಯಸೂಚಕ;

        AstLogAnd* logandp = ಹೊಸ AstLogAnd{fl, ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}},
                                           ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}}};

        ಗಾಗಿ (AstVar* ಸ್ಥಿರಾಂಕ varp : m_ಒಳಹರಿವು_Vars) {
            ಆದರೆ (!iNodep) ನಿಲ್ಲಿಸು;
            ಆದರೆ (Ast_Udp_ಕೋಷ್ಟಕ_ಸಾಲು_Val* linevalp = VN_CAST(iNodep, Udp_ಕೋಷ್ಟಕ_ಸಾಲು_Val)) {
                string valName = linevalp->name();
                ಆದರೆ (is_ಅಂಚು_Trig(valName)) {
                    ಆದರೆ (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    ಆದರೆ (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{
                        fl, ಹೊಸ Ast_Sen_ಅಂಶ{fl, VEdgeType::ET_BOTHEDGE,
                                           ಹೊಸ AstVarRef{fl, varp, VAccess::READ}}};
                }
                ಆದರೆ (valName == "0" || valName == "f") {
                    logandp = ಹೊಸ AstLogAnd{
                        fl, logandp, ಹೊಸ AstLogNot{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::READ}}};
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (valName == "1" || valName == "r") {
                    logandp = ಹೊಸ AstLogAnd{fl, logandp, ಹೊಸ AstVarRef{fl, varp, VAccess::READ}};
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    ಆದರೆ (edgetrigp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(edgetrigp), edgetrigp);
                    ಆದರೆ (logandp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(logandp), logandp);
                    ಹಿಂತಿರುಗಿ;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        ಆದರೆ (inputvars != m_ಒಳಹರಿವು_Vars.ಗಾತ್ರ_2()) {
            nodep->v3error("Incorrect number of input values, expected " << m_ಒಳಹರಿವು_Vars.ಗಾತ್ರ_2()
                                                                         << ", got " << inputvars);
        }

        string ಸ್ಥಿರಾಂಕ oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        ಆದರೆ (oValName == "-") {
            ಆದರೆ (edgetrigp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(edgetrigp), edgetrigp);
            ಆದರೆ (logandp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(logandp), logandp);
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (!nodep->udpIsCombo()) {
            ಆದರೆ (oNodep->name() == "0") {
                logandp = ಹೊಸ AstLogAnd{
                    fl, logandp,
                    ಹೊಸ AstLogNot{fl, ಹೊಸ AstVarRef{fl, m_o_ಕ್ಷೇತ್ರ_Varp, VAccess::READ}}};
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (oNodep->name() == "1") {
                logandp
                    = ಹೊಸ AstLogAnd{fl, logandp, ಹೊಸ AstVarRef{fl, m_o_ಕ್ಷೇತ್ರ_Varp, VAccess::READ}};
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                ಆದರೆ (edgetrigp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(edgetrigp), edgetrigp);
                ಆದರೆ (logandp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(logandp), logandp);
                ಹಿಂತಿರುಗಿ;
            }
        }

        fl->warnOff(V3_Error_ಸಂಕೇತ::LATCH, ನಿಜ);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ assignp
            = new_Output_ನಿಯೋಜಿಸು(fl, get_Output_ಅಂಕೆ(nodep, oValName), edgetrigp != ಶೂನ್ಯಸೂಚಕ);
        AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{fl, logandp, assignp};
        ಆದರೆ (nodep->udpIsCombo()) {
            ಆದರೆ (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->ಕೂಡಿಸು_Stmtsp(ifp);
            ಆದರೆ (edgetrigp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(edgetrigp), edgetrigp);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogAnd* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_ಅಂಚು_Trig(std::string& valName) {
        ಆದರೆ (valName == "x" || valName == "X") ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (valName == "*") ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (valName.ಗಾತ್ರ_2() == 2) {
            ಆದರೆ (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isCombOutputSig(ಸ್ಥಿರಾಂಕ std::string& valName) {
        ಹಿಂತಿರುಗಿ (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSequentOutputSig(ಸ್ಥಿರಾಂಕ std::string& valName) {
        ಹಿಂತಿರುಗಿ (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    Ast_ಗಂಟು_Stmt* new_Output_ನಿಯೋಜಿಸು(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು_ಅಂಕೆ, ತಾರ್ಕಿಕ isDelayed) ಸ್ಥಿರಾಂಕ {
        AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstVarRef{fl, m_o_ಕ್ಷೇತ್ರ_Varp, VAccess::WRITE};
        AstConst* ಸ್ಥಿರಾಂಕ rhsp = ಹೊಸ AstConst{fl, ಹೊರಹೋಗು_ಅಂಕೆ};
        ಆದರೆ (isDelayed) ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{fl, lhsp, rhsp};
        ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, lhsp, rhsp};
    }
    ಸ್ಥಿರ V3_ಸಂಖ್ಯೆ get_Output_ಅಂಕೆ(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ std::string& ಕ್ಷೇತ್ರ_ಹೆಸರುಗಳು) {
        V3_ಸಂಖ್ಯೆ output_ಅಂಕೆ{nodep, 1};
        ಆದರೆ (ಕ್ಷೇತ್ರ_ಹೆಸರುಗಳು == "0") {
            output_ಅಂಕೆ.ಗಣ_ಬಿಟ್(0, 0);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಕ್ಷೇತ್ರ_ಹೆಸರುಗಳು == "1") {
            output_ಅಂಕೆ.ಗಣ_ಬಿಟ್(0, 1);
        } ಇಲ್ಲದಿದ್ದರೆ {
            output_ಅಂಕೆ.ಗಣ_ಬಿಟ್(0, 'x');
        }
        ಹಿಂತಿರುಗಿ output_ಅಂಕೆ;
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

ಶೂನ್ಯ V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ಸ್ಥಿರಾಂಕ UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("udp", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
