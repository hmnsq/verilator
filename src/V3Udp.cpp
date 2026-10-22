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
    منطقي m_درون_Initial = خطأ;  // Is inside of an initial block
    AstVar* m_o_فیلد_Varp = اشاره_تهی;  // Output filed var of table line
    std::وکتور<AstVar*> m_درون_داد_Vars;  // All the input vars in the AstPrimitive
    std::وکتور<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = اشاره_تهی;  // The current primitive
    منطقي m_is_اولی_Output = خطأ;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = اشاره_تهی;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = اشاره_تهی;  // Main Always block in UDP transform

    فراغ بازدید(AstInitial* nodep) override {
        VL_RESTORER(m_درون_Initial);
        إذا (m_primp) m_درون_Initial = درست;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_تخصیص_بده* nodep) override {
        إذا (m_درون_Initial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    فراغ بازدید(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_is_اولی_Output);
        VL_RESTORER_CLEAR(m_درون_داد_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = اشاره_تهی;
        m_primp = nodep;
        m_is_اولی_Output = خطأ;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        إذا (m_primp) {
            إذا (nodep->isIO()) {
                إذا (nodep->is_درون_داد()) {
                    m_درون_داد_Vars.افزودن_پشت(nodep);
                } وإلا {
                    m_outputVars.افزودن_پشت(nodep);
                }
                إذا ((m_درون_داد_Vars.اندازه() == 0) && (m_outputVars.اندازه() == 1)) {
                    m_is_اولی_Output = درست;
                }
            }
        }
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_Udp_جدول* nodep) override {
        File_سطر* ثابت fl = nodep->fileline();
        إذا (m_outputVars.اندازه() != 1) {
            m_outputVars.پشت()->v3error(
                m_outputVars.اندازه()
                << " output ports for UDP table, there must be one output port");
        }
        إذا (!m_is_اولی_Output && m_outputVars.اندازه()) {
            m_درون_داد_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_o_فیلد_Varp = m_outputVars[0];

        m_alwaysBlockp = جديد AstAlways{fl, VAlwaysKwd::ALWAYS, اشاره_تهی, اشاره_تهی};
        fl->warnOff(V3_Error_کد::LATCH, درست);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    فراغ بازدید(Ast_Udp_جدول_سطر* nodep) override {
        File_سطر* ثابت fl = nodep->fileline();
        إذا (!nodep->udpIsCombo() && !m_o_فیلد_Varp->is_بیت_Logic()) {
            m_o_فیلد_Varp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        إذا (nodep->udpIsCombo() && m_o_فیلد_Varp->is_بیت_Logic()) {
            m_o_فیلد_Varp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_گره* iNodep = nodep->iFieldsp();
        Ast_گره* oNodep = nodep->oFieldsp();
        Ast_Sen_درخت* edgetrigp = اشاره_تهی;

        AstLogAnd* logandp = جديد AstLogAnd{fl, جديد AstConst{fl, AstConst::بیت_True{}},
                                           جديد AstConst{fl, AstConst::بیت_True{}}};

        لكل (AstVar* ثابت varp : m_درون_داد_Vars) {
            إذا (!iNodep) اكسر;
            إذا (Ast_Udp_جدول_سطر_Val* linevalp = VN_CAST(iNodep, Udp_جدول_سطر_Val)) {
                string valName = linevalp->name();
                إذا (is_یال_Trig(valName)) {
                    إذا (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    إذا (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(افزودن_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = جديد Ast_Sen_درخت{
                        fl, جديد Ast_Sen_قلم{fl, VEdgeType::ET_BOTHEDGE,
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
                    إذا (edgetrigp) VL_DO_DANGLING(افزودن_Deletep(edgetrigp), edgetrigp);
                    إذا (logandp) VL_DO_DANGLING(افزودن_Deletep(logandp), logandp);
                    أرجع;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        لكل (ثابت Ast_گره* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        إذا (inputvars != m_درون_داد_Vars.اندازه()) {
            nodep->v3error("Incorrect number of input values, expected " << m_درون_داد_Vars.اندازه()
                                                                         << ", got " << inputvars);
        }

        string ثابت oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        إذا (oValName == "-") {
            إذا (edgetrigp) VL_DO_DANGLING(افزودن_Deletep(edgetrigp), edgetrigp);
            إذا (logandp) VL_DO_DANGLING(افزودن_Deletep(logandp), logandp);
            أرجع;
        }

        إذا (!nodep->udpIsCombo()) {
            إذا (oNodep->name() == "0") {
                logandp = جديد AstLogAnd{
                    fl, logandp,
                    جديد AstLogNot{fl, جديد AstVarRef{fl, m_o_فیلد_Varp, VAccess::READ}}};
            } وإلا إذا (oNodep->name() == "1") {
                logandp
                    = جديد AstLogAnd{fl, logandp, جديد AstVarRef{fl, m_o_فیلد_Varp, VAccess::READ}};
            } وإلا إذا (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                إذا (edgetrigp) VL_DO_DANGLING(افزودن_Deletep(edgetrigp), edgetrigp);
                إذا (logandp) VL_DO_DANGLING(افزودن_Deletep(logandp), logandp);
                أرجع;
            }
        }

        fl->warnOff(V3_Error_کد::LATCH, درست);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_گره_Stmt* ثابت assignp
            = new_Output_تخصیص_بده(fl, get_Output_رقم(nodep, oValName), edgetrigp != اشاره_تهی);
        AstIf* ثابت ifp = جديد AstIf{fl, logandp, assignp};
        إذا (nodep->udpIsCombo()) {
            إذا (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->جمع_کن_Stmtsp(ifp);
            إذا (edgetrigp) VL_DO_DANGLING(افزودن_Deletep(edgetrigp), edgetrigp);
            أرجع;
        }
        إذا (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->جمع_کن_بعدی(جديد AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }
    فراغ بازدید(AstLogAnd* nodep) override { iterateChildren(nodep); }
    فراغ بازدید(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    ساكن منطقي is_یال_Trig(std::string& valName) {
        إذا (valName == "x" || valName == "X") أرجع خطأ;
        إذا (valName == "*") أرجع درست;
        إذا (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            أرجع درست;
        }
        إذا (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            أرجع درست;
        }
        إذا (valName.اندازه() == 2) {
            إذا (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            وإلا إذا (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            وإلا إذا (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            أرجع درست;
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
    Ast_گره_Stmt* new_Output_تخصیص_بده(File_سطر* fl, ثابت V3_عدد& بیرون_رقم, منطقي isDelayed) ثابت {
        AstVarRef* ثابت lhsp = جديد AstVarRef{fl, m_o_فیلد_Varp, VAccess::WRITE};
        AstConst* ثابت rhsp = جديد AstConst{fl, بیرون_رقم};
        إذا (isDelayed) أرجع جديد Ast_تخصیص_بده_Dly{fl, lhsp, rhsp};
        أرجع جديد Ast_تخصیص_بده{fl, lhsp, rhsp};
    }
    ساكن V3_عدد get_Output_رقم(Ast_گره* nodep, ثابت std::string& فیلد_نامها) {
        V3_عدد output_رقم{nodep, 1};
        إذا (فیلد_نامها == "0") {
            output_رقم.مجموعه_بیت(0, 0);
        } وإلا إذا (فیلد_نامها == "1") {
            output_رقم.مجموعه_بیت(0, 1);
        } وإلا {
            output_رقم.مجموعه_بیت(0, 'x');
        }
        أرجع output_رقم;
    }

عام:
    // CONSTRUCTORS
    صریح UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = افتراضي;
};

فراغ V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ثابت UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("udp", 0, dump_درخت_Either_Level() >= 3);
}
