// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Verilator Control Files (.vlt) handling
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2010-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3Control.h"

#ಅನ್ತರ್ಭಾವ "V3InstrCount.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T>
ವರ್ಗ V3ControlWildcardResolver final {
    ಬದಲಾಯಿಸಬಹುದಾದ V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<ಸ್ಥಿರಾಂಕ std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<ಸ್ಥಿರಾಂಕ std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

ಸಾರ್ವಜನಿಕ:
    V3ControlWildcardResolver() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~V3ControlWildcardResolver() = ಪೂರ್ವನಿಯೋಜಿತ;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    ಶೂನ್ಯ update(ಸ್ಥಿರಾಂಕ V3ControlWildcardResolver& other) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{m_mutex};
        ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard other_ಬೀಗ{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : other.m_mapPatterns) m_mapPatterns[itr.ಮೊದಲ].update(itr.ಎರಡನೇ);
    }

    // Access and create a pattern entry
    T& at(ಸ್ಥಿರಾಂಕ string& name) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        ಹಿಂತಿರುಗಿ m_mapPatterns[V3Os::filename_Slash_ಮಾರ್ಗ(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(ಸ್ಥಿರಾಂಕ string& name) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{m_mutex};
        // Lookup if it was resolved before, typically not
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_mapResolved.emplace(name, ಶೂನ್ಯಸೂಚಕ);
        std::unique_ptr<T>& entryr = pair.ಮೊದಲ->ಎರಡನೇ;
        // Resolve entry when first requested, cache the result
        ಆದರೆ (pair.ಎರಡನೇ) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            ಸ್ಥಿರಾಂಕ string slashName = V3Os::filename_Slash_ಮಾರ್ಗ(name);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& patEnt : m_mapPatterns) {
                ಆದರೆ (VString::wildmatch(slashName, patEnt.ಮೊದಲ)) {
                    ಆದರೆ (!entryr) entryr.reset(ಹೊಸ T{});
                    entryr->update(patEnt.ಎರಡನೇ);
                }
            }
            // Perform final actions that needed all updates completed
            ಆದರೆ (entryr) entryr->updateFinalize();
        }
        ಹಿಂತಿರುಗಿ entryr.get();
    }
};

// List of attributes for variables
ವರ್ಗ V3ControlVar final {
    std::ಸದಿಶ<VAttrType> m_attrs;  // The list of attributes
ಸಾರ್ವಜನಿಕ:
    // Add new attribugte
    ಶೂನ್ಯ ಕೂಡಿಸು(VAttrType attr) { m_attrs.emplace_ಹಿಂಭಾಗ(attr); }
    // Update from other by copying all attributes
    ಶೂನ್ಯ update(ಸ್ಥಿರಾಂಕ V3ControlVar& other) {
        m_attrs.reserve(m_attrs.ಗಾತ್ರ_2() + other.m_attrs.ಗಾತ್ರ_2());
        m_attrs.ಸೇರಿಸು(m_attrs.ಕೊನೆ(), other.m_attrs.begin(), other.m_attrs.ಕೊನೆ());
    }
    ಶೂನ್ಯ updateFinalize() {}
    // Apply all attributes to the variable
    ಶೂನ್ಯ apply(AstVar* varp) ಸ್ಥಿರಾಂಕ {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ VAttrType attr : m_attrs) {
            varp->ಕೂಡಿಸು_Attrsp(ಹೊಸ Ast_Attr_ದ{varp->fileline(), attr});
        }
    }
};

ಬಳಸಿ V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

ವರ್ಗ WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<ಸ್ಥಿರಾಂಕ std::string, ತಾರ್ಕಿಕ> m_mapPatterns;  // Pattern match results
    std::deque<string> m_ಸಾಲುಗಳು;  // Source text lines

    // METHODS
    ಸ್ಥಿರ WildcardContents& s() {  // Singleton
        ಸ್ಥಿರ WildcardContents s_s;
        ಹಿಂತಿರುಗಿ s_s;
    }
    ಶೂನ್ಯ clearCacheImp() { m_mapPatterns.clear(); }
    ಶೂನ್ಯ ತಳ್ಳು_ಪಠ್ಯ_Imp(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        ಸ್ಥಿರಾಂಕ string leftover = m_ಸಾಲುಗಳು.ಹಿಂಭಾಗ() + ಪಠ್ಯ;
        m_ಸಾಲುಗಳು.ತೆಗೆ_ಹಿಂಭಾಗ();

        // Insert line-by-line
        string::ಗಾತ್ರ_type ಸಾಲು_ಆರಂಭ = 0;
        ಆಗ (ನಿಜ) {
            ಸ್ಥಿರಾಂಕ string::ಗಾತ್ರ_type ಸಾಲು_ಕೊನೆ = leftover.ಕಂಡುಹಿಡಿ('\n', ಸಾಲು_ಆರಂಭ);
            ಆದರೆ (ಸಾಲು_ಕೊನೆ != string::npos) {
                ಸ್ಥಿರಾಂಕ string oneline(leftover, ಸಾಲು_ಆರಂಭ, ಸಾಲು_ಕೊನೆ - ಸಾಲು_ಆರಂಭ + 1);
                ಆದರೆ (oneline.ಗಾತ್ರ_2() > 1) m_ಸಾಲುಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_ಸಾಲುಗಳು.ಗಾತ್ರ_2() - 1) << "]: " << oneline);
                ಸಾಲು_ಆರಂಭ = ಸಾಲು_ಕೊನೆ + 1;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ನಿಲ್ಲಿಸು;
            }
        }
        // Keep leftover for next time
        m_ಸಾಲುಗಳು.emplace_ಹಿಂಭಾಗ(string(leftover, ಸಾಲು_ಆರಂಭ));  // Might be ""
        clearCacheImp();
    }

    ತಾರ್ಕಿಕ resolveUncachedImp(ಸ್ಥಿರಾಂಕ string& contentsRegexp) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& i : m_ಸಾಲುಗಳು) {
            ಆದರೆ (VString::wildmatch(i, contentsRegexp)) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ತಾರ್ಕಿಕ resolveCachedImp(ಸ್ಥಿರಾಂಕ string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        ಆದರೆ (contentsRegexp.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_mapPatterns.emplace(contentsRegexp, ಸುಳ್ಳು);
        ತಾರ್ಕಿಕ& entryr = pair.ಮೊದಲ->ಎರಡನೇ;
        // Resolve entry when first requested, cache the result
        ಆದರೆ (pair.ಎರಡನೇ) entryr = resolveUncachedImp(contentsRegexp);
        ಹಿಂತಿರುಗಿ entryr;
    }

ಸಾರ್ವಜನಿಕ:
    WildcardContents() {
        m_ಸಾಲುಗಳು.emplace_ಹಿಂಭಾಗ("");  // start with no leftover
    }
    ~WildcardContents() = ಪೂರ್ವನಿಯೋಜಿತ;
    // Return true iff contentsRegexp in parsed contents
    ಸ್ಥಿರ ತಾರ್ಕಿಕ resolve(ಸ್ಥಿರಾಂಕ string& contentsRegexp) {
        ಹಿಂತಿರುಗಿ s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    ಸ್ಥಿರ ಶೂನ್ಯ ತಳ್ಳು_ಪಠ್ಯ(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) { s().ತಳ್ಳು_ಪಠ್ಯ_Imp(ಪಠ್ಯ); }
};

//######################################################################
// Function or task: Have variables and properties

ವರ್ಗ V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    ತಾರ್ಕಿಕ m_noinline = ಸುಳ್ಳು;  // Don't inline function/task
    ತಾರ್ಕಿಕ m_ಬಹಿರಂಗ = ಸುಳ್ಳು;  // Public function/task

ಸಾರ್ವಜನಿಕ:
    V3ControlFTask() = ಪೂರ್ವನಿಯೋಜಿತ;
    ಶೂನ್ಯ update(ಸ್ಥಿರಾಂಕ V3ControlFTask& f) {
        // Don't overwrite true with false
        ಆದರೆ (f.m_noinline) m_noinline = ನಿಜ;
        ಆದರೆ (f.m_ಬಹಿರಂಗ) m_ಬಹಿರಂಗ = ನಿಜ;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    ಶೂನ್ಯ updateFinalize() {}

    V3ControlVarResolver& params() { ಹಿಂತಿರುಗಿ m_params; }
    V3ControlVarResolver& ports() { ಹಿಂತಿರುಗಿ m_ports; }
    V3ControlVarResolver& vars() { ಹಿಂತಿರುಗಿ m_vars; }

    ಶೂನ್ಯ ಗಣ_No_Inline(ತಾರ್ಕಿಕ ಗಣ) { m_noinline = ಗಣ; }
    ಶೂನ್ಯ ಗಣ_ಬಹಿರಂಗ(ತಾರ್ಕಿಕ ಗಣ) { m_ಬಹಿರಂಗ = ಗಣ; }

    ಶೂನ್ಯ apply(Ast_ಗಂಟು_FTask* ftaskp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_noinline)
            ftaskp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        ಆದರೆ (m_ಬಹಿರಂಗ)
            ftaskp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstPragma{ftaskp->fileline(), VPragmaType::ಬಹಿರಂಗ_TASK});
    }
};

