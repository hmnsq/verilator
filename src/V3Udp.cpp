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
    منطقي m_inInitial = خطأ;  // Is inside of an initial block
    AstVar* m_oFieldVarp = nullptr;  // Output filed var of table line
    std::vector<AstVar*> m_داخلہ_Vars;  // All the input vars in the AstPrimitive
    std::vector<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = nullptr;  // The current primitive
    منطقي m_isFirstOutput = خطأ;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = nullptr;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = nullptr;  // Main Always block in UDP transform

    فراغ visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        إذا (m_primp) m_inInitial = true;
        iterateChildren(nodep);
    }
    فراغ visit(AstAssign* nodep) override {
        إذا (m_inInitial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    فراغ visit(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_isFirstOutput);
        VL_RESTORER_CLEAR(m_داخلہ_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = nullptr;
        m_primp = nodep;
        m_isFirstOutput = خطأ;
        iterateChildren(nodep);
    }
    فراغ visit(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        إذا (m_primp) {
            إذا (nodep->isIO()) {
                إذا (nodep->is_داخلہ()) {
                    m_داخلہ_Vars.ڈالو_back(nodep);
                } وإلا {
                    m_outputVars.ڈالو_back(nodep);
                }
                إذا ((m_داخلہ_Vars.size() == 0) && (m_outputVars.size() == 1)) {
                    m_isFirstOutput = true;
                }
            }
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstUdpTable* nodep) override {
        File_سطر* ثابت fl = nodep->fileline();
        إذا (m_outputVars.size() != 1) {
            m_outputVars.back()->v3error(
                m_outputVars.size()
                << " output ports for UDP table, there must be one output port");
        }
        إذا (!m_isFirstOutput && m_outputVars.size()) {
            m_داخلہ_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_oFieldVarp = m_outputVars[0];

        m_alwaysBlockp = جديد AstAlways{fl, VAlwaysKwd::ALWAYS, nullptr, nullptr};
        fl->warnOff(V3ErrorCode::LATCH, true);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ visit(Ast_Udp_Table_سطر* nodep) override {
        File_سطر* ثابت fl = nodep->fileline();
        إذا (!nodep->udpIsCombo() && !m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        إذا (nodep->udpIsCombo() && m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_گرہ* iNodep = nodep->iFieldsp();
        Ast_گرہ* oNodep = nodep->oFieldsp();
        Ast_Sen_درخت* edgetrigp = nullptr;

        AstLogAnd* logandp = جديد AstLogAnd{fl, جديد AstConst{fl, AstConst::BitTrue{}},
                                           جديد AstConst{fl, AstConst::BitTrue{}}};

        لكل (AstVar* ثابت varp : m_داخلہ_Vars) {
            إذا (!iNodep) اكسر;
            إذا (Ast_Udp_Table_سطر_Val* linevalp = VN_CAST(iNodep, Udp_Table_سطر_Val)) {
                string valName = linevalp->name();
                إذا (isEdgeTrig(valName)) {
                    إذا (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    إذا (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(ڈالو_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = جديد Ast_Sen_درخت{
                        fl, جديد AstSenItem{fl, VEdgeType::ET_BOTHEDGE,
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
                    إذا (edgetrigp) VL_DO_DANGLING(ڈالو_Deletep(edgetrigp), edgetrigp);
                    إذا (logandp) VL_DO_DANGLING(ڈالو_Deletep(logandp), logandp);
                    أرجع;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        لكل (ثابت Ast_گرہ* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        إذا (inputvars != m_داخلہ_Vars.size()) {
            nodep->v3error("Incorrect number of input values, expected " << m_داخلہ_Vars.size()
                                                                         << ", got " << inputvars);
        }

        string ثابت oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        إذا (oValName == "-") {
            إذا (edgetrigp) VL_DO_DANGLING(ڈالو_Deletep(edgetrigp), edgetrigp);
            إذا (logandp) VL_DO_DANGLING(ڈالو_Deletep(logandp), logandp);
            أرجع;
        }

        إذا (!nodep->udpIsCombo()) {
            إذا (oNodep->name() == "0") {
                logandp = جديد AstLogAnd{
                    fl, logandp,
                    جديد AstLogNot{fl, جديد AstVarRef{fl, m_oFieldVarp, VAccess::READ}}};
            } وإلا إذا (oNodep->name() == "1") {
                logandp
                    = جديد AstLogAnd{fl, logandp, جديد AstVarRef{fl, m_oFieldVarp, VAccess::READ}};
            } وإلا إذا (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                إذا (edgetrigp) VL_DO_DANGLING(ڈالو_Deletep(edgetrigp), edgetrigp);
                إذا (logandp) VL_DO_DANGLING(ڈالو_Deletep(logandp), logandp);
                أرجع;
            }
        }

        fl->warnOff(V3ErrorCode::LATCH, true);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_گرہ_Stmt* ثابت assignp
            = newOutputAssign(fl, get_Output_عدد(nodep, oValName), edgetrigp != nullptr);
        AstIf* ثابت ifp = جديد AstIf{fl, logandp, assignp};
        إذا (nodep->udpIsCombo()) {
            إذا (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->addStmtsp(ifp);
            إذا (edgetrigp) VL_DO_DANGLING(ڈالو_Deletep(edgetrigp), edgetrigp);
            أرجع;
        }
        إذا (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->addNext(جديد AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    فراغ visit(Ast_گرہ* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstLogAnd* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    ساكن منطقي isEdgeTrig(std::string& valName) {
        إذا (valName == "x" || valName == "X") أرجع خطأ;
        إذا (valName == "*") أرجع true;
        إذا (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            أرجع true;
        }
        إذا (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            أرجع true;
        }
        إذا (valName.size() == 2) {
            إذا (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            وإلا إذا (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            وإلا إذا (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            أرجع true;
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
    Ast_گرہ_Stmt* newOutputAssign(File_سطر* fl, ثابت V3_نمبر& out_عدد, منطقي isDelayed) ثابت {
        AstVarRef* ثابت lhsp = جديد AstVarRef{fl, m_oFieldVarp, VAccess::WRITE};
        AstConst* ثابت rhsp = جديد AstConst{fl, out_عدد};
        إذا (isDelayed) أرجع جديد AstAssignDly{fl, lhsp, rhsp};
        أرجع جديد AstAssign{fl, lhsp, rhsp};
    }
    ساكن V3_نمبر get_Output_عدد(Ast_گرہ* nodep, ثابت std::string& fieldNames) {
        V3_نمبر output_عدد{nodep, 1};
        إذا (fieldNames == "0") {
            output_عدد.setBit(0, 0);
        } وإلا إذا (fieldNames == "1") {
            output_عدد.setBit(0, 1);
        } وإلا {
            output_عدد.setBit(0, 'x');
        }
        أرجع output_عدد;
    }

عام:
    // CONSTRUCTORS
    explicit UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = افتراضي;
};

فراغ V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ثابت UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("udp", 0, dump_درخت_Either_Level() >= 3);
}
