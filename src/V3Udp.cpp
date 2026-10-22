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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Udp.h"

#அந்தர்பாவ "V3Error.h"

#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

வகுப்பு UdpVisitor final : பொது VNVisitor {
    தருக்க m_inInitial = பொய்;  // Is inside of an initial block
    AstVar* m_oFieldVarp = nullptr;  // Output filed var of table line
    std::vector<AstVar*> m_உள்ளீடு_Vars;  // All the input vars in the AstPrimitive
    std::vector<AstVar*> m_outputVars;  // All the output vars in the AstPrimitive
    AstPrimitive* m_primp = nullptr;  // The current primitive
    தருக்க m_isFirstOutput = பொய்;  // Whether the first IO port is output
    AstVarRef* m_outputInitVerfp = nullptr;  // Initial output value for sequential UDP
    AstAlways* m_alwaysBlockp = nullptr;  // Main Always block in UDP transform

    வெறுமை visit(AstInitial* nodep) override {
        VL_RESTORER(m_inInitial);
        எனில் (m_primp) m_inInitial = உண்மை;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstAssign* nodep) override {
        எனில் (m_inInitial) { m_outputInitVerfp = VN_CAST(nodep->lhsp(), VarRef); }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstPrimitive* nodep) override {
        UASSERT_OBJ(!m_primp, nodep, "Primitives cannot nest");
        VL_RESTORER(m_primp);
        VL_RESTORER(m_outputInitVerfp);
        VL_RESTORER(m_isFirstOutput);
        VL_RESTORER_CLEAR(m_உள்ளீடு_Vars);
        VL_RESTORER_CLEAR(m_outputVars);
        m_outputInitVerfp = nullptr;
        m_primp = nodep;
        m_isFirstOutput = பொய்;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstVar* nodep) override {
        // Push the input and output vars for primitive.
        எனில் (m_primp) {
            எனில் (nodep->isIO()) {
                எனில் (nodep->is_உள்ளீடு()) {
                    m_உள்ளீடு_Vars.தள்ளு_back(nodep);
                } இல்லையெனில் {
                    m_outputVars.தள்ளு_back(nodep);
                }
                எனில் ((m_உள்ளீடு_Vars.size() == 0) && (m_outputVars.size() == 1)) {
                    m_isFirstOutput = உண்மை;
                }
            }
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstUdpTable* nodep) override {
        File_வரி* மாறிலி fl = nodep->fileline();
        எனில் (m_outputVars.size() != 1) {
            m_outputVars.back()->v3error(
                m_outputVars.size()
                << " output ports for UDP table, there must be one output port");
        }
        எனில் (!m_isFirstOutput && m_outputVars.size()) {
            m_உள்ளீடு_Vars[0]->v3error("First UDP port must be the output port");
        }
        m_oFieldVarp = m_outputVars[0];

        m_alwaysBlockp = புதிய AstAlways{fl, VAlwaysKwd::ALWAYS, nullptr, nullptr};
        fl->warnOff(V3ErrorCode::LATCH, உண்மை);
        iterateChildren(nodep);

        nodep->replaceWith(m_alwaysBlockp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(Ast_Udp_Table_வரி* nodep) override {
        File_வரி* மாறிலி fl = nodep->fileline();
        எனில் (!nodep->udpIsCombo() && !m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error("For sequential UDP, the output must be of 'reg' data type");
        }
        எனில் (nodep->udpIsCombo() && m_oFieldVarp->isBitLogic()) {
            m_oFieldVarp->v3error(
                "For combinational UDP, the output must not be a 'reg' data type");
        }
        Ast_முனை* iNodep = nodep->iFieldsp();
        Ast_முனை* oNodep = nodep->oFieldsp();
        Ast_Sen_மரம்* edgetrigp = nullptr;

        AstLogAnd* logandp = புதிய AstLogAnd{fl, புதிய AstConst{fl, AstConst::BitTrue{}},
                                           புதிய AstConst{fl, AstConst::BitTrue{}}};

        க்கு (AstVar* மாறிலி varp : m_உள்ளீடு_Vars) {
            எனில் (!iNodep) நிறுத்து;
            எனில் (Ast_Udp_Table_வரி_Val* linevalp = VN_CAST(iNodep, Udp_Table_வரி_Val)) {
                string valName = linevalp->name();
                எனில் (isEdgeTrig(valName)) {
                    எனில் (nodep->udpIsCombo()) {
                        linevalp->v3error(
                            "There should not be a edge trigger for combinational UDP table line");
                    }
                    எனில் (edgetrigp) {
                        linevalp->v3error("There can be only one edge trigger signal");
                        VL_DO_DANGLING(தள்ளு_Deletep(edgetrigp), edgetrigp);
                    }
                    edgetrigp = புதிய Ast_Sen_மரம்{
                        fl, புதிய AstSenItem{fl, VEdgeType::ET_BOTHEDGE,
                                           புதிய AstVarRef{fl, varp, VAccess::READ}}};
                }
                எனில் (valName == "0" || valName == "f") {
                    logandp = புதிய AstLogAnd{
                        fl, logandp, புதிய AstLogNot{fl, புதிய AstVarRef{fl, varp, VAccess::READ}}};
                } இல்லையெனில் எனில் (valName == "1" || valName == "r") {
                    logandp = புதிய AstLogAnd{fl, logandp, புதிய AstVarRef{fl, varp, VAccess::READ}};
                } இல்லையெனில் எனில் (valName == "x" || valName == "X") {
                    // No x inputs supported yet, so this whole table line
                    // can never match. Drop the whole thing.
                    எனில் (edgetrigp) VL_DO_DANGLING(தள்ளு_Deletep(edgetrigp), edgetrigp);
                    எனில் (logandp) VL_DO_DANGLING(தள்ளு_Deletep(logandp), logandp);
                    திரும்பு;
                }
            }
            iNodep = iNodep->nextp();
        }

        uint32_t inputvars = 0;
        க்கு (மாறிலி Ast_முனை* icountp = nodep->iFieldsp(); icountp; icountp = icountp->nextp())
            ++inputvars;
        எனில் (inputvars != m_உள்ளீடு_Vars.size()) {
            nodep->v3error("Incorrect number of input values, expected " << m_உள்ளீடு_Vars.size()
                                                                         << ", got " << inputvars);
        }

        string மாறிலி oValName = nodep->udpIsCombo() ? oNodep->name() : oNodep->nextp()->name();
        எனில் (oValName == "-") {
            எனில் (edgetrigp) VL_DO_DANGLING(தள்ளு_Deletep(edgetrigp), edgetrigp);
            எனில் (logandp) VL_DO_DANGLING(தள்ளு_Deletep(logandp), logandp);
            திரும்பு;
        }

        எனில் (!nodep->udpIsCombo()) {
            எனில் (oNodep->name() == "0") {
                logandp = புதிய AstLogAnd{
                    fl, logandp,
                    புதிய AstLogNot{fl, புதிய AstVarRef{fl, m_oFieldVarp, VAccess::READ}}};
            } இல்லையெனில் எனில் (oNodep->name() == "1") {
                logandp
                    = புதிய AstLogAnd{fl, logandp, புதிய AstVarRef{fl, m_oFieldVarp, VAccess::READ}};
            } இல்லையெனில் எனில் (oNodep->name() == "x" || oNodep->name() == "X") {
                // No x inputs supported yet, so this whole table line
                // can never match. Drop the whole thing.
                எனில் (edgetrigp) VL_DO_DANGLING(தள்ளு_Deletep(edgetrigp), edgetrigp);
                எனில் (logandp) VL_DO_DANGLING(தள்ளு_Deletep(logandp), logandp);
                திரும்பு;
            }
        }

        fl->warnOff(V3ErrorCode::LATCH, உண்மை);
        // Edge-triggered lines are clocked logic, so update like a flop:
        // other clocked logic reading the output sees the pre-edge value, regardless of ordering
        Ast_முனை_Stmt* மாறிலி assignp
            = newOutputAssign(fl, get_Output_இலக்கம்(nodep, oValName), edgetrigp != nullptr);
        AstIf* மாறிலி ifp = புதிய AstIf{fl, logandp, assignp};
        எனில் (nodep->udpIsCombo()) {
            எனில் (!isCombOutputSig(oValName)) {
                oNodep->v3error("Illegal value for combinational UDP line output");
            }
            m_alwaysBlockp->addStmtsp(ifp);
            எனில் (edgetrigp) VL_DO_DANGLING(தள்ளு_Deletep(edgetrigp), edgetrigp);
            திரும்பு;
        }
        எனில் (!isSequentOutputSig(oValName)) {
            oNodep->nextp()->v3error("Illegal value for sequential UDP line output");
        }
        m_alwaysBlockp->addNext(புதிய AstAlways{fl, VAlwaysKwd::ALWAYS, edgetrigp, ifp});
    }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }
    வெறுமை visit(AstLogAnd* nodep) override { iterateChildren(nodep); }
    வெறுமை visit(AstLogNot* nodep) override { iterateChildren(nodep); }
    // For logic processing.
    நிலையான தருக்க isEdgeTrig(std::string& valName) {
        எனில் (valName == "x" || valName == "X") திரும்பு பொய்;
        எனில் (valName == "*") திரும்பு உண்மை;
        எனில் (valName == "01" || valName == "p" || valName == "P" || valName == "r"
            || valName == "R") {
            valName = "r";
            திரும்பு உண்மை;
        }
        எனில் (valName == "10" || valName == "n" || valName == "N" || valName == "f"
            || valName == "F") {
            valName = "f";
            திரும்பு உண்மை;
        }
        எனில் (valName.size() == 2) {
            எனில் (valName[0] == 'x' || valName[0] == 'X' || valName[1] == 'x' || valName[1] == 'X')
                valName = "x";
            இல்லையெனில் எனில் (valName[0] == '1' || valName[1] == '0')
                valName = "f";
            இல்லையெனில் எனில் (valName[0] == '0' || valName[1] == '1')
                valName = "r";
            திரும்பு உண்மை;
        }
        எனில் (valName[0] != '0' && valName[0] != '1') { valName = "?"; }
        திரும்பு பொய்;
    }
    நிலையான தருக்க isCombOutputSig(மாறிலி std::string& valName) {
        திரும்பு (valName == "0" || valName == "1" || valName == "x" || valName == "X");
    }
    நிலையான தருக்க isSequentOutputSig(மாறிலி std::string& valName) {
        திரும்பு (valName == "0" || valName == "1" || valName == "x" || valName == "X"
                || valName == "-");
    }
    Ast_முனை_Stmt* newOutputAssign(File_வரி* fl, மாறிலி V3_எண்& out_இலக்கம், தருக்க isDelayed) மாறிலி {
        AstVarRef* மாறிலி lhsp = புதிய AstVarRef{fl, m_oFieldVarp, VAccess::WRITE};
        AstConst* மாறிலி rhsp = புதிய AstConst{fl, out_இலக்கம்};
        எனில் (isDelayed) திரும்பு புதிய AstAssignDly{fl, lhsp, rhsp};
        திரும்பு புதிய AstAssign{fl, lhsp, rhsp};
    }
    நிலையான V3_எண் get_Output_இலக்கம்(Ast_முனை* nodep, மாறிலி std::string& fieldNames) {
        V3_எண் output_இலக்கம்{nodep, 1};
        எனில் (fieldNames == "0") {
            output_இலக்கம்.setBit(0, 0);
        } இல்லையெனில் எனில் (fieldNames == "1") {
            output_இலக்கம்.setBit(0, 1);
        } இல்லையெனில் {
            output_இலக்கம்.setBit(0, 'x');
        }
        திரும்பு output_இலக்கம்;
    }

பொது:
    // CONSTRUCTORS
    explicit UdpVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~UdpVisitor() override = இயல்பு;
};

வெறுமை V3Udp::udpResolve(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { மாறிலி UdpVisitor visitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("udp", 0, dump_மரம்_Either_Level() >= 3);
}
