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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Udp.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ UdpVisitor final : ਜਨਤਕ VNVisitor {
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Initial = ਝੂਠ;  // Is inside of an initial block
    AstVar* m_o_ਖੇਤਰ_Varp = ਖਾਲੀ_ਸੂਚਕ;  // Output filed var of table line
    std::ਸਦਿਸ਼<AstVar*> m_ਨਿਵੇਸ਼_Vars;  // All the input vars in the AstPrimitive
    std::ਸਦਿਸ਼<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = ਖਾਲੀ_ਸੂਚਕ;  // The current primitive
    ਤਾਰਕਿਕ m_is_ਪਹਿਲਾ_Output = ਝੂਠ;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = ਖਾਲੀ_ਸੂਚਕ;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = ਖਾਲੀ_ਸੂਚਕ;  // Main Always block in UDP transform

    ਖਾਲੀ ਦੇਖੋ(AstInitial* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Initial);
        ਜੇ (m_primp) m_ਵਿੱਚ_Initial = ਸੱਚ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ* nodep) override {
        ਜੇ (m_ਵਿੱਚ_Initial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_is_ਪਹਿਲਾ_Output);
        VL_RESTORER_CLEAR(m_ਨਿਵੇਸ਼_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = ਖਾਲੀ_ਸੂਚਕ;
        m_primp = nodep;
        m_is_ਪਹਿਲਾ_Output = ਝੂਠ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        ਜੇ (m_primp) {
            ਜੇ (nodep->isIO()) {
                ਜੇ (nodep->is_ਨਿਵੇਸ਼()) {
                    m_ਨਿਵੇਸ਼_Vars.ਧੱਕੋ_ਪਿੱਠ(nodep);
                } ਨਹੀਂਤਾਂ {
                    m_outputVars.ਧੱਕੋ_ਪਿੱਠ(nodep);
                }
                ਜੇ ((m_ਨਿਵੇਸ਼_Vars.ਮਾਪ() == 0) && (m_outputVars.ਮਾਪ() == 1)) {
                    m_is_ਪਹਿਲਾ_Output = ਸੱਚ;
                }
            }
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Udp_ਸਾਰਣੀ* nodep) override {
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        ਜੇ (m_outputVars.ਮਾਪ() != 1) {
            m_outputVars.ਪਿੱਠ()->v3error(
                m_outputVars.ਮਾਪ()
                << " output ports for UDP table, there must be one output port");
        }
        ਜੇ (!m_is_ਪਹਿਲਾ_Output && m_outputVars.ਮਾਪ()) {
            m_ਨਿਵੇਸ਼_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_o_ਖੇਤਰ_Varp = m_outputVars[0];

        m_alwaysBlockp = ਨਵਾਂ AstAlways{fl, VAlwaysKwd::ALWAYS, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        fl->warnOff(V3_Error_ਕੋਡ::LATCH, ਸੱਚ);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Udp_ਸਾਰਣੀ_ਸਤਰ* nodep) override {
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        ਜੇ (!nodep->udpIsCombo() && !m_o_ਖੇਤਰ_Varp->is_ਦੋਆਧਾਰੀ_ਅੰਕ_Logic()) {
            m_o_ਖੇਤਰ_Varp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        ਜੇ (nodep->udpIsCombo() && m_o_ਖੇਤਰ_Varp->is_ਦੋਆਧਾਰੀ_ਅੰਕ_Logic()) {
            m_o_ਖੇਤਰ_Varp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_ਗੰਢ* iNodep = nodep->iFieldsp();
        Ast_ਗੰਢ* oNodep = nodep->oFieldsp();
        Ast_Sen_ਰੁੱਖ* edgetrigp = ਖਾਲੀ_ਸੂਚਕ;

        AstLogAnd* logandp = ਨਵਾਂ AstLogAnd{fl, ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}},
                                           ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}};

        ਲਈ (AstVar* ਅਚਲ varp : m_ਨਿਵੇਸ਼_Vars) {
            ਜੇ (!iNodep) ਰੋਕੋ;
            ਜੇ (Ast_Udp_ਸਾਰਣੀ_ਸਤਰ_Val* linevalp = VN_CAST(iNodep, Udp_ਸਾਰਣੀ_ਸਤਰ_Val)) {
                string valName = linevalp->name();
                ਜੇ (is_ਕਿਨਾਰਾ_Trig(valName)) {
                    ਜੇ (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    ਜੇ (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{
                        fl, ਨਵਾਂ Ast_Sen_ਵਸਤੂ{fl, VEdgeType::ET_BOTHEDGE,
                                           ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ}}};
                }
                ਜੇ (valName == "0" || valName == "f") {
                    logandp = ਨਵਾਂ AstLogAnd{
                        fl, logandp, ਨਵਾਂ AstLogNot{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ}}};
                } ਨਹੀਂਤਾਂ ਜੇ (valName == "1" || valName == "r") {
                    logandp = ਨਵਾਂ AstLogAnd{fl, logandp, ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ}};
                } ਨਹੀਂਤਾਂ ਜੇ (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    ਜੇ (edgetrigp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(edgetrigp), edgetrigp);
                    ਜੇ (logandp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(logandp), logandp);
                    ਵਾਪਸ;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        ਜੇ (inputvars != m_ਨਿਵੇਸ਼_Vars.ਮਾਪ()) {
            nodep->v3error("Incorrect number of input values, expected " << m_ਨਿਵੇਸ਼_Vars.ਮਾਪ()
                                                                         << ", got " << inputvars);
        }

        string ਅਚਲ oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        ਜੇ (oValName == "-") {
            ਜੇ (edgetrigp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(edgetrigp), edgetrigp);
            ਜੇ (logandp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(logandp), logandp);
            ਵਾਪਸ;
        }

        ਜੇ (!nodep->udpIsCombo()) {
            ਜੇ (oNodep->name() == "0") {
                logandp = ਨਵਾਂ AstLogAnd{
                    fl, logandp,
                    ਨਵਾਂ AstLogNot{fl, ਨਵਾਂ AstVarRef{fl, m_o_ਖੇਤਰ_Varp, VAccess::READ}}};
            } ਨਹੀਂਤਾਂ ਜੇ (oNodep->name() == "1") {
                logandp
                    = ਨਵਾਂ AstLogAnd{fl, logandp, ਨਵਾਂ AstVarRef{fl, m_o_ਖੇਤਰ_Varp, VAccess::READ}};
            } ਨਹੀਂਤਾਂ ਜੇ (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                ਜੇ (edgetrigp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(edgetrigp), edgetrigp);
                ਜੇ (logandp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(logandp), logandp);
                ਵਾਪਸ;
            }
        }

        fl->warnOff(V3_Error_ਕੋਡ::LATCH, ਸੱਚ);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_ਗੰਢ_Stmt* ਅਚਲ assignp
            = new_Output_ਸੌਂਪੋ(fl, get_Output_ਅੰਕ(nodep, oValName), edgetrigp != ਖਾਲੀ_ਸੂਚਕ);
        AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{fl, logandp, assignp};
        ਜੇ (nodep->udpIsCombo()) {
            ਜੇ (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->ਜੋੜੋ_Stmtsp(ifp);
            ਜੇ (edgetrigp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(edgetrigp), edgetrigp);
            ਵਾਪਸ;
        }
        ਜੇ (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstLogAnd* nodep) override { iterateChildren(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    ਸਥਿਰ ਤਾਰਕਿਕ is_ਕਿਨਾਰਾ_Trig(std::string& valName) {
        ਜੇ (valName == "x" || valName == "X") ਵਾਪਸ ਝੂਠ;
        ਜੇ (valName == "*") ਵਾਪਸ ਸੱਚ;
        ਜੇ (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (valName.ਮਾਪ() == 2) {
            ਜੇ (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            ਨਹੀਂਤਾਂ ਜੇ (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            ਨਹੀਂਤਾਂ ਜੇ (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        ਵਾਪਸ ਝੂਠ;
    }
    ਸਥਿਰ ਤਾਰਕਿਕ isCombOutputSig(ਅਚਲ std::string& valName) {
        ਵਾਪਸ (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    ਸਥਿਰ ਤਾਰਕਿਕ isSequentOutputSig(ਅਚਲ std::string& valName) {
        ਵਾਪਸ (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    Ast_ਗੰਢ_Stmt* new_Output_ਸੌਂਪੋ(File_ਸਤਰ* fl, ਅਚਲ V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ_ਅੰਕ, ਤਾਰਕਿਕ isDelayed) ਅਚਲ {
        AstVarRef* ਅਚਲ lhsp = ਨਵਾਂ AstVarRef{fl, m_o_ਖੇਤਰ_Varp, VAccess::WRITE};
        AstConst* ਅਚਲ rhsp = ਨਵਾਂ AstConst{fl, ਬਾਹਰਲਾ_ਅੰਕ};
        ਜੇ (isDelayed) ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{fl, lhsp, rhsp};
        ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, lhsp, rhsp};
    }
    ਸਥਿਰ V3_ਸੰਖਿਆ get_Output_ਅੰਕ(Ast_ਗੰਢ* nodep, ਅਚਲ std::string& ਖੇਤਰ_ਨਾਂ) {
        V3_ਸੰਖਿਆ output_ਅੰਕ{nodep, 1};
        ਜੇ (ਖੇਤਰ_ਨਾਂ == "0") {
            output_ਅੰਕ.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(0, 0);
        } ਨਹੀਂਤਾਂ ਜੇ (ਖੇਤਰ_ਨਾਂ == "1") {
            output_ਅੰਕ.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(0, 1);
        } ਨਹੀਂਤਾਂ {
            output_ਅੰਕ.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(0, 'x');
        }
        ਵਾਪਸ output_ਅੰਕ;
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = ਮੂਲ;
};

ਖਾਲੀ V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ਅਚਲ UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("udp", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