ಬಳಸಿ V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

ವರ್ಗ V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::Fsm_ರಿಜಿಸ್ಟರ್_Wrapper m_fsm_ರಿಜಿಸ್ಟರ್_Wrapper;  // FSM wrapper descriptor
    std::unordered_ಗಣ<std::string> m_coverage_Off_ಖಂಡಗಳು;  // List of block names for coverage_off
    std::ಗಣ<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    ತಾರ್ಕಿಕ m_has_Fsm_ರಿಜಿಸ್ಟರ್_Wrapper = ಸುಳ್ಳು;  // Whether descriptor has been set
    ತಾರ್ಕಿಕ m_inline = ಸುಳ್ಳು;  // Whether to force the inline
    ತಾರ್ಕಿಕ m_inline_ಮೌಲ್ಯ = ಸುಳ್ಳು;  // The inline value (on/off)

ಸಾರ್ವಜನಿಕ:
    V3ControlModule() = ಪೂರ್ವನಿಯೋಜಿತ;

    ಶೂನ್ಯ update(ಸ್ಥಿರಾಂಕ V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        ಆದರೆ (m.m_has_Fsm_ರಿಜಿಸ್ಟರ್_Wrapper) {
            m_fsm_ರಿಜಿಸ್ಟರ್_Wrapper = m.m_fsm_ರಿಜಿಸ್ಟರ್_Wrapper;
            m_has_Fsm_ರಿಜಿಸ್ಟರ್_Wrapper = ನಿಜ;
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& i : m.m_coverage_Off_ಖಂಡಗಳು) m_coverage_Off_ಖಂಡಗಳು.ಸೇರಿಸು(i);
        ಆದರೆ (!m_inline) {
            m_inline = m.m_inline;
            m_inline_ಮೌಲ್ಯ = m.m_inline_ಮೌಲ್ಯ;
        }
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.ಸೇರಿಸು(*it);
        }
    }
    ಶೂನ್ಯ updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { ಹಿಂತಿರುಗಿ m_tasks; }
    V3ControlVarResolver& params() { ಹಿಂತಿರುಗಿ m_params; }
    V3ControlVarResolver& ports() { ಹಿಂತಿರುಗಿ m_ports; }
    V3ControlVarResolver& vars() { ಹಿಂತಿರುಗಿ m_vars; }

    ಶೂನ್ಯ ಕೂಡಿಸು_Coverage_ಖಂಡ_Off(ಸ್ಥಿರಾಂಕ string& name) { m_coverage_Off_ಖಂಡಗಳು.ಸೇರಿಸು(name); }
    ಶೂನ್ಯ ಗಣ_Inline(ತಾರ್ಕಿಕ ಗಣ) {
        m_inline = ನಿಜ;
        m_inline_ಮೌಲ್ಯ = ಗಣ;
    }
    ಶೂನ್ಯ ಗಣ_Fsm_ರಿಜಿಸ್ಟರ್_Wrapper(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ V3Control::Fsm_ರಿಜಿಸ್ಟರ್_Wrapper& desc) {
        ಆದರೆ (m_has_Fsm_ರಿಜಿಸ್ಟರ್_Wrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_ಗಂಟು::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsm_ರಿಜಿಸ್ಟರ್_Wrapper = desc;
        m_has_Fsm_ರಿಜಿಸ್ಟರ್_Wrapper = ನಿಜ;
    }
    ಸ್ಥಿರಾಂಕ V3Control::Fsm_ರಿಜಿಸ್ಟರ್_Wrapper* fsm_ರಿಜಿಸ್ಟರ್_Wrapperp() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_has_Fsm_ರಿಜಿಸ್ಟರ್_Wrapper ? &m_fsm_ರಿಜಿಸ್ಟರ್_Wrapper : ಶೂನ್ಯಸೂಚಕ;
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_Module_Pragma(VPragmaType pragma) { m_modPragmas.ಸೇರಿಸು(pragma); }

    ಶೂನ್ಯ apply(Ast_ಗಂಟು_Module* modp) {
        ಆದರೆ (m_inline) {
            ಸ್ಥಿರಾಂಕ VPragmaType type
                = m_inline_ಮೌಲ್ಯ ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = ಹೊಸ AstPragma{modp->fileline(), type};
            modp->ಕೂಡಿಸು_Stmtsp(nodep);
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            ಆದರೆ (itr == VPragmaType::HIER_PARAMS) modp->hierParams(ನಿಜ);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = ಹೊಸ AstPragma{modp->fileline(), itr};
            modp->ಕೂಡಿಸು_Stmtsp(nodep);
        }
    }

    ಶೂನ್ಯ apply_ಖಂಡ(Ast_Gen_ಖಂಡ* nodep) {
        ಸ್ಥಿರಾಂಕ VPragmaType pragma = VPragmaType::COVERAGE_ಖಂಡ_OFF;
        ಆದರೆ (!nodep->unnamed()) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& i : m_coverage_Off_ಖಂಡಗಳು) {
                ಆದರೆ (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->ಕೂಡಿಸು_Itemsp(ಹೊಸ AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    ಶೂನ್ಯ apply_ಖಂಡ(Ast_ಗಂಟು_ಖಂಡ* nodep) {
        ಸ್ಥಿರಾಂಕ VPragmaType pragma = VPragmaType::COVERAGE_ಖಂಡ_OFF;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        ಆದರೆ (!nodep->unnamed()) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& i : m_coverage_Off_ಖಂಡಗಳು) {
                ಆದರೆ (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstStmtPragma{flp, ಹೊಸ AstPragma{flp, pragma}});
                }
            }
        }
    }
};

ಬಳಸಿ V3ControlModuleResolver = V3ControlWildcardResolver<V3ControlModule>;

//######################################################################
// Files have:
//  - Line ignores (lint/coverage/tracing on/off)
//  - Line attributes: Attributes attached to lines

// lint/coverage/tracing on/off
ವರ್ಗ V3_Control_Ignores_ಸಾಲು final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_ಸಾಲು_Min;  // Minimum line number to make change at
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_ಸಾಲು_Max;  // Maximum line number to make change at (inclusive)
    ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ m_ಸಂಕೇತ;  // Error code
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_on;  // True to enable message
    V3_Control_Ignores_ಸಾಲು(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, ಪೂರ್ಣಾಂಕ linemin, ಪೂರ್ಣಾಂಕ linemax, ತಾರ್ಕಿಕ on)
        : m_ಸಾಲು_Min{linemin}
        , m_ಸಾಲು_Max{linemax}
        , m_ಸಂಕೇತ{ಸಂಕೇತ_2}
        , m_on{on} {}
    ~V3_Control_Ignores_ಸಾಲು() = ಪೂರ್ವನಿಯೋಜಿತ;
    ತಾರ್ಕಿಕ every_ಸಾಲು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸಾಲು_Min == 0 && m_ಸಾಲು_Max == 0; }
    ತಾರ್ಕಿಕ ಒಳ_Range(ಪೂರ್ಣಾಂಕ lineno) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ (lineno >= m_ಸಾಲು_Min && lineno <= m_ಸಾಲು_Max) || every_ಸಾಲು();
    }
};
std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ V3_Control_Ignores_ಸಾಲು& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.m_ಸಾಲು_Min << "-" << rhs.m_ಸಾಲು_Max << ", " << rhs.m_ಸಂಕೇತ << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
ಸ್ಥಿರ std::ಸದಿಶ<V3_Control_Ignores_ಸಾಲು> control_Ign_ಸಾಲುಗಳು;

ಬಳಸಿ IgnIndices = std::ಸದಿಶ<uint32_t>;  // List of {s_ignLines indces}

ವರ್ಗ VInterval_ವೃಕ್ಷ final {
    ರಚನೆ ನಮೂದು_2 final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        ಪೂರ್ಣಾಂಕ m_center;  // Line number of central point
        std::unique_ptr<ನಮೂದು_2> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<ನಮೂದು_2> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        ಸ್ಪಷ್ಟ ನಮೂದು_2(ಪೂರ್ಣಾಂಕ center)
            : m_center{center} {}
    };

    std::unique_ptr<ನಮೂದು_2> m_rootp;  // Root of interval tree
    IgnIndices m_every_ಸಾಲುಗಳು;  // All-line disables

