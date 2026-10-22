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

رده UdpVisitor final : عمومی VNVisitor {
    منطقی m_inInitial = نادرست;  // Is inside of an initial block
    AstVar* m_oFieldVarp = nullptr;  // Output filed var of table line
    std::vector<AstVar*> m_درون_داد_Vars;  // All the input vars in the AstPrimitive
    std::vector<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = nullptr;  // The current primitive
    منطقی m_isFirstOutput = نادرست;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = nullptr;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = nullptr;  // Main Always block in UDP transform

    تهی visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        اگر (m_primp) m_inInitial = درست;
        iterateChildren(nodep);
    }
    تهی visit(AstAssign* nodep) override {
        اگر (m_inInitial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    تهی visit(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_isFirstOutput);
        VL_RESTORER_CLEAR(m_درون_داد_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = nullptr;
        m_primp = nodep;
        m_isFirstOutput = نادرست;
        iterateChildren(nodep);
    }
    تهی visit(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        اگر (m_primp) {
            اگر (nodep->isIO()) {
                اگر (nodep->is_درون_داد()) {
                    m_درون_داد_Vars.افزودن_back(nodep);
                } وگرنه {
                    m_outputVars.افزودن_back(nodep);
                }
                اگر ((m_درون_داد_Vars.size() == 0) && (m_outputVars.size() == 1)) {
                    m_isFirstOutput = درست;
                }
            }
        }
        iterateChildren(nodep);
    }
    تهی visit(AstUdpTable* nodep) override {
        File_سطر* ثابت fl = nodep->fileline();
        اگر (m_outputVars.size() != 1) {
            m_outputVars.back()->v3error(
                m_outputVars.size()
                << " output ports for UDP table, there must be one output port");
        }
        اگر (!m_isFirstOutput && m_outputVars.size()) {
            m_درون_داد_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_oFieldVarp = m_outputVars[0];

        m_alwaysBlockp = جدید AstAlways{fl, VAlwaysKwd::ALWAYS, nullptr, nullptr};
        fl->warnOff(V3ErrorCode::LATCH, درست);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(Ast_Udp_Table_سطر* nodep) override {
        File_سطر* ثابت fl = nodep->fileline();
        اگر (!nodep->udpIsCombo() && !m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        اگر (nodep->udpIsCombo() && m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_گره* iNodep = nodep->iFieldsp();
        Ast_گره* oNodep = nodep->oFieldsp();
        Ast_Sen_درخت* edgetrigp = nullptr;

        AstLogAnd* logandp = جدید AstLogAnd{fl, جدید AstConst{fl, AstConst::BitTrue{}},
                                           جدید AstConst{fl, AstConst::BitTrue{}}};

        برای (AstVar* ثابت varp : m_درون_داد_Vars) {
            اگر (!iNodep) بشکن;
            اگر (Ast_Udp_Table_سطر_Val* linevalp = VN_CAST(iNodep, Udp_Table_سطر_Val)) {
                string valName = linevalp->name();
                اگر (isEdgeTrig(valName)) {
                    اگر (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    اگر (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(افزودن_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = جدید Ast_Sen_درخت{
                        fl, جدید AstSenItem{fl, VEdgeType::ET_BOTHEDGE,
                                           جدید AstVarRef{fl, varp, VAccess::READ}}};
                }
                اگر (valName == "0" || valName == "f") {
                    logandp = جدید AstLogAnd{
                        fl, logandp, جدید AstLogNot{fl, جدید AstVarRef{fl, varp, VAccess::READ}}};
                } وگرنه اگر (valName == "1" || valName == "r") {
                    logandp = جدید AstLogAnd{fl, logandp, جدید AstVarRef{fl, varp, VAccess::READ}};
                } وگرنه اگر (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    اگر (edgetrigp) VL_DO_DANGLING(افزودن_Deletep(edgetrigp), edgetrigp);
                    اگر (logandp) VL_DO_DANGLING(افزودن_Deletep(logandp), logandp);
                    بازگشت;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        برای (ثابت Ast_گره* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        اگر (inputvars != m_درون_داد_Vars.size()) {
            nodep->v3error("Incorrect number of input values, expected " << m_درون_داد_Vars.size()
                                                                         << ", got " << inputvars);
        }

        string ثابت oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        اگر (oValName == "-") {
            اگر (edgetrigp) VL_DO_DANGLING(افزودن_Deletep(edgetrigp), edgetrigp);
            اگر (logandp) VL_DO_DANGLING(افزودن_Deletep(logandp), logandp);
            بازگشت;
        }

        اگر (!nodep->udpIsCombo()) {
            اگر (oNodep->name() == "0") {
                logandp = جدید AstLogAnd{
                    fl, logandp,
                    جدید AstLogNot{fl, جدید AstVarRef{fl, m_oFieldVarp, VAccess::READ}}};
            } وگرنه اگر (oNodep->name() == "1") {
                logandp
                    = جدید AstLogAnd{fl, logandp, جدید AstVarRef{fl, m_oFieldVarp, VAccess::READ}};
            } وگرنه اگر (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                اگر (edgetrigp) VL_DO_DANGLING(افزودن_Deletep(edgetrigp), edgetrigp);
                اگر (logandp) VL_DO_DANGLING(افزودن_Deletep(logandp), logandp);
                بازگشت;
            }
        }

        fl->warnOff(V3ErrorCode::LATCH, درست);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_گره_Stmt* ثابت assignp
            = newOutputAssign(fl, get_Output_رقم(nodep, oValName), edgetrigp != nullptr);
        AstIf* ثابت ifp = جدید AstIf{fl, logandp, assignp};
        اگر (nodep->udpIsCombo()) {
            اگر (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->addStmtsp(ifp);
            اگر (edgetrigp) VL_DO_DANGLING(افزودن_Deletep(edgetrigp), edgetrigp);
            بازگشت;
        }
        اگر (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->addNext(جدید AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }
    تهی visit(AstLogAnd* nodep) override { iterateChildren(nodep); }
    تهی visit(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    ایستا منطقی isEdgeTrig(std::string& valName) {
        اگر (valName == "x" || valName == "X") بازگشت نادرست;
        اگر (valName == "*") بازگشت درست;
        اگر (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            بازگشت درست;
        }
        اگر (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            بازگشت درست;
        }
        اگر (valName.size() == 2) {
            اگر (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            وگرنه اگر (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            وگرنه اگر (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            بازگشت درست;
        }
        اگر (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        بازگشت نادرست;
    }
    ایستا منطقی isCombOutputSig(ثابت std::string& valName) {
        بازگشت (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    ایستا منطقی isSequentOutputSig(ثابت std::string& valName) {
        بازگشت (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    Ast_گره_Stmt* newOutputAssign(File_سطر* fl, ثابت V3_عدد& out_رقم, منطقی isDelayed) ثابت {
        AstVarRef* ثابت lhsp = جدید AstVarRef{fl, m_oFieldVarp, VAccess::WRITE};
        AstConst* ثابت rhsp = جدید AstConst{fl, out_رقم};
        اگر (isDelayed) بازگشت جدید AstAssignDly{fl, lhsp, rhsp};
        بازگشت جدید AstAssign{fl, lhsp, rhsp};
    }
    ایستا V3_عدد get_Output_رقم(Ast_گره* nodep, ثابت std::string& fieldNames) {
        V3_عدد output_رقم{nodep, 1};
        اگر (fieldNames == "0") {
            output_رقم.setBit(0, 0);
        } وگرنه اگر (fieldNames == "1") {
            output_رقم.setBit(0, 1);
        } وگرنه {
            output_رقم.setBit(0, 'x');
        }
        بازگشت output_رقم;
    }

عمومی:
    // CONSTRUCTORS
    explicit UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = پیشفرض;
};

تهی V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ثابت UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("udp", 0, dump_درخت_Either_Level() >= 3);
}
