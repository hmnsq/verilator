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

#समावेश "V3PchAstMT.h"

#समावेश "V3Control.h"

#समावेश "V3InstrCount.h"
#समावेश "V3Os.h"
#समावेश "V3String.h"

#समावेश <memory>
#समावेश <set>
#समावेश <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
ढाँचा <typename T>
श्रेणी V3ControlWildcardResolver final {
    mutable V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<स्थिर std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<स्थिर std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

खुला:
    V3ControlWildcardResolver() = शेष;
    ~V3ControlWildcardResolver() = शेष;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    व्योम update(स्थिर V3ControlWildcardResolver& other) VL_MT_SAFE_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        स्थिर V3LockGuard lock{m_mutex};
        स्थिर V3LockGuard otherLock{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.सफा();
        क्रम (स्थिर स्वतः& itr : other.m_mapPatterns) m_mapPatterns[itr.first].update(itr.second);
    }

    // Access and create a pattern entry
    T& at(स्थिर string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        स्थिर V3LockGuard lock{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.सफा();
        // Patterns use '/' as separator; normalize Windows backslashes
        वापस m_mapPatterns[V3Os::filenameSlashPath(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(स्थिर string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        स्थिर V3LockGuard lock{m_mutex};
        // Lookup if it was resolved before, typically not
        स्थिर स्वतः pair = m_mapResolved.emplace(name, nullptr);
        std::unique_ptr<T>& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        अगर (pair.second) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            स्थिर string slashName = V3Os::filenameSlashPath(name);
            क्रम (स्थिर स्वतः& patEnt : m_mapPatterns) {
                अगर (VString::wildmatch(slashName, patEnt.first)) {
                    अगर (!entryr) entryr.reset(नया T{});
                    entryr->update(patEnt.second);
                }
            }
            // Perform final actions that needed all updates completed
            अगर (entryr) entryr->updateFinalize();
        }
        वापस entryr.लो();
    }
};

// List of attributes for variables
श्रेणी V3ControlVar final {
    std::vector<VAttrType> m_attrs;  // The list of attributes
खुला:
    // Add new attribugte
    व्योम add(VAttrType attr) { m_attrs.emplace_back(attr); }
    // Update from other by copying all attributes
    व्योम update(स्थिर V3ControlVar& other) {
        m_attrs.reserve(m_attrs.आकार() + other.m_attrs.आकार());
        m_attrs.जोड़ो(m_attrs.end(), other.m_attrs.begin(), other.m_attrs.end());
    }
    व्योम updateFinalize() {}
    // Apply all attributes to the variable
    व्योम apply(AstVar* varp) स्थिर {
        क्रम (स्थिर VAttrType attr : m_attrs) {
            varp->addAttrsp(नया AstAttrOf{varp->fileline(), attr});
        }
    }
};

using V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

श्रेणी WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<स्थिर std::string, bool> m_mapPatterns;  // Pattern match results
    std::deque<string> m_पंक्तियाँ;  // Source text lines

    // METHODS
    अटल WildcardContents& s() {  // Singleton
        अटल WildcardContents s_s;
        वापस s_s;
    }
    व्योम clearCacheImp() { m_mapPatterns.सफा(); }
    व्योम धकेलो_Text_Imp(स्थिर string& text) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        स्थिर string leftover = m_पंक्तियाँ.back() + text;
        m_पंक्तियाँ.निकालो_back();

        // Insert line-by-line
        string::आकार_type पंक्ति_start = 0;
        जबतक (true) {
            स्थिर string::आकार_type पंक्ति_end = leftover.find('\n', पंक्ति_start);
            अगर (पंक्ति_end != string::npos) {
                स्थिर string oneline(leftover, पंक्ति_start, पंक्ति_end - पंक्ति_start + 1);
                अगर (oneline.आकार() > 1) m_पंक्तियाँ.धकेलो_back(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_पंक्तियाँ.आकार() - 1) << "]: " << oneline);
                पंक्ति_start = पंक्ति_end + 1;
            } अन्यथा {
                अवरोध;
            }
        }
        // Keep leftover for next time
        m_पंक्तियाँ.emplace_back(string(leftover, पंक्ति_start));  // Might be ""
        clearCacheImp();
    }

    bool resolveUncachedImp(स्थिर string& contentsRegexp) {
        क्रम (स्थिर string& i : m_पंक्तियाँ) {
            अगर (VString::wildmatch(i, contentsRegexp)) वापस true;
        }
        वापस false;
    }
    bool resolveCachedImp(स्थिर string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        अगर (contentsRegexp.empty()) वापस true;
        स्थिर स्वतः pair = m_mapPatterns.emplace(contentsRegexp, false);
        bool& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        अगर (pair.second) entryr = resolveUncachedImp(contentsRegexp);
        वापस entryr;
    }

खुला:
    WildcardContents() {
        m_पंक्तियाँ.emplace_back("");  // start with no leftover
    }
    ~WildcardContents() = शेष;
    // Return true iff contentsRegexp in parsed contents
    अटल bool resolve(स्थिर string& contentsRegexp) {
        वापस s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    अटल व्योम धकेलो_Text(स्थिर string& text) { s().धकेलो_Text_Imp(text); }
};

//######################################################################
// Function or task: Have variables and properties

श्रेणी V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    bool m_noinline = false;  // Don't inline function/task
    bool m_public = false;  // Public function/task

खुला:
    V3ControlFTask() = शेष;
    व्योम update(स्थिर V3ControlFTask& f) {
        // Don't overwrite true with false
        अगर (f.m_noinline) m_noinline = true;
        अगर (f.m_public) m_public = true;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    व्योम updateFinalize() {}

    V3ControlVarResolver& params() { वापस m_params; }
    V3ControlVarResolver& ports() { वापस m_ports; }
    V3ControlVarResolver& vars() { वापस m_vars; }

    व्योम setNoInline(bool set) { m_noinline = set; }
    व्योम setPublic(bool set) { m_public = set; }

    व्योम apply(Ast_गाँठ_FTask* ftaskp) स्थिर {
        अगर (m_noinline)
            ftaskp->addStmtsp(नया AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        अगर (m_public)
            ftaskp->addStmtsp(नया AstPragma{ftaskp->fileline(), VPragmaType::PUBLIC_TASK});
    }
};

using V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

श्रेणी V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::FsmRegisterWrapper m_fsmRegisterWrapper;  // FSM wrapper descriptor
    std::unordered_set<std::string> m_coverageOffBlocks;  // List of block names for coverage_off
    std::set<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    bool m_hasFsmRegisterWrapper = false;  // Whether descriptor has been set
    bool m_inline = false;  // Whether to force the inline
    bool m_inline_मूल्य = false;  // The inline value (on/off)

खुला:
    V3ControlModule() = शेष;

    व्योम update(स्थिर V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        अगर (m.m_hasFsmRegisterWrapper) {
            m_fsmRegisterWrapper = m.m_fsmRegisterWrapper;
            m_hasFsmRegisterWrapper = true;
        }
        क्रम (स्थिर string& i : m.m_coverageOffBlocks) m_coverageOffBlocks.जोड़ो(i);
        अगर (!m_inline) {
            m_inline = m.m_inline;
            m_inline_मूल्य = m.m_inline_मूल्य;
        }
        क्रम (स्वतः it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.जोड़ो(*it);
        }
    }
    व्योम updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { वापस m_tasks; }
    V3ControlVarResolver& params() { वापस m_params; }
    V3ControlVarResolver& ports() { वापस m_ports; }
    V3ControlVarResolver& vars() { वापस m_vars; }

    व्योम addCoverageBlockOff(स्थिर string& name) { m_coverageOffBlocks.जोड़ो(name); }
    व्योम setInline(bool set) {
        m_inline = true;
        m_inline_मूल्य = set;
    }
    व्योम setFsmRegisterWrapper(File_पंक्ति* fl, स्थिर V3Control::FsmRegisterWrapper& desc) {
        अगर (m_hasFsmRegisterWrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_गाँठ::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsmRegisterWrapper = desc;
        m_hasFsmRegisterWrapper = true;
    }
    स्थिर V3Control::FsmRegisterWrapper* fsmRegisterWrapperp() स्थिर {
        वापस m_hasFsmRegisterWrapper ? &m_fsmRegisterWrapper : nullptr;
    }
    व्योम addModulePragma(VPragmaType pragma) { m_modPragmas.जोड़ो(pragma); }

    व्योम apply(Ast_गाँठ_Module* modp) {
        अगर (m_inline) {
            स्थिर VPragmaType type
                = m_inline_मूल्य ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_गाँठ* स्थिर nodep = नया AstPragma{modp->fileline(), type};
            modp->addStmtsp(nodep);
        }
        क्रम (स्थिर स्वतः& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            अगर (itr == VPragmaType::HIER_PARAMS) modp->hierParams(true);
            Ast_गाँठ* स्थिर nodep = नया AstPragma{modp->fileline(), itr};
            modp->addStmtsp(nodep);
        }
    }

    व्योम applyBlock(AstGenBlock* nodep) {
        स्थिर VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        अगर (!nodep->unnamed()) {
            क्रम (स्थिर string& i : m_coverageOffBlocks) {
                अगर (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addItemsp(नया AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    व्योम applyBlock(Ast_गाँठ_Block* nodep) {
        स्थिर VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_पंक्ति* स्थिर flp = nodep->fileline();
        अगर (!nodep->unnamed()) {
            क्रम (स्थिर string& i : m_coverageOffBlocks) {
                अगर (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addStmtsp(नया AstStmtPragma{flp, नया AstPragma{flp, pragma}});
                }
            }
        }
    }
};

using V3ControlModuleResolver = V3ControlWildcardResolver<V3ControlModule>;

//######################################################################
// Files have:
//  - Line ignores (lint/coverage/tracing on/off)
//  - Line attributes: Attributes attached to lines

// lint/coverage/tracing on/off
श्रेणी V3_Control_Ignores_पंक्ति final {
खुला:
    स्थिर पूर्णांक m_पंक्ति_Min;  // Minimum line number to make change at
    स्थिर पूर्णांक m_पंक्ति_Max;  // Maximum line number to make change at (inclusive)
    स्थिर V3ErrorCode m_code;  // Error code
    स्थिर bool m_on;  // True to enable message
    V3_Control_Ignores_पंक्ति(V3ErrorCode code, पूर्णांक linemin, पूर्णांक linemax, bool on)
        : m_पंक्ति_Min{linemin}
        , m_पंक्ति_Max{linemax}
        , m_code{code}
        , m_on{on} {}
    ~V3_Control_Ignores_पंक्ति() = शेष;
    bool every_पंक्ति() स्थिर { वापस m_पंक्ति_Min == 0 && m_पंक्ति_Max == 0; }
    bool inRange(पूर्णांक lineno) स्थिर {
        वापस (lineno >= m_पंक्ति_Min && lineno <= m_पंक्ति_Max) || every_पंक्ति();
    }
};
std::ब_प्रवाह& चालक<<(std::ब_प्रवाह& os, स्थिर V3_Control_Ignores_पंक्ति& rhs) {
    वापस os << rhs.m_पंक्ति_Min << "-" << rhs.m_पंक्ति_Max << ", " << rhs.m_code << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
अटल std::vector<V3_Control_Ignores_पंक्ति> control_Ign_पंक्तियाँ;

using IgnIndices = std::vector<uint32_t>;  // List of {s_ignLines indces}

श्रेणी VInterval_वृक्ष final {
    काष्ठा Entry final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        पूर्णांक m_center;  // Line number of central point
        std::unique_ptr<Entry> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<Entry> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        explicit Entry(पूर्णांक center)
            : m_center{center} {}
    };

    std::unique_ptr<Entry> m_rootp;  // Root of interval tree
    IgnIndices m_every_पंक्तियाँ;  // All-line disables

निजी:
    std::unique_ptr<Entry> build_वृक्ष(स्थिर IgnIndices& points) {
        अगर (points.empty()) वापस nullptr;

        पूर्णांक minval = std::numeric_limits<पूर्णांक>::max();
        पूर्णांक maxval = std::numeric_limits<पूर्णांक>::min();
        क्रम (स्थिर स्वतः& it : points) {
            स्थिर V3_Control_Ignores_पंक्ति& cign = control_Ign_पंक्तियाँ[it];
            अगर (cign.every_पंक्ति()) {
                m_every_पंक्तियाँ.emplace_back(it);
                जारी;
            }
            minval = std::min(minval, cign.m_पंक्ति_Min);
            maxval = std::max(maxval, cign.m_पंक्ति_Max);
        }

        स्थिर पूर्णांक center = minval + (maxval - minval) / 2;
        स्वतः entp = std::unique_ptr<Entry>(नया Entry{center});

        IgnIndices बायाँ_Points;
        IgnIndices दायाँ_Points;
        क्रम (स्थिर स्वतः& it : points) {
            स्थिर V3_Control_Ignores_पंक्ति& cign = control_Ign_पंक्तियाँ[it];
            अगर (cign.every_पंक्ति()) जारी;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            अगर (cign.m_पंक्ति_Max < center)
                बायाँ_Points.emplace_back(it);
            अन्यथा अगर (cign.m_पंक्ति_Min > center)
                दायाँ_Points.emplace_back(it);
            अन्यथा {
                entp->m_byMin.धकेलो_back(it);
                entp->m_byMax.धकेलो_back(it);
            }
        }

        std::sort(entp->m_byMin.begin(), entp->m_byMin.end(), [](uint32_t a, uint32_t b) {
            वापस control_Ign_पंक्तियाँ[a].m_पंक्ति_Min < control_Ign_पंक्तियाँ[b].m_पंक्ति_Min;
        });
        std::sort(entp->m_byMax.begin(), entp->m_byMax.end(), [](uint32_t a, uint32_t b) {
            वापस control_Ign_पंक्तियाँ[a].m_पंक्ति_Max > control_Ign_पंक्तियाँ[b].m_पंक्ति_Max;
        });

        entp->m_leftp = build_वृक्ष(बायाँ_Points);
        entp->m_rightp = build_वृक्ष(दायाँ_Points);
        वापस entp;
    }

    व्योम find_वृक्ष(पूर्णांक lineno, Entry* entp, IgnIndices& resultsr, पूर्णांक& अग्र_Changer) स्थिर {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        अगर (lineno < entp->m_center) {
            अग्र_Changer = std::min(अग्र_Changer, entp->m_center);
            क्रम (स्थिर स्वतः& it : entp->m_byMin) {
                स्थिर V3_Control_Ignores_पंक्ति& cign = control_Ign_पंक्तियाँ[it];
                अगर (cign.m_पंक्ति_Min <= lineno) {
                    resultsr.emplace_back(it);
                } अन्यथा {
                    अग्र_Changer = std::min(अग्र_Changer, cign.m_पंक्ति_Min);
                    अवरोध;
                }
            }
            अगर (entp->m_leftp) find_वृक्ष(lineno, entp->m_leftp.लो(), resultsr, अग्र_Changer);
        } अन्यथा अगर (lineno > entp->m_center) {
            क्रम (स्थिर स्वतः& it : entp->m_byMax) {
                स्थिर V3_Control_Ignores_पंक्ति& cign = control_Ign_पंक्तियाँ[it];
                अगर (cign.m_पंक्ति_Max >= lineno) {
                    अग्र_Changer = std::min(अग्र_Changer, cign.m_पंक्ति_Max + 1);
                    resultsr.emplace_back(it);
                } अन्यथा {
                    अवरोध;
                }
            }
            अगर (entp->m_rightp) find_वृक्ष(lineno, entp->m_rightp.लो(), resultsr, अग्र_Changer);
        } अन्यथा {
            क्रम (स्थिर स्वतः& it : entp->m_byMin) {
                स्थिर V3_Control_Ignores_पंक्ति& cign = control_Ign_पंक्तियाँ[it];
                resultsr.emplace_back(it);
                अगर (cign.m_पंक्ति_Max >= lineno)
                    अग्र_Changer = std::min(अग्र_Changer, cign.m_पंक्ति_Max + 1);
            }
            अगर (entp->m_leftp) find_वृक्ष(lineno, entp->m_leftp.लो(), resultsr, अग्र_Changer);
            अगर (entp->m_rightp) find_वृक्ष(lineno, entp->m_rightp.लो(), resultsr, अग्र_Changer);
        }
    }

खुला:
    // CONSTRUCTORS
    VInterval_वृक्ष() = शेष;
    ~VInterval_वृक्ष() = शेष;
    // METHODS
    व्योम सफा() { m_rootp = nullptr; }
    व्योम build(स्थिर IgnIndices& points) {
        सफा();
        m_rootp = build_वृक्ष(points);
    }
    व्योम find(पूर्णांक lineno, IgnIndices& resultsr, पूर्णांक& अग्र_Changer) स्थिर {
        resultsr = m_every_पंक्तियाँ;
        अग्र_Changer = std::numeric_limits<पूर्णांक>::max();

        अगर (m_rootp) find_वृक्ष(lineno, m_rootp.लो(), resultsr, अग्र_Changer);
        // Sort indices, so can process in parse order
        std::sort(resultsr.begin(), resultsr.end());
    }

    अटल व्योम selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices तथ्य;
        std::vector<std::pair<पूर्णांक, पूर्णांक>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        क्रम (स्थिर स्वतः& it : points) {
            control_Ign_पंक्तियाँ.emplace_back(
                V3_Control_Ignores_पंक्ति{V3ErrorCode::I_LINT, it.first, it.second, true});
            तथ्य.emplace_back(static_cast<uint32_t>(control_Ign_पंक्तियाँ.आकार() - 1));
        }
        VInterval_वृक्ष वृक्ष;
        वृक्ष.build(तथ्य);

        IgnIndices results;
        पूर्णांक अग्र_Change = 0;
        वृक्ष.find(0, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 0);
        UASSERT_SELFTEST(अग्र_Change, 10);
        वृक्ष.find(10, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(अग्र_Change, 11);
        वृक्ष.find(11, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(अग्र_Change, 15);  // Center, or would be 20
        वृक्ष.find(20, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(अग्र_Change, 21);
        वृक्ष.find(21, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(अग्र_Change, 25);  // Center, or would be 30
        वृक्ष.find(30, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(अग्र_Change, 31);
        वृक्ष.find(40, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(अग्र_Change, 41);
        वृक्ष.find(41, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 0);
        UASSERT_SELFTEST(अग्र_Change, std::numeric_limits<पूर्णांक>::max());
        //
        points = {{0, 0}};
        क्रम (स्थिर स्वतः& it : points) {
            control_Ign_पंक्तियाँ.emplace_back(
                V3_Control_Ignores_पंक्ति{V3ErrorCode::I_LINT, it.first, it.second, true});
            तथ्य.emplace_back(static_cast<uint32_t>(control_Ign_पंक्तियाँ.आकार() - 1));
        }
        वृक्ष.build(तथ्य);
        //
        वृक्ष.find(50, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
using V3_Control_पंक्ति_Attribute = std::bitset<VPragmaType::ENUM_आकार>;

श्रेणी WaiverSetting final {
खुला:
    V3ErrorCode m_code;  // Error code
    string m_contents;  // --contents regexp
    string m_match;  // --match regexp
    WaiverSetting(V3ErrorCode code, स्थिर string& contents, स्थिर string& match)
        : m_code{code}
        , m_contents{contents}
        , m_match{match} {}
    ~WaiverSetting() = शेष;
    WaiverSetting& चालक=(स्थिर WaiverSetting& rhs) = शेष;
};

// File entity
श्रेणी V3ControlFile final {
    using पंक्ति_Attr_Map = std::map<पूर्णांक, V3_Control_पंक्ति_Attribute>;  // Map line->bitset of attributes
    using Waivers = std::vector<WaiverSetting>;  // List of {code,wildcard string}

    पंक्ति_Attr_Map m_पंक्ति_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_वृक्ष m_interval_वृक्ष;  // Tree of indices to IgnLines

    काष्ठा {
        पूर्णांक filenameno = -1;  // Last filename
        पूर्णांक lineno = -1;  // Last linenumber
        पूर्णांक अग्र_Change = -1;  // Line number of next change
    } m_lastIgnore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    bool पंक्ति_Match(पूर्णांक lineno, VPragmaType type) {
        अगर (m_पंक्ति_Attrs.find(0) != m_पंक्ति_Attrs.end() && m_पंक्ति_Attrs[0][type]) वापस true;
        अगर (m_पंक्ति_Attrs.find(lineno) == m_पंक्ति_Attrs.end()) वापस false;
        वापस m_पंक्ति_Attrs[lineno][type];
    }

खुला:
    V3ControlFile() = शेष;
    व्योम update(स्थिर V3ControlFile& file) {
        // Copy in all attributes and waivers
        क्रम (स्थिर स्वतः& itr : file.m_पंक्ति_Attrs) m_पंक्ति_Attrs[itr.first] |= itr.second;
        m_waivers.reserve(m_waivers.आकार() + file.m_waivers.आकार());
        m_waivers.जोड़ो(m_waivers.end(), file.m_waivers.begin(), file.m_waivers.end());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.आकार() + file.m_ignIndices.आकार());
        m_ignIndices.जोड़ो(m_ignIndices.end(), file.m_ignIndices.begin(),
                            file.m_ignIndices.end());
        // updateFinalize() will soon build tree
    }
    व्योम updateFinalize() { m_interval_वृक्ष.build(m_ignIndices); }
    व्योम add_पंक्ति_Attribute(पूर्णांक lineno, VPragmaType attr) { m_पंक्ति_Attrs[lineno].set(attr); }
    व्योम addIgnore(V3ErrorCode code, पूर्णांक min, पूर्णांक max, bool on) {
        control_Ign_पंक्तियाँ.emplace_back(V3_Control_Ignores_पंक्ति{code, min, max, on});
        m_ignIndices.emplace_back(static_cast<uint32_t>(control_Ign_पंक्तियाँ.आकार() - 1));
        m_lastIgnore.अग्र_Change = -1;
    }
    व्योम addIgnoreMatch(V3ErrorCode code, स्थिर string& contents, स्थिर string& match) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string newMatch = match;
        अगर (newMatch.empty() || newMatch.back() != '*') newMatch += '*';
        m_waivers.emplace_back(code, contents, newMatch);
    }

    व्योम applyBlock(AstGenBlock* nodep) {
        // Apply to block at this line
        स्थिर VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        अगर (पंक्ति_Match(nodep->fileline()->lineno(), pragma)) {
            nodep->addItemsp(नया AstPragma{nodep->fileline(), pragma});
        }
    }
    व्योम applyBlock(Ast_गाँठ_Block* nodep) {
        // Apply to block at this line
        स्थिर VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_पंक्ति* स्थिर flp = nodep->fileline();
        अगर (पंक्ति_Match(flp->lineno(), pragma)) {
            nodep->addStmtsp(नया AstStmtPragma{flp, नया AstPragma{flp, pragma}});
        }
    }
    व्योम applyCase(AstCase* nodep) {
        // Apply to this case at this line
        स्थिर पूर्णांक lineno = nodep->fileline()->lineno();
        अगर (पंक्ति_Match(lineno, VPragmaType::FULL_CASE)) nodep->fullPragma(true);
        अगर (पंक्ति_Match(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(true);
    }
    व्योम applyIgnores(File_पंक्ति* filelinep) {
        // HOT routine, called each parsed token line of this filename
        अगर (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() == m_lastIgnore.lineno)
            वापस;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        अगर (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() < m_lastIgnore.अग्र_Change) {
            m_lastIgnore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            वापस;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_lastIgnore.filenameno = filelinep->filenameno();
        m_lastIgnore.lineno = filelinep->lineno();
        m_lastIgnore.अग्र_Change = std::numeric_limits<पूर्णांक>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_वृक्ष.find(filelinep->lineno(), results /*ref*/, m_lastIgnore.अग्र_Change /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VErrorBitSet bitset{VErrorBitSet::AllOnes{}};
        क्रम (स्थिर स्वतः& it : results) {
            स्थिर V3_Control_Ignores_पंक्ति& cign = control_Ign_पंक्तियाँ[it];
            UASSERT(cign.inRange(filelinep->lineno()),
                    "Interval tree returned lines outside range");
            UINFO(9, "     Hit " << cign);
            cign.m_code.forDelegateCodes(
                [&](V3ErrorCode subcode) { bitset.set(subcode, cign.m_on); });
        }

        filelinep->warnSetCtrlBitSet(bitset);
        m_lastIgnore.अग्र_Change = std::max(m_lastIgnore.अग्र_Change, filelinep->lineno() + 1);
        UINFO(9, "    AppliedIgnores " << filelinep << " next@ln " << std::डेसी
                                       << m_lastIgnore.अग्र_Change
                                       << " Messages: " << bitset.ascii());
    }
    bool waive(V3ErrorCode code, स्थिर string& message) {
        अगर (code.hardError()) वापस false;
        क्रम (स्थिर स्वतः& itr : m_waivers) {
            अगर ((code.isUnder(itr.m_code) || (itr.m_code == V3ErrorCode::I_LINT))
                && VString::wildmatch(message, itr.m_match)
                && WildcardContents::resolve(itr.m_contents)) {
                वापस true;
            }
        }
        वापस false;
    }
};

using V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

श्रेणी V3ControlScopeTraceEntry final {
खुला:
    स्थिर string m_scope;  // Scope or regexp to match
    स्थिर bool m_on;  // True to enable message
    पूर्णांक m_levels = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3ControlScopeTraceEntry(स्थिर string& scope, bool on, पूर्णांक levels)
        : m_scope{scope}
        , m_on{on}
        , m_levels{levels} {}
    ~V3ControlScopeTraceEntry() = शेष;
    bool चालक<(स्थिर V3ControlScopeTraceEntry& other) स्थिर {
        अगर (m_on < other.m_on) वापस true;
        अगर (m_on > other.m_on) वापस false;
        अगर (m_levels < other.m_levels) वापस true;
        अगर (m_levels > other.m_levels) वापस false;
        वापस m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
श्रेणी V3ControlScopeTraceEntryMatch final {
खुला:
    स्थिर V3ControlScopeTraceEntry* m_entryp;  // Entry the match refers to
    स्थिर string m_scopepart;  // Scope prefix being matched against the entry
    V3ControlScopeTraceEntryMatch(स्थिर V3ControlScopeTraceEntry* entryp, स्थिर string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    bool चालक<(स्थिर V3ControlScopeTraceEntryMatch& other) स्थिर {
        अगर (m_entryp < other.m_entryp) वापस true;
        अगर (m_entryp > other.m_entryp) वापस false;
        वापस m_scopepart < other.m_scopepart;
    }
};

श्रेणी V3ControlScopeTraceResolver final {
    std::vector<V3ControlScopeTraceEntry> m_entries;  // User specified on/offs and levels
    std::map<V3ControlScopeTraceEntryMatch, bool> m_matchCache;  // Matching entries for speed

खुला:
    व्योम addScopeTraceOn(bool on, स्थिर string& scope, पूर्णांक levels) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << levels);
        m_entries.emplace_back(V3ControlScopeTraceEntry{scope, on, levels});
        m_matchCache.सफा();
    }

    bool getEntryMatch(स्थिर V3ControlScopeTraceEntry* entp, स्थिर string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        स्थिर V3ControlScopeTraceEntryMatch कुंजी{entp, scopepart};
        स्थिर स्वतः pair = m_matchCache.emplace(कुंजी, false);
        अगर (pair.second) pair.first->second = VString::wildmatch(scopepart, entp->m_scope);
        वापस pair.first->second;
    }

    bool getScopeTraceOn(स्थिर string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        पूर्णांक maxLevel = 1;
        क्रम (स्थिर स्वतः& ch : scope) {
            अगर (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        bool enabled = true;
        क्रम (स्थिर स्वतः& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            पूर्णांक partLevel = 1;
            क्रम (string::आकार_type partEnd = 0; true;) {
                partEnd = scope.find('.', partEnd + 1);
                अगर (partEnd == string::npos) partEnd = scope.लंबाई();
                स्थिर std::string scopepart = scope.substr(0, partEnd);
                अगर (getEntryMatch(&ent, scopepart)) {
                    स्थिर bool levelMatch
                        = !ent.m_levels || (ent.m_levels >= maxLevel - partLevel);
                    अगर (levelMatch) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (levelMatch ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    अवरोध;
                }
                अगर (partEnd == scope.लंबाई()) अवरोध;
                ++partLevel;
            }
        }
        वापस enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

श्रेणी V3ControlResolverHierWorkerEntry final {
    स्थिर पूर्णांक m_workers;  // Number of workers requested for the hierarchical block
    File_पंक्ति* स्थिर m_flp;  // Location the worker count was specified at

खुला:
    explicit V3ControlResolverHierWorkerEntry(पूर्णांक workers, File_पंक्ति* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    पूर्णांक workers() स्थिर { वापस m_workers; }
    File_पंक्ति* flp() स्थिर { वापस m_flp; }
};

श्रेणी V3ControlResolver final {
    क्रमागत Profile_तथ्य_Mode : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profile_तथ्य;  // Access to profile_data records
    uint8_t m_mode = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3ControlResolverHierWorkerEntry> m_hierWorkers;
    File_पंक्ति* m_profile_File_पंक्ति = nullptr;  // Location profile_data was read from

    V3ControlResolver() = शेष;
    ~V3ControlResolver() = शेष;

खुला:
    अटल V3ControlResolver& s() {
        अटल V3ControlResolver s_singleton;
        वापस s_singleton;
    }
    V3ControlModuleResolver& modules() { वापस m_modules; }
    V3ControlFileResolver& files() { वापस m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { वापस m_scopeTraces; }

    व्योम add_Profile_तथ्य(File_पंक्ति* fl, स्थिर string& hierDpi, uint64_t cost) {
        // Empty key for hierarchical DPI wrapper costs.
        add_Profile_तथ्य(fl, hierDpi, "", cost, HIER_DPI);
    }
    व्योम add_Profile_तथ्य(File_पंक्ति* fl, स्थिर string& model, स्थिर string& कुंजी, uint64_t cost,
                        Profile_तथ्य_Mode mode = MTASK) {
        अगर (!m_profile_File_पंक्ति) m_profile_File_पंक्ति = fl;
        अगर (cost == 0) cost = 1;  // Cost 0 means delete (or no data)
        अगर (mode == MTASK) {
            m_profile_तथ्य[model][कुंजी] += cost;
        } अन्यथा अगर (mode == HIER_DPI) {
            m_profile_तथ्य[model][कुंजी] = std::max(m_profile_तथ्य[model][कुंजी], cost);
        }
        m_mode |= mode;
    }
    bool contains_MTask_Profile_तथ्य() स्थिर { वापस m_mode & MTASK; }
    uint64_t get_Profile_तथ्य(स्थिर string& hierDpi) स्थिर {
        // Empty key for hierarchical DPI wrapper costs.
        वापस get_Profile_तथ्य(hierDpi, "");
    }
    व्योम addHierWorkers(File_पंक्ति* flp, स्थिर string& model, पूर्णांक workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    पूर्णांक getHierWorkers(स्थिर string& model) स्थिर {
        स्थिर स्वतः mit = m_hierWorkers.find(model);
        // Assign a single worker if no specified.
        वापस mit != m_hierWorkers.cend() ? mit->second.workers() : 0;
    }
    File_पंक्ति* get_Hier_Workers_File_पंक्ति(स्थिर string& model) स्थिर {
        स्थिर स्वतः mit = m_hierWorkers.find(model);
        वापस mit != m_hierWorkers.cend() ? mit->second.flp() : v3Global.rootp()->fileline();
    }
    uint64_t get_Profile_तथ्य(स्थिर string& model, स्थिर string& कुंजी) स्थिर {
        स्थिर स्वतः mit = m_profile_तथ्य.find(model);
        अगर (mit == m_profile_तथ्य.cend()) वापस 0;
        स्थिर स्वतः it = mit->second.find(कुंजी);
        अगर (it == mit->second.cend()) वापस 0;
        वापस it->second;
    }
    File_पंक्ति* get_Profile_तथ्य_File_पंक्ति() स्थिर { वापस m_profile_File_पंक्ति; }  // Maybe null
    अटल uint64_t getCurrentHierBlockCost() {
        अगर (uint64_t cost = V3Control::get_Profile_तथ्य(v3Global.opt.prefix())) {
            UINFO(9, "Fetching cost from profile info: " << cost);
            वापस cost;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* स्थिर netlistp = v3Global.rootp();
        uint64_t cost = 0;
        क्रम (पूर्णांक i = 0; i < VEval::_ENUM_END; ++i) {
            स्थिर VEval eval{i};
            अगर (eval.slow()) जारी;
            cost += V3_Instr_गिनती::गिनती(netlistp->evalFuncp(eval), false);
        }
        UINFO(9, "Evaluating cost: " << cost);
        वापस cost;
    }
};

//######################################################################
// V3Control

व्योम V3Control::addCaseFull(स्थिर string& filename, पूर्णांक lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_पंक्ति_Attribute(lineno, VPragmaType::FULL_CASE);
}
व्योम V3Control::addCaseParallel(स्थिर string& filename, पूर्णांक lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_पंक्ति_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
व्योम V3Control::addCoverageBlockOff(स्थिर string& filename, पूर्णांक lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_पंक्ति_Attribute(lineno, VPragmaType::COVERAGE_BLOCK_OFF);
}
व्योम V3Control::addCoverageBlockOff(स्थिर string& module, स्थिर string& blockname) {
    V3ControlResolver::s().modules().at(module).addCoverageBlockOff(blockname);
}

व्योम V3Control::addHierWorkers(File_पंक्ति* fl, स्थिर string& model, पूर्णांक workers) {
    V3ControlResolver::s().addHierWorkers(fl, model, workers);
}

व्योम V3Control::addFsmRegisterWrapper(File_पंक्ति* fl, स्थिर string& module, स्थिर string& d,
                                      स्थिर string& q, स्थिर string& घड़ी, स्थिर string& reset,
                                      स्थिर string& reset_मूल्य) {
    string missing;
    अगर (module.empty()) missing += "-module";
    अगर (d.empty()) missing = VString::dot(missing, ", ", "-d");
    अगर (q.empty()) missing = VString::dot(missing, ", ", "-q");
    अगर (घड़ी.empty()) missing = VString::dot(missing, ", ", "-clock");
    अगर (!missing.empty()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        वापस;
    }

    स्थिर FsmRegisterWrapper desc{module, d, q, घड़ी, reset, reset_मूल्य};
    V3ControlResolver::s().modules().at(module).setFsmRegisterWrapper(fl, desc);
}

व्योम V3Control::addIgnore(V3ErrorCode code, bool on, स्थिर string& filename, पूर्णांक min, पूर्णांक max) {
    UINFO(9, "addIgnore " << code << " " << min << "-" << max << " fn=" << filename);
    अगर (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_पंक्ति::globalWarnOff(code, !on);
    } अन्यथा {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).addIgnore(code, min, max, on);
    }
}
व्योम V3Control::addIgnoreMatch(V3ErrorCode code, स्थिर string& filename, स्थिर string& contents,
                               स्थिर string& match) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).addIgnoreMatch(code, contents, match);
}

व्योम V3Control::addInline(File_पंक्ति* fl, स्थिर string& module, स्थिर string& ftask, bool on) {
    अगर (ftask.empty()) {
        V3ControlResolver::s().modules().at(module).setInline(on);
    } अन्यथा {
        अगर (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } अन्यथा {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setNoInline(on);
        }
    }
}

व्योम V3Control::addModulePragma(स्थिर string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).addModulePragma(pragma);
}

व्योम V3Control::add_Profile_तथ्य(File_पंक्ति* fl, स्थिर string& hierDpi, uint64_t cost) {
    V3ControlResolver::s().add_Profile_तथ्य(fl, hierDpi, cost);
}
व्योम V3Control::add_Profile_तथ्य(File_पंक्ति* fl, स्थिर string& model, स्थिर string& कुंजी,
                               uint64_t cost) {
    V3ControlResolver::s().add_Profile_तथ्य(fl, model, कुंजी, cost);
}

व्योम V3Control::addScopeTraceOn(bool on, स्थिर string& scope, पूर्णांक levels) {
    V3ControlResolver::s().scopeTraces().addScopeTraceOn(on, scope, levels);
}

व्योम V3Control::addVarAttr(File_पंक्ति* fl, स्थिर string& module, स्थिर string& ftask,
                           VarSpecKind kind, स्थिर string& pattern, VAttrType attr,
                           Ast_Sen_वृक्ष* sensep) {
    अगर (sensep) {
        File_पंक्ति* स्थिर flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_वृक्ष(), sensep);
        // Used to be only accepted on public_flat_rw
        अगर (attr != VAttrType::VAR_PUBLIC_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            वापस;
        }
    }

    अगर (kind != VarSpecKind::VAR) {
        चयन (attr) {
        हाल VAttrType::VAR_PUBLIC_FLAT:
        हाल VAttrType::VAR_PUBLIC_FLAT_RD:
        हाल VAttrType::VAR_PUBLIC_FLAT_RW: अवरोध;
        शेष:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            वापस;
        }
    }

    // Semantics: Most of the attributes operate on signals
    अगर (pattern.empty()) {
        अगर (attr == VAttrType::VAR_PUBLIC) {
            अगर (ftask.empty()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).addModulePragma(
                    VPragmaType::PUBLIC_MODULE);
            } अन्यथा {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setPublic(true);
            }
        } अन्यथा {
            fl->v3error("missing -var");
        }
    } अन्यथा {
        अगर (attr == VAttrType::VAR_FORCEABLE) {
            अगर (module.empty()) {
                fl->v3error("forceable missing -module");
            } अन्यथा अगर (!ftask.empty()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } अन्यथा {
                V3ControlResolver::s().modules().at(module).vars().at(pattern).add(attr);
            }
        } अन्यथा {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                अगर (ftask.empty()) {
                    अगर (kind == VarSpecKind::PARAM) वापस mod.params().at(pattern);
                    अगर (kind == VarSpecKind::PORT) वापस mod.ports().at(pattern);
                    UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                    वापस mod.vars().at(pattern);
                }
                अगर (kind == VarSpecKind::PARAM) वापस mod.ftasks().at(ftask).params().at(pattern);
                अगर (kind == VarSpecKind::PORT) वापस mod.ftasks().at(ftask).ports().at(pattern);
                UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                वापस mod.ftasks().at(ftask).vars().at(pattern);
            }();
            controlVar.add(attr);
        }
    }
}

व्योम V3Control::applyCase(AstCase* nodep) {
    स्थिर string& filename = nodep->fileline()->filename();
    V3ControlFile* स्थिर filep = V3ControlResolver::s().files().resolve(filename);
    अगर (filep) filep->applyCase(nodep);
}

व्योम V3Control::applyCoverageBlock(Ast_गाँठ_Module* modulep, AstBegin* nodep) {
    स्थिर string& filename = nodep->fileline()->filename();
    V3ControlFile* स्थिर filep = V3ControlResolver::s().files().resolve(filename);
    अगर (filep) filep->applyBlock(nodep);
    स्थिर string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* स्थिर modp = V3ControlResolver::s().modules().resolve(modname);
    अगर (modp) modp->applyBlock(nodep);
}
व्योम V3Control::applyCoverageBlock(Ast_गाँठ_Module* modulep, AstGenBlock* nodep) {
    स्थिर string& filename = nodep->fileline()->filename();
    V3ControlFile* स्थिर filep = V3ControlResolver::s().files().resolve(filename);
    अगर (filep) filep->applyBlock(nodep);
    स्थिर string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* स्थिर modp = V3ControlResolver::s().modules().resolve(modname);
    अगर (modp) modp->applyBlock(nodep);
}

व्योम V3Control::applyIgnores(File_पंक्ति* filelinep) {
    स्थिर string& filename = filelinep->filename();
    V3ControlFile* स्थिर filep = V3ControlResolver::s().files().resolve(filename);
    अगर (filep) filep->applyIgnores(filelinep);
}

व्योम V3Control::applyModule(Ast_गाँठ_Module* modulep) {
    स्थिर string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* स्थिर modp = V3ControlResolver::s().modules().resolve(modname);
    अगर (modp) modp->apply(modulep);
}

व्योम V3Control::applyFTask(Ast_गाँठ_Module* modulep, Ast_गाँठ_FTask* ftaskp) {
    स्थिर string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* स्थिर modp = V3ControlResolver::s().modules().resolve(modname);
    अगर (!modp) वापस;
    स्थिर V3ControlFTask* स्थिर ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    अगर (ftp) ftp->apply(ftaskp);
}

ढाँचा <typename T_Resolver>
अटल व्योम resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    स्थिर std::string name = varp->prettyDehashOrigOrName();
    अगर (स्थिर V3ControlVar* स्थिर vp = resolver.vars().resolve(name)) vp->apply(varp);
    अगर (varp->isParam()) {
        अगर (स्थिर V3ControlVar* स्थिर vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    अगर (varp->isIO()) {
        अगर (स्थिर V3ControlVar* स्थिर vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

व्योम V3Control::applyVarAttr(स्थिर Ast_गाँठ_Module* modulep, स्थिर Ast_गाँठ_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* स्थिर modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    अगर (!modp) वापस;
    अगर (ftaskp) {
        V3ControlFTask* स्थिर ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        अगर (!ftp) वापस;
        resolveThenApply(*ftp, varp);
    } अन्यथा {
        resolveThenApply(*modp, varp);
    }
}

पूर्णांक V3Control::getHierWorkers(स्थिर string& model) {
    वापस V3ControlResolver::s().getHierWorkers(model);
}
File_पंक्ति* V3Control::get_Hier_Workers_File_पंक्ति(स्थिर string& model) {
    वापस V3ControlResolver::s().get_Hier_Workers_File_पंक्ति(model);
}
स्थिर V3Control::FsmRegisterWrapper* V3Control::getFsmRegisterWrapper(स्थिर string& module) {
    V3ControlModule* स्थिर modp = V3ControlResolver::s().modules().resolve(module);
    वापस modp ? modp->fsmRegisterWrapperp() : nullptr;
}
uint64_t V3Control::get_Profile_तथ्य(स्थिर string& hierDpi) {
    वापस V3ControlResolver::s().get_Profile_तथ्य(hierDpi);
}
uint64_t V3Control::get_Profile_तथ्य(स्थिर string& model, स्थिर string& कुंजी) {
    वापस V3ControlResolver::s().get_Profile_तथ्य(model, कुंजी);
}
File_पंक्ति* V3Control::get_Profile_तथ्य_File_पंक्ति() {
    वापस V3ControlResolver::s().get_Profile_तथ्य_File_पंक्ति();
}
bool V3Control::getScopeTraceOn(स्थिर string& scope) {
    वापस V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

व्योम V3Control::contents_धकेलो_Text(स्थिर string& text) { WildcardContents::धकेलो_Text(text); }

bool V3Control::contains_MTask_Profile_तथ्य() {
    वापस V3ControlResolver::s().contains_MTask_Profile_तथ्य();
}
uint64_t V3Control::getCurrentHierBlockCost() {
    वापस V3ControlResolver::s().getCurrentHierBlockCost();
}

bool V3Control::waive(स्थिर File_पंक्ति* filelinep, V3ErrorCode code, स्थिर string& message) {
    V3ControlFile* स्थिर filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    अगर (!filep) वापस false;
    वापस filep->waive(code, message);
}

व्योम V3Control::selfTest() { VInterval_वृक्ष::selfTest(); }