ಖಾಸಗಿ:
    std::unique_ptr<ನಮೂದು_2> build_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ IgnIndices& points) {
        ಆದರೆ (points.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

        ಪೂರ್ಣಾಂಕ minval = std::numeric_limits<ಪೂರ್ಣಾಂಕ>::max();
        ಪೂರ್ಣಾಂಕ maxval = std::numeric_limits<ಪೂರ್ಣಾಂಕ>::min();
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& it : points) {
            ಸ್ಥಿರಾಂಕ V3_Control_Ignores_ಸಾಲು& cign = control_Ign_ಸಾಲುಗಳು[it];
            ಆದರೆ (cign.every_ಸಾಲು()) {
                m_every_ಸಾಲುಗಳು.emplace_ಹಿಂಭಾಗ(it);
                ಮುಂದುವರಿ;
            }
            minval = std::min(minval, cign.m_ಸಾಲು_Min);
            maxval = std::max(maxval, cign.m_ಸಾಲು_Max);
        }

        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ center = minval + (maxval - minval) / 2;
        ಸ್ವಯಂಚಾಲಿತ entp = std::unique_ptr<ನಮೂದು_2>(ಹೊಸ ನಮೂದು_2{center});

        IgnIndices ಎಡ_Points;
        IgnIndices ಬಲ_Points;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& it : points) {
            ಸ್ಥಿರಾಂಕ V3_Control_Ignores_ಸಾಲು& cign = control_Ign_ಸಾಲುಗಳು[it];
            ಆದರೆ (cign.every_ಸಾಲು()) ಮುಂದುವರಿ;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            ಆದರೆ (cign.m_ಸಾಲು_Max < center)
                ಎಡ_Points.emplace_ಹಿಂಭಾಗ(it);
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (cign.m_ಸಾಲು_Min > center)
                ಬಲ_Points.emplace_ಹಿಂಭಾಗ(it);
            ಇಲ್ಲದಿದ್ದರೆ {
                entp->m_byMin.ತಳ್ಳು_ಹಿಂಭಾಗ(it);
                entp->m_byMax.ತಳ್ಳು_ಹಿಂಭಾಗ(it);
            }
        }

        std::ವಿಂಗಡಿಸು(entp->m_byMin.begin(), entp->m_byMin.ಕೊನೆ(), [](uint32_t a, uint32_t b) {
            ಹಿಂತಿರುಗಿ control_Ign_ಸಾಲುಗಳು[a].m_ಸಾಲು_Min < control_Ign_ಸಾಲುಗಳು[b].m_ಸಾಲು_Min;
        });
        std::ವಿಂಗಡಿಸು(entp->m_byMax.begin(), entp->m_byMax.ಕೊನೆ(), [](uint32_t a, uint32_t b) {
            ಹಿಂತಿರುಗಿ control_Ign_ಸಾಲುಗಳು[a].m_ಸಾಲು_Max > control_Ign_ಸಾಲುಗಳು[b].m_ಸಾಲು_Max;
        });

        entp->m_leftp = build_ವೃಕ್ಷ(ಎಡ_Points);
        entp->m_rightp = build_ವೃಕ್ಷ(ಬಲ_Points);
        ಹಿಂತಿರುಗಿ entp;
    }

    ಶೂನ್ಯ ಕಂಡುಹಿಡಿ_ವೃಕ್ಷ(ಪೂರ್ಣಾಂಕ lineno, ನಮೂದು_2* entp, IgnIndices& resultsr, ಪೂರ್ಣಾಂಕ& ಮುಂದಿನ_Changer) ಸ್ಥಿರಾಂಕ {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        ಆದರೆ (lineno < entp->m_center) {
            ಮುಂದಿನ_Changer = std::min(ಮುಂದಿನ_Changer, entp->m_center);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& it : entp->m_byMin) {
                ಸ್ಥಿರಾಂಕ V3_Control_Ignores_ಸಾಲು& cign = control_Ign_ಸಾಲುಗಳು[it];
                ಆದರೆ (cign.m_ಸಾಲು_Min <= lineno) {
                    resultsr.emplace_ಹಿಂಭಾಗ(it);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಮುಂದಿನ_Changer = std::min(ಮುಂದಿನ_Changer, cign.m_ಸಾಲು_Min);
                    ನಿಲ್ಲಿಸು;
                }
            }
            ಆದರೆ (entp->m_leftp) ಕಂಡುಹಿಡಿ_ವೃಕ್ಷ(lineno, entp->m_leftp.get(), resultsr, ಮುಂದಿನ_Changer);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lineno > entp->m_center) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& it : entp->m_byMax) {
                ಸ್ಥಿರಾಂಕ V3_Control_Ignores_ಸಾಲು& cign = control_Ign_ಸಾಲುಗಳು[it];
                ಆದರೆ (cign.m_ಸಾಲು_Max >= lineno) {
                    ಮುಂದಿನ_Changer = std::min(ಮುಂದಿನ_Changer, cign.m_ಸಾಲು_Max + 1);
                    resultsr.emplace_ಹಿಂಭಾಗ(it);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ನಿಲ್ಲಿಸು;
                }
            }
            ಆದರೆ (entp->m_rightp) ಕಂಡುಹಿಡಿ_ವೃಕ್ಷ(lineno, entp->m_rightp.get(), resultsr, ಮುಂದಿನ_Changer);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& it : entp->m_byMin) {
                ಸ್ಥಿರಾಂಕ V3_Control_Ignores_ಸಾಲು& cign = control_Ign_ಸಾಲುಗಳು[it];
                resultsr.emplace_ಹಿಂಭಾಗ(it);
                ಆದರೆ (cign.m_ಸಾಲು_Max >= lineno)
                    ಮುಂದಿನ_Changer = std::min(ಮುಂದಿನ_Changer, cign.m_ಸಾಲು_Max + 1);
            }
            ಆದರೆ (entp->m_leftp) ಕಂಡುಹಿಡಿ_ವೃಕ್ಷ(lineno, entp->m_leftp.get(), resultsr, ಮುಂದಿನ_Changer);
            ಆದರೆ (entp->m_rightp) ಕಂಡುಹಿಡಿ_ವೃಕ್ಷ(lineno, entp->m_rightp.get(), resultsr, ಮುಂದಿನ_Changer);
        }
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    VInterval_ವೃಕ್ಷ() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~VInterval_ವೃಕ್ಷ() = ಪೂರ್ವನಿಯೋಜಿತ;
    // METHODS
    ಶೂನ್ಯ clear() { m_rootp = ಶೂನ್ಯಸೂಚಕ; }
    ಶೂನ್ಯ build(ಸ್ಥಿರಾಂಕ IgnIndices& points) {
        clear();
        m_rootp = build_ವೃಕ್ಷ(points);
    }
    ಶೂನ್ಯ ಕಂಡುಹಿಡಿ(ಪೂರ್ಣಾಂಕ lineno, IgnIndices& resultsr, ಪೂರ್ಣಾಂಕ& ಮುಂದಿನ_Changer) ಸ್ಥಿರಾಂಕ {
        resultsr = m_every_ಸಾಲುಗಳು;
        ಮುಂದಿನ_Changer = std::numeric_limits<ಪೂರ್ಣಾಂಕ>::max();

        ಆದರೆ (m_rootp) ಕಂಡುಹಿಡಿ_ವೃಕ್ಷ(lineno, m_rootp.get(), resultsr, ಮುಂದಿನ_Changer);
        // Sort indices, so can process in parse order
        std::ವಿಂಗಡಿಸು(resultsr.begin(), resultsr.ಕೊನೆ());
    }

    ಸ್ಥಿರ ಶೂನ್ಯ selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices ದತ್ತಾಂಶ;
        std::ಸದಿಶ<std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& it : points) {
            control_Ign_ಸಾಲುಗಳು.emplace_ಹಿಂಭಾಗ(
                V3_Control_Ignores_ಸಾಲು{V3_Error_ಸಂಕೇತ::I_LINT, it.ಮೊದಲ, it.ಎರಡನೇ, ನಿಜ});
            ದತ್ತಾಂಶ.emplace_ಹಿಂಭಾಗ(static_cast<uint32_t>(control_Ign_ಸಾಲುಗಳು.ಗಾತ್ರ_2() - 1));
        }
        VInterval_ವೃಕ್ಷ ವೃಕ್ಷ;
        ವೃಕ್ಷ.build(ದತ್ತಾಂಶ);

        IgnIndices results;
        ಪೂರ್ಣಾಂಕ ಮುಂದಿನ_Change = 0;
        ವೃಕ್ಷ.ಕಂಡುಹಿಡಿ(0, results, ಮುಂದಿನ_Change);
        UASSERT_SELFTEST(results.ಗಾತ್ರ_2(), 0);
        UASSERT_SELFTEST(ಮುಂದಿನ_Change, 10);
        ವೃಕ್ಷ.ಕಂಡುಹಿಡಿ(10, results, ಮುಂದಿನ_Change);
        UASSERT_SELFTEST(results.ಗಾತ್ರ_2(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(ಮುಂದಿನ_Change, 11);
        ವೃಕ್ಷ.ಕಂಡುಹಿಡಿ(11, results, ಮುಂದಿನ_Change);
        UASSERT_SELFTEST(results.ಗಾತ್ರ_2(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(ಮುಂದಿನ_Change, 15);  // Center, or would be 20
        ವೃಕ್ಷ.ಕಂಡುಹಿಡಿ(20, results, ಮುಂದಿನ_Change);
        UASSERT_SELFTEST(results.ಗಾತ್ರ_2(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(ಮುಂದಿನ_Change, 21);
        ವೃಕ್ಷ.ಕಂಡುಹಿಡಿ(21, results, ಮುಂದಿನ_Change);
        UASSERT_SELFTEST(results.ಗಾತ್ರ_2(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(ಮುಂದಿನ_Change, 25);  // Center, or would be 30
        ವೃಕ್ಷ.ಕಂಡುಹಿಡಿ(30, results, ಮುಂದಿನ_Change);
        UASSERT_SELFTEST(results.ಗಾತ್ರ_2(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(ಮುಂದಿನ_Change, 31);
        ವೃಕ್ಷ.ಕಂಡುಹಿಡಿ(40, results, ಮುಂದಿನ_Change);
        UASSERT_SELFTEST(results.ಗಾತ್ರ_2(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(ಮುಂದಿನ_Change, 41);
        ವೃಕ್ಷ.ಕಂಡುಹಿಡಿ(41, results, ಮುಂದಿನ_Change);
        UASSERT_SELFTEST(results.ಗಾತ್ರ_2(), 0);
        UASSERT_SELFTEST(ಮುಂದಿನ_Change, std::numeric_limits<ಪೂರ್ಣಾಂಕ>::max());
        //
        points = {{0, 0}};
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& it : points) {
            control_Ign_ಸಾಲುಗಳು.emplace_ಹಿಂಭಾಗ(
                V3_Control_Ignores_ಸಾಲು{V3_Error_ಸಂಕೇತ::I_LINT, it.ಮೊದಲ, it.ಎರಡನೇ, ನಿಜ});
            ದತ್ತಾಂಶ.emplace_ಹಿಂಭಾಗ(static_cast<uint32_t>(control_Ign_ಸಾಲುಗಳು.ಗಾತ್ರ_2() - 1));
        }
        ವೃಕ್ಷ.build(ದತ್ತಾಂಶ);
        //
        ವೃಕ್ಷ.ಕಂಡುಹಿಡಿ(50, results, ಮುಂದಿನ_Change);
        UASSERT_SELFTEST(results.ಗಾತ್ರ_2(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
ಬಳಸಿ V3_Control_ಸಾಲು_Attribute = std::bitset<VPragmaType::ENUM_ಗಾತ್ರ>;

ವರ್ಗ WaiverSetting final {
ಸಾರ್ವಜನಿಕ:
    V3_Error_ಸಂಕೇತ m_ಸಂಕೇತ;  // Error code
    string m_contents;  // --contents regexp
    string m_ಹೊಂದಿಕೆ;  // --match regexp
    WaiverSetting(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, ಸ್ಥಿರಾಂಕ string& contents, ಸ್ಥಿರಾಂಕ string& ಹೊಂದಿಕೆ)
        : m_ಸಂಕೇತ{ಸಂಕೇತ_2}
        , m_contents{contents}
        , m_ಹೊಂದಿಕೆ{ಹೊಂದಿಕೆ} {}
    ~WaiverSetting() = ಪೂರ್ವನಿಯೋಜಿತ;
    WaiverSetting& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಿರಾಂಕ WaiverSetting& rhs) = ಪೂರ್ವನಿಯೋಜಿತ;
};

// File entity
ವರ್ಗ V3ControlFile final {
    ಬಳಸಿ ಸಾಲು_Attr_Map = std::map<ಪೂರ್ಣಾಂಕ, V3_Control_ಸಾಲು_Attribute>;  // Map line->bitset of attributes
    ಬಳಸಿ Waivers = std::ಸದಿಶ<WaiverSetting>;  // List of {code,wildcard string}

    ಸಾಲು_Attr_Map m_ಸಾಲು_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_ವೃಕ್ಷ m_interval_ವೃಕ್ಷ;  // Tree of indices to IgnLines

    ರಚನೆ {
        ಪೂರ್ಣಾಂಕ filenameno = -1;  // Last filename
        ಪೂರ್ಣಾಂಕ lineno = -1;  // Last linenumber
        ಪೂರ್ಣಾಂಕ ಮುಂದಿನ_Change = -1;  // Line number of next change
    } m_ಕೊನೆಯ_Ignore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    ತಾರ್ಕಿಕ ಸಾಲು_ಹೊಂದಿಕೆ(ಪೂರ್ಣಾಂಕ lineno, VPragmaType type) {
        ಆದರೆ (m_ಸಾಲು_Attrs.ಕಂಡುಹಿಡಿ(0) != m_ಸಾಲು_Attrs.ಕೊನೆ() && m_ಸಾಲು_Attrs[0][type]) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (m_ಸಾಲು_Attrs.ಕಂಡುಹಿಡಿ(lineno) == m_ಸಾಲು_Attrs.ಕೊನೆ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ m_ಸಾಲು_Attrs[lineno][type];
    }

ಸಾರ್ವಜನಿಕ:
    V3ControlFile() = ಪೂರ್ವನಿಯೋಜಿತ;
    ಶೂನ್ಯ update(ಸ್ಥಿರಾಂಕ V3ControlFile& file) {
        // Copy in all attributes and waivers
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : file.m_ಸಾಲು_Attrs) m_ಸಾಲು_Attrs[itr.ಮೊದಲ] |= itr.ಎರಡನೇ;
        m_waivers.reserve(m_waivers.ಗಾತ್ರ_2() + file.m_waivers.ಗಾತ್ರ_2());
        m_waivers.ಸೇರಿಸು(m_waivers.ಕೊನೆ(), file.m_waivers.begin(), file.m_waivers.ಕೊನೆ());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.ಗಾತ್ರ_2() + file.m_ignIndices.ಗಾತ್ರ_2());
        m_ignIndices.ಸೇರಿಸು(m_ignIndices.ಕೊನೆ(), file.m_ignIndices.begin(),
                            file.m_ignIndices.ಕೊನೆ());
        // updateFinalize() will soon build tree
    }
    ಶೂನ್ಯ updateFinalize() { m_interval_ವೃಕ್ಷ.build(m_ignIndices); }
    ಶೂನ್ಯ ಕೂಡಿಸು_ಸಾಲು_Attribute(ಪೂರ್ಣಾಂಕ lineno, VPragmaType attr) { m_ಸಾಲು_Attrs[lineno].ಗಣ(attr); }
    ಶೂನ್ಯ ಕೂಡಿಸು_Ignore(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, ಪೂರ್ಣಾಂಕ min, ಪೂರ್ಣಾಂಕ max, ತಾರ್ಕಿಕ on) {
        control_Ign_ಸಾಲುಗಳು.emplace_ಹಿಂಭಾಗ(V3_Control_Ignores_ಸಾಲು{ಸಂಕೇತ_2, min, max, on});
        m_ignIndices.emplace_ಹಿಂಭಾಗ(static_cast<uint32_t>(control_Ign_ಸಾಲುಗಳು.ಗಾತ್ರ_2() - 1));
        m_ಕೊನೆಯ_Ignore.ಮುಂದಿನ_Change = -1;
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_Ignore_ಹೊಂದಿಕೆ(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, ಸ್ಥಿರಾಂಕ string& contents, ಸ್ಥಿರಾಂಕ string& ಹೊಂದಿಕೆ) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string new_ಹೊಂದಿಕೆ = ಹೊಂದಿಕೆ;
        ಆದರೆ (new_ಹೊಂದಿಕೆ.ಖಾಲಿಗಳು() || new_ಹೊಂದಿಕೆ.ಹಿಂಭಾಗ() != '*') new_ಹೊಂದಿಕೆ += '*';
        m_waivers.emplace_ಹಿಂಭಾಗ(ಸಂಕೇತ_2, contents, new_ಹೊಂದಿಕೆ);
    }

    ಶೂನ್ಯ apply_ಖಂಡ(Ast_Gen_ಖಂಡ* nodep) {
        // Apply to block at this line
        ಸ್ಥಿರಾಂಕ VPragmaType pragma = VPragmaType::COVERAGE_ಖಂಡ_OFF;
        ಆದರೆ (ಸಾಲು_ಹೊಂದಿಕೆ(nodep->fileline()->lineno(), pragma)) {
            nodep->ಕೂಡಿಸು_Itemsp(ಹೊಸ AstPragma{nodep->fileline(), pragma});
        }
    }
    ಶೂನ್ಯ apply_ಖಂಡ(Ast_ಗಂಟು_ಖಂಡ* nodep) {
        // Apply to block at this line
        ಸ್ಥಿರಾಂಕ VPragmaType pragma = VPragmaType::COVERAGE_ಖಂಡ_OFF;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        ಆದರೆ (ಸಾಲು_ಹೊಂದಿಕೆ(flp->lineno(), pragma)) {
            nodep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstStmtPragma{flp, ಹೊಸ AstPragma{flp, pragma}});
        }
    }
    ಶೂನ್ಯ applyCase(AstCase* nodep) {
        // Apply to this case at this line
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lineno = nodep->fileline()->lineno();
        ಆದರೆ (ಸಾಲು_ಹೊಂದಿಕೆ(lineno, VPragmaType::ತುಂಬಿದ_CASE)) nodep->ತುಂಬಿದ_Pragma(ನಿಜ);
        ಆದರೆ (ಸಾಲು_ಹೊಂದಿಕೆ(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(ನಿಜ);
    }
    ಶೂನ್ಯ applyIgnores(File_ಸಾಲು* filelinep) {
        // HOT routine, called each parsed token line of this filename
        ಆದರೆ (filelinep->filenameno() == m_ಕೊನೆಯ_Ignore.filenameno
            && filelinep->lineno() == m_ಕೊನೆಯ_Ignore.lineno)
            ಹಿಂತಿರುಗಿ;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        ಆದರೆ (filelinep->filenameno() == m_ಕೊನೆಯ_Ignore.filenameno
            && filelinep->lineno() < m_ಕೊನೆಯ_Ignore.ಮುಂದಿನ_Change) {
            m_ಕೊನೆಯ_Ignore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            ಹಿಂತಿರುಗಿ;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_ಕೊನೆಯ_Ignore.filenameno = filelinep->filenameno();
        m_ಕೊನೆಯ_Ignore.lineno = filelinep->lineno();
        m_ಕೊನೆಯ_Ignore.ಮುಂದಿನ_Change = std::numeric_limits<ಪೂರ್ಣಾಂಕ>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_ವೃಕ್ಷ.ಕಂಡುಹಿಡಿ(filelinep->lineno(), results /*ref*/, m_ಕೊನೆಯ_Ignore.ಮುಂದಿನ_Change /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VError_ಬಿಟ್_ಗಣ bitset{VError_ಬಿಟ್_ಗಣ::AllOnes{}};
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& it : results) {
            ಸ್ಥಿರಾಂಕ V3_Control_Ignores_ಸಾಲು& cign = control_Ign_ಸಾಲುಗಳು[it];
            UASSERT(cign.ಒಳ_Range(filelinep->lineno()),
                    "Interval tree returned lines outside range");
            UINFO(9, "     Hit " << cign);
            cign.m_ಸಂಕೇತ.forDelegateCodes(
                [&](V3_Error_ಸಂಕೇತ subcode) { bitset.ಗಣ(subcode, cign.m_on); });
        }

        filelinep->warn_ಗಣ_Ctrl_ಬಿಟ್_ಗಣ(bitset);
        m_ಕೊನೆಯ_Ignore.ಮುಂದಿನ_Change = std::max(m_ಕೊನೆಯ_Ignore.ಮುಂದಿನ_Change, filelinep->lineno() + 1);
        UINFO(9, "    AppliedIgnores " << filelinep << " next@ln " << std::dec
                                       << m_ಕೊನೆಯ_Ignore.ಮುಂದಿನ_Change
                                       << " Messages: " << bitset.ascii());
    }
    ತಾರ್ಕಿಕ waive(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, ಸ್ಥಿರಾಂಕ string& ಸಂದೇಶ) {
        ಆದರೆ (ಸಂಕೇತ_2.hardError()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : m_waivers) {
            ಆದರೆ ((ಸಂಕೇತ_2.isUnder(itr.m_ಸಂಕೇತ) || (itr.m_ಸಂಕೇತ == V3_Error_ಸಂಕೇತ::I_LINT))
                && VString::wildmatch(ಸಂದೇಶ, itr.m_ಹೊಂದಿಕೆ)
                && WildcardContents::resolve(itr.m_contents)) {
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
};

ಬಳಸಿ V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

ವರ್ಗ V3_Control_Scope_Trace_ನಮೂದು final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರಾಂಕ string m_scope;  // Scope or regexp to match
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_on;  // True to enable message
    ಪೂರ್ಣಾಂಕ m_ಮಟ್ಟಗಳು = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3_Control_Scope_Trace_ನಮೂದು(ಸ್ಥಿರಾಂಕ string& scope, ತಾರ್ಕಿಕ on, ಪೂರ್ಣಾಂಕ ಮಟ್ಟಗಳು)
        : m_scope{scope}
        , m_on{on}
        , m_ಮಟ್ಟಗಳು{ಮಟ್ಟಗಳು} {}
    ~V3_Control_Scope_Trace_ನಮೂದು() = ಪೂರ್ವನಿಯೋಜಿತ;
    ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ V3_Control_Scope_Trace_ನಮೂದು& other) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_on < other.m_on) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (m_on > other.m_on) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (m_ಮಟ್ಟಗಳು < other.m_ಮಟ್ಟಗಳು) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (m_ಮಟ್ಟಗಳು > other.m_ಮಟ್ಟಗಳು) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
ವರ್ಗ V3_Control_Scope_Trace_ನಮೂದು_ಹೊಂದಿಕೆ final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರಾಂಕ V3_Control_Scope_Trace_ನಮೂದು* m_entryp;  // Entry the match refers to
    ಸ್ಥಿರಾಂಕ string m_scopepart;  // Scope prefix being matched against the entry
    V3_Control_Scope_Trace_ನಮೂದು_ಹೊಂದಿಕೆ(ಸ್ಥಿರಾಂಕ V3_Control_Scope_Trace_ನಮೂದು* entryp, ಸ್ಥಿರಾಂಕ string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ V3_Control_Scope_Trace_ನಮೂದು_ಹೊಂದಿಕೆ& other) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_entryp < other.m_entryp) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (m_entryp > other.m_entryp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ m_scopepart < other.m_scopepart;
    }
};

ವರ್ಗ V3ControlScopeTraceResolver final {
    std::ಸದಿಶ<V3_Control_Scope_Trace_ನಮೂದು> m_entries;  // User specified on/offs and levels
    std::map<V3_Control_Scope_Trace_ನಮೂದು_ಹೊಂದಿಕೆ, ತಾರ್ಕಿಕ> m_ಹೊಂದಿಕೆ_Cache;  // Matching entries for speed

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ ಕೂಡಿಸು_Scope_Trace_On(ತಾರ್ಕಿಕ on, ಸ್ಥಿರಾಂಕ string& scope, ಪೂರ್ಣಾಂಕ ಮಟ್ಟಗಳು) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << ಮಟ್ಟಗಳು);
        m_entries.emplace_ಹಿಂಭಾಗ(V3_Control_Scope_Trace_ನಮೂದು{scope, on, ಮಟ್ಟಗಳು});
        m_ಹೊಂದಿಕೆ_Cache.clear();
    }

    ತಾರ್ಕಿಕ get_ನಮೂದು_ಹೊಂದಿಕೆ(ಸ್ಥಿರಾಂಕ V3_Control_Scope_Trace_ನಮೂದು* entp, ಸ್ಥಿರಾಂಕ string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        ಸ್ಥಿರಾಂಕ V3_Control_Scope_Trace_ನಮೂದು_ಹೊಂದಿಕೆ ಕೀಲಿ{entp, scopepart};
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_ಹೊಂದಿಕೆ_Cache.emplace(ಕೀಲಿ, ಸುಳ್ಳು);
        ಆದರೆ (pair.ಎರಡನೇ) pair.ಮೊದಲ->ಎರಡನೇ = VString::wildmatch(scopepart, entp->m_scope);
        ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;
    }

    ತಾರ್ಕಿಕ getScopeTraceOn(ಸ್ಥಿರಾಂಕ string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        ಪೂರ್ಣಾಂಕ maxLevel = 1;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ch : scope) {
            ಆದರೆ (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        ತಾರ್ಕಿಕ enabled = ನಿಜ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            ಪೂರ್ಣಾಂಕ partLevel = 1;
            ಗಾಗಿ (string::ಗಾತ್ರ_type part_ಕೊನೆ = 0; ನಿಜ;) {
                part_ಕೊನೆ = scope.ಕಂಡುಹಿಡಿ('.', part_ಕೊನೆ + 1);
                ಆದರೆ (part_ಕೊನೆ == string::npos) part_ಕೊನೆ = scope.ಉದ್ದಳತೆ();
                ಸ್ಥಿರಾಂಕ std::string scopepart = scope.substr(0, part_ಕೊನೆ);
                ಆದರೆ (get_ನಮೂದು_ಹೊಂದಿಕೆ(&ent, scopepart)) {
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ level_ಹೊಂದಿಕೆ
                        = !ent.m_ಮಟ್ಟಗಳು || (ent.m_ಮಟ್ಟಗಳು >= maxLevel - partLevel);
                    ಆದರೆ (level_ಹೊಂದಿಕೆ) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (level_ಹೊಂದಿಕೆ ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    ನಿಲ್ಲಿಸು;
                }
                ಆದರೆ (part_ಕೊನೆ == scope.ಉದ್ದಳತೆ()) ನಿಲ್ಲಿಸು;
                ++partLevel;
            }
        }
        ಹಿಂತಿರುಗಿ enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

ವರ್ಗ V3_Control_Resolver_Hier_ಕೆಲಸಗಾರ_ನಮೂದು final {
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_workers;  // Number of workers requested for the hierarchical block
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ m_flp;  // Location the worker count was specified at

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ V3_Control_Resolver_Hier_ಕೆಲಸಗಾರ_ನಮೂದು(ಪೂರ್ಣಾಂಕ workers, File_ಸಾಲು* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    ಪೂರ್ಣಾಂಕ workers() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_workers; }
    File_ಸಾಲು* flp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_flp; }
};

ವರ್ಗ V3ControlResolver final {
    ಎಣಿಕೆ Profile_ದತ್ತಾಂಶ_ಬಹುಲಕ : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profile_ದತ್ತಾಂಶ;  // Access to profile_data records
    uint8_t m_ಬಹುಲಕ = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3_Control_Resolver_Hier_ಕೆಲಸಗಾರ_ನಮೂದು> m_hierWorkers;
    File_ಸಾಲು* m_profile_File_ಸಾಲು = ಶೂನ್ಯಸೂಚಕ;  // Location profile_data was read from

    V3ControlResolver() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~V3ControlResolver() = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ V3ControlResolver& s() {
        ಸ್ಥಿರ V3ControlResolver s_singleton;
        ಹಿಂತಿರುಗಿ s_singleton;
    }
    V3ControlModuleResolver& modules() { ಹಿಂತಿರುಗಿ m_modules; }
    V3ControlFileResolver& files() { ಹಿಂತಿರುಗಿ m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { ಹಿಂತಿರುಗಿ m_scopeTraces; }

    ಶೂನ್ಯ ಕೂಡಿಸು_Profile_ದತ್ತಾಂಶ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& hierDpi, uint64_t ವೆಚ್ಚ) {
        // Empty key for hierarchical DPI wrapper costs.
        ಕೂಡಿಸು_Profile_ದತ್ತಾಂಶ(fl, hierDpi, "", ವೆಚ್ಚ, HIER_DPI);
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_Profile_ದತ್ತಾಂಶ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& model, ಸ್ಥಿರಾಂಕ string& ಕೀಲಿ, uint64_t ವೆಚ್ಚ,
                        Profile_ದತ್ತಾಂಶ_ಬಹುಲಕ ಬಹುಲಕ = MTASK) {
        ಆದರೆ (!m_profile_File_ಸಾಲು) m_profile_File_ಸಾಲು = fl;
        ಆದರೆ (ವೆಚ್ಚ == 0) ವೆಚ್ಚ = 1;  // Cost 0 means delete (or no data)
        ಆದರೆ (ಬಹುಲಕ == MTASK) {
            m_profile_ದತ್ತಾಂಶ[model][ಕೀಲಿ] += ವೆಚ್ಚ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಬಹುಲಕ == HIER_DPI) {
            m_profile_ದತ್ತಾಂಶ[model][ಕೀಲಿ] = std::max(m_profile_ದತ್ತಾಂಶ[model][ಕೀಲಿ], ವೆಚ್ಚ);
        }
        m_ಬಹುಲಕ |= ಬಹುಲಕ;
    }
    ತಾರ್ಕಿಕ ಒಳಗೊಂಡಿದೆ_MTask_Profile_ದತ್ತಾಂಶ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಹುಲಕ & MTASK; }
    uint64_t get_Profile_ದತ್ತಾಂಶ(ಸ್ಥಿರಾಂಕ string& hierDpi) ಸ್ಥಿರಾಂಕ {
        // Empty key for hierarchical DPI wrapper costs.
        ಹಿಂತಿರುಗಿ get_Profile_ದತ್ತಾಂಶ(hierDpi, "");
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_Hier_Workers(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ string& model, ಪೂರ್ಣಾಂಕ workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    ಪೂರ್ಣಾಂಕ getHierWorkers(ಸ್ಥಿರಾಂಕ string& model) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ mit = m_hierWorkers.ಕಂಡುಹಿಡಿ(model);
        // Assign a single worker if no specified.
        ಹಿಂತಿರುಗಿ mit != m_hierWorkers.cend() ? mit->ಎರಡನೇ.workers() : 0;
    }
    File_ಸಾಲು* get_Hier_Workers_File_ಸಾಲು(ಸ್ಥಿರಾಂಕ string& model) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ mit = m_hierWorkers.ಕಂಡುಹಿಡಿ(model);
        ಹಿಂತಿರುಗಿ mit != m_hierWorkers.cend() ? mit->ಎರಡನೇ.flp() : v3Global.rootp()->fileline();
    }
    uint64_t get_Profile_ದತ್ತಾಂಶ(ಸ್ಥಿರಾಂಕ string& model, ಸ್ಥಿರಾಂಕ string& ಕೀಲಿ) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ mit = m_profile_ದತ್ತಾಂಶ.ಕಂಡುಹಿಡಿ(model);
        ಆದರೆ (mit == m_profile_ದತ್ತಾಂಶ.cend()) ಹಿಂತಿರುಗಿ 0;
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = mit->ಎರಡನೇ.ಕಂಡುಹಿಡಿ(ಕೀಲಿ);
        ಆದರೆ (it == mit->ಎರಡನೇ.cend()) ಹಿಂತಿರುಗಿ 0;
        ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;
    }
    File_ಸಾಲು* get_Profile_ದತ್ತಾಂಶ_File_ಸಾಲು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_profile_File_ಸಾಲು; }  // Maybe null
    ಸ್ಥಿರ uint64_t get_Current_Hier_ಖಂಡ_ವೆಚ್ಚ() {
        ಆದರೆ (uint64_t ವೆಚ್ಚ = V3Control::get_Profile_ದತ್ತಾಂಶ(v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ())) {
            UINFO(9, "Fetching cost from profile info: " << ವೆಚ್ಚ);
            ಹಿಂತಿರುಗಿ ವೆಚ್ಚ;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* ಸ್ಥಿರಾಂಕ netlistp = v3Global.rootp();
        uint64_t ವೆಚ್ಚ = 0;
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < VEval::ENUM_ಕೊನೆ; ++i) {
            ಸ್ಥಿರಾಂಕ VEval eval{i};
            ಆದರೆ (eval.slow()) ಮುಂದುವರಿ;
            ವೆಚ್ಚ += V3_Instr_ಎಣಿಕೆ::ಎಣಿಕೆ_2(netlistp->evalFuncp(eval), ಸುಳ್ಳು);
        }
        UINFO(9, "Evaluating cost: " << ವೆಚ್ಚ);
        ಹಿಂತಿರುಗಿ ವೆಚ್ಚ;
    }
};

//######################################################################
// V3Control

ಶೂನ್ಯ V3Control::ಕೂಡಿಸು_Case_ತುಂಬಿದ(ಸ್ಥಿರಾಂಕ string& filename, ಪೂರ್ಣಾಂಕ lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.ಕೂಡಿಸು_ಸಾಲು_Attribute(lineno, VPragmaType::ತುಂಬಿದ_CASE);
}
ಶೂನ್ಯ V3Control::ಕೂಡಿಸು_Case_Parallel(ಸ್ಥಿರಾಂಕ string& filename, ಪೂರ್ಣಾಂಕ lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.ಕೂಡಿಸು_ಸಾಲು_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
ಶೂನ್ಯ V3Control::ಕೂಡಿಸು_Coverage_ಖಂಡ_Off(ಸ್ಥಿರಾಂಕ string& filename, ಪೂರ್ಣಾಂಕ lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.ಕೂಡಿಸು_ಸಾಲು_Attribute(lineno, VPragmaType::COVERAGE_ಖಂಡ_OFF);
}
ಶೂನ್ಯ V3Control::ಕೂಡಿಸು_Coverage_ಖಂಡ_Off(ಸ್ಥಿರಾಂಕ string& module, ಸ್ಥಿರಾಂಕ string& blockname) {
    V3ControlResolver::s().modules().at(module).ಕೂಡಿಸು_Coverage_ಖಂಡ_Off(blockname);
}

ಶೂನ್ಯ V3Control::ಕೂಡಿಸು_Hier_Workers(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& model, ಪೂರ್ಣಾಂಕ workers) {
    V3ControlResolver::s().ಕೂಡಿಸು_Hier_Workers(fl, model, workers);
}

ಶೂನ್ಯ V3Control::ಕೂಡಿಸು_Fsm_ರಿಜಿಸ್ಟರ್_Wrapper(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& module, ಸ್ಥಿರಾಂಕ string& d,
                                      ಸ್ಥಿರಾಂಕ string& q, ಸ್ಥಿರಾಂಕ string& clock, ಸ್ಥಿರಾಂಕ string& reset,
                                      ಸ್ಥಿರಾಂಕ string& reset_ಮೌಲ್ಯ) {
    string missing;
    ಆದರೆ (module.ಖಾಲಿಗಳು()) missing += "-module";
    ಆದರೆ (d.ಖಾಲಿಗಳು()) missing = VString::dot(missing, ", ", "-d");
    ಆದರೆ (q.ಖಾಲಿಗಳು()) missing = VString::dot(missing, ", ", "-q");
    ಆದರೆ (clock.ಖಾಲಿಗಳು()) missing = VString::dot(missing, ", ", "-clock");
    ಆದರೆ (!missing.ಖಾಲಿಗಳು()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿರಾಂಕ Fsm_ರಿಜಿಸ್ಟರ್_Wrapper desc{module, d, q, clock, reset, reset_ಮೌಲ್ಯ};
    V3ControlResolver::s().modules().at(module).ಗಣ_Fsm_ರಿಜಿಸ್ಟರ್_Wrapper(fl, desc);
}

ಶೂನ್ಯ V3Control::ಕೂಡಿಸು_Ignore(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, ತಾರ್ಕಿಕ on, ಸ್ಥಿರಾಂಕ string& filename, ಪೂರ್ಣಾಂಕ min, ಪೂರ್ಣಾಂಕ max) {
    UINFO(9, "addIgnore " << ಸಂಕೇತ_2 << " " << min << "-" << max << " fn=" << filename);
    ಆದರೆ (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_ಸಾಲು::globalWarnOff(ಸಂಕೇತ_2, !on);
    } ಇಲ್ಲದಿದ್ದರೆ {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).ಕೂಡಿಸು_Ignore(ಸಂಕೇತ_2, min, max, on);
    }
}
ಶೂನ್ಯ V3Control::ಕೂಡಿಸು_Ignore_ಹೊಂದಿಕೆ(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& contents,
                               ಸ್ಥಿರಾಂಕ string& ಹೊಂದಿಕೆ) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).ಕೂಡಿಸು_Ignore_ಹೊಂದಿಕೆ(ಸಂಕೇತ_2, contents, ಹೊಂದಿಕೆ);
}

ಶೂನ್ಯ V3Control::ಕೂಡಿಸು_Inline(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& module, ಸ್ಥಿರಾಂಕ string& ftask, ತಾರ್ಕಿಕ on) {
    ಆದರೆ (ftask.ಖಾಲಿಗಳು()) {
        V3ControlResolver::s().modules().at(module).ಗಣ_Inline(on);
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಆದರೆ (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } ಇಲ್ಲದಿದ್ದರೆ {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).ಗಣ_No_Inline(on);
        }
    }
}

ಶೂನ್ಯ V3Control::ಕೂಡಿಸು_Module_Pragma(ಸ್ಥಿರಾಂಕ string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).ಕೂಡಿಸು_Module_Pragma(pragma);
}

ಶೂನ್ಯ V3Control::ಕೂಡಿಸು_Profile_ದತ್ತಾಂಶ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& hierDpi, uint64_t ವೆಚ್ಚ) {
    V3ControlResolver::s().ಕೂಡಿಸು_Profile_ದತ್ತಾಂಶ(fl, hierDpi, ವೆಚ್ಚ);
}
ಶೂನ್ಯ V3Control::ಕೂಡಿಸು_Profile_ದತ್ತಾಂಶ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& model, ಸ್ಥಿರಾಂಕ string& ಕೀಲಿ,
                               uint64_t ವೆಚ್ಚ) {
    V3ControlResolver::s().ಕೂಡಿಸು_Profile_ದತ್ತಾಂಶ(fl, model, ಕೀಲಿ, ವೆಚ್ಚ);
}

ಶೂನ್ಯ V3Control::ಕೂಡಿಸು_Scope_Trace_On(ತಾರ್ಕಿಕ on, ಸ್ಥಿರಾಂಕ string& scope, ಪೂರ್ಣಾಂಕ ಮಟ್ಟಗಳು) {
    V3ControlResolver::s().scopeTraces().ಕೂಡಿಸು_Scope_Trace_On(on, scope, ಮಟ್ಟಗಳು);
}

ಶೂನ್ಯ V3Control::ಕೂಡಿಸು_Var_Attr(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& module, ಸ್ಥಿರಾಂಕ string& ftask,
                           Var_Spec_ಬಗೆ ಬಗೆ, ಸ್ಥಿರಾಂಕ string& ನಮೂನೆ_2, VAttrType attr,
                           Ast_Sen_ವೃಕ್ಷ* sensep) {
    ಆದರೆ (sensep) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_ವೃಕ್ಷ(), sensep);
        // Used to be only accepted on public_flat_rw
        ಆದರೆ (attr != VAttrType::VAR_ಬಹಿರಂಗ_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            ಹಿಂತಿರುಗಿ;
        }
    }

    ಆದರೆ (ಬಗೆ != Var_Spec_ಬಗೆ::VAR) {
        ಆಯ್ಕೆ (attr) {
        ಸ್ಥಿತಿ VAttrType::VAR_ಬಹಿರಂಗ_FLAT:
        ಸ್ಥಿತಿ VAttrType::VAR_ಬಹಿರಂಗ_FLAT_RD:
        ಸ್ಥಿತಿ VAttrType::VAR_ಬಹಿರಂಗ_FLAT_RW: ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            ಹಿಂತಿರುಗಿ;
        }
    }

    // Semantics: Most of the attributes operate on signals
    ಆದರೆ (ನಮೂನೆ_2.ಖಾಲಿಗಳು()) {
        ಆದರೆ (attr == VAttrType::VAR_ಬಹಿರಂಗ) {
            ಆದರೆ (ftask.ಖಾಲಿಗಳು()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).ಕೂಡಿಸು_Module_Pragma(
                    VPragmaType::ಬಹಿರಂಗ_MODULE);
            } ಇಲ್ಲದಿದ್ದರೆ {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).ಗಣ_ಬಹಿರಂಗ(ನಿಜ);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            fl->v3error("missing -var");
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಆದರೆ (attr == VAttrType::VAR_FORCEABLE) {
            ಆದರೆ (module.ಖಾಲಿಗಳು()) {
                fl->v3error("forceable missing -module");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!ftask.ಖಾಲಿಗಳು()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } ಇಲ್ಲದಿದ್ದರೆ {
                V3ControlResolver::s().modules().at(module).vars().at(ನಮೂನೆ_2).ಕೂಡಿಸು(attr);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                ಆದರೆ (ftask.ಖಾಲಿಗಳು()) {
                    ಆದರೆ (ಬಗೆ == Var_Spec_ಬಗೆ::PARAM) ಹಿಂತಿರುಗಿ mod.params().at(ನಮೂನೆ_2);
                    ಆದರೆ (ಬಗೆ == Var_Spec_ಬಗೆ::ಪೋರ್ಟ್) ಹಿಂತಿರುಗಿ mod.ports().at(ನಮೂನೆ_2);
                    UASSERT_OBJ(ಬಗೆ == Var_Spec_ಬಗೆ::VAR, fl, "Unexpected VarSpecKind");
                    ಹಿಂತಿರುಗಿ mod.vars().at(ನಮೂನೆ_2);
                }
                ಆದರೆ (ಬಗೆ == Var_Spec_ಬಗೆ::PARAM) ಹಿಂತಿರುಗಿ mod.ftasks().at(ftask).params().at(ನಮೂನೆ_2);
                ಆದರೆ (ಬಗೆ == Var_Spec_ಬಗೆ::ಪೋರ್ಟ್) ಹಿಂತಿರುಗಿ mod.ftasks().at(ftask).ports().at(ನಮೂನೆ_2);
                UASSERT_OBJ(ಬಗೆ == Var_Spec_ಬಗೆ::VAR, fl, "Unexpected VarSpecKind");
                ಹಿಂತಿರುಗಿ mod.ftasks().at(ftask).vars().at(ನಮೂನೆ_2);
            }();
            controlVar.ಕೂಡಿಸು(attr);
        }
    }
}

ಶೂನ್ಯ V3Control::applyCase(AstCase* nodep) {
    ಸ್ಥಿರಾಂಕ string& filename = nodep->fileline()->filename();
    V3ControlFile* ಸ್ಥಿರಾಂಕ filep = V3ControlResolver::s().files().resolve(filename);
    ಆದರೆ (filep) filep->applyCase(nodep);
}

ಶೂನ್ಯ V3Control::apply_Coverage_ಖಂಡ(Ast_ಗಂಟು_Module* modulep, AstBegin* nodep) {
    ಸ್ಥಿರಾಂಕ string& filename = nodep->fileline()->filename();
    V3ControlFile* ಸ್ಥಿರಾಂಕ filep = V3ControlResolver::s().files().resolve(filename);
    ಆದರೆ (filep) filep->apply_ಖಂಡ(nodep);
    ಸ್ಥಿರಾಂಕ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ಸ್ಥಿರಾಂಕ modp = V3ControlResolver::s().modules().resolve(modname);
    ಆದರೆ (modp) modp->apply_ಖಂಡ(nodep);
}
ಶೂನ್ಯ V3Control::apply_Coverage_ಖಂಡ(Ast_ಗಂಟು_Module* modulep, Ast_Gen_ಖಂಡ* nodep) {
    ಸ್ಥಿರಾಂಕ string& filename = nodep->fileline()->filename();
    V3ControlFile* ಸ್ಥಿರಾಂಕ filep = V3ControlResolver::s().files().resolve(filename);
    ಆದರೆ (filep) filep->apply_ಖಂಡ(nodep);
    ಸ್ಥಿರಾಂಕ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ಸ್ಥಿರಾಂಕ modp = V3ControlResolver::s().modules().resolve(modname);
    ಆದರೆ (modp) modp->apply_ಖಂಡ(nodep);
}

ಶೂನ್ಯ V3Control::applyIgnores(File_ಸಾಲು* filelinep) {
    ಸ್ಥಿರಾಂಕ string& filename = filelinep->filename();
    V3ControlFile* ಸ್ಥಿರಾಂಕ filep = V3ControlResolver::s().files().resolve(filename);
    ಆದರೆ (filep) filep->applyIgnores(filelinep);
}

ಶೂನ್ಯ V3Control::applyModule(Ast_ಗಂಟು_Module* modulep) {
    ಸ್ಥಿರಾಂಕ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ಸ್ಥಿರಾಂಕ modp = V3ControlResolver::s().modules().resolve(modname);
    ಆದರೆ (modp) modp->apply(modulep);
}

ಶೂನ್ಯ V3Control::applyFTask(Ast_ಗಂಟು_Module* modulep, Ast_ಗಂಟು_FTask* ftaskp) {
    ಸ್ಥಿರಾಂಕ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ಸ್ಥಿರಾಂಕ modp = V3ControlResolver::s().modules().resolve(modname);
    ಆದರೆ (!modp) ಹಿಂತಿರುಗಿ;
    ಸ್ಥಿರಾಂಕ V3ControlFTask* ಸ್ಥಿರಾಂಕ ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    ಆದರೆ (ftp) ftp->apply(ftaskp);
}

ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Resolver>
ಸ್ಥಿರ ಶೂನ್ಯ resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    ಸ್ಥಿರಾಂಕ std::string name = varp->prettyDehashOrigOrName();
    ಆದರೆ (ಸ್ಥಿರಾಂಕ V3ControlVar* ಸ್ಥಿರಾಂಕ vp = resolver.vars().resolve(name)) vp->apply(varp);
    ಆದರೆ (varp->isParam()) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ V3ControlVar* ಸ್ಥಿರಾಂಕ vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    ಆದರೆ (varp->isIO()) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ V3ControlVar* ಸ್ಥಿರಾಂಕ vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

ಶೂನ್ಯ V3Control::applyVarAttr(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modulep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* ಸ್ಥಿರಾಂಕ modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    ಆದರೆ (!modp) ಹಿಂತಿರುಗಿ;
    ಆದರೆ (ftaskp) {
        V3ControlFTask* ಸ್ಥಿರಾಂಕ ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        ಆದರೆ (!ftp) ಹಿಂತಿರುಗಿ;
        resolveThenApply(*ftp, varp);
    } ಇಲ್ಲದಿದ್ದರೆ {
        resolveThenApply(*modp, varp);
    }
}

ಪೂರ್ಣಾಂಕ V3Control::getHierWorkers(ಸ್ಥಿರಾಂಕ string& model) {
    ಹಿಂತಿರುಗಿ V3ControlResolver::s().getHierWorkers(model);
}
File_ಸಾಲು* V3Control::get_Hier_Workers_File_ಸಾಲು(ಸ್ಥಿರಾಂಕ string& model) {
    ಹಿಂತಿರುಗಿ V3ControlResolver::s().get_Hier_Workers_File_ಸಾಲು(model);
}
ಸ್ಥಿರಾಂಕ V3Control::Fsm_ರಿಜಿಸ್ಟರ್_Wrapper* V3Control::get_Fsm_ರಿಜಿಸ್ಟರ್_Wrapper(ಸ್ಥಿರಾಂಕ string& module) {
    V3ControlModule* ಸ್ಥಿರಾಂಕ modp = V3ControlResolver::s().modules().resolve(module);
    ಹಿಂತಿರುಗಿ modp ? modp->fsm_ರಿಜಿಸ್ಟರ್_Wrapperp() : ಶೂನ್ಯಸೂಚಕ;
}
uint64_t V3Control::get_Profile_ದತ್ತಾಂಶ(ಸ್ಥಿರಾಂಕ string& hierDpi) {
    ಹಿಂತಿರುಗಿ V3ControlResolver::s().get_Profile_ದತ್ತಾಂಶ(hierDpi);
}
uint64_t V3Control::get_Profile_ದತ್ತಾಂಶ(ಸ್ಥಿರಾಂಕ string& model, ಸ್ಥಿರಾಂಕ string& ಕೀಲಿ) {
    ಹಿಂತಿರುಗಿ V3ControlResolver::s().get_Profile_ದತ್ತಾಂಶ(model, ಕೀಲಿ);
}
File_ಸಾಲು* V3Control::get_Profile_ದತ್ತಾಂಶ_File_ಸಾಲು() {
    ಹಿಂತಿರುಗಿ V3ControlResolver::s().get_Profile_ದತ್ತಾಂಶ_File_ಸಾಲು();
}
ತಾರ್ಕಿಕ V3Control::getScopeTraceOn(ಸ್ಥಿರಾಂಕ string& scope) {
    ಹಿಂತಿರುಗಿ V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

ಶೂನ್ಯ V3Control::contents_ತಳ್ಳು_ಪಠ್ಯ(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) { WildcardContents::ತಳ್ಳು_ಪಠ್ಯ(ಪಠ್ಯ); }

ತಾರ್ಕಿಕ V3Control::ಒಳಗೊಂಡಿದೆ_MTask_Profile_ದತ್ತಾಂಶ() {
    ಹಿಂತಿರುಗಿ V3ControlResolver::s().ಒಳಗೊಂಡಿದೆ_MTask_Profile_ದತ್ತಾಂಶ();
}
uint64_t V3Control::get_Current_Hier_ಖಂಡ_ವೆಚ್ಚ() {
    ಹಿಂತಿರುಗಿ V3ControlResolver::s().get_Current_Hier_ಖಂಡ_ವೆಚ್ಚ();
}

ತಾರ್ಕಿಕ V3Control::waive(ಸ್ಥಿರಾಂಕ File_ಸಾಲು* filelinep, V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, ಸ್ಥಿರಾಂಕ string& ಸಂದೇಶ) {
    V3ControlFile* ಸ್ಥಿರಾಂಕ filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    ಆದರೆ (!filep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಹಿಂತಿರುಗಿ filep->waive(ಸಂಕೇತ_2, ಸಂದೇಶ);
}

ಶೂನ್ಯ V3Control::selfTest() { VInterval_ವೃಕ್ಷ::selfTest(); }
