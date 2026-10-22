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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Udp.h"

#অন্তর্ভাব "V3Error.h"

#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

বর্গ UdpVisitor final : সার্বজনিক VNVisitor {
    তার্কিক m_inInitial = অসত্য;  // Is inside of an initial block
    AstVar* m_oFieldVarp = nullptr;  // Output filed var of table line
    std::vector<AstVar*> m_প্রবেশ_Vars;  // All the input vars in the AstPrimitive
    std::vector<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = nullptr;  // The current primitive
    তার্কিক m_isFirstOutput = অসত্য;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = nullptr;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = nullptr;  // Main Always block in UDP transform

    শূন্য visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        যদি (m_primp) m_inInitial = সত্য;
        iterateChildren(nodep);
    }
    শূন্য visit(AstAssign* nodep) override {
        যদি (m_inInitial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    শূন্য visit(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_isFirstOutput);
        VL_RESTORER_CLEAR(m_প্রবেশ_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = nullptr;
        m_primp = nodep;
        m_isFirstOutput = অসত্য;
        iterateChildren(nodep);
    }
    শূন্য visit(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        যদি (m_primp) {
            যদি (nodep->isIO()) {
                যদি (nodep->is_প্রবেশ()) {
                    m_প্রবেশ_Vars.ঠেলো_back(nodep);
                } অন্যথা {
                    m_outputVars.ঠেলো_back(nodep);
                }
                যদি ((m_প্রবেশ_Vars.size() == 0) && (m_outputVars.size() == 1)) {
                    m_isFirstOutput = সত্য;
                }
            }
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstUdpTable* nodep) override {
        File_সারি* ধ্রুব fl = nodep->fileline();
        যদি (m_outputVars.size() != 1) {
            m_outputVars.back()->v3error(
                m_outputVars.size()
                << " output ports for UDP table, there must be one output port");
        }
        যদি (!m_isFirstOutput && m_outputVars.size()) {
            m_প্রবেশ_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_oFieldVarp = m_outputVars[0];

        m_alwaysBlockp = নূতন AstAlways{fl, VAlwaysKwd::ALWAYS, nullptr, nullptr};
        fl->warnOff(V3ErrorCode::LATCH, সত্য);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(Ast_Udp_Table_সারি* nodep) override {
        File_সারি* ধ্রুব fl = nodep->fileline();
        যদি (!nodep->udpIsCombo() && !m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        যদি (nodep->udpIsCombo() && m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_গ্রন্থি* iNodep = nodep->iFieldsp();
        Ast_গ্রন্থি* oNodep = nodep->oFieldsp();
        Ast_Sen_বৃক্ষ* edgetrigp = nullptr;

        AstLogAnd* logandp = নূতন AstLogAnd{fl, নূতন AstConst{fl, AstConst::BitTrue{}},
                                           নূতন AstConst{fl, AstConst::BitTrue{}}};

        প্রতি (AstVar* ধ্রুব varp : m_প্রবেশ_Vars) {
            যদি (!iNodep) ভঙ্গ;
            যদি (Ast_Udp_Table_সারি_Val* linevalp = VN_CAST(iNodep, Udp_Table_সারি_Val)) {
                string valName = linevalp->name();
                যদি (isEdgeTrig(valName)) {
                    যদি (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    যদি (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(ঠেলো_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = নূতন Ast_Sen_বৃক্ষ{
                        fl, নূতন AstSenItem{fl, VEdgeType::ET_BOTHEDGE,
                                           নূতন AstVarRef{fl, varp, VAccess::READ}}};
                }
                যদি (valName == "0" || valName == "f") {
                    logandp = নূতন AstLogAnd{
                        fl, logandp, নূতন AstLogNot{fl, নূতন AstVarRef{fl, varp, VAccess::READ}}};
                } অন্যথা যদি (valName == "1" || valName == "r") {
                    logandp = নূতন AstLogAnd{fl, logandp, নূতন AstVarRef{fl, varp, VAccess::READ}};
                } অন্যথা যদি (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    যদি (edgetrigp) VL_DO_DANGLING(ঠেলো_Deletep(edgetrigp), edgetrigp);
                    যদি (logandp) VL_DO_DANGLING(ঠেলো_Deletep(logandp), logandp);
                    প্রত্যাগম;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        প্রতি (ধ্রুব Ast_গ্রন্থি* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        যদি (inputvars != m_প্রবেশ_Vars.size()) {
            nodep->v3error("Incorrect number of input values, expected " << m_প্রবেশ_Vars.size()
                                                                         << ", got " << inputvars);
        }

        string ধ্রুব oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        যদি (oValName == "-") {
            যদি (edgetrigp) VL_DO_DANGLING(ঠেলো_Deletep(edgetrigp), edgetrigp);
            যদি (logandp) VL_DO_DANGLING(ঠেলো_Deletep(logandp), logandp);
            প্রত্যাগম;
        }

        যদি (!nodep->udpIsCombo()) {
            যদি (oNodep->name() == "0") {
                logandp = নূতন AstLogAnd{
                    fl, logandp,
                    নূতন AstLogNot{fl, নূতন AstVarRef{fl, m_oFieldVarp, VAccess::READ}}};
            } অন্যথা যদি (oNodep->name() == "1") {
                logandp
                    = নূতন AstLogAnd{fl, logandp, নূতন AstVarRef{fl, m_oFieldVarp, VAccess::READ}};
            } অন্যথা যদি (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                যদি (edgetrigp) VL_DO_DANGLING(ঠেলো_Deletep(edgetrigp), edgetrigp);
                যদি (logandp) VL_DO_DANGLING(ঠেলো_Deletep(logandp), logandp);
                প্রত্যাগম;
            }
        }

        fl->warnOff(V3ErrorCode::LATCH, সত্য);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_গ্রন্থি_Stmt* ধ্রুব assignp
            = newOutputAssign(fl, get_Output_অঙ্ক(nodep, oValName), edgetrigp != nullptr);
        AstIf* ধ্রুব ifp = নূতন AstIf{fl, logandp, assignp};
        যদি (nodep->udpIsCombo()) {
            যদি (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->addStmtsp(ifp);
            যদি (edgetrigp) VL_DO_DANGLING(ঠেলো_Deletep(edgetrigp), edgetrigp);
            প্রত্যাগম;
        }
        যদি (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->addNext(নূতন AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }
    শূন্য visit(AstLogAnd* nodep) override { iterateChildren(nodep); }
    শূন্য visit(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    স্থির তার্কিক isEdgeTrig(std::string& valName) {
        যদি (valName == "x" || valName == "X") প্রত্যাগম অসত্য;
        যদি (valName == "*") প্রত্যাগম সত্য;
        যদি (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            প্রত্যাগম সত্য;
        }
        যদি (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            প্রত্যাগম সত্য;
        }
        যদি (valName.size() == 2) {
            যদি (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            অন্যথা যদি (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            অন্যথা যদি (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            প্রত্যাগম সত্য;
        }
        যদি (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        প্রত্যাগম অসত্য;
    }
    স্থির তার্কিক isCombOutputSig(ধ্রুব std::string& valName) {
        প্রত্যাগম (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    স্থির তার্কিক isSequentOutputSig(ধ্রুব std::string& valName) {
        প্রত্যাগম (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    Ast_গ্রন্থি_Stmt* newOutputAssign(File_সারি* fl, ধ্রুব V3_সংখ্যা& out_অঙ্ক, তার্কিক isDelayed) ধ্রুব {
        AstVarRef* ধ্রুব lhsp = নূতন AstVarRef{fl, m_oFieldVarp, VAccess::WRITE};
        AstConst* ধ্রুব rhsp = নূতন AstConst{fl, out_অঙ্ক};
        যদি (isDelayed) প্রত্যাগম নূতন AstAssignDly{fl, lhsp, rhsp};
        প্রত্যাগম নূতন AstAssign{fl, lhsp, rhsp};
    }
    স্থির V3_সংখ্যা get_Output_অঙ্ক(Ast_গ্রন্থি* nodep, ধ্রুব std::string& fieldNames) {
        V3_সংখ্যা output_অঙ্ক{nodep, 1};
        যদি (fieldNames == "0") {
            output_অঙ্ক.setBit(0, 0);
        } অন্যথা যদি (fieldNames == "1") {
            output_অঙ্ক.setBit(0, 1);
        } অন্যথা {
            output_অঙ্ক.setBit(0, 'x');
        }
        প্রত্যাগম output_অঙ্ক;
    }

সার্বজনিক:
    // CONSTRUCTORS
    explicit UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = স্বভাব;
};

শূন্য V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ধ্রুব UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("udp", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
