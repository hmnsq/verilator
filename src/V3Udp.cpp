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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Udp.h"

#അന്തര്ഭാവ "V3Error.h"

#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

വർഗ്ഗം UdpVisitor final : പൊതു VNVisitor {
    തർക്കം m_inInitial = അസത്യം;  // Is inside of an initial block
    AstVar* m_oFieldVarp = nullptr;  // Output filed var of table line
    std::vector<AstVar*> m_നിവേശം_Vars;  // All the input vars in the AstPrimitive
    std::vector<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = nullptr;  // The current primitive
    തർക്കം m_isFirstOutput = അസത്യം;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = nullptr;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = nullptr;  // Main Always block in UDP transform

    ശൂന്യം visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        എങ്കിൽ (m_primp) m_inInitial = സത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstAssign* nodep) override {
        എങ്കിൽ (m_inInitial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_isFirstOutput);
        VL_RESTORER_CLEAR(m_നിവേശം_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = nullptr;
        m_primp = nodep;
        m_isFirstOutput = അസത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        എങ്കിൽ (m_primp) {
            എങ്കിൽ (nodep->isIO()) {
                എങ്കിൽ (nodep->is_നിവേശം()) {
                    m_നിവേശം_Vars.തള്ളുക_back(nodep);
                } അല്ലെങ്കിൽ {
                    m_outputVars.തള്ളുക_back(nodep);
                }
                എങ്കിൽ ((m_നിവേശം_Vars.size() == 0) && (m_outputVars.size() == 1)) {
                    m_isFirstOutput = സത്യം;
                }
            }
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstUdpTable* nodep) override {
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        എങ്കിൽ (m_outputVars.size() != 1) {
            m_outputVars.back()->v3error(
                m_outputVars.size()
                << " output ports for UDP table, there must be one output port");
        }
        എങ്കിൽ (!m_isFirstOutput && m_outputVars.size()) {
            m_നിവേശം_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_oFieldVarp = m_outputVars[0];

        m_alwaysBlockp = പുതിയ AstAlways{fl, VAlwaysKwd::ALWAYS, nullptr, nullptr};
        fl->warnOff(V3ErrorCode::LATCH, സത്യം);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(Ast_Udp_Table_വരി* nodep) override {
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        എങ്കിൽ (!nodep->udpIsCombo() && !m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        എങ്കിൽ (nodep->udpIsCombo() && m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_കണ്ണി* iNodep = nodep->iFieldsp();
        Ast_കണ്ണി* oNodep = nodep->oFieldsp();
        Ast_Sen_വൃക്ഷം* edgetrigp = nullptr;

        AstLogAnd* logandp = പുതിയ AstLogAnd{fl, പുതിയ AstConst{fl, AstConst::BitTrue{}},
                                           പുതിയ AstConst{fl, AstConst::BitTrue{}}};

        വേണ്ടി (AstVar* സ്ഥിരം varp : m_നിവേശം_Vars) {
            എങ്കിൽ (!iNodep) നിർത്തുക;
            എങ്കിൽ (Ast_Udp_Table_വരി_Val* linevalp = VN_CAST(iNodep, Udp_Table_വരി_Val)) {
                string valName = linevalp->name();
                എങ്കിൽ (isEdgeTrig(valName)) {
                    എങ്കിൽ (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    എങ്കിൽ (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(തള്ളുക_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = പുതിയ Ast_Sen_വൃക്ഷം{
                        fl, പുതിയ AstSenItem{fl, VEdgeType::ET_BOTHEDGE,
                                           പുതിയ AstVarRef{fl, varp, VAccess::READ}}};
                }
                എങ്കിൽ (valName == "0" || valName == "f") {
                    logandp = പുതിയ AstLogAnd{
                        fl, logandp, പുതിയ AstLogNot{fl, പുതിയ AstVarRef{fl, varp, VAccess::READ}}};
                } അല്ലെങ്കിൽ എങ്കിൽ (valName == "1" || valName == "r") {
                    logandp = പുതിയ AstLogAnd{fl, logandp, പുതിയ AstVarRef{fl, varp, VAccess::READ}};
                } അല്ലെങ്കിൽ എങ്കിൽ (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    എങ്കിൽ (edgetrigp) VL_DO_DANGLING(തള്ളുക_Deletep(edgetrigp), edgetrigp);
                    എങ്കിൽ (logandp) VL_DO_DANGLING(തള്ളുക_Deletep(logandp), logandp);
                    തിരികെ;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        എങ്കിൽ (inputvars != m_നിവേശം_Vars.size()) {
            nodep->v3error("Incorrect number of input values, expected " << m_നിവേശം_Vars.size()
                                                                         << ", got " << inputvars);
        }

        string സ്ഥിരം oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        എങ്കിൽ (oValName == "-") {
            എങ്കിൽ (edgetrigp) VL_DO_DANGLING(തള്ളുക_Deletep(edgetrigp), edgetrigp);
            എങ്കിൽ (logandp) VL_DO_DANGLING(തള്ളുക_Deletep(logandp), logandp);
            തിരികെ;
        }

        എങ്കിൽ (!nodep->udpIsCombo()) {
            എങ്കിൽ (oNodep->name() == "0") {
                logandp = പുതിയ AstLogAnd{
                    fl, logandp,
                    പുതിയ AstLogNot{fl, പുതിയ AstVarRef{fl, m_oFieldVarp, VAccess::READ}}};
            } അല്ലെങ്കിൽ എങ്കിൽ (oNodep->name() == "1") {
                logandp
                    = പുതിയ AstLogAnd{fl, logandp, പുതിയ AstVarRef{fl, m_oFieldVarp, VAccess::READ}};
            } അല്ലെങ്കിൽ എങ്കിൽ (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                എങ്കിൽ (edgetrigp) VL_DO_DANGLING(തള്ളുക_Deletep(edgetrigp), edgetrigp);
                എങ്കിൽ (logandp) VL_DO_DANGLING(തള്ളുക_Deletep(logandp), logandp);
                തിരികെ;
            }
        }

        fl->warnOff(V3ErrorCode::LATCH, സത്യം);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_കണ്ണി_Stmt* സ്ഥിരം assignp
            = newOutputAssign(fl, get_Output_അക്കം(nodep, oValName), edgetrigp != nullptr);
        AstIf* സ്ഥിരം ifp = പുതിയ AstIf{fl, logandp, assignp};
        എങ്കിൽ (nodep->udpIsCombo()) {
            എങ്കിൽ (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->addStmtsp(ifp);
            എങ്കിൽ (edgetrigp) VL_DO_DANGLING(തള്ളുക_Deletep(edgetrigp), edgetrigp);
            തിരികെ;
        }
        എങ്കിൽ (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->addNext(പുതിയ AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }
    ശൂന്യം visit(AstLogAnd* nodep) override { iterateChildren(nodep); }
    ശൂന്യം visit(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    സ്ഥിതം തർക്കം isEdgeTrig(std::string& valName) {
        എങ്കിൽ (valName == "x" || valName == "X") തിരികെ അസത്യം;
        എങ്കിൽ (valName == "*") തിരികെ സത്യം;
        എങ്കിൽ (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            തിരികെ സത്യം;
        }
        എങ്കിൽ (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            തിരികെ സത്യം;
        }
        എങ്കിൽ (valName.size() == 2) {
            എങ്കിൽ (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            അല്ലെങ്കിൽ എങ്കിൽ (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            അല്ലെങ്കിൽ എങ്കിൽ (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            തിരികെ സത്യം;
        }
        എങ്കിൽ (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        തിരികെ അസത്യം;
    }
    സ്ഥിതം തർക്കം isCombOutputSig(സ്ഥിരം std::string& valName) {
        തിരികെ (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    സ്ഥിതം തർക്കം isSequentOutputSig(സ്ഥിരം std::string& valName) {
        തിരികെ (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    Ast_കണ്ണി_Stmt* newOutputAssign(File_വരി* fl, സ്ഥിരം V3_സംഖ്യ& out_അക്കം, തർക്കം isDelayed) സ്ഥിരം {
        AstVarRef* സ്ഥിരം lhsp = പുതിയ AstVarRef{fl, m_oFieldVarp, VAccess::WRITE};
        AstConst* സ്ഥിരം rhsp = പുതിയ AstConst{fl, out_അക്കം};
        എങ്കിൽ (isDelayed) തിരികെ പുതിയ AstAssignDly{fl, lhsp, rhsp};
        തിരികെ പുതിയ AstAssign{fl, lhsp, rhsp};
    }
    സ്ഥിതം V3_സംഖ്യ get_Output_അക്കം(Ast_കണ്ണി* nodep, സ്ഥിരം std::string& fieldNames) {
        V3_സംഖ്യ output_അക്കം{nodep, 1};
        എങ്കിൽ (fieldNames == "0") {
            output_അക്കം.setBit(0, 0);
        } അല്ലെങ്കിൽ എങ്കിൽ (fieldNames == "1") {
            output_അക്കം.setBit(0, 1);
        } അല്ലെങ്കിൽ {
            output_അക്കം.setBit(0, 'x');
        }
        തിരികെ output_അക്കം;
    }

പൊതു:
    // CONSTRUCTORS
    explicit UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = സ്ഥിരസ്ഥിതി;
};

ശൂന്യം V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { സ്ഥിരം UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("udp", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
