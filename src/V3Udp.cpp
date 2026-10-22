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
    std::vector<AstVar*> m_inputVars;  // All the input vars in the AstPrimitive
    std::vector<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = nullptr;  // The current primitive
    منطقي m_isFirstOutput = خطأ;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = nullptr;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = nullptr;  // Main Always block in UDP transform

    فراغ visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        𐡄𐡍 (m_primp) m_inInitial = 𐡒𐡔𐡈;
        iterateChildren(nodep);
    }
    فراغ visit(AstAssign* nodep) override {
        𐡄𐡍 (m_inInitial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    فراغ visit(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_isFirstOutput);
        VL_RESTORER_CLEAR(m_inputVars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = nullptr;
        m_primp = nodep;
        m_isFirstOutput = خطأ;
        iterateChildren(nodep);
    }
    فراغ visit(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        𐡄𐡍 (m_primp) {
            𐡄𐡍 (nodep->isIO()) {
                𐡄𐡍 (nodep->isInput()) {
                    m_inputVars.push_back(nodep);
                } وإلا {
                    m_outputVars.push_back(nodep);
                }
                𐡄𐡍 ((m_inputVars.size() == 0) && (m_outputVars.size() == 1)) {
                    m_isFirstOutput = 𐡒𐡔𐡈;
                }
            }
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstUdpTable* nodep) override {
        FileLine* ثابت fl = nodep->fileline();
        𐡄𐡍 (m_outputVars.size() != 1) {
            m_outputVars.back()->v3error(
                m_outputVars.size()
                << " output ports for UDP table, there must be one output port");
        }
        𐡄𐡍 (!m_isFirstOutput && m_outputVars.size()) {
            m_inputVars[0]->v3error("First UDP port must be the output port");
        }
        m_oFieldVarp = m_outputVars[0];

        m_alwaysBlockp = جديد AstAlways{fl, VAlwaysKwd::ALWAYS, nullptr, nullptr};
        fl->warnOff(V3ErrorCode::LATCH, 𐡒𐡔𐡈);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstUdpTableLine* nodep) override {
        FileLine* ثابت fl = nodep->fileline();
        𐡄𐡍 (!nodep->udpIsCombo() && !m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        𐡄𐡍 (nodep->udpIsCombo() && m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        AstNode* iNodep = nodep->iFieldsp();
        AstNode* oNodep = nodep->oFieldsp();
        AstSenTree* edgetrigp = nullptr;

        AstLogAnd* logandp = جديد AstLogAnd{fl, جديد AstConst{fl, AstConst::BitTrue{}},
                                           جديد AstConst{fl, AstConst::BitTrue{}}};

        لكل (AstVar* ثابت varp : m_inputVars) {
            𐡄𐡍 (!iNodep) اكسر;
            𐡄𐡍 (AstUdpTableLineVal* linevalp = VN_CAST(iNodep, UdpTableLineVal)) {
                string valName = linevalp->name();
                𐡄𐡍 (isEdgeTrig(valName)) {
                    𐡄𐡍 (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    𐡄𐡍 (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(pushDeletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = جديد AstSenTree{
                        fl, جديد AstSenItem{fl, VEdgeType::ET_BOTHEDGE,
                                           جديد AstVarRef{fl, varp, VAccess::READ}}};
                }
                𐡄𐡍 (valName == "0" || valName == "f") {
                    logandp = جديد AstLogAnd{
                        fl, logandp, جديد AstLogNot{fl, جديد AstVarRef{fl, varp, VAccess::READ}}};
                } وإلا 𐡄𐡍 (valName == "1" || valName == "r") {
                    logandp = جديد AstLogAnd{fl, logandp, جديد AstVarRef{fl, varp, VAccess::READ}};
                } وإلا 𐡄𐡍 (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    𐡄𐡍 (edgetrigp) VL_DO_DANGLING(pushDeletep(edgetrigp), edgetrigp);
                    𐡄𐡍 (logandp) VL_DO_DANGLING(pushDeletep(logandp), logandp);
                    أرجع;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        لكل (ثابت AstNode* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        𐡄𐡍 (inputvars != m_inputVars.size()) {
            nodep->v3error("Incorrect number of input values, expected " << m_inputVars.size()
                                                                         << ", got " << inputvars);
        }

        string ثابت oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        𐡄𐡍 (oValName == "-") {
            𐡄𐡍 (edgetrigp) VL_DO_DANGLING(pushDeletep(edgetrigp), edgetrigp);
            𐡄𐡍 (logandp) VL_DO_DANGLING(pushDeletep(logandp), logandp);
            أرجع;
        }

        𐡄𐡍 (!nodep->udpIsCombo()) {
            𐡄𐡍 (oNodep->name() == "0") {
                logandp = جديد AstLogAnd{
                    fl, logandp,
                    جديد AstLogNot{fl, جديد AstVarRef{fl, m_oFieldVarp, VAccess::READ}}};
            } وإلا 𐡄𐡍 (oNodep->name() == "1") {
                logandp
                    = جديد AstLogAnd{fl, logandp, جديد AstVarRef{fl, m_oFieldVarp, VAccess::READ}};
            } وإلا 𐡄𐡍 (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                𐡄𐡍 (edgetrigp) VL_DO_DANGLING(pushDeletep(edgetrigp), edgetrigp);
                𐡄𐡍 (logandp) VL_DO_DANGLING(pushDeletep(logandp), logandp);
                أرجع;
            }
        }

        fl->warnOff(V3ErrorCode::LATCH, 𐡒𐡔𐡈);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        AstNodeStmt* ثابت assignp
            = newOutputAssign(fl, getOutputNum(nodep, oValName), edgetrigp != nullptr);
        AstIf* ثابت ifp = جديد AstIf{fl, logandp, assignp};
        𐡄𐡍 (nodep->udpIsCombo()) {
            𐡄𐡍 (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->addStmtsp(ifp);
            𐡄𐡍 (edgetrigp) VL_DO_DANGLING(pushDeletep(edgetrigp), edgetrigp);
            أرجع;
        }
        𐡄𐡍 (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->addNext(جديد AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstLogAnd* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    ساكن منطقي isEdgeTrig(std::string& valName) {
        𐡄𐡍 (valName == "x" || valName == "X") أرجع خطأ;
        𐡄𐡍 (valName == "*") أرجع 𐡒𐡔𐡈;
        𐡄𐡍 (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            أرجع 𐡒𐡔𐡈;
        }
        𐡄𐡍 (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            أرجع 𐡒𐡔𐡈;
        }
        𐡄𐡍 (valName.size() == 2) {
            𐡄𐡍 (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            وإلا 𐡄𐡍 (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            وإلا 𐡄𐡍 (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            أرجع 𐡒𐡔𐡈;
        }
        𐡄𐡍 (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        أرجع خطأ;
    }
    ساكن منطقي isCombOutputSig(ثابت std::string& valName) {
        أرجع (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    ساكن منطقي isSequentOutputSig(ثابت std::string& valName) {
        أرجع (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    AstNodeStmt* newOutputAssign(FileLine* fl, ثابت V3Number& outNum, منطقي isDelayed) ثابت {
        AstVarRef* ثابت lhsp = جديد AstVarRef{fl, m_oFieldVarp, VAccess::WRITE};
        AstConst* ثابت rhsp = جديد AstConst{fl, outNum};
        𐡄𐡍 (isDelayed) أرجع جديد AstAssignDly{fl, lhsp, rhsp};
        أرجع جديد AstAssign{fl, lhsp, rhsp};
    }
    ساكن V3Number getOutputNum(AstNode* nodep, ثابت std::string& fieldNames) {
        V3Number outputNum{nodep, 1};
        𐡄𐡍 (fieldNames == "0") {
            outputNum.setBit(0, 0);
        } وإلا 𐡄𐡍 (fieldNames == "1") {
            outputNum.setBit(0, 1);
        } وإلا {
            outputNum.setBit(0, 'x');
        }
        أرجع outputNum;
    }

عام:
    // CONSTRUCTORS
    explicit UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = افتراضي;
};

فراغ V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { ثابت UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("udp", 0, dumpTreeEitherLevel() >= 3);
}
