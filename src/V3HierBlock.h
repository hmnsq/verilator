// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Plan for Hierarchical Verilation
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

#నిర్వచితం_కాకపోతే VERILATOR_V3HIERBLOCK_H_
#నిర్వచించు VERILATOR_V3HIERBLOCK_H_

#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Graph.h"

#అన్తర్భావ <map>
#అన్తర్భావ <set>
#అన్తర్భావ <string>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <unordered_set>
#అన్తర్భావ <utility>
#అన్తర్భావ <vector>

class AstNetlist;
class Ast_కణుపు_Module;
class AstParamTypeDType;
class AstVar;

//######################################################################

class V3_Hier_లేఖాచిత్రం final : బహిరంగ V3_లేఖాచిత్రం {
బహిరంగ:
    V3_Hier_లేఖాచిత్రం() = అప్రమేయ;
    ~V3_Hier_లేఖాచిత్రం() = అప్రమేయ;
    VL_UNCOPYABLE(V3_Hier_లేఖాచిత్రం);
    VL_UNMOVABLE(V3_Hier_లేఖాచిత్రం);

    // Explicit HDL inputs followed by the library-discovered top module file, if any.
    స్థిర VStringList ఆధారం_Files(స్థిరరాశి string& శిఖరం_Module_File = "") VL_MT_DISABLED;
    // Write command line arguments to .f files for child Verilation run
    శూన్య write_Command_వాదనలు_Files(bool forMkJson) స్థిరరాశి VL_MT_DISABLED;
    శూన్య writeParametersFiles() స్థిరరాశి VL_MT_DISABLED;
    స్థిర string శిఖరం_Command_వాదనలు_Filename(bool forMkJson) VL_MT_DISABLED;
};

class V3_Hier_ఖండం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(V3_Hier_ఖండం, V3_లేఖాచిత్రం_శీర్షం)

    // TYPES
    // Parameter name, stringified value
    using StrGParam = std::pair<string, string>;
    using StrGParams = std::సదిశ<StrGParam>;

    // MEMBERS
    స్థిరరాశి AstModule* స్థిరరాశి m_modp;  // Hierarchical block module
    // Value parameters that are overridden by #(.param(value)) syntax.
    స్థిరరాశి std::సదిశ<AstVar*> m_params;
    // Types parameters that are overridden by #(.param(value)) syntax.
    స్థిరరాశి std::సదిశ<AstParamTypeDType*> m_typeParams;

    // METHODS
    స్థిర StrGParams stringifyParams(స్థిరరాశి std::సదిశ<AstVar*>& params,
                                      bool forGOption) VL_MT_DISABLED;

బహిరంగ:
    // CONSTRUCTORs
    V3_Hier_ఖండం(V3_Hier_లేఖాచిత్రం* graphp, స్థిరరాశి AstModule* modp, స్థిరరాశి std::సదిశ<AstVar*>& params,
                స్థిరరాశి std::సదిశ<AstParamTypeDType*>& typeParams)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_modp{modp}
        , m_params{params}
        , m_typeParams{typeParams} {}
    ~V3_Hier_ఖండం() VL_MT_DISABLED = అప్రమేయ;
    VL_UNCOPYABLE(V3_Hier_ఖండం);
    VL_UNMOVABLE(V3_Hier_ఖండం);

    స్థిరరాశి AstModule* modp() స్థిరరాశి { తిరిగి m_modp; }

    // Whether a string parameter value can be passed to a hierarchical block's Verilation
    స్థిర bool stringParamPassable(స్థిరరాశి string& విలువ) VL_శుద్ధ;

    // For emitting Makefile and build definition JSON
    VStringList command_వాదనలు(bool forMkJson) స్థిరరాశి VL_MT_DISABLED;
    VStringList hier_ఖండం_వాదనలు() స్థిరరాశి VL_MT_DISABLED;
    string hier_ఉపసర్గ() స్థిరరాశి VL_MT_DISABLED;
    string hierSomeFilename(bool withDir, స్థిరరాశి అక్షరం* ఉపసర్గ,
                            స్థిరరాశి అక్షరం* suffix) స్థిరరాశి VL_MT_DISABLED;
    string hierWrapperFilename(bool withDir) స్థిరరాశి VL_MT_DISABLED;
    string hierMkFilename(bool withDir) స్థిరరాశి VL_MT_DISABLED;
    string hierLibFilename(bool withDir) స్థిరరాశి VL_MT_DISABLED;
    string hierGeneratedFilenames(bool withDir) స్థిరరాశి VL_MT_DISABLED;
    // Returns the original HDL file if it is not included in v3Global.opt.vFiles().
    string vFileIfNecessary() స్థిరరాశి VL_MT_DISABLED;
    // Write command line arguments to .f file for this hierarchical block
    శూన్య write_Command_వాదనలు_File(bool forMkJson) స్థిరరాశి VL_MT_DISABLED;
    శూన్య writeParametersFile() స్థిరరాశి VL_MT_DISABLED;
    string command_వాదనలు_Filename(bool forMkJson) స్థిరరాశి VL_MT_DISABLED;
    string typeParametersFilename() స్థిరరాశి VL_MT_DISABLED;

    // For Graphviz dumps only
    std::string name() స్థిరరాశి override { తిరిగి m_modp->prettyNameQ(); }
    std::string dotShape() స్థిరరాశి override { తిరిగి "box"; }
};

//######################################################################
// Pass to create hierarcical block graph

class V3_క్రమానుగత final {
బహిరంగ:
    స్థిర శూన్య create_లేఖాచిత్రం(AstNetlist* nodep) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // guard
