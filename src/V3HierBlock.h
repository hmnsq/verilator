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

#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Graph.h"

#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <string>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <unordered_set>
#അന്തര്ഭാവ <utility>
#അന്തര്ഭാവ <vector>

class AstNetlist;
class Ast_കണ്ണി_Module;
class AstParamTypeDType;
class AstVar;

//######################################################################

class V3HierGraph final : പൊതു V3Graph {
പൊതു:
    V3HierGraph() = സ്ഥിരസ്ഥിതി;
    ~V3HierGraph() = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(V3HierGraph);
    VL_UNMOVABLE(V3HierGraph);

    // Explicit HDL inputs followed by the library-discovered top module file, if any.
    സ്ഥിതം VStringList sourceFiles(സ്ഥിരം string& മുകൾ_Module_File = "") VL_MT_DISABLED;
    // Write command line arguments to .f files for child Verilation run
    ശൂന്യം writeCommandArgsFiles(bool forMkJson) സ്ഥിരം VL_MT_DISABLED;
    ശൂന്യം writeParametersFiles() സ്ഥിരം VL_MT_DISABLED;
    സ്ഥിതം string മുകൾ_Command_Args_Filename(bool forMkJson) VL_MT_DISABLED;
};

class V3HierBlock final : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(V3HierBlock, V3GraphVertex)

    // TYPES
    // Parameter name, stringified value
    using StrGParam = std::pair<string, string>;
    using StrGParams = std::vector<StrGParam>;

    // MEMBERS
    സ്ഥിരം AstModule* സ്ഥിരം m_modp;  // Hierarchical block module
    // Value parameters that are overridden by #(.param(value)) syntax.
    സ്ഥിരം std::vector<AstVar*> m_params;
    // Types parameters that are overridden by #(.param(value)) syntax.
    സ്ഥിരം std::vector<AstParamTypeDType*> m_typeParams;

    // METHODS
    സ്ഥിതം StrGParams stringifyParams(സ്ഥിരം std::vector<AstVar*>& params,
                                      bool forGOption) VL_MT_DISABLED;

പൊതു:
    // CONSTRUCTORs
    V3HierBlock(V3HierGraph* graphp, സ്ഥിരം AstModule* modp, സ്ഥിരം std::vector<AstVar*>& params,
                സ്ഥിരം std::vector<AstParamTypeDType*>& typeParams)
        : V3GraphVertex{graphp}
        , m_modp{modp}
        , m_params{params}
        , m_typeParams{typeParams} {}
    ~V3HierBlock() VL_MT_DISABLED = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(V3HierBlock);
    VL_UNMOVABLE(V3HierBlock);

    സ്ഥിരം AstModule* modp() സ്ഥിരം { തിരികെ m_modp; }

    // Whether a string parameter value can be passed to a hierarchical block's Verilation
    സ്ഥിതം bool stringParamPassable(സ്ഥിരം string& മൂല്യം) VL_PURE;

    // For emitting Makefile and build definition JSON
    VStringList commandArgs(bool forMkJson) സ്ഥിരം VL_MT_DISABLED;
    VStringList hierBlockArgs() സ്ഥിരം VL_MT_DISABLED;
    string hierPrefix() സ്ഥിരം VL_MT_DISABLED;
    string hierSomeFilename(bool withDir, സ്ഥിരം അക്ഷരം* prefix,
                            സ്ഥിരം അക്ഷരം* suffix) സ്ഥിരം VL_MT_DISABLED;
    string hierWrapperFilename(bool withDir) സ്ഥിരം VL_MT_DISABLED;
    string hierMkFilename(bool withDir) സ്ഥിരം VL_MT_DISABLED;
    string hierLibFilename(bool withDir) സ്ഥിരം VL_MT_DISABLED;
    string hierGeneratedFilenames(bool withDir) സ്ഥിരം VL_MT_DISABLED;
    // Returns the original HDL file if it is not included in v3Global.opt.vFiles().
    string vFileIfNecessary() സ്ഥിരം VL_MT_DISABLED;
    // Write command line arguments to .f file for this hierarchical block
    ശൂന്യം writeCommandArgsFile(bool forMkJson) സ്ഥിരം VL_MT_DISABLED;
    ശൂന്യം writeParametersFile() സ്ഥിരം VL_MT_DISABLED;
    string commandArgsFilename(bool forMkJson) സ്ഥിരം VL_MT_DISABLED;
    string typeParametersFilename() സ്ഥിരം VL_MT_DISABLED;

    // For Graphviz dumps only
    std::string name() സ്ഥിരം override { തിരികെ m_modp->prettyNameQ(); }
    std::string dotShape() സ്ഥിരം override { തിരികെ "box"; }
};

//######################################################################
// Pass to create hierarcical block graph

class V3Hierarchical final {
പൊതു:
    സ്ഥിതം ശൂന്യം createGraph(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // guard
