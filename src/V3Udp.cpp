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
    तार्किक m_अन्तर्_Initial = असत्य;  // Is inside of an initial block
    AstVar* m_o_क्षेत्रम्_Varp = शून्यसूचकः;  // Output filed var of table line
    std::सदिशम्<AstVar*> m_निवेशः_Vars;  // All the input vars in the AstPrimitive
    std::सदिशम्<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = शून्यसूचकः;  // The current primitive
    तार्किक m_isFirstOutput = असत्य;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = शून्यसूचकः;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = शून्यसूचकः;  // Main Always block in UDP transform

    शून्य दर्शय(AstInitial* nodep) override {
        VL_RESTORER(m_अन्तर्_Initial);
        यदि (m_primp) m_अन्तर्_Initial = सत्य;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_नियोजय* nodep) override {
        यदि (m_अन्तर्_Initial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_isFirstOutput);
        VL_RESTORER_CLEAR(m_निवेशः_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = शून्यसूचकः;
        m_primp = nodep;
        m_isFirstOutput = असत्य;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        यदि (m_primp) {
            यदि (nodep->isIO()) {
                यदि (nodep->is_निवेशः()) {
                    m_निवेशः_Vars.प्रक्षिप_पृष्ठभागः(nodep);
                } अन्यथा {
                    m_outputVars.प्रक्षिप_पृष्ठभागः(nodep);
                }
                यदि ((m_निवेशः_Vars.आकार() == 0) && (m_outputVars.आकार() == 1)) {
                    m_isFirstOutput = सत्य;
                }
            }
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_Udp_सारणी* nodep) override {
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        यदि (m_outputVars.आकार() != 1) {
            m_outputVars.पृष्ठभागः()->v3error(
                m_outputVars.आकार()
                << " output ports for UDP table, there must be one output port");
        }
        यदि (!m_isFirstOutput && m_outputVars.आकार()) {
            m_निवेशः_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_o_क्षेत्रम्_Varp = m_outputVars[0];

        m_alwaysBlockp = नूतन AstAlways{fl, VAlwaysKwd::ALWAYS, शून्यसूचकः, शून्यसूचकः};
        fl->warnOff(V3_Error_संहिता::LATCH, सत्य);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(Ast_Udp_सारणी_पङ्क्तिः* nodep) override {
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        यदि (!nodep->udpIsCombo() && !m_o_क्षेत्रम्_Varp->isBitLogic()) {
            m_o_क्षेत्रम्_Varp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        यदि (nodep->udpIsCombo() && m_o_क्षेत्रम्_Varp->isBitLogic()) {
            m_o_क्षेत्रम्_Varp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_ग्रन्थिः* iNodep = nodep->iFieldsp();
        Ast_ग्रन्थिः* oNodep = nodep->oFieldsp();
        Ast_Sen_वृक्षः* edgetrigp = शून्यसूचकः;

        AstLogAnd* logandp = नूतन AstLogAnd{fl, नूतन AstConst{fl, AstConst::BitTrue{}},
                                           नूतन AstConst{fl, AstConst::BitTrue{}}};

        प्रति (AstVar* ध्रुव varp : m_निवेशः_Vars) {
            यदि (!iNodep) भङ्ग;
            यदि (Ast_Udp_सारणी_पङ्क्तिः_Val* linevalp = VN_CAST(iNodep, Udp_सारणी_पङ्क्तिः_Val)) {
                string valName = linevalp->name();
                यदि (isEdgeTrig(valName)) {
                    यदि (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    यदि (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(प्रक्षिप_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = नूतन Ast_Sen_वृक्षः{
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
                    यदि (edgetrigp) VL_DO_DANGLING(प्रक्षिप_Deletep(edgetrigp), edgetrigp);
                    यदि (logandp) VL_DO_DANGLING(प्रक्षिप_Deletep(logandp), logandp);
                    प्रत्यागम;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        प्रति (ध्रुव Ast_ग्रन्थिः* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        यदि (inputvars != m_निवेशः_Vars.आकार()) {
            nodep->v3error("Incorrect number of input values, expected " << m_निवेशः_Vars.आकार()
                                                                         << ", got " << inputvars);
        }

        string ध्रुव oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        यदि (oValName == "-") {
            यदि (edgetrigp) VL_DO_DANGLING(प्रक्षिप_Deletep(edgetrigp), edgetrigp);
            यदि (logandp) VL_DO_DANGLING(प्रक्षिप_Deletep(logandp), logandp);
            प्रत्यागम;
        }

        यदि (!nodep->udpIsCombo()) {
            यदि (oNodep->name() == "0") {
                logandp = नूतन AstLogAnd{
                    fl, logandp,
                    नूतन AstLogNot{fl, नूतन AstVarRef{fl, m_o_क्षेत्रम्_Varp, VAccess::READ}}};
            } अन्यथा यदि (oNodep->name() == "1") {
                logandp
                    = नूतन AstLogAnd{fl, logandp, नूतन AstVarRef{fl, m_o_क्षेत्रम्_Varp, VAccess::READ}};
            } अन्यथा यदि (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                यदि (edgetrigp) VL_DO_DANGLING(प्रक्षिप_Deletep(edgetrigp), edgetrigp);
                यदि (logandp) VL_DO_DANGLING(प्रक्षिप_Deletep(logandp), logandp);
                प्रत्यागम;
            }
        }

        fl->warnOff(V3_Error_संहिता::LATCH, सत्य);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_ग्रन्थिः_Stmt* ध्रुव assignp
            = new_Output_नियोजय(fl, get_Output_अङ्कः(nodep, oValName), edgetrigp != शून्यसूचकः);
        AstIf* ध्रुव ifp = नूतन AstIf{fl, logandp, assignp};
        यदि (nodep->udpIsCombo()) {
            यदि (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->सङ्कलय_Stmtsp(ifp);
            यदि (edgetrigp) VL_DO_DANGLING(प्रक्षिप_Deletep(edgetrigp), edgetrigp);
            प्रत्यागम;
        }
        यदि (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->सङ्कलय_अग्र(नूतन AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }
    शून्य दर्शय(AstLogAnd* nodep) override { iterateChildren(nodep); }
    शून्य दर्शय(AstLogNot* nodep) override { iterateChildren(nodep); }
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
        यदि (valName.आकार() == 2) {
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
    Ast_ग्रन्थिः_Stmt* new_Output_नियोजय(File_पङ्क्तिः* fl, ध्रुव V3_सङ्ख्या& बहिर्गमनम्_अङ्कः, तार्किक isDelayed) ध्रुव {
        AstVarRef* ध्रुव lhsp = नूतन AstVarRef{fl, m_o_क्षेत्रम्_Varp, VAccess::WRITE};
        AstConst* ध्रुव rhsp = नूतन AstConst{fl, बहिर्गमनम्_अङ्कः};
        यदि (isDelayed) प्रत्यागम नूतन Ast_नियोजय_Dly{fl, lhsp, rhsp};
        प्रत्यागम नूतन Ast_नियोजय{fl, lhsp, rhsp};
    }
    स्थिर V3_सङ्ख्या get_Output_अङ्कः(Ast_ग्रन्थिः* nodep, ध्रुव std::string& क्षेत्रम्_नामानि) {
        V3_सङ्ख्या output_अङ्कः{nodep, 1};
        यदि (क्षेत्रम्_नामानि == "0") {
            output_अङ्कः.समुच्चयः_Bit(0, 0);
        } अन्यथा यदि (क्षेत्रम्_नामानि == "1") {
            output_अङ्कः.समुच्चयः_Bit(0, 1);
        } अन्यथा {
            output_अङ्कः.समुच्चयः_Bit(0, 'x');
        }
        प्रत्यागम output_अङ्कः;
    }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = स्वभाव;
};

शून्य V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ध्रुव UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("udp", 0, dump_वृक्षः_Either_Level() >= 3);
}
