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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Udp.h"

#ضمن "V3Error.h"

#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

صنف UdpVisitor final : عام VNVisitor {
    منطقي m_اندريون_Initial = خطأ;  // Is inside of an initial block
    AstVar* m_o_ميدان_Varp = خالي_اشارو;  // Output filed var of table line
    std::ويڪٽر<AstVar*> m_داخلا_Vars;  // All the input vars in the AstPrimitive
    std::ويڪٽر<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = خالي_اشارو;  // The current primitive
    منطقي m_is_پهريون_Output = خطأ;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = خالي_اشارو;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = خالي_اشارو;  // Main Always block in UDP transform

    فراغ گهمو(AstInitial* nodep) override {
        VL_RESTORER(m_اندريون_Initial);
        إذا (m_primp) m_اندريون_Initial = سچ;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_سونپيو* nodep) override {
        إذا (m_اندريون_Initial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    فراغ گهمو(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_is_پهريون_Output);
        VL_RESTORER_CLEAR(m_داخلا_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = خالي_اشارو;
        m_primp = nodep;
        m_is_پهريون_Output = خطأ;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        إذا (m_primp) {
            إذا (nodep->isIO()) {
                إذا (nodep->is_داخلا()) {
                    m_داخلا_Vars.ڌڪو_پٺ(nodep);
                } وإلا {
                    m_outputVars.ڌڪو_پٺ(nodep);
                }
                إذا ((m_داخلا_Vars.سائيز() == 0) && (m_outputVars.سائيز() == 1)) {
                    m_is_پهريون_Output = سچ;
                }
            }
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_Udp_جدول* nodep) override {
        File_سٽ* ثابت fl = nodep->fileline();
        إذا (m_outputVars.سائيز() != 1) {
            m_outputVars.پٺ()->v3error(
                m_outputVars.سائيز()
                << " output ports for UDP table, there must be one output port");
        }
        إذا (!m_is_پهريون_Output && m_outputVars.سائيز()) {
            m_داخلا_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_o_ميدان_Varp = m_outputVars[0];

        m_alwaysBlockp = جديد AstAlways{fl, VAlwaysKwd::ALWAYS, خالي_اشارو, خالي_اشارو};
        fl->warnOff(V3_Error_ڪوڊ::LATCH, سچ);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(Ast_Udp_جدول_سٽ* nodep) override {
        File_سٽ* ثابت fl = nodep->fileline();
        إذا (!nodep->udpIsCombo() && !m_o_ميدان_Varp->is_ٻٽو_انگ_Logic()) {
            m_o_ميدان_Varp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        إذا (nodep->udpIsCombo() && m_o_ميدان_Varp->is_ٻٽو_انگ_Logic()) {
            m_o_ميدان_Varp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_ڳنڍ* iNodep = nodep->iFieldsp();
        Ast_ڳنڍ* oNodep = nodep->oFieldsp();
        Ast_Sen_وڻ* edgetrigp = خالي_اشارو;

        AstLogAnd* logandp = جديد AstLogAnd{fl, جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}},
                                           جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}}};

        لكل (AstVar* ثابت varp : m_داخلا_Vars) {
            إذا (!iNodep) اكسر;
            إذا (Ast_Udp_جدول_سٽ_Val* linevalp = VN_CAST(iNodep, Udp_جدول_سٽ_Val)) {
                string valName = linevalp->name();
                إذا (is_ڪنارو_Trig(valName)) {
                    إذا (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    إذا (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(ڌڪو_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = جديد Ast_Sen_وڻ{
                        fl, جديد Ast_Sen_شي{fl, VEdgeType::ET_BOTHEDGE,
                                           جديد AstVarRef{fl, varp, VAccess::READ}}};
                }
                إذا (valName == "0" || valName == "f") {
                    logandp = جديد AstLogAnd{
                        fl, logandp, جديد AstLogNot{fl, جديد AstVarRef{fl, varp, VAccess::READ}}};
                } وإلا إذا (valName == "1" || valName == "r") {
                    logandp = جديد AstLogAnd{fl, logandp, جديد AstVarRef{fl, varp, VAccess::READ}};
                } وإلا إذا (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    إذا (edgetrigp) VL_DO_DANGLING(ڌڪو_Deletep(edgetrigp), edgetrigp);
                    إذا (logandp) VL_DO_DANGLING(ڌڪو_Deletep(logandp), logandp);
                    أرجع;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        لكل (ثابت Ast_ڳنڍ* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        إذا (inputvars != m_داخلا_Vars.سائيز()) {
            nodep->v3error("Incorrect number of input values, expected " << m_داخلا_Vars.سائيز()
                                                                         << ", got " << inputvars);
        }

        string ثابت oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        إذا (oValName == "-") {
            إذا (edgetrigp) VL_DO_DANGLING(ڌڪو_Deletep(edgetrigp), edgetrigp);
            إذا (logandp) VL_DO_DANGLING(ڌڪو_Deletep(logandp), logandp);
            أرجع;
        }

        إذا (!nodep->udpIsCombo()) {
            إذا (oNodep->name() == "0") {
                logandp = جديد AstLogAnd{
                    fl, logandp,
                    جديد AstLogNot{fl, جديد AstVarRef{fl, m_o_ميدان_Varp, VAccess::READ}}};
            } وإلا إذا (oNodep->name() == "1") {
                logandp
                    = جديد AstLogAnd{fl, logandp, جديد AstVarRef{fl, m_o_ميدان_Varp, VAccess::READ}};
            } وإلا إذا (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                إذا (edgetrigp) VL_DO_DANGLING(ڌڪو_Deletep(edgetrigp), edgetrigp);
                إذا (logandp) VL_DO_DANGLING(ڌڪو_Deletep(logandp), logandp);
                أرجع;
            }
        }

        fl->warnOff(V3_Error_ڪوڊ::LATCH, سچ);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_ڳنڍ_Stmt* ثابت assignp
            = new_Output_سونپيو(fl, get_Output_انگ(nodep, oValName), edgetrigp != خالي_اشارو);
        AstIf* ثابت ifp = جديد AstIf{fl, logandp, assignp};
        إذا (nodep->udpIsCombo()) {
            إذا (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->جوڙيو_Stmtsp(ifp);
            إذا (edgetrigp) VL_DO_DANGLING(ڌڪو_Deletep(edgetrigp), edgetrigp);
            أرجع;
        }
        إذا (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->جوڙيو_ايندڙ(جديد AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }
    فراغ گهمو(AstLogAnd* nodep) override { iterateChildren(nodep); }
    فراغ گهمو(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    ساكن منطقي is_ڪنارو_Trig(std::string& valName) {
        إذا (valName == "x" || valName == "X") أرجع خطأ;
        إذا (valName == "*") أرجع سچ;
        إذا (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            أرجع سچ;
        }
        إذا (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            أرجع سچ;
        }
        إذا (valName.سائيز() == 2) {
            إذا (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            وإلا إذا (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            وإلا إذا (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            أرجع سچ;
        }
        إذا (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        أرجع خطأ;
    }
    ساكن منطقي isCombOutputSig(ثابت std::string& valName) {
        أرجع (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    ساكن منطقي isSequentOutputSig(ثابت std::string& valName) {
        أرجع (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    Ast_ڳنڍ_Stmt* new_Output_سونپيو(File_سٽ* fl, ثابت V3_عدد& ٻاهريون_انگ, منطقي isDelayed) ثابت {
        AstVarRef* ثابت lhsp = جديد AstVarRef{fl, m_o_ميدان_Varp, VAccess::WRITE};
        AstConst* ثابت rhsp = جديد AstConst{fl, ٻاهريون_انگ};
        إذا (isDelayed) أرجع جديد Ast_سونپيو_Dly{fl, lhsp, rhsp};
        أرجع جديد Ast_سونپيو{fl, lhsp, rhsp};
    }
    ساكن V3_عدد get_Output_انگ(Ast_ڳنڍ* nodep, ثابت std::string& ميدان_نالا) {
        V3_عدد output_انگ{nodep, 1};
        إذا (ميدان_نالا == "0") {
            output_انگ.سيٽ_ٻٽو_انگ(0, 0);
        } وإلا إذا (ميدان_نالا == "1") {
            output_انگ.سيٽ_ٻٽو_انگ(0, 1);
        } وإلا {
            output_انگ.سيٽ_ٻٽو_انگ(0, 'x');
        }
        أرجع output_انگ;
    }

عام:
    // CONSTRUCTORS
    واضح UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = افتراضي;
};

فراغ V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ثابت UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("udp", 0, dump_وڻ_Either_Level() >= 3);
}
