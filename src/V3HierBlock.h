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

#ifndef VERILATOR_V3HIERBLOCK_H_
#define VERILATOR_V3HIERBLOCK_H_

#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Graph.h"

#અન્તર્ભાવ <map>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <string>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <unordered_set>
#અન્તર્ભાવ <utility>
#અન્તર્ભાવ <vector>

class AstNetlist;
class Ast_ગાંઠ_Module;
class AstParamTypeDType;
class AstVar;

//######################################################################

class V3HierGraph final : જાહેર V3Graph {
જાહેર:
    V3HierGraph() = મૂળભૂત;
    ~V3HierGraph() = મૂળભૂત;
    VL_UNCOPYABLE(V3HierGraph);
    VL_UNMOVABLE(V3HierGraph);

    // Explicit HDL inputs followed by the library-discovered top module file, if any.
    સ્થિર VStringList sourceFiles(અચલ string& ટોચ_Module_File = "") VL_MT_DISABLED;
    // Write command line arguments to .f files for child Verilation run
    શૂન્ય writeCommandArgsFiles(bool forMkJson) અચલ VL_MT_DISABLED;
    શૂન્ય writeParametersFiles() અચલ VL_MT_DISABLED;
    સ્થિર string ટોચ_Command_Args_Filename(bool forMkJson) VL_MT_DISABLED;
};

class V3HierBlock final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(V3HierBlock, V3GraphVertex)

    // TYPES
    // Parameter name, stringified value
    using StrGParam = std::pair<string, string>;
    using StrGParams = std::vector<StrGParam>;

    // MEMBERS
    અચલ AstModule* અચલ m_modp;  // Hierarchical block module
    // Value parameters that are overridden by #(.param(value)) syntax.
    અચલ std::vector<AstVar*> m_params;
    // Types parameters that are overridden by #(.param(value)) syntax.
    અચલ std::vector<AstParamTypeDType*> m_typeParams;

    // METHODS
    સ્થિર StrGParams stringifyParams(અચલ std::vector<AstVar*>& params,
                                      bool forGOption) VL_MT_DISABLED;

જાહેર:
    // CONSTRUCTORs
    V3HierBlock(V3HierGraph* graphp, અચલ AstModule* modp, અચલ std::vector<AstVar*>& params,
                અચલ std::vector<AstParamTypeDType*>& typeParams)
        : V3GraphVertex{graphp}
        , m_modp{modp}
        , m_params{params}
        , m_typeParams{typeParams} {}
    ~V3HierBlock() VL_MT_DISABLED = મૂળભૂત;
    VL_UNCOPYABLE(V3HierBlock);
    VL_UNMOVABLE(V3HierBlock);

    અચલ AstModule* modp() અચલ { પાછા m_modp; }

    // Whether a string parameter value can be passed to a hierarchical block's Verilation
    સ્થિર bool stringParamPassable(અચલ string& કિંમત) VL_PURE;

    // For emitting Makefile and build definition JSON
    VStringList commandArgs(bool forMkJson) અચલ VL_MT_DISABLED;
    VStringList hierBlockArgs() અચલ VL_MT_DISABLED;
    string hierPrefix() અચલ VL_MT_DISABLED;
    string hierSomeFilename(bool withDir, અચલ અક્ષર* prefix,
                            અચલ અક્ષર* suffix) અચલ VL_MT_DISABLED;
    string hierWrapperFilename(bool withDir) અચલ VL_MT_DISABLED;
    string hierMkFilename(bool withDir) અચલ VL_MT_DISABLED;
    string hierLibFilename(bool withDir) અચલ VL_MT_DISABLED;
    string hierGeneratedFilenames(bool withDir) અચલ VL_MT_DISABLED;
    // Returns the original HDL file if it is not included in v3Global.opt.vFiles().
    string vFileIfNecessary() અચલ VL_MT_DISABLED;
    // Write command line arguments to .f file for this hierarchical block
    શૂન્ય writeCommandArgsFile(bool forMkJson) અચલ VL_MT_DISABLED;
    શૂન્ય writeParametersFile() અચલ VL_MT_DISABLED;
    string commandArgsFilename(bool forMkJson) અચલ VL_MT_DISABLED;
    string typeParametersFilename() અચલ VL_MT_DISABLED;

    // For Graphviz dumps only
    std::string name() અચલ override { પાછા m_modp->prettyNameQ(); }
    std::string dotShape() અચલ override { પાછા "box"; }
};

//######################################################################
// Pass to create hierarcical block graph

class V3Hierarchical final {
જાહેર:
    સ્થિર શૂન્ય createGraph(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // guard
