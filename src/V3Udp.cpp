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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Udp.h"

#ଅନ୍ତର୍ଭାଵ "V3Error.h"

#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ଶ୍ରେଣୀ UdpVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    ତାର୍କିକ m_inInitial = ମିଥ୍ୟା;  // Is inside of an initial block
    AstVar* m_oFieldVarp = nullptr;  // Output filed var of table line
    std::vector<AstVar*> m_ନିବେଶ_Vars;  // All the input vars in the AstPrimitive
    std::vector<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = nullptr;  // The current primitive
    ତାର୍କିକ m_isFirstOutput = ମିଥ୍ୟା;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = nullptr;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = nullptr;  // Main Always block in UDP transform

    ଶୂନ୍ୟ visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        ଯଦି (m_primp) m_inInitial = ସତ୍ୟ;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstAssign* nodep) override {
        ଯଦି (m_inInitial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_isFirstOutput);
        VL_RESTORER_CLEAR(m_ନିବେଶ_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = nullptr;
        m_primp = nodep;
        m_isFirstOutput = ମିଥ୍ୟା;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        ଯଦି (m_primp) {
            ଯଦି (nodep->isIO()) {
                ଯଦି (nodep->is_ନିବେଶ()) {
                    m_ନିବେଶ_Vars.ଠେଲ_back(nodep);
                } ଅନ୍ୟଥା {
                    m_outputVars.ଠେଲ_back(nodep);
                }
                ଯଦି ((m_ନିବେଶ_Vars.size() == 0) && (m_outputVars.size() == 1)) {
                    m_isFirstOutput = ସତ୍ୟ;
                }
            }
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstUdpTable* nodep) override {
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        ଯଦି (m_outputVars.size() != 1) {
            m_outputVars.back()->v3error(
                m_outputVars.size()
                << " output ports for UDP table, there must be one output port");
        }
        ଯଦି (!m_isFirstOutput && m_outputVars.size()) {
            m_ନିବେଶ_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_oFieldVarp = m_outputVars[0];

        m_alwaysBlockp = ନୂତନ AstAlways{fl, VAlwaysKwd::ALWAYS, nullptr, nullptr};
        fl->warnOff(V3ErrorCode::LATCH, ସତ୍ୟ);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_Udp_Table_ଧାଡ଼ି* nodep) override {
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        ଯଦି (!nodep->udpIsCombo() && !m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        ଯଦି (nodep->udpIsCombo() && m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_ଗଣ୍ଠି* iNodep = nodep->iFieldsp();
        Ast_ଗଣ୍ଠି* oNodep = nodep->oFieldsp();
        Ast_Sen_ବୃକ୍ଷ* edgetrigp = nullptr;

        AstLogAnd* logandp = ନୂତନ AstLogAnd{fl, ନୂତନ AstConst{fl, AstConst::BitTrue{}},
                                           ନୂତନ AstConst{fl, AstConst::BitTrue{}}};

        ପାଇଁ (AstVar* ଅଚଳ varp : m_ନିବେଶ_Vars) {
            ଯଦି (!iNodep) ଭାଙ୍ଗ;
            ଯଦି (Ast_Udp_Table_ଧାଡ଼ି_Val* linevalp = VN_CAST(iNodep, Udp_Table_ଧାଡ଼ି_Val)) {
                string valName = linevalp->name();
                ଯଦି (isEdgeTrig(valName)) {
                    ଯଦି (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    ଯଦି (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(ଠେଲ_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = ନୂତନ Ast_Sen_ବୃକ୍ଷ{
                        fl, ନୂତନ AstSenItem{fl, VEdgeType::ET_BOTHEDGE,
                                           ନୂତନ AstVarRef{fl, varp, VAccess::READ}}};
                }
                ଯଦି (valName == "0" || valName == "f") {
                    logandp = ନୂତନ AstLogAnd{
                        fl, logandp, ନୂତନ AstLogNot{fl, ନୂତନ AstVarRef{fl, varp, VAccess::READ}}};
                } ଅନ୍ୟଥା ଯଦି (valName == "1" || valName == "r") {
                    logandp = ନୂତନ AstLogAnd{fl, logandp, ନୂତନ AstVarRef{fl, varp, VAccess::READ}};
                } ଅନ୍ୟଥା ଯଦି (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    ଯଦି (edgetrigp) VL_DO_DANGLING(ଠେଲ_Deletep(edgetrigp), edgetrigp);
                    ଯଦି (logandp) VL_DO_DANGLING(ଠେଲ_Deletep(logandp), logandp);
                    ଫେରନ୍ତୁ;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        ଯଦି (inputvars != m_ନିବେଶ_Vars.size()) {
            nodep->v3error("Incorrect number of input values, expected " << m_ନିବେଶ_Vars.size()
                                                                         << ", got " << inputvars);
        }

        string ଅଚଳ oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        ଯଦି (oValName == "-") {
            ଯଦି (edgetrigp) VL_DO_DANGLING(ଠେଲ_Deletep(edgetrigp), edgetrigp);
            ଯଦି (logandp) VL_DO_DANGLING(ଠେଲ_Deletep(logandp), logandp);
            ଫେରନ୍ତୁ;
        }

        ଯଦି (!nodep->udpIsCombo()) {
            ଯଦି (oNodep->name() == "0") {
                logandp = ନୂତନ AstLogAnd{
                    fl, logandp,
                    ନୂତନ AstLogNot{fl, ନୂତନ AstVarRef{fl, m_oFieldVarp, VAccess::READ}}};
            } ଅନ୍ୟଥା ଯଦି (oNodep->name() == "1") {
                logandp
                    = ନୂତନ AstLogAnd{fl, logandp, ନୂତନ AstVarRef{fl, m_oFieldVarp, VAccess::READ}};
            } ଅନ୍ୟଥା ଯଦି (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                ଯଦି (edgetrigp) VL_DO_DANGLING(ଠେଲ_Deletep(edgetrigp), edgetrigp);
                ଯଦି (logandp) VL_DO_DANGLING(ଠେଲ_Deletep(logandp), logandp);
                ଫେରନ୍ତୁ;
            }
        }

        fl->warnOff(V3ErrorCode::LATCH, ସତ୍ୟ);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ assignp
            = newOutputAssign(fl, get_Output_ଅଙ୍କ(nodep, oValName), edgetrigp != nullptr);
        AstIf* ଅଚଳ ifp = ନୂତନ AstIf{fl, logandp, assignp};
        ଯଦି (nodep->udpIsCombo()) {
            ଯଦି (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->addStmtsp(ifp);
            ଯଦି (edgetrigp) VL_DO_DANGLING(ଠେଲ_Deletep(edgetrigp), edgetrigp);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->addNext(ନୂତନ AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }
    ଶୂନ୍ୟ visit(AstLogAnd* nodep) override { iterateChildren(nodep); }
    ଶୂନ୍ୟ visit(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    ସ୍ଥିର ତାର୍କିକ isEdgeTrig(std::string& valName) {
        ଯଦି (valName == "x" || valName == "X") ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (valName == "*") ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (valName.size() == 2) {
            ଯଦି (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            ଅନ୍ୟଥା ଯଦି (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            ଅନ୍ୟଥା ଯଦି (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ସ୍ଥିର ତାର୍କିକ isCombOutputSig(ଅଚଳ std::string& valName) {
        ଫେରନ୍ତୁ (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    ସ୍ଥିର ତାର୍କିକ isSequentOutputSig(ଅଚଳ std::string& valName) {
        ଫେରନ୍ତୁ (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    Ast_ଗଣ୍ଠି_Stmt* newOutputAssign(File_ଧାଡ଼ି* fl, ଅଚଳ V3_ସଂଖ୍ୟା& out_ଅଙ୍କ, ତାର୍କିକ isDelayed) ଅଚଳ {
        AstVarRef* ଅଚଳ lhsp = ନୂତନ AstVarRef{fl, m_oFieldVarp, VAccess::WRITE};
        AstConst* ଅଚଳ rhsp = ନୂତନ AstConst{fl, out_ଅଙ୍କ};
        ଯଦି (isDelayed) ଫେରନ୍ତୁ ନୂତନ AstAssignDly{fl, lhsp, rhsp};
        ଫେରନ୍ତୁ ନୂତନ AstAssign{fl, lhsp, rhsp};
    }
    ସ୍ଥିର V3_ସଂଖ୍ୟା get_Output_ଅଙ୍କ(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ std::string& fieldNames) {
        V3_ସଂଖ୍ୟା output_ଅଙ୍କ{nodep, 1};
        ଯଦି (fieldNames == "0") {
            output_ଅଙ୍କ.setBit(0, 0);
        } ଅନ୍ୟଥା ଯଦି (fieldNames == "1") {
            output_ଅଙ୍କ.setBit(0, 1);
        } ଅନ୍ୟଥା {
            output_ଅଙ୍କ.setBit(0, 'x');
        }
        ଫେରନ୍ତୁ output_ଅଙ୍କ;
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

ଶୂନ୍ୟ V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ଅଚଳ UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("udp", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
