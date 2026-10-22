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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Udp.h"

#અન્તર્ભાવ "V3Error.h"

#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

વર્ગ UdpVisitor final : જાહેર VNVisitor {
    તાર્કિક m_inInitial = ખોટું;  // Is inside of an initial block
    AstVar* m_oFieldVarp = nullptr;  // Output filed var of table line
    std::vector<AstVar*> m_આગત_Vars;  // All the input vars in the AstPrimitive
    std::vector<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = nullptr;  // The current primitive
    તાર્કિક m_isFirstOutput = ખોટું;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = nullptr;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = nullptr;  // Main Always block in UDP transform

    શૂન્ય visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        જો (m_primp) m_inInitial = સાચું;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstAssign* nodep) override {
        જો (m_inInitial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_isFirstOutput);
        VL_RESTORER_CLEAR(m_આગત_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = nullptr;
        m_primp = nodep;
        m_isFirstOutput = ખોટું;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        જો (m_primp) {
            જો (nodep->isIO()) {
                જો (nodep->is_આગત()) {
                    m_આગત_Vars.ધકેલો_back(nodep);
                } નહીંતર {
                    m_outputVars.ધકેલો_back(nodep);
                }
                જો ((m_આગત_Vars.size() == 0) && (m_outputVars.size() == 1)) {
                    m_isFirstOutput = સાચું;
                }
            }
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstUdpTable* nodep) override {
        File_લીટી* અચલ fl = nodep->fileline();
        જો (m_outputVars.size() != 1) {
            m_outputVars.back()->v3error(
                m_outputVars.size()
                << " output ports for UDP table, there must be one output port");
        }
        જો (!m_isFirstOutput && m_outputVars.size()) {
            m_આગત_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_oFieldVarp = m_outputVars[0];

        m_alwaysBlockp = નવું AstAlways{fl, VAlwaysKwd::ALWAYS, nullptr, nullptr};
        fl->warnOff(V3ErrorCode::LATCH, સાચું);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(Ast_Udp_Table_લીટી* nodep) override {
        File_લીટી* અચલ fl = nodep->fileline();
        જો (!nodep->udpIsCombo() && !m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        જો (nodep->udpIsCombo() && m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_ગાંઠ* iNodep = nodep->iFieldsp();
        Ast_ગાંઠ* oNodep = nodep->oFieldsp();
        Ast_Sen_વૃક્ષ* edgetrigp = nullptr;

        AstLogAnd* logandp = નવું AstLogAnd{fl, નવું AstConst{fl, AstConst::BitTrue{}},
                                           નવું AstConst{fl, AstConst::BitTrue{}}};

        માટે (AstVar* અચલ varp : m_આગત_Vars) {
            જો (!iNodep) તોડો;
            જો (Ast_Udp_Table_લીટી_Val* linevalp = VN_CAST(iNodep, Udp_Table_લીટી_Val)) {
                string valName = linevalp->name();
                જો (isEdgeTrig(valName)) {
                    જો (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    જો (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(ધકેલો_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = નવું Ast_Sen_વૃક્ષ{
                        fl, નવું AstSenItem{fl, VEdgeType::ET_BOTHEDGE,
                                           નવું AstVarRef{fl, varp, VAccess::READ}}};
                }
                જો (valName == "0" || valName == "f") {
                    logandp = નવું AstLogAnd{
                        fl, logandp, નવું AstLogNot{fl, નવું AstVarRef{fl, varp, VAccess::READ}}};
                } નહીંતર જો (valName == "1" || valName == "r") {
                    logandp = નવું AstLogAnd{fl, logandp, નવું AstVarRef{fl, varp, VAccess::READ}};
                } નહીંતર જો (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    જો (edgetrigp) VL_DO_DANGLING(ધકેલો_Deletep(edgetrigp), edgetrigp);
                    જો (logandp) VL_DO_DANGLING(ધકેલો_Deletep(logandp), logandp);
                    પાછા;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        માટે (અચલ Ast_ગાંઠ* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        જો (inputvars != m_આગત_Vars.size()) {
            nodep->v3error("Incorrect number of input values, expected " << m_આગત_Vars.size()
                                                                         << ", got " << inputvars);
        }

        string અચલ oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        જો (oValName == "-") {
            જો (edgetrigp) VL_DO_DANGLING(ધકેલો_Deletep(edgetrigp), edgetrigp);
            જો (logandp) VL_DO_DANGLING(ધકેલો_Deletep(logandp), logandp);
            પાછા;
        }

        જો (!nodep->udpIsCombo()) {
            જો (oNodep->name() == "0") {
                logandp = નવું AstLogAnd{
                    fl, logandp,
                    નવું AstLogNot{fl, નવું AstVarRef{fl, m_oFieldVarp, VAccess::READ}}};
            } નહીંતર જો (oNodep->name() == "1") {
                logandp
                    = નવું AstLogAnd{fl, logandp, નવું AstVarRef{fl, m_oFieldVarp, VAccess::READ}};
            } નહીંતર જો (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                જો (edgetrigp) VL_DO_DANGLING(ધકેલો_Deletep(edgetrigp), edgetrigp);
                જો (logandp) VL_DO_DANGLING(ધકેલો_Deletep(logandp), logandp);
                પાછા;
            }
        }

        fl->warnOff(V3ErrorCode::LATCH, સાચું);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_ગાંઠ_Stmt* અચલ assignp
            = newOutputAssign(fl, get_Output_અંક(nodep, oValName), edgetrigp != nullptr);
        AstIf* અચલ ifp = નવું AstIf{fl, logandp, assignp};
        જો (nodep->udpIsCombo()) {
            જો (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->addStmtsp(ifp);
            જો (edgetrigp) VL_DO_DANGLING(ધકેલો_Deletep(edgetrigp), edgetrigp);
            પાછા;
        }
        જો (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->addNext(નવું AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }
    શૂન્ય visit(AstLogAnd* nodep) override { iterateChildren(nodep); }
    શૂન્ય visit(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    સ્થિર તાર્કિક isEdgeTrig(std::string& valName) {
        જો (valName == "x" || valName == "X") પાછા ખોટું;
        જો (valName == "*") પાછા સાચું;
        જો (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            પાછા સાચું;
        }
        જો (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            પાછા સાચું;
        }
        જો (valName.size() == 2) {
            જો (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            નહીંતર જો (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            નહીંતર જો (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            પાછા સાચું;
        }
        જો (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        પાછા ખોટું;
    }
    સ્થિર તાર્કિક isCombOutputSig(અચલ std::string& valName) {
        પાછા (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    સ્થિર તાર્કિક isSequentOutputSig(અચલ std::string& valName) {
        પાછા (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    Ast_ગાંઠ_Stmt* newOutputAssign(File_લીટી* fl, અચલ V3_સંખ્યા& out_અંક, તાર્કિક isDelayed) અચલ {
        AstVarRef* અચલ lhsp = નવું AstVarRef{fl, m_oFieldVarp, VAccess::WRITE};
        AstConst* અચલ rhsp = નવું AstConst{fl, out_અંક};
        જો (isDelayed) પાછા નવું AstAssignDly{fl, lhsp, rhsp};
        પાછા નવું AstAssign{fl, lhsp, rhsp};
    }
    સ્થિર V3_સંખ્યા get_Output_અંક(Ast_ગાંઠ* nodep, અચલ std::string& fieldNames) {
        V3_સંખ્યા output_અંક{nodep, 1};
        જો (fieldNames == "0") {
            output_અંક.setBit(0, 0);
        } નહીંતર જો (fieldNames == "1") {
            output_અંક.setBit(0, 1);
        } નહીંતર {
            output_અંક.setBit(0, 'x');
        }
        પાછા output_અંક;
    }

જાહેર:
    // CONSTRUCTORS
    explicit UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = મૂળભૂત;
};

શૂન્ય V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { અચલ UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("udp", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
