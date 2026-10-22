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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3Control.h"

#अन्तर्भाव "V3InstrCount.h"
#अन्तर्भाव "V3Os.h"
#अन्तर्भाव "V3String.h"

#अन्तर्भाव <memory>
#अन्तर्भाव <set>
#अन्तर्भाव <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
ढाँचा <typename T>
वर्ग V3ControlWildcardResolver final {
    mutable V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<स्थिरांक std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<स्थिरांक std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

सार्वजनिक:
    V3ControlWildcardResolver() = पूर्वनिर्धारित;
    ~V3ControlWildcardResolver() = पूर्वनिर्धारित;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    शून्य update(स्थिरांक V3ControlWildcardResolver& other) VL_MT_SAFE_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        स्थिरांक V3LockGuard lock{m_mutex};
        स्थिरांक V3LockGuard otherLock{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        साठी (स्थिरांक स्वतः& itr : other.m_mapPatterns) m_mapPatterns[itr.first].update(itr.second);
    }

    // Access and create a pattern entry
    T& at(स्थिरांक string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        स्थिरांक V3LockGuard lock{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        परत m_mapPatterns[V3Os::filenameSlashPath(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(स्थिरांक string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        स्थिरांक V3LockGuard lock{m_mutex};
        // Lookup if it was resolved before, typically not
        स्थिरांक स्वतः pair = m_mapResolved.emplace(name, nullptr);
        std::unique_ptr<T>& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        जर (pair.second) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            स्थिरांक string slashName = V3Os::filenameSlashPath(name);
            साठी (स्थिरांक स्वतः& patEnt : m_mapPatterns) {
                जर (VString::wildmatch(slashName, patEnt.first)) {
                    जर (!entryr) entryr.reset(नवीन T{});
                    entryr->update(patEnt.second);
                }
            }
            // Perform final actions that needed all updates completed
            जर (entryr) entryr->updateFinalize();
        }
        परत entryr.get();
    }
};

// List of attributes for variables
वर्ग V3ControlVar final {
    std::vector<VAttrType> m_attrs;  // The list of attributes
सार्वजनिक:
    // Add new attribugte
    शून्य add(VAttrType attr) { m_attrs.emplace_back(attr); }
    // Update from other by copying all attributes
    शून्य update(स्थिरांक V3ControlVar& other) {
        m_attrs.reserve(m_attrs.size() + other.m_attrs.size());
        m_attrs.घाला(m_attrs.end(), other.m_attrs.begin(), other.m_attrs.end());
    }
    शून्य updateFinalize() {}
    // Apply all attributes to the variable
    शून्य apply(AstVar* varp) स्थिरांक {
        साठी (स्थिरांक VAttrType attr : m_attrs) {
            varp->addAttrsp(नवीन AstAttrOf{varp->fileline(), attr});
        }
    }
};

using V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

वर्ग WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<स्थिरांक std::string, तार्किक> m_mapPatterns;  // Pattern match results
    std::deque<string> m_ओळी;  // Source text lines

    // METHODS
    स्थिर WildcardContents& s() {  // Singleton
        स्थिर WildcardContents s_s;
        परत s_s;
    }
    शून्य clearCacheImp() { m_mapPatterns.clear(); }
    शून्य ढकला_Text_Imp(स्थिरांक string& text) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        स्थिरांक string leftover = m_ओळी.back() + text;
        m_ओळी.काढा_back();

        // Insert line-by-line
        string::size_type ओळ_start = 0;
        पर्यंत (सत्य) {
            स्थिरांक string::size_type ओळ_end = leftover.find('\n', ओळ_start);
            जर (ओळ_end != string::npos) {
                स्थिरांक string oneline(leftover, ओळ_start, ओळ_end - ओळ_start + 1);
                जर (oneline.size() > 1) m_ओळी.ढकला_back(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_ओळी.size() - 1) << "]: " << oneline);
                ओळ_start = ओळ_end + 1;
            } अन्यथा {
                थांबा;
            }
        }
        // Keep leftover for next time
        m_ओळी.emplace_back(string(leftover, ओळ_start));  // Might be ""
        clearCacheImp();
    }

    तार्किक resolveUncachedImp(स्थिरांक string& contentsRegexp) {
        साठी (स्थिरांक string& i : m_ओळी) {
            जर (VString::wildmatch(i, contentsRegexp)) परत सत्य;
        }
        परत असत्य;
    }
    तार्किक resolveCachedImp(स्थिरांक string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        जर (contentsRegexp.empty()) परत सत्य;
        स्थिरांक स्वतः pair = m_mapPatterns.emplace(contentsRegexp, असत्य);
        तार्किक& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        जर (pair.second) entryr = resolveUncachedImp(contentsRegexp);
        परत entryr;
    }

सार्वजनिक:
    WildcardContents() {
        m_ओळी.emplace_back("");  // start with no leftover
    }
    ~WildcardContents() = पूर्वनिर्धारित;
    // Return true iff contentsRegexp in parsed contents
    स्थिर तार्किक resolve(स्थिरांक string& contentsRegexp) {
        परत s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    स्थिर शून्य ढकला_Text(स्थिरांक string& text) { s().ढकला_Text_Imp(text); }
};

//######################################################################
// Function or task: Have variables and properties

वर्ग V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    तार्किक m_noinline = असत्य;  // Don't inline function/task
    तार्किक m_public = असत्य;  // Public function/task

सार्वजनिक:
    V3ControlFTask() = पूर्वनिर्धारित;
    शून्य update(स्थिरांक V3ControlFTask& f) {
        // Don't overwrite true with false
        जर (f.m_noinline) m_noinline = सत्य;
        जर (f.m_public) m_public = सत्य;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    शून्य updateFinalize() {}

    V3ControlVarResolver& params() { परत m_params; }
    V3ControlVarResolver& ports() { परत m_ports; }
    V3ControlVarResolver& vars() { परत m_vars; }

    शून्य setNoInline(तार्किक set) { m_noinline = set; }
    शून्य setPublic(तार्किक set) { m_public = set; }

    शून्य apply(Ast_गाठ_FTask* ftaskp) स्थिरांक {
        जर (m_noinline)
            ftaskp->addStmtsp(नवीन AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        जर (m_public)
            ftaskp->addStmtsp(नवीन AstPragma{ftaskp->fileline(), VPragmaType::PUBLIC_TASK});
    }
};

using V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

वर्ग V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::FsmRegisterWrapper m_fsmRegisterWrapper;  // FSM wrapper descriptor
    std::unordered_set<std::string> m_coverageOffBlocks;  // List of block names for coverage_off
    std::set<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    तार्किक m_hasFsmRegisterWrapper = असत्य;  // Whether descriptor has been set
    तार्किक m_inline = असत्य;  // Whether to force the inline
    तार्किक m_inline_मूल्य = असत्य;  // The inline value (on/off)

सार्वजनिक:
    V3ControlModule() = पूर्वनिर्धारित;

    शून्य update(स्थिरांक V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        जर (m.m_hasFsmRegisterWrapper) {
            m_fsmRegisterWrapper = m.m_fsmRegisterWrapper;
            m_hasFsmRegisterWrapper = सत्य;
        }
        साठी (स्थिरांक string& i : m.m_coverageOffBlocks) m_coverageOffBlocks.घाला(i);
        जर (!m_inline) {
            m_inline = m.m_inline;
            m_inline_मूल्य = m.m_inline_मूल्य;
        }
        साठी (स्वतः it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.घाला(*it);
        }
    }
    शून्य updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { परत m_tasks; }
    V3ControlVarResolver& params() { परत m_params; }
    V3ControlVarResolver& ports() { परत m_ports; }
    V3ControlVarResolver& vars() { परत m_vars; }

    शून्य addCoverageBlockOff(स्थिरांक string& name) { m_coverageOffBlocks.घाला(name); }
    शून्य setInline(तार्किक set) {
        m_inline = सत्य;
        m_inline_मूल्य = set;
    }
    शून्य setFsmRegisterWrapper(File_ओळ* fl, स्थिरांक V3Control::FsmRegisterWrapper& desc) {
        जर (m_hasFsmRegisterWrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_गाठ::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsmRegisterWrapper = desc;
        m_hasFsmRegisterWrapper = सत्य;
    }
    स्थिरांक V3Control::FsmRegisterWrapper* fsmRegisterWrapperp() स्थिरांक {
        परत m_hasFsmRegisterWrapper ? &m_fsmRegisterWrapper : nullptr;
    }
    शून्य addModulePragma(VPragmaType pragma) { m_modPragmas.घाला(pragma); }

    शून्य apply(Ast_गाठ_Module* modp) {
        जर (m_inline) {
            स्थिरांक VPragmaType type
                = m_inline_मूल्य ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_गाठ* स्थिरांक nodep = नवीन AstPragma{modp->fileline(), type};
            modp->addStmtsp(nodep);
        }
        साठी (स्थिरांक स्वतः& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            जर (itr == VPragmaType::HIER_PARAMS) modp->hierParams(सत्य);
            Ast_गाठ* स्थिरांक nodep = नवीन AstPragma{modp->fileline(), itr};
            modp->addStmtsp(nodep);
        }
    }

    शून्य applyBlock(AstGenBlock* nodep) {
        स्थिरांक VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        जर (!nodep->unnamed()) {
            साठी (स्थिरांक string& i : m_coverageOffBlocks) {
                जर (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addItemsp(नवीन AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    शून्य applyBlock(Ast_गाठ_Block* nodep) {
        स्थिरांक VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_ओळ* स्थिरांक flp = nodep->fileline();
        जर (!nodep->unnamed()) {
            साठी (स्थिरांक string& i : m_coverageOffBlocks) {
                जर (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addStmtsp(नवीन AstStmtPragma{flp, नवीन AstPragma{flp, pragma}});
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
वर्ग V3_Control_Ignores_ओळ final {
सार्वजनिक:
    स्थिरांक पूर्णांक m_ओळ_Min;  // Minimum line number to make change at
    स्थिरांक पूर्णांक m_ओळ_Max;  // Maximum line number to make change at (inclusive)
    स्थिरांक V3ErrorCode m_code;  // Error code
    स्थिरांक तार्किक m_on;  // True to enable message
    V3_Control_Ignores_ओळ(V3ErrorCode code, पूर्णांक linemin, पूर्णांक linemax, तार्किक on)
        : m_ओळ_Min{linemin}
        , m_ओळ_Max{linemax}
        , m_code{code}
        , m_on{on} {}
    ~V3_Control_Ignores_ओळ() = पूर्वनिर्धारित;
    तार्किक every_ओळ() स्थिरांक { परत m_ओळ_Min == 0 && m_ओळ_Max == 0; }
    तार्किक inRange(पूर्णांक lineno) स्थिरांक {
        परत (lineno >= m_ओळ_Min && lineno <= m_ओळ_Max) || every_ओळ();
    }
};
std::ostream& चालक<<(std::ostream& os, स्थिरांक V3_Control_Ignores_ओळ& rhs) {
    परत os << rhs.m_ओळ_Min << "-" << rhs.m_ओळ_Max << ", " << rhs.m_code << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
स्थिर std::vector<V3_Control_Ignores_ओळ> control_Ign_ओळी;

using IgnIndices = std::vector<uint32_t>;  // List of {s_ignLines indces}

वर्ग VInterval_वृक्ष final {
    संरचना Entry final {
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
    IgnIndices m_every_ओळी;  // All-line disables

खाजगी:
    std::unique_ptr<Entry> build_वृक्ष(स्थिरांक IgnIndices& points) {
        जर (points.empty()) परत nullptr;

        पूर्णांक minval = std::numeric_limits<पूर्णांक>::max();
        पूर्णांक maxval = std::numeric_limits<पूर्णांक>::min();
        साठी (स्थिरांक स्वतः& it : points) {
            स्थिरांक V3_Control_Ignores_ओळ& cign = control_Ign_ओळी[it];
            जर (cign.every_ओळ()) {
                m_every_ओळी.emplace_back(it);
                सुरूठेवा;
            }
            minval = std::min(minval, cign.m_ओळ_Min);
            maxval = std::max(maxval, cign.m_ओळ_Max);
        }

        स्थिरांक पूर्णांक center = minval + (maxval - minval) / 2;
        स्वतः entp = std::unique_ptr<Entry>(नवीन Entry{center});

        IgnIndices डावा_Points;
        IgnIndices उजवा_Points;
        साठी (स्थिरांक स्वतः& it : points) {
            स्थिरांक V3_Control_Ignores_ओळ& cign = control_Ign_ओळी[it];
            जर (cign.every_ओळ()) सुरूठेवा;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            जर (cign.m_ओळ_Max < center)
                डावा_Points.emplace_back(it);
            अन्यथा जर (cign.m_ओळ_Min > center)
                उजवा_Points.emplace_back(it);
            अन्यथा {
                entp->m_byMin.ढकला_back(it);
                entp->m_byMax.ढकला_back(it);
            }
        }

        std::sort(entp->m_byMin.begin(), entp->m_byMin.end(), [](uint32_t a, uint32_t b) {
            परत control_Ign_ओळी[a].m_ओळ_Min < control_Ign_ओळी[b].m_ओळ_Min;
        });
        std::sort(entp->m_byMax.begin(), entp->m_byMax.end(), [](uint32_t a, uint32_t b) {
            परत control_Ign_ओळी[a].m_ओळ_Max > control_Ign_ओळी[b].m_ओळ_Max;
        });

        entp->m_leftp = build_वृक्ष(डावा_Points);
        entp->m_rightp = build_वृक्ष(उजवा_Points);
        परत entp;
    }

    शून्य find_वृक्ष(पूर्णांक lineno, Entry* entp, IgnIndices& resultsr, पूर्णांक& nextChanger) स्थिरांक {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        जर (lineno < entp->m_center) {
            nextChanger = std::min(nextChanger, entp->m_center);
            साठी (स्थिरांक स्वतः& it : entp->m_byMin) {
                स्थिरांक V3_Control_Ignores_ओळ& cign = control_Ign_ओळी[it];
                जर (cign.m_ओळ_Min <= lineno) {
                    resultsr.emplace_back(it);
                } अन्यथा {
                    nextChanger = std::min(nextChanger, cign.m_ओळ_Min);
                    थांबा;
                }
            }
            जर (entp->m_leftp) find_वृक्ष(lineno, entp->m_leftp.get(), resultsr, nextChanger);
        } अन्यथा जर (lineno > entp->m_center) {
            साठी (स्थिरांक स्वतः& it : entp->m_byMax) {
                स्थिरांक V3_Control_Ignores_ओळ& cign = control_Ign_ओळी[it];
                जर (cign.m_ओळ_Max >= lineno) {
                    nextChanger = std::min(nextChanger, cign.m_ओळ_Max + 1);
                    resultsr.emplace_back(it);
                } अन्यथा {
                    थांबा;
                }
            }
            जर (entp->m_rightp) find_वृक्ष(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        } अन्यथा {
            साठी (स्थिरांक स्वतः& it : entp->m_byMin) {
                स्थिरांक V3_Control_Ignores_ओळ& cign = control_Ign_ओळी[it];
                resultsr.emplace_back(it);
                जर (cign.m_ओळ_Max >= lineno)
                    nextChanger = std::min(nextChanger, cign.m_ओळ_Max + 1);
            }
            जर (entp->m_leftp) find_वृक्ष(lineno, entp->m_leftp.get(), resultsr, nextChanger);
            जर (entp->m_rightp) find_वृक्ष(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    VInterval_वृक्ष() = पूर्वनिर्धारित;
    ~VInterval_वृक्ष() = पूर्वनिर्धारित;
    // METHODS
    शून्य clear() { m_rootp = nullptr; }
    शून्य build(स्थिरांक IgnIndices& points) {
        clear();
        m_rootp = build_वृक्ष(points);
    }
    शून्य find(पूर्णांक lineno, IgnIndices& resultsr, पूर्णांक& nextChanger) स्थिरांक {
        resultsr = m_every_ओळी;
        nextChanger = std::numeric_limits<पूर्णांक>::max();

        जर (m_rootp) find_वृक्ष(lineno, m_rootp.get(), resultsr, nextChanger);
        // Sort indices, so can process in parse order
        std::sort(resultsr.begin(), resultsr.end());
    }

    स्थिर शून्य selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices data;
        std::vector<std::pair<पूर्णांक, पूर्णांक>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        साठी (स्थिरांक स्वतः& it : points) {
            control_Ign_ओळी.emplace_back(
                V3_Control_Ignores_ओळ{V3ErrorCode::I_LINT, it.first, it.second, सत्य});
            data.emplace_back(static_cast<uint32_t>(control_Ign_ओळी.size() - 1));
        }
        VInterval_वृक्ष वृक्ष;
        वृक्ष.build(data);

        IgnIndices results;
        पूर्णांक nextChange = 0;
        वृक्ष.find(0, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, 10);
        वृक्ष.find(10, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(nextChange, 11);
        वृक्ष.find(11, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(nextChange, 15);  // Center, or would be 20
        वृक्ष.find(20, results, nextChange);
        UASSERT_SELFTEST(results.size(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(nextChange, 21);
        वृक्ष.find(21, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 25);  // Center, or would be 30
        वृक्ष.find(30, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 31);
        वृक्ष.find(40, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 41);
        वृक्ष.find(41, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, std::numeric_limits<पूर्णांक>::max());
        //
        points = {{0, 0}};
        साठी (स्थिरांक स्वतः& it : points) {
            control_Ign_ओळी.emplace_back(
                V3_Control_Ignores_ओळ{V3ErrorCode::I_LINT, it.first, it.second, सत्य});
            data.emplace_back(static_cast<uint32_t>(control_Ign_ओळी.size() - 1));
        }
        वृक्ष.build(data);
        //
        वृक्ष.find(50, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
using V3_Control_ओळ_Attribute = std::bitset<VPragmaType::_ENUM_SIZE>;

वर्ग WaiverSetting final {
सार्वजनिक:
    V3ErrorCode m_code;  // Error code
    string m_contents;  // --contents regexp
    string m_match;  // --match regexp
    WaiverSetting(V3ErrorCode code, स्थिरांक string& contents, स्थिरांक string& match)
        : m_code{code}
        , m_contents{contents}
        , m_match{match} {}
    ~WaiverSetting() = पूर्वनिर्धारित;
    WaiverSetting& चालक=(स्थिरांक WaiverSetting& rhs) = पूर्वनिर्धारित;
};

// File entity
वर्ग V3ControlFile final {
    using ओळ_Attr_Map = std::map<पूर्णांक, V3_Control_ओळ_Attribute>;  // Map line->bitset of attributes
    using Waivers = std::vector<WaiverSetting>;  // List of {code,wildcard string}

    ओळ_Attr_Map m_ओळ_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_वृक्ष m_interval_वृक्ष;  // Tree of indices to IgnLines

    संरचना {
        पूर्णांक filenameno = -1;  // Last filename
        पूर्णांक lineno = -1;  // Last linenumber
        पूर्णांक nextChange = -1;  // Line number of next change
    } m_lastIgnore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    तार्किक ओळ_Match(पूर्णांक lineno, VPragmaType type) {
        जर (m_ओळ_Attrs.find(0) != m_ओळ_Attrs.end() && m_ओळ_Attrs[0][type]) परत सत्य;
        जर (m_ओळ_Attrs.find(lineno) == m_ओळ_Attrs.end()) परत असत्य;
        परत m_ओळ_Attrs[lineno][type];
    }

सार्वजनिक:
    V3ControlFile() = पूर्वनिर्धारित;
    शून्य update(स्थिरांक V3ControlFile& file) {
        // Copy in all attributes and waivers
        साठी (स्थिरांक स्वतः& itr : file.m_ओळ_Attrs) m_ओळ_Attrs[itr.first] |= itr.second;
        m_waivers.reserve(m_waivers.size() + file.m_waivers.size());
        m_waivers.घाला(m_waivers.end(), file.m_waivers.begin(), file.m_waivers.end());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.size() + file.m_ignIndices.size());
        m_ignIndices.घाला(m_ignIndices.end(), file.m_ignIndices.begin(),
                            file.m_ignIndices.end());
        // updateFinalize() will soon build tree
    }
    शून्य updateFinalize() { m_interval_वृक्ष.build(m_ignIndices); }
    शून्य add_ओळ_Attribute(पूर्णांक lineno, VPragmaType attr) { m_ओळ_Attrs[lineno].set(attr); }
    शून्य addIgnore(V3ErrorCode code, पूर्णांक min, पूर्णांक max, तार्किक on) {
        control_Ign_ओळी.emplace_back(V3_Control_Ignores_ओळ{code, min, max, on});
        m_ignIndices.emplace_back(static_cast<uint32_t>(control_Ign_ओळी.size() - 1));
        m_lastIgnore.nextChange = -1;
    }
    शून्य addIgnoreMatch(V3ErrorCode code, स्थिरांक string& contents, स्थिरांक string& match) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string newMatch = match;
        जर (newMatch.empty() || newMatch.back() != '*') newMatch += '*';
        m_waivers.emplace_back(code, contents, newMatch);
    }

    शून्य applyBlock(AstGenBlock* nodep) {
        // Apply to block at this line
        स्थिरांक VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        जर (ओळ_Match(nodep->fileline()->lineno(), pragma)) {
            nodep->addItemsp(नवीन AstPragma{nodep->fileline(), pragma});
        }
    }
    शून्य applyBlock(Ast_गाठ_Block* nodep) {
        // Apply to block at this line
        स्थिरांक VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_ओळ* स्थिरांक flp = nodep->fileline();
        जर (ओळ_Match(flp->lineno(), pragma)) {
            nodep->addStmtsp(नवीन AstStmtPragma{flp, नवीन AstPragma{flp, pragma}});
        }
    }
    शून्य applyCase(AstCase* nodep) {
        // Apply to this case at this line
        स्थिरांक पूर्णांक lineno = nodep->fileline()->lineno();
        जर (ओळ_Match(lineno, VPragmaType::FULL_CASE)) nodep->fullPragma(सत्य);
        जर (ओळ_Match(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(सत्य);
    }
    शून्य applyIgnores(File_ओळ* filelinep) {
        // HOT routine, called each parsed token line of this filename
        जर (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() == m_lastIgnore.lineno)
            परत;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        जर (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() < m_lastIgnore.nextChange) {
            m_lastIgnore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            परत;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_lastIgnore.filenameno = filelinep->filenameno();
        m_lastIgnore.lineno = filelinep->lineno();
        m_lastIgnore.nextChange = std::numeric_limits<पूर्णांक>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_वृक्ष.find(filelinep->lineno(), results /*ref*/, m_lastIgnore.nextChange /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VErrorBitSet bitset{VErrorBitSet::AllOnes{}};
        साठी (स्थिरांक स्वतः& it : results) {
            स्थिरांक V3_Control_Ignores_ओळ& cign = control_Ign_ओळी[it];
            UASSERT(cign.inRange(filelinep->lineno()),
                    "Interval tree returned lines outside range");
            UINFO(9, "     Hit " << cign);
            cign.m_code.forDelegateCodes(
                [&](V3ErrorCode subcode) { bitset.set(subcode, cign.m_on); });
        }

        filelinep->warnSetCtrlBitSet(bitset);
        m_lastIgnore.nextChange = std::max(m_lastIgnore.nextChange, filelinep->lineno() + 1);
        UINFO(9, "    AppliedIgnores " << filelinep << " next@ln " << std::dec
                                       << m_lastIgnore.nextChange
                                       << " Messages: " << bitset.ascii());
    }
    तार्किक waive(V3ErrorCode code, स्थिरांक string& message) {
        जर (code.hardError()) परत असत्य;
        साठी (स्थिरांक स्वतः& itr : m_waivers) {
            जर ((code.isUnder(itr.m_code) || (itr.m_code == V3ErrorCode::I_LINT))
                && VString::wildmatch(message, itr.m_match)
                && WildcardContents::resolve(itr.m_contents)) {
                परत सत्य;
            }
        }
        परत असत्य;
    }
};

using V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

वर्ग V3ControlScopeTraceEntry final {
सार्वजनिक:
    स्थिरांक string m_scope;  // Scope or regexp to match
    स्थिरांक तार्किक m_on;  // True to enable message
    पूर्णांक m_levels = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3ControlScopeTraceEntry(स्थिरांक string& scope, तार्किक on, पूर्णांक levels)
        : m_scope{scope}
        , m_on{on}
        , m_levels{levels} {}
    ~V3ControlScopeTraceEntry() = पूर्वनिर्धारित;
    तार्किक चालक<(स्थिरांक V3ControlScopeTraceEntry& other) स्थिरांक {
        जर (m_on < other.m_on) परत सत्य;
        जर (m_on > other.m_on) परत असत्य;
        जर (m_levels < other.m_levels) परत सत्य;
        जर (m_levels > other.m_levels) परत असत्य;
        परत m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
वर्ग V3ControlScopeTraceEntryMatch final {
सार्वजनिक:
    स्थिरांक V3ControlScopeTraceEntry* m_entryp;  // Entry the match refers to
    स्थिरांक string m_scopepart;  // Scope prefix being matched against the entry
    V3ControlScopeTraceEntryMatch(स्थिरांक V3ControlScopeTraceEntry* entryp, स्थिरांक string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    तार्किक चालक<(स्थिरांक V3ControlScopeTraceEntryMatch& other) स्थिरांक {
        जर (m_entryp < other.m_entryp) परत सत्य;
        जर (m_entryp > other.m_entryp) परत असत्य;
        परत m_scopepart < other.m_scopepart;
    }
};

वर्ग V3ControlScopeTraceResolver final {
    std::vector<V3ControlScopeTraceEntry> m_entries;  // User specified on/offs and levels
    std::map<V3ControlScopeTraceEntryMatch, तार्किक> m_matchCache;  // Matching entries for speed

सार्वजनिक:
    शून्य addScopeTraceOn(तार्किक on, स्थिरांक string& scope, पूर्णांक levels) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << levels);
        m_entries.emplace_back(V3ControlScopeTraceEntry{scope, on, levels});
        m_matchCache.clear();
    }

    तार्किक getEntryMatch(स्थिरांक V3ControlScopeTraceEntry* entp, स्थिरांक string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        स्थिरांक V3ControlScopeTraceEntryMatch किल्ली{entp, scopepart};
        स्थिरांक स्वतः pair = m_matchCache.emplace(किल्ली, असत्य);
        जर (pair.second) pair.first->second = VString::wildmatch(scopepart, entp->m_scope);
        परत pair.first->second;
    }

    तार्किक getScopeTraceOn(स्थिरांक string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        पूर्णांक maxLevel = 1;
        साठी (स्थिरांक स्वतः& ch : scope) {
            जर (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        तार्किक enabled = सत्य;
        साठी (स्थिरांक स्वतः& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            पूर्णांक partLevel = 1;
            साठी (string::size_type partEnd = 0; सत्य;) {
                partEnd = scope.find('.', partEnd + 1);
                जर (partEnd == string::npos) partEnd = scope.length();
                स्थिरांक std::string scopepart = scope.substr(0, partEnd);
                जर (getEntryMatch(&ent, scopepart)) {
                    स्थिरांक तार्किक levelMatch
                        = !ent.m_levels || (ent.m_levels >= maxLevel - partLevel);
                    जर (levelMatch) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (levelMatch ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    थांबा;
                }
                जर (partEnd == scope.length()) थांबा;
                ++partLevel;
            }
        }
        परत enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

वर्ग V3ControlResolverHierWorkerEntry final {
    स्थिरांक पूर्णांक m_workers;  // Number of workers requested for the hierarchical block
    File_ओळ* स्थिरांक m_flp;  // Location the worker count was specified at

सार्वजनिक:
    explicit V3ControlResolverHierWorkerEntry(पूर्णांक workers, File_ओळ* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    पूर्णांक workers() स्थिरांक { परत m_workers; }
    File_ओळ* flp() स्थिरांक { परत m_flp; }
};

वर्ग V3ControlResolver final {
    गणना ProfileDataMode : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profileData;  // Access to profile_data records
    uint8_t m_mode = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3ControlResolverHierWorkerEntry> m_hierWorkers;
    File_ओळ* m_profile_File_ओळ = nullptr;  // Location profile_data was read from

    V3ControlResolver() = पूर्वनिर्धारित;
    ~V3ControlResolver() = पूर्वनिर्धारित;

सार्वजनिक:
    स्थिर V3ControlResolver& s() {
        स्थिर V3ControlResolver s_singleton;
        परत s_singleton;
    }
    V3ControlModuleResolver& modules() { परत m_modules; }
    V3ControlFileResolver& files() { परत m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { परत m_scopeTraces; }

    शून्य addProfileData(File_ओळ* fl, स्थिरांक string& hierDpi, uint64_t cost) {
        // Empty key for hierarchical DPI wrapper costs.
        addProfileData(fl, hierDpi, "", cost, HIER_DPI);
    }
    शून्य addProfileData(File_ओळ* fl, स्थिरांक string& model, स्थिरांक string& किल्ली, uint64_t cost,
                        ProfileDataMode mode = MTASK) {
        जर (!m_profile_File_ओळ) m_profile_File_ओळ = fl;
        जर (cost == 0) cost = 1;  // Cost 0 means delete (or no data)
        जर (mode == MTASK) {
            m_profileData[model][किल्ली] += cost;
        } अन्यथा जर (mode == HIER_DPI) {
            m_profileData[model][किल्ली] = std::max(m_profileData[model][किल्ली], cost);
        }
        m_mode |= mode;
    }
    तार्किक containsMTaskProfileData() स्थिरांक { परत m_mode & MTASK; }
    uint64_t getProfileData(स्थिरांक string& hierDpi) स्थिरांक {
        // Empty key for hierarchical DPI wrapper costs.
        परत getProfileData(hierDpi, "");
    }
    शून्य addHierWorkers(File_ओळ* flp, स्थिरांक string& model, पूर्णांक workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    पूर्णांक getHierWorkers(स्थिरांक string& model) स्थिरांक {
        स्थिरांक स्वतः mit = m_hierWorkers.find(model);
        // Assign a single worker if no specified.
        परत mit != m_hierWorkers.cend() ? mit->second.workers() : 0;
    }
    File_ओळ* get_Hier_Workers_File_ओळ(स्थिरांक string& model) स्थिरांक {
        स्थिरांक स्वतः mit = m_hierWorkers.find(model);
        परत mit != m_hierWorkers.cend() ? mit->second.flp() : v3Global.rootp()->fileline();
    }
    uint64_t getProfileData(स्थिरांक string& model, स्थिरांक string& किल्ली) स्थिरांक {
        स्थिरांक स्वतः mit = m_profileData.find(model);
        जर (mit == m_profileData.cend()) परत 0;
        स्थिरांक स्वतः it = mit->second.find(किल्ली);
        जर (it == mit->second.cend()) परत 0;
        परत it->second;
    }
    File_ओळ* get_Profile_Data_File_ओळ() स्थिरांक { परत m_profile_File_ओळ; }  // Maybe null
    स्थिर uint64_t getCurrentHierBlockCost() {
        जर (uint64_t cost = V3Control::getProfileData(v3Global.opt.prefix())) {
            UINFO(9, "Fetching cost from profile info: " << cost);
            परत cost;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* स्थिरांक netlistp = v3Global.rootp();
        uint64_t cost = 0;
        साठी (पूर्णांक i = 0; i < VEval::_ENUM_END; ++i) {
            स्थिरांक VEval eval{i};
            जर (eval.slow()) सुरूठेवा;
            cost += V3_Instr_मोजणी::मोजणी(netlistp->evalFuncp(eval), असत्य);
        }
        UINFO(9, "Evaluating cost: " << cost);
        परत cost;
    }
};

//######################################################################
// V3Control

शून्य V3Control::addCaseFull(स्थिरांक string& filename, पूर्णांक lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_ओळ_Attribute(lineno, VPragmaType::FULL_CASE);
}
शून्य V3Control::addCaseParallel(स्थिरांक string& filename, पूर्णांक lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_ओळ_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
शून्य V3Control::addCoverageBlockOff(स्थिरांक string& filename, पूर्णांक lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_ओळ_Attribute(lineno, VPragmaType::COVERAGE_BLOCK_OFF);
}
शून्य V3Control::addCoverageBlockOff(स्थिरांक string& module, स्थिरांक string& blockname) {
    V3ControlResolver::s().modules().at(module).addCoverageBlockOff(blockname);
}

शून्य V3Control::addHierWorkers(File_ओळ* fl, स्थिरांक string& model, पूर्णांक workers) {
    V3ControlResolver::s().addHierWorkers(fl, model, workers);
}

शून्य V3Control::addFsmRegisterWrapper(File_ओळ* fl, स्थिरांक string& module, स्थिरांक string& d,
                                      स्थिरांक string& q, स्थिरांक string& घड़ी, स्थिरांक string& reset,
                                      स्थिरांक string& reset_मूल्य) {
    string missing;
    जर (module.empty()) missing += "-module";
    जर (d.empty()) missing = VString::dot(missing, ", ", "-d");
    जर (q.empty()) missing = VString::dot(missing, ", ", "-q");
    जर (घड़ी.empty()) missing = VString::dot(missing, ", ", "-clock");
    जर (!missing.empty()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        परत;
    }

    स्थिरांक FsmRegisterWrapper desc{module, d, q, घड़ी, reset, reset_मूल्य};
    V3ControlResolver::s().modules().at(module).setFsmRegisterWrapper(fl, desc);
}

शून्य V3Control::addIgnore(V3ErrorCode code, तार्किक on, स्थिरांक string& filename, पूर्णांक min, पूर्णांक max) {
    UINFO(9, "addIgnore " << code << " " << min << "-" << max << " fn=" << filename);
    जर (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_ओळ::globalWarnOff(code, !on);
    } अन्यथा {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).addIgnore(code, min, max, on);
    }
}
शून्य V3Control::addIgnoreMatch(V3ErrorCode code, स्थिरांक string& filename, स्थिरांक string& contents,
                               स्थिरांक string& match) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).addIgnoreMatch(code, contents, match);
}

शून्य V3Control::addInline(File_ओळ* fl, स्थिरांक string& module, स्थिरांक string& ftask, तार्किक on) {
    जर (ftask.empty()) {
        V3ControlResolver::s().modules().at(module).setInline(on);
    } अन्यथा {
        जर (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } अन्यथा {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setNoInline(on);
        }
    }
}

शून्य V3Control::addModulePragma(स्थिरांक string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).addModulePragma(pragma);
}

शून्य V3Control::addProfileData(File_ओळ* fl, स्थिरांक string& hierDpi, uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, hierDpi, cost);
}
शून्य V3Control::addProfileData(File_ओळ* fl, स्थिरांक string& model, स्थिरांक string& किल्ली,
                               uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, model, किल्ली, cost);
}

शून्य V3Control::addScopeTraceOn(तार्किक on, स्थिरांक string& scope, पूर्णांक levels) {
    V3ControlResolver::s().scopeTraces().addScopeTraceOn(on, scope, levels);
}

शून्य V3Control::addVarAttr(File_ओळ* fl, स्थिरांक string& module, स्थिरांक string& ftask,
                           VarSpecKind kind, स्थिरांक string& pattern, VAttrType attr,
                           Ast_Sen_वृक्ष* sensep) {
    जर (sensep) {
        File_ओळ* स्थिरांक flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_वृक्ष(), sensep);
        // Used to be only accepted on public_flat_rw
        जर (attr != VAttrType::VAR_PUBLIC_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            परत;
        }
    }

    जर (kind != VarSpecKind::VAR) {
        निवड (attr) {
        स्थिती VAttrType::VAR_PUBLIC_FLAT:
        स्थिती VAttrType::VAR_PUBLIC_FLAT_RD:
        स्थिती VAttrType::VAR_PUBLIC_FLAT_RW: थांबा;
        पूर्वनिर्धारित:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            परत;
        }
    }

    // Semantics: Most of the attributes operate on signals
    जर (pattern.empty()) {
        जर (attr == VAttrType::VAR_PUBLIC) {
            जर (ftask.empty()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).addModulePragma(
                    VPragmaType::PUBLIC_MODULE);
            } अन्यथा {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setPublic(सत्य);
            }
        } अन्यथा {
            fl->v3error("missing -var");
        }
    } अन्यथा {
        जर (attr == VAttrType::VAR_FORCEABLE) {
            जर (module.empty()) {
                fl->v3error("forceable missing -module");
            } अन्यथा जर (!ftask.empty()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } अन्यथा {
                V3ControlResolver::s().modules().at(module).vars().at(pattern).add(attr);
            }
        } अन्यथा {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                जर (ftask.empty()) {
                    जर (kind == VarSpecKind::PARAM) परत mod.params().at(pattern);
                    जर (kind == VarSpecKind::PORT) परत mod.ports().at(pattern);
                    UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                    परत mod.vars().at(pattern);
                }
                जर (kind == VarSpecKind::PARAM) परत mod.ftasks().at(ftask).params().at(pattern);
                जर (kind == VarSpecKind::PORT) परत mod.ftasks().at(ftask).ports().at(pattern);
                UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                परत mod.ftasks().at(ftask).vars().at(pattern);
            }();
            controlVar.add(attr);
        }
    }
}

शून्य V3Control::applyCase(AstCase* nodep) {
    स्थिरांक string& filename = nodep->fileline()->filename();
    V3ControlFile* स्थिरांक filep = V3ControlResolver::s().files().resolve(filename);
    जर (filep) filep->applyCase(nodep);
}

शून्य V3Control::applyCoverageBlock(Ast_गाठ_Module* modulep, AstBegin* nodep) {
    स्थिरांक string& filename = nodep->fileline()->filename();
    V3ControlFile* स्थिरांक filep = V3ControlResolver::s().files().resolve(filename);
    जर (filep) filep->applyBlock(nodep);
    स्थिरांक string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* स्थिरांक modp = V3ControlResolver::s().modules().resolve(modname);
    जर (modp) modp->applyBlock(nodep);
}
शून्य V3Control::applyCoverageBlock(Ast_गाठ_Module* modulep, AstGenBlock* nodep) {
    स्थिरांक string& filename = nodep->fileline()->filename();
    V3ControlFile* स्थिरांक filep = V3ControlResolver::s().files().resolve(filename);
    जर (filep) filep->applyBlock(nodep);
    स्थिरांक string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* स्थिरांक modp = V3ControlResolver::s().modules().resolve(modname);
    जर (modp) modp->applyBlock(nodep);
}

शून्य V3Control::applyIgnores(File_ओळ* filelinep) {
    स्थिरांक string& filename = filelinep->filename();
    V3ControlFile* स्थिरांक filep = V3ControlResolver::s().files().resolve(filename);
    जर (filep) filep->applyIgnores(filelinep);
}

शून्य V3Control::applyModule(Ast_गाठ_Module* modulep) {
    स्थिरांक string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* स्थिरांक modp = V3ControlResolver::s().modules().resolve(modname);
    जर (modp) modp->apply(modulep);
}

शून्य V3Control::applyFTask(Ast_गाठ_Module* modulep, Ast_गाठ_FTask* ftaskp) {
    स्थिरांक string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* स्थिरांक modp = V3ControlResolver::s().modules().resolve(modname);
    जर (!modp) परत;
    स्थिरांक V3ControlFTask* स्थिरांक ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    जर (ftp) ftp->apply(ftaskp);
}

ढाँचा <typename T_Resolver>
स्थिर शून्य resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    स्थिरांक std::string name = varp->prettyDehashOrigOrName();
    जर (स्थिरांक V3ControlVar* स्थिरांक vp = resolver.vars().resolve(name)) vp->apply(varp);
    जर (varp->isParam()) {
        जर (स्थिरांक V3ControlVar* स्थिरांक vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    जर (varp->isIO()) {
        जर (स्थिरांक V3ControlVar* स्थिरांक vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

शून्य V3Control::applyVarAttr(स्थिरांक Ast_गाठ_Module* modulep, स्थिरांक Ast_गाठ_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* स्थिरांक modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    जर (!modp) परत;
    जर (ftaskp) {
        V3ControlFTask* स्थिरांक ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        जर (!ftp) परत;
        resolveThenApply(*ftp, varp);
    } अन्यथा {
        resolveThenApply(*modp, varp);
    }
}

पूर्णांक V3Control::getHierWorkers(स्थिरांक string& model) {
    परत V3ControlResolver::s().getHierWorkers(model);
}
File_ओळ* V3Control::get_Hier_Workers_File_ओळ(स्थिरांक string& model) {
    परत V3ControlResolver::s().get_Hier_Workers_File_ओळ(model);
}
स्थिरांक V3Control::FsmRegisterWrapper* V3Control::getFsmRegisterWrapper(स्थिरांक string& module) {
    V3ControlModule* स्थिरांक modp = V3ControlResolver::s().modules().resolve(module);
    परत modp ? modp->fsmRegisterWrapperp() : nullptr;
}
uint64_t V3Control::getProfileData(स्थिरांक string& hierDpi) {
    परत V3ControlResolver::s().getProfileData(hierDpi);
}
uint64_t V3Control::getProfileData(स्थिरांक string& model, स्थिरांक string& किल्ली) {
    परत V3ControlResolver::s().getProfileData(model, किल्ली);
}
File_ओळ* V3Control::get_Profile_Data_File_ओळ() {
    परत V3ControlResolver::s().get_Profile_Data_File_ओळ();
}
तार्किक V3Control::getScopeTraceOn(स्थिरांक string& scope) {
    परत V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

शून्य V3Control::contents_ढकला_Text(स्थिरांक string& text) { WildcardContents::ढकला_Text(text); }

तार्किक V3Control::containsMTaskProfileData() {
    परत V3ControlResolver::s().containsMTaskProfileData();
}
uint64_t V3Control::getCurrentHierBlockCost() {
    परत V3ControlResolver::s().getCurrentHierBlockCost();
}

तार्किक V3Control::waive(स्थिरांक File_ओळ* filelinep, V3ErrorCode code, स्थिरांक string& message) {
    V3ControlFile* स्थिरांक filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    जर (!filep) परत असत्य;
    परत filep->waive(code, message);
}

शून्य V3Control::selfTest() { VInterval_वृक्ष::selfTest(); }
