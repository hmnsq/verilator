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

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"

#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>
#ಅನ್ತರ್ಭಾವ <utility>
#ಅನ್ತರ್ಭಾವ <vector>

class AstNetlist;
class Ast_ಗಂಟು_Module;
class AstParamTypeDType;
class AstVar;

//######################################################################

class V3HierGraph final : ಸಾರ್ವಜನಿಕ V3Graph {
ಸಾರ್ವಜನಿಕ:
    V3HierGraph() = ಸ್ವಭಾವ;
    ~V3HierGraph() = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(V3HierGraph);
    VL_UNMOVABLE(V3HierGraph);

    // Explicit HDL inputs followed by the library-discovered top module file, if any.
    ಸ್ಥಿರ VStringList sourceFiles(ಧ್ರುವ string& ತುದಿ_Module_File = "") VL_MT_DISABLED;
    // Write command line arguments to .f files for child Verilation run
    ಶೂನ್ಯ writeCommandArgsFiles(bool forMkJson) ಧ್ರುವ VL_MT_DISABLED;
    ಶೂನ್ಯ writeParametersFiles() ಧ್ರುವ VL_MT_DISABLED;
    ಸ್ಥಿರ string ತುದಿ_Command_Args_Filename(bool forMkJson) VL_MT_DISABLED;
};

class V3HierBlock final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(V3HierBlock, V3GraphVertex)

    // TYPES
    // Parameter name, stringified value
    using StrGParam = std::pair<string, string>;
    using StrGParams = std::vector<StrGParam>;

    // MEMBERS
    ಧ್ರುವ AstModule* ಧ್ರುವ m_modp;  // Hierarchical block module
    // Value parameters that are overridden by #(.param(value)) syntax.
    ಧ್ರುವ std::vector<AstVar*> m_params;
    // Types parameters that are overridden by #(.param(value)) syntax.
    ಧ್ರುವ std::vector<AstParamTypeDType*> m_typeParams;

    // METHODS
    ಸ್ಥಿರ StrGParams stringifyParams(ಧ್ರುವ std::vector<AstVar*>& params,
                                      bool forGOption) VL_MT_DISABLED;

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORs
    V3HierBlock(V3HierGraph* graphp, ಧ್ರುವ AstModule* modp, ಧ್ರುವ std::vector<AstVar*>& params,
                ಧ್ರುವ std::vector<AstParamTypeDType*>& typeParams)
        : V3GraphVertex{graphp}
        , m_modp{modp}
        , m_params{params}
        , m_typeParams{typeParams} {}
    ~V3HierBlock() VL_MT_DISABLED = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(V3HierBlock);
    VL_UNMOVABLE(V3HierBlock);

    ಧ್ರುವ AstModule* modp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_modp; }

    // Whether a string parameter value can be passed to a hierarchical block's Verilation
    ಸ್ಥಿರ bool stringParamPassable(ಧ್ರುವ string& ಮೌಲ್ಯ) VL_PURE;

    // For emitting Makefile and build definition JSON
    VStringList commandArgs(bool forMkJson) ಧ್ರುವ VL_MT_DISABLED;
    VStringList hierBlockArgs() ಧ್ರುವ VL_MT_DISABLED;
    string hierPrefix() ಧ್ರುವ VL_MT_DISABLED;
    string hierSomeFilename(bool withDir, ಧ್ರುವ ವರ್ಣ* prefix,
                            ಧ್ರುವ ವರ್ಣ* suffix) ಧ್ರುವ VL_MT_DISABLED;
    string hierWrapperFilename(bool withDir) ಧ್ರುವ VL_MT_DISABLED;
    string hierMkFilename(bool withDir) ಧ್ರುವ VL_MT_DISABLED;
    string hierLibFilename(bool withDir) ಧ್ರುವ VL_MT_DISABLED;
    string hierGeneratedFilenames(bool withDir) ಧ್ರುವ VL_MT_DISABLED;
    // Returns the original HDL file if it is not included in v3Global.opt.vFiles().
    string vFileIfNecessary() ಧ್ರುವ VL_MT_DISABLED;
    // Write command line arguments to .f file for this hierarchical block
    ಶೂನ್ಯ writeCommandArgsFile(bool forMkJson) ಧ್ರುವ VL_MT_DISABLED;
    ಶೂನ್ಯ writeParametersFile() ಧ್ರುವ VL_MT_DISABLED;
    string commandArgsFilename(bool forMkJson) ಧ್ರುವ VL_MT_DISABLED;
    string typeParametersFilename() ಧ್ರುವ VL_MT_DISABLED;

    // For Graphviz dumps only
    std::string name() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_modp->prettyNameQ(); }
    std::string dotShape() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "box"; }
};

//######################################################################
// Pass to create hierarcical block graph

class V3Hierarchical final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ createGraph(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // guard
