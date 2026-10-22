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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Udp.h"

#شامل "V3Error.h"

#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

جماعت UdpVisitor final : عوامی VNVisitor {
    منطقی m_اندر_Initial = جھوٹ;  // Is inside of an initial block
    AstVar* m_o_میدان_Varp = خالی_اشاریہ;  // Output filed var of table line
    std::سمتیہ<AstVar*> m_داخلہ_Vars;  // All the input vars in the AstPrimitive
    std::سمتیہ<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = خالی_اشاریہ;  // The current primitive
    منطقی m_is_پہلا_Output = جھوٹ;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = خالی_اشاریہ;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = خالی_اشاریہ;  // Main Always block in UDP transform

    خالی دورہ(AstInitial* nodep) override {
        VL_RESTORER(m_اندر_Initial);
        اگر (m_primp) m_اندر_Initial = سچ;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_تفویض* nodep) override {
        اگر (m_اندر_Initial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    خالی دورہ(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_is_پہلا_Output);
        VL_RESTORER_CLEAR(m_داخلہ_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = خالی_اشاریہ;
        m_primp = nodep;
        m_is_پہلا_Output = جھوٹ;
        iterateChildren(nodep);
    }
    خالی دورہ(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        اگر (m_primp) {
            اگر (nodep->isIO()) {
                اگر (nodep->is_داخلہ()) {
                    m_داخلہ_Vars.ڈالو_عقب(nodep);
                } ورنہ {
                    m_outputVars.ڈالو_عقب(nodep);
                }
                اگر ((m_داخلہ_Vars.سائز() == 0) && (m_outputVars.سائز() == 1)) {
                    m_is_پہلا_Output = سچ;
                }
            }
        }
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_Udp_جدول* nodep) override {
        File_سطر* ثابت fl = nodep->fileline();
        اگر (m_outputVars.سائز() != 1) {
            m_outputVars.عقب()->v3error(
                m_outputVars.سائز()
                << " output ports for UDP table, there must be one output port");
        }
        اگر (!m_is_پہلا_Output && m_outputVars.سائز()) {
            m_داخلہ_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_o_میدان_Varp = m_outputVars[0];

        m_alwaysBlockp = نیا AstAlways{fl, VAlwaysKwd::ALWAYS, خالی_اشاریہ, خالی_اشاریہ};
        fl->warnOff(V3_Error_رمز::LATCH, سچ);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(Ast_Udp_جدول_سطر* nodep) override {
        File_سطر* ثابت fl = nodep->fileline();
        اگر (!nodep->udpIsCombo() && !m_o_میدان_Varp->is_ثنائی_ہندسہ_Logic()) {
            m_o_میدان_Varp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        اگر (nodep->udpIsCombo() && m_o_میدان_Varp->is_ثنائی_ہندسہ_Logic()) {
            m_o_میدان_Varp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_گرہ* iNodep = nodep->iFieldsp();
        Ast_گرہ* oNodep = nodep->oFieldsp();
        Ast_Sen_درخت* edgetrigp = خالی_اشاریہ;

        AstLogAnd* logandp = نیا AstLogAnd{fl, نیا AstConst{fl, AstConst::ثنائی_ہندسہ_True{}},
                                           نیا AstConst{fl, AstConst::ثنائی_ہندسہ_True{}}};

        برائے (AstVar* ثابت varp : m_داخلہ_Vars) {
            اگر (!iNodep) توڑو;
            اگر (Ast_Udp_جدول_سطر_Val* linevalp = VN_CAST(iNodep, Udp_جدول_سطر_Val)) {
                string valName = linevalp->name();
                اگر (is_کنارہ_Trig(valName)) {
                    اگر (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    اگر (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(ڈالو_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = نیا Ast_Sen_درخت{
                        fl, نیا Ast_Sen_شے{fl, VEdgeType::ET_BOTHEDGE,
                                           نیا AstVarRef{fl, varp, VAccess::READ}}};
                }
                اگر (valName == "0" || valName == "f") {
                    logandp = نیا AstLogAnd{
                        fl, logandp, نیا AstLogNot{fl, نیا AstVarRef{fl, varp, VAccess::READ}}};
                } ورنہ اگر (valName == "1" || valName == "r") {
                    logandp = نیا AstLogAnd{fl, logandp, نیا AstVarRef{fl, varp, VAccess::READ}};
                } ورنہ اگر (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    اگر (edgetrigp) VL_DO_DANGLING(ڈالو_Deletep(edgetrigp), edgetrigp);
                    اگر (logandp) VL_DO_DANGLING(ڈالو_Deletep(logandp), logandp);
                    واپس;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        برائے (ثابت Ast_گرہ* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        اگر (inputvars != m_داخلہ_Vars.سائز()) {
            nodep->v3error("Incorrect number of input values, expected " << m_داخلہ_Vars.سائز()
                                                                         << ", got " << inputvars);
        }

        string ثابت oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        اگر (oValName == "-") {
            اگر (edgetrigp) VL_DO_DANGLING(ڈالو_Deletep(edgetrigp), edgetrigp);
            اگر (logandp) VL_DO_DANGLING(ڈالو_Deletep(logandp), logandp);
            واپس;
        }

        اگر (!nodep->udpIsCombo()) {
            اگر (oNodep->name() == "0") {
                logandp = نیا AstLogAnd{
                    fl, logandp,
                    نیا AstLogNot{fl, نیا AstVarRef{fl, m_o_میدان_Varp, VAccess::READ}}};
            } ورنہ اگر (oNodep->name() == "1") {
                logandp
                    = نیا AstLogAnd{fl, logandp, نیا AstVarRef{fl, m_o_میدان_Varp, VAccess::READ}};
            } ورنہ اگر (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                اگر (edgetrigp) VL_DO_DANGLING(ڈالو_Deletep(edgetrigp), edgetrigp);
                اگر (logandp) VL_DO_DANGLING(ڈالو_Deletep(logandp), logandp);
                واپس;
            }
        }

        fl->warnOff(V3_Error_رمز::LATCH, سچ);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_گرہ_Stmt* ثابت assignp
            = new_Output_تفویض(fl, get_Output_عدد(nodep, oValName), edgetrigp != خالی_اشاریہ);
        AstIf* ثابت ifp = نیا AstIf{fl, logandp, assignp};
        اگر (nodep->udpIsCombo()) {
            اگر (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->جوڑو_Stmtsp(ifp);
            اگر (edgetrigp) VL_DO_DANGLING(ڈالو_Deletep(edgetrigp), edgetrigp);
            واپس;
        }
        اگر (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->جوڑو_اگلا(نیا AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }
    خالی دورہ(AstLogAnd* nodep) override { iterateChildren(nodep); }
    خالی دورہ(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    ساکن منطقی is_کنارہ_Trig(std::string& valName) {
        اگر (valName == "x" || valName == "X") واپس جھوٹ;
        اگر (valName == "*") واپس سچ;
        اگر (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            واپس سچ;
        }
        اگر (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            واپس سچ;
        }
        اگر (valName.سائز() == 2) {
            اگر (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            ورنہ اگر (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            ورنہ اگر (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            واپس سچ;
        }
        اگر (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        واپس جھوٹ;
    }
    ساکن منطقی isCombOutputSig(ثابت std::string& valName) {
        واپس (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    ساکن منطقی isSequentOutputSig(ثابت std::string& valName) {
        واپس (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    Ast_گرہ_Stmt* new_Output_تفویض(File_سطر* fl, ثابت V3_نمبر& برآمد_عدد, منطقی isDelayed) ثابت {
        AstVarRef* ثابت lhsp = نیا AstVarRef{fl, m_o_میدان_Varp, VAccess::WRITE};
        AstConst* ثابت rhsp = نیا AstConst{fl, برآمد_عدد};
        اگر (isDelayed) واپس نیا Ast_تفویض_Dly{fl, lhsp, rhsp};
        واپس نیا Ast_تفویض{fl, lhsp, rhsp};
    }
    ساکن V3_نمبر get_Output_عدد(Ast_گرہ* nodep, ثابت std::string& میدان_نام) {
        V3_نمبر output_عدد{nodep, 1};
        اگر (میدان_نام == "0") {
            output_عدد.سیٹ_ثنائی_ہندسہ(0, 0);
        } ورنہ اگر (میدان_نام == "1") {
            output_عدد.سیٹ_ثنائی_ہندسہ(0, 1);
        } ورنہ {
            output_عدد.سیٹ_ثنائی_ہندسہ(0, 'x');
        }
        واپس output_عدد;
    }

عوامی:
    // CONSTRUCTORS
    صریح UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = طےشدہ;
};

خالی V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ثابت UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("udp", 0, dump_درخت_Either_Level() >= 3);
}
