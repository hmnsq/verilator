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

#ضمن "V3PchAstMT.h"

#ضمن "V3Control.h"

#ضمن "V3InstrCount.h"
#ضمن "V3Os.h"
#ضمن "V3String.h"

#ضمن <memory>
#ضمن <set>
#ضمن <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
قالب <نام_نوع T>
صنف V3ControlWildcardResolver final {
    تغییرپذیر V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<ثابت std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<ثابت std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

عام:
    V3ControlWildcardResolver() = افتراضي;
    ~V3ControlWildcardResolver() = افتراضي;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    فراغ update(ثابت V3ControlWildcardResolver& other) VL_MT_امن_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        ثابت V3_قفل_Guard قفل_2{m_mutex};
        ثابت V3_قفل_Guard other_قفل{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        لكل (ثابت خودکار& itr : other.m_mapPatterns) m_mapPatterns[itr.اولی].update(itr.دومی);
    }

    // Access and create a pattern entry
    T& at(ثابت string& name) VL_MT_امن_EXCLUDES(m_mutex) {
        ثابت V3_قفل_Guard قفل_2{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        أرجع m_mapPatterns[V3Os::filename_Slash_مسیر(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(ثابت string& name) VL_MT_امن_EXCLUDES(m_mutex) {
        ثابت V3_قفل_Guard قفل_2{m_mutex};
        // Lookup if it was resolved before, typically not
        ثابت خودکار pair = m_mapResolved.emplace(name, اشاره_تهی);
        std::unique_ptr<T>& entryr = pair.اولی->دومی;
        // Resolve entry when first requested, cache the result
        إذا (pair.دومی) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            ثابت string slashName = V3Os::filename_Slash_مسیر(name);
            لكل (ثابت خودکار& patEnt : m_mapPatterns) {
                إذا (VString::wildmatch(slashName, patEnt.اولی)) {
                    إذا (!entryr) entryr.reset(جديد T{});
                    entryr->update(patEnt.دومی);
                }
            }
            // Perform final actions that needed all updates completed
            إذا (entryr) entryr->updateFinalize();
        }
        أرجع entryr.get();
    }
};

// List of attributes for variables
صنف V3ControlVar final {
    std::وکتور<VAttrType> m_attrs;  // The list of attributes
عام:
    // Add new attribugte
    فراغ جمع_کن(VAttrType attr) { m_attrs.emplace_پشت(attr); }
    // Update from other by copying all attributes
    فراغ update(ثابت V3ControlVar& other) {
        m_attrs.reserve(m_attrs.اندازه() + other.m_attrs.اندازه());
        m_attrs.درج(m_attrs.انتها(), other.m_attrs.begin(), other.m_attrs.انتها());
    }
    فراغ updateFinalize() {}
    // Apply all attributes to the variable
    فراغ apply(AstVar* varp) ثابت {
        لكل (ثابت VAttrType attr : m_attrs) {
            varp->جمع_کن_Attrsp(جديد Ast_Attr_از{varp->fileline(), attr});
        }
    }
};

استفاده V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

صنف WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<ثابت std::string, منطقي> m_mapPatterns;  // Pattern match results
    std::deque<string> m_سطرها;  // Source text lines

    // METHODS
    ساكن WildcardContents& s() {  // Singleton
        ساكن WildcardContents s_s;
        أرجع s_s;
    }
    فراغ clearCacheImp() { m_mapPatterns.clear(); }
    فراغ افزودن_متن_Imp(ثابت string& متن) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        ثابت string leftover = m_سطرها.پشت() + متن;
        m_سطرها.برداشت_پشت();

        // Insert line-by-line
        string::اندازه_type سطر_شروع = 0;
        طالما (درست) {
            ثابت string::اندازه_type سطر_انتها = leftover.پیدا_کن('\n', سطر_شروع);
            إذا (سطر_انتها != string::npos) {
                ثابت string oneline(leftover, سطر_شروع, سطر_انتها - سطر_شروع + 1);
                إذا (oneline.اندازه() > 1) m_سطرها.افزودن_پشت(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_سطرها.اندازه() - 1) << "]: " << oneline);
                سطر_شروع = سطر_انتها + 1;
            } وإلا {
                اكسر;
            }
        }
        // Keep leftover for next time
        m_سطرها.emplace_پشت(string(leftover, سطر_شروع));  // Might be ""
        clearCacheImp();
    }

    منطقي resolveUncachedImp(ثابت string& contentsRegexp) {
        لكل (ثابت string& i : m_سطرها) {
            إذا (VString::wildmatch(i, contentsRegexp)) أرجع درست;
        }
        أرجع خطأ;
    }
    منطقي resolveCachedImp(ثابت string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        إذا (contentsRegexp.خالی()) أرجع درست;
        ثابت خودکار pair = m_mapPatterns.emplace(contentsRegexp, خطأ);
        منطقي& entryr = pair.اولی->دومی;
        // Resolve entry when first requested, cache the result
        إذا (pair.دومی) entryr = resolveUncachedImp(contentsRegexp);
        أرجع entryr;
    }

عام:
    WildcardContents() {
        m_سطرها.emplace_پشت("");  // start with no leftover
    }
    ~WildcardContents() = افتراضي;
    // Return true iff contentsRegexp in parsed contents
    ساكن منطقي resolve(ثابت string& contentsRegexp) {
        أرجع s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    ساكن فراغ افزودن_متن(ثابت string& متن) { s().افزودن_متن_Imp(متن); }
};

//######################################################################
// Function or task: Have variables and properties

صنف V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    منطقي m_noinline = خطأ;  // Don't inline function/task
    منطقي m_عمومی = خطأ;  // Public function/task

عام:
    V3ControlFTask() = افتراضي;
    فراغ update(ثابت V3ControlFTask& f) {
        // Don't overwrite true with false
        إذا (f.m_noinline) m_noinline = درست;
        إذا (f.m_عمومی) m_عمومی = درست;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    فراغ updateFinalize() {}

    V3ControlVarResolver& params() { أرجع m_params; }
    V3ControlVarResolver& ports() { أرجع m_ports; }
    V3ControlVarResolver& vars() { أرجع m_vars; }

    فراغ مجموعه_No_Inline(منطقي مجموعه) { m_noinline = مجموعه; }
    فراغ مجموعه_عمومی(منطقي مجموعه) { m_عمومی = مجموعه; }

    فراغ apply(Ast_گره_FTask* ftaskp) ثابت {
        إذا (m_noinline)
            ftaskp->جمع_کن_Stmtsp(جديد AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        إذا (m_عمومی)
            ftaskp->جمع_کن_Stmtsp(جديد AstPragma{ftaskp->fileline(), VPragmaType::عمومی_TASK});
    }
};

استفاده V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

صنف V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::Fsm_ثبات_Wrapper m_fsm_ثبات_Wrapper;  // FSM wrapper descriptor
    std::unordered_مجموعه<std::string> m_coverage_Off_بلوکها;  // List of block names for coverage_off
    std::مجموعه<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    منطقي m_has_Fsm_ثبات_Wrapper = خطأ;  // Whether descriptor has been set
    منطقي m_inline = خطأ;  // Whether to force the inline
    منطقي m_inline_مقدار = خطأ;  // The inline value (on/off)

عام:
    V3ControlModule() = افتراضي;

    فراغ update(ثابت V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        إذا (m.m_has_Fsm_ثبات_Wrapper) {
            m_fsm_ثبات_Wrapper = m.m_fsm_ثبات_Wrapper;
            m_has_Fsm_ثبات_Wrapper = درست;
        }
        لكل (ثابت string& i : m.m_coverage_Off_بلوکها) m_coverage_Off_بلوکها.درج(i);
        إذا (!m_inline) {
            m_inline = m.m_inline;
            m_inline_مقدار = m.m_inline_مقدار;
        }
        لكل (خودکار it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.درج(*it);
        }
    }
    فراغ updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { أرجع m_tasks; }
    V3ControlVarResolver& params() { أرجع m_params; }
    V3ControlVarResolver& ports() { أرجع m_ports; }
    V3ControlVarResolver& vars() { أرجع m_vars; }

    فراغ جمع_کن_Coverage_بلوک_Off(ثابت string& name) { m_coverage_Off_بلوکها.درج(name); }
    فراغ مجموعه_Inline(منطقي مجموعه) {
        m_inline = درست;
        m_inline_مقدار = مجموعه;
    }
    فراغ مجموعه_Fsm_ثبات_Wrapper(File_سطر* fl, ثابت V3Control::Fsm_ثبات_Wrapper& desc) {
        إذا (m_has_Fsm_ثبات_Wrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_گره::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsm_ثبات_Wrapper = desc;
        m_has_Fsm_ثبات_Wrapper = درست;
    }
    ثابت V3Control::Fsm_ثبات_Wrapper* fsm_ثبات_Wrapperp() ثابت {
        أرجع m_has_Fsm_ثبات_Wrapper ? &m_fsm_ثبات_Wrapper : اشاره_تهی;
    }
    فراغ جمع_کن_Module_Pragma(VPragmaType pragma) { m_modPragmas.درج(pragma); }

    فراغ apply(Ast_گره_Module* modp) {
        إذا (m_inline) {
            ثابت VPragmaType type
                = m_inline_مقدار ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_گره* ثابت nodep = جديد AstPragma{modp->fileline(), type};
            modp->جمع_کن_Stmtsp(nodep);
        }
        لكل (ثابت خودکار& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            إذا (itr == VPragmaType::HIER_PARAMS) modp->hierParams(درست);
            Ast_گره* ثابت nodep = جديد AstPragma{modp->fileline(), itr};
            modp->جمع_کن_Stmtsp(nodep);
        }
    }

    فراغ apply_بلوک(Ast_Gen_بلوک* nodep) {
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلوک_OFF;
        إذا (!nodep->unnamed()) {
            لكل (ثابت string& i : m_coverage_Off_بلوکها) {
                إذا (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->جمع_کن_Itemsp(جديد AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    فراغ apply_بلوک(Ast_گره_بلوک* nodep) {
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلوک_OFF;
        File_سطر* ثابت flp = nodep->fileline();
        إذا (!nodep->unnamed()) {
            لكل (ثابت string& i : m_coverage_Off_بلوکها) {
                إذا (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->جمع_کن_Stmtsp(جديد AstStmtPragma{flp, جديد AstPragma{flp, pragma}});
                }
            }
        }
    }
};

استفاده V3ControlModuleResolver = V3ControlWildcardResolver<V3ControlModule>;

//######################################################################
// Files have:
//  - Line ignores (lint/coverage/tracing on/off)
//  - Line attributes: Attributes attached to lines

// lint/coverage/tracing on/off
صنف V3_Control_Ignores_سطر final {
عام:
    ثابت صحيح m_سطر_Min;  // Minimum line number to make change at
    ثابت صحيح m_سطر_Max;  // Maximum line number to make change at (inclusive)
    ثابت V3_Error_کد m_کد;  // Error code
    ثابت منطقي m_on;  // True to enable message
    V3_Control_Ignores_سطر(V3_Error_کد کد, صحيح linemin, صحيح linemax, منطقي on)
        : m_سطر_Min{linemin}
        , m_سطر_Max{linemax}
        , m_کد{کد}
        , m_on{on} {}
    ~V3_Control_Ignores_سطر() = افتراضي;
    منطقي every_سطر() ثابت { أرجع m_سطر_Min == 0 && m_سطر_Max == 0; }
    منطقي درون_Range(صحيح lineno) ثابت {
        أرجع (lineno >= m_سطر_Min && lineno <= m_سطر_Max) || every_سطر();
    }
};
std::ostream& عملگر<<(std::ostream& os, ثابت V3_Control_Ignores_سطر& rhs) {
    أرجع os << rhs.m_سطر_Min << "-" << rhs.m_سطر_Max << ", " << rhs.m_کد << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
ساكن std::وکتور<V3_Control_Ignores_سطر> control_Ign_سطرها;

استفاده IgnIndices = std::وکتور<uint32_t>;  // List of {s_ignLines indces}

صنف VInterval_درخت final {
    بنية مدخل_2 final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        صحيح m_center;  // Line number of central point
        std::unique_ptr<مدخل_2> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<مدخل_2> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        صریح مدخل_2(صحيح center)
            : m_center{center} {}
    };

    std::unique_ptr<مدخل_2> m_rootp;  // Root of interval tree
    IgnIndices m_every_سطرها;  // All-line disables

خاص:
    std::unique_ptr<مدخل_2> build_درخت(ثابت IgnIndices& points) {
        إذا (points.خالی()) أرجع اشاره_تهی;

        صحيح minval = std::numeric_limits<صحيح>::max();
        صحيح maxval = std::numeric_limits<صحيح>::min();
        لكل (ثابت خودکار& it : points) {
            ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطرها[it];
            إذا (cign.every_سطر()) {
                m_every_سطرها.emplace_پشت(it);
                استمر;
            }
            minval = std::min(minval, cign.m_سطر_Min);
            maxval = std::max(maxval, cign.m_سطر_Max);
        }

        ثابت صحيح center = minval + (maxval - minval) / 2;
        خودکار entp = std::unique_ptr<مدخل_2>(جديد مدخل_2{center});

        IgnIndices چپ_Points;
        IgnIndices راست_Points;
        لكل (ثابت خودکار& it : points) {
            ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطرها[it];
            إذا (cign.every_سطر()) استمر;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            إذا (cign.m_سطر_Max < center)
                چپ_Points.emplace_پشت(it);
            وإلا إذا (cign.m_سطر_Min > center)
                راست_Points.emplace_پشت(it);
            وإلا {
                entp->m_byMin.افزودن_پشت(it);
                entp->m_byMax.افزودن_پشت(it);
            }
        }

        std::مرتب_سازی(entp->m_byMin.begin(), entp->m_byMin.انتها(), [](uint32_t a, uint32_t b) {
            أرجع control_Ign_سطرها[a].m_سطر_Min < control_Ign_سطرها[b].m_سطر_Min;
        });
        std::مرتب_سازی(entp->m_byMax.begin(), entp->m_byMax.انتها(), [](uint32_t a, uint32_t b) {
            أرجع control_Ign_سطرها[a].m_سطر_Max > control_Ign_سطرها[b].m_سطر_Max;
        });

        entp->m_leftp = build_درخت(چپ_Points);
        entp->m_rightp = build_درخت(راست_Points);
        أرجع entp;
    }

    فراغ پیدا_کن_درخت(صحيح lineno, مدخل_2* entp, IgnIndices& resultsr, صحيح& بعدی_Changer) ثابت {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        إذا (lineno < entp->m_center) {
            بعدی_Changer = std::min(بعدی_Changer, entp->m_center);
            لكل (ثابت خودکار& it : entp->m_byMin) {
                ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطرها[it];
                إذا (cign.m_سطر_Min <= lineno) {
                    resultsr.emplace_پشت(it);
                } وإلا {
                    بعدی_Changer = std::min(بعدی_Changer, cign.m_سطر_Min);
                    اكسر;
                }
            }
            إذا (entp->m_leftp) پیدا_کن_درخت(lineno, entp->m_leftp.get(), resultsr, بعدی_Changer);
        } وإلا إذا (lineno > entp->m_center) {
            لكل (ثابت خودکار& it : entp->m_byMax) {
                ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطرها[it];
                إذا (cign.m_سطر_Max >= lineno) {
                    بعدی_Changer = std::min(بعدی_Changer, cign.m_سطر_Max + 1);
                    resultsr.emplace_پشت(it);
                } وإلا {
                    اكسر;
                }
            }
            إذا (entp->m_rightp) پیدا_کن_درخت(lineno, entp->m_rightp.get(), resultsr, بعدی_Changer);
        } وإلا {
            لكل (ثابت خودکار& it : entp->m_byMin) {
                ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطرها[it];
                resultsr.emplace_پشت(it);
                إذا (cign.m_سطر_Max >= lineno)
                    بعدی_Changer = std::min(بعدی_Changer, cign.m_سطر_Max + 1);
            }
            إذا (entp->m_leftp) پیدا_کن_درخت(lineno, entp->m_leftp.get(), resultsr, بعدی_Changer);
            إذا (entp->m_rightp) پیدا_کن_درخت(lineno, entp->m_rightp.get(), resultsr, بعدی_Changer);
        }
    }

عام:
    // CONSTRUCTORS
    VInterval_درخت() = افتراضي;
    ~VInterval_درخت() = افتراضي;
    // METHODS
    فراغ clear() { m_rootp = اشاره_تهی; }
    فراغ build(ثابت IgnIndices& points) {
        clear();
        m_rootp = build_درخت(points);
    }
    فراغ پیدا_کن(صحيح lineno, IgnIndices& resultsr, صحيح& بعدی_Changer) ثابت {
        resultsr = m_every_سطرها;
        بعدی_Changer = std::numeric_limits<صحيح>::max();

        إذا (m_rootp) پیدا_کن_درخت(lineno, m_rootp.get(), resultsr, بعدی_Changer);
        // Sort indices, so can process in parse order
        std::مرتب_سازی(resultsr.begin(), resultsr.انتها());
    }

    ساكن فراغ selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices داده;
        std::وکتور<std::pair<صحيح, صحيح>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        لكل (ثابت خودکار& it : points) {
            control_Ign_سطرها.emplace_پشت(
                V3_Control_Ignores_سطر{V3_Error_کد::I_LINT, it.اولی, it.دومی, درست});
            داده.emplace_پشت(static_cast<uint32_t>(control_Ign_سطرها.اندازه() - 1));
        }
        VInterval_درخت درخت;
        درخت.build(داده);

        IgnIndices results;
        صحيح بعدی_Change = 0;
        درخت.پیدا_کن(0, results, بعدی_Change);
        UASSERT_SELFTEST(results.اندازه(), 0);
        UASSERT_SELFTEST(بعدی_Change, 10);
        درخت.پیدا_کن(10, results, بعدی_Change);
        UASSERT_SELFTEST(results.اندازه(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(بعدی_Change, 11);
        درخت.پیدا_کن(11, results, بعدی_Change);
        UASSERT_SELFTEST(results.اندازه(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(بعدی_Change, 15);  // Center, or would be 20
        درخت.پیدا_کن(20, results, بعدی_Change);
        UASSERT_SELFTEST(results.اندازه(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(بعدی_Change, 21);
        درخت.پیدا_کن(21, results, بعدی_Change);
        UASSERT_SELFTEST(results.اندازه(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(بعدی_Change, 25);  // Center, or would be 30
        درخت.پیدا_کن(30, results, بعدی_Change);
        UASSERT_SELFTEST(results.اندازه(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(بعدی_Change, 31);
        درخت.پیدا_کن(40, results, بعدی_Change);
        UASSERT_SELFTEST(results.اندازه(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(بعدی_Change, 41);
        درخت.پیدا_کن(41, results, بعدی_Change);
        UASSERT_SELFTEST(results.اندازه(), 0);
        UASSERT_SELFTEST(بعدی_Change, std::numeric_limits<صحيح>::max());
        //
        points = {{0, 0}};
        لكل (ثابت خودکار& it : points) {
            control_Ign_سطرها.emplace_پشت(
                V3_Control_Ignores_سطر{V3_Error_کد::I_LINT, it.اولی, it.دومی, درست});
            داده.emplace_پشت(static_cast<uint32_t>(control_Ign_سطرها.اندازه() - 1));
        }
        درخت.build(داده);
        //
        درخت.پیدا_کن(50, results, بعدی_Change);
        UASSERT_SELFTEST(results.اندازه(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
استفاده V3_Control_سطر_Attribute = std::bitset<VPragmaType::ENUM_اندازه>;

صنف WaiverSetting final {
عام:
    V3_Error_کد m_کد;  // Error code
    string m_contents;  // --contents regexp
    string m_تطابق;  // --match regexp
    WaiverSetting(V3_Error_کد کد, ثابت string& contents, ثابت string& تطابق)
        : m_کد{کد}
        , m_contents{contents}
        , m_تطابق{تطابق} {}
    ~WaiverSetting() = افتراضي;
    WaiverSetting& عملگر=(ثابت WaiverSetting& rhs) = افتراضي;
};

// File entity
صنف V3ControlFile final {
    استفاده سطر_Attr_Map = std::map<صحيح, V3_Control_سطر_Attribute>;  // Map line->bitset of attributes
    استفاده Waivers = std::وکتور<WaiverSetting>;  // List of {code,wildcard string}

    سطر_Attr_Map m_سطر_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_درخت m_interval_درخت;  // Tree of indices to IgnLines

    بنية {
        صحيح filenameno = -1;  // Last filename
        صحيح lineno = -1;  // Last linenumber
        صحيح بعدی_Change = -1;  // Line number of next change
    } m_آخرین_Ignore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    منطقي سطر_تطابق(صحيح lineno, VPragmaType type) {
        إذا (m_سطر_Attrs.پیدا_کن(0) != m_سطر_Attrs.انتها() && m_سطر_Attrs[0][type]) أرجع درست;
        إذا (m_سطر_Attrs.پیدا_کن(lineno) == m_سطر_Attrs.انتها()) أرجع خطأ;
        أرجع m_سطر_Attrs[lineno][type];
    }

عام:
    V3ControlFile() = افتراضي;
    فراغ update(ثابت V3ControlFile& file) {
        // Copy in all attributes and waivers
        لكل (ثابت خودکار& itr : file.m_سطر_Attrs) m_سطر_Attrs[itr.اولی] |= itr.دومی;
        m_waivers.reserve(m_waivers.اندازه() + file.m_waivers.اندازه());
        m_waivers.درج(m_waivers.انتها(), file.m_waivers.begin(), file.m_waivers.انتها());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.اندازه() + file.m_ignIndices.اندازه());
        m_ignIndices.درج(m_ignIndices.انتها(), file.m_ignIndices.begin(),
                            file.m_ignIndices.انتها());
        // updateFinalize() will soon build tree
    }
    فراغ updateFinalize() { m_interval_درخت.build(m_ignIndices); }
    فراغ جمع_کن_سطر_Attribute(صحيح lineno, VPragmaType attr) { m_سطر_Attrs[lineno].مجموعه(attr); }
    فراغ جمع_کن_Ignore(V3_Error_کد کد, صحيح min, صحيح max, منطقي on) {
        control_Ign_سطرها.emplace_پشت(V3_Control_Ignores_سطر{کد, min, max, on});
        m_ignIndices.emplace_پشت(static_cast<uint32_t>(control_Ign_سطرها.اندازه() - 1));
        m_آخرین_Ignore.بعدی_Change = -1;
    }
    فراغ جمع_کن_Ignore_تطابق(V3_Error_کد کد, ثابت string& contents, ثابت string& تطابق) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string new_تطابق = تطابق;
        إذا (new_تطابق.خالی() || new_تطابق.پشت() != '*') new_تطابق += '*';
        m_waivers.emplace_پشت(کد, contents, new_تطابق);
    }

    فراغ apply_بلوک(Ast_Gen_بلوک* nodep) {
        // Apply to block at this line
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلوک_OFF;
        إذا (سطر_تطابق(nodep->fileline()->lineno(), pragma)) {
            nodep->جمع_کن_Itemsp(جديد AstPragma{nodep->fileline(), pragma});
        }
    }
    فراغ apply_بلوک(Ast_گره_بلوک* nodep) {
        // Apply to block at this line
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلوک_OFF;
        File_سطر* ثابت flp = nodep->fileline();
        إذا (سطر_تطابق(flp->lineno(), pragma)) {
            nodep->جمع_کن_Stmtsp(جديد AstStmtPragma{flp, جديد AstPragma{flp, pragma}});
        }
    }
    فراغ applyCase(AstCase* nodep) {
        // Apply to this case at this line
        ثابت صحيح lineno = nodep->fileline()->lineno();
        إذا (سطر_تطابق(lineno, VPragmaType::پر_CASE)) nodep->پر_Pragma(درست);
        إذا (سطر_تطابق(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(درست);
    }
    فراغ applyIgnores(File_سطر* filelinep) {
        // HOT routine, called each parsed token line of this filename
        إذا (filelinep->filenameno() == m_آخرین_Ignore.filenameno
            && filelinep->lineno() == m_آخرین_Ignore.lineno)
            أرجع;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        إذا (filelinep->filenameno() == m_آخرین_Ignore.filenameno
            && filelinep->lineno() < m_آخرین_Ignore.بعدی_Change) {
            m_آخرین_Ignore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            أرجع;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_آخرین_Ignore.filenameno = filelinep->filenameno();
        m_آخرین_Ignore.lineno = filelinep->lineno();
        m_آخرین_Ignore.بعدی_Change = std::numeric_limits<صحيح>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_درخت.پیدا_کن(filelinep->lineno(), results /*ref*/, m_آخرین_Ignore.بعدی_Change /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VError_بیت_مجموعه bitset{VError_بیت_مجموعه::AllOnes{}};
        لكل (ثابت خودکار& it : results) {
            ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطرها[it];
            UASSERT(cign.درون_Range(filelinep->lineno()),
                    "Interval tree returned lines outside range");
            UINFO(9, "     Hit " << cign);
            cign.m_کد.forDelegateCodes(
                [&](V3_Error_کد subcode) { bitset.مجموعه(subcode, cign.m_on); });
        }

        filelinep->warn_مجموعه_Ctrl_بیت_مجموعه(bitset);
        m_آخرین_Ignore.بعدی_Change = std::max(m_آخرین_Ignore.بعدی_Change, filelinep->lineno() + 1);
        UINFO(9, "    AppliedIgnores " << filelinep << " next@ln " << std::dec
                                       << m_آخرین_Ignore.بعدی_Change
                                       << " Messages: " << bitset.ascii());
    }
    منطقي waive(V3_Error_کد کد, ثابت string& پیام) {
        إذا (کد.hardError()) أرجع خطأ;
        لكل (ثابت خودکار& itr : m_waivers) {
            إذا ((کد.isUnder(itr.m_کد) || (itr.m_کد == V3_Error_کد::I_LINT))
                && VString::wildmatch(پیام, itr.m_تطابق)
                && WildcardContents::resolve(itr.m_contents)) {
                أرجع درست;
            }
        }
        أرجع خطأ;
    }
};

استفاده V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

صنف V3_Control_Scope_Trace_مدخل final {
عام:
    ثابت string m_scope;  // Scope or regexp to match
    ثابت منطقي m_on;  // True to enable message
    صحيح m_سطوح = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3_Control_Scope_Trace_مدخل(ثابت string& scope, منطقي on, صحيح سطوح)
        : m_scope{scope}
        , m_on{on}
        , m_سطوح{سطوح} {}
    ~V3_Control_Scope_Trace_مدخل() = افتراضي;
    منطقي عملگر<(ثابت V3_Control_Scope_Trace_مدخل& other) ثابت {
        إذا (m_on < other.m_on) أرجع درست;
        إذا (m_on > other.m_on) أرجع خطأ;
        إذا (m_سطوح < other.m_سطوح) أرجع درست;
        إذا (m_سطوح > other.m_سطوح) أرجع خطأ;
        أرجع m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
صنف V3_Control_Scope_Trace_مدخل_تطابق final {
عام:
    ثابت V3_Control_Scope_Trace_مدخل* m_entryp;  // Entry the match refers to
    ثابت string m_scopepart;  // Scope prefix being matched against the entry
    V3_Control_Scope_Trace_مدخل_تطابق(ثابت V3_Control_Scope_Trace_مدخل* entryp, ثابت string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    منطقي عملگر<(ثابت V3_Control_Scope_Trace_مدخل_تطابق& other) ثابت {
        إذا (m_entryp < other.m_entryp) أرجع درست;
        إذا (m_entryp > other.m_entryp) أرجع خطأ;
        أرجع m_scopepart < other.m_scopepart;
    }
};

صنف V3ControlScopeTraceResolver final {
    std::وکتور<V3_Control_Scope_Trace_مدخل> m_entries;  // User specified on/offs and levels
    std::map<V3_Control_Scope_Trace_مدخل_تطابق, منطقي> m_تطابق_Cache;  // Matching entries for speed

عام:
    فراغ جمع_کن_Scope_Trace_On(منطقي on, ثابت string& scope, صحيح سطوح) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << سطوح);
        m_entries.emplace_پشت(V3_Control_Scope_Trace_مدخل{scope, on, سطوح});
        m_تطابق_Cache.clear();
    }

    منطقي get_مدخل_تطابق(ثابت V3_Control_Scope_Trace_مدخل* entp, ثابت string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        ثابت V3_Control_Scope_Trace_مدخل_تطابق کلید{entp, scopepart};
        ثابت خودکار pair = m_تطابق_Cache.emplace(کلید, خطأ);
        إذا (pair.دومی) pair.اولی->دومی = VString::wildmatch(scopepart, entp->m_scope);
        أرجع pair.اولی->دومی;
    }

    منطقي getScopeTraceOn(ثابت string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        صحيح maxLevel = 1;
        لكل (ثابت خودکار& ch : scope) {
            إذا (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        منطقي enabled = درست;
        لكل (ثابت خودکار& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            صحيح partLevel = 1;
            لكل (string::اندازه_type part_انتها = 0; درست;) {
                part_انتها = scope.پیدا_کن('.', part_انتها + 1);
                إذا (part_انتها == string::npos) part_انتها = scope.درازی();
                ثابت std::string scopepart = scope.substr(0, part_انتها);
                إذا (get_مدخل_تطابق(&ent, scopepart)) {
                    ثابت منطقي level_تطابق
                        = !ent.m_سطوح || (ent.m_سطوح >= maxLevel - partLevel);
                    إذا (level_تطابق) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (level_تطابق ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    اكسر;
                }
                إذا (part_انتها == scope.درازی()) اكسر;
                ++partLevel;
            }
        }
        أرجع enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

صنف V3_Control_Resolver_Hier_کارگر_مدخل final {
    ثابت صحيح m_workers;  // Number of workers requested for the hierarchical block
    File_سطر* ثابت m_flp;  // Location the worker count was specified at

عام:
    صریح V3_Control_Resolver_Hier_کارگر_مدخل(صحيح workers, File_سطر* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    صحيح workers() ثابت { أرجع m_workers; }
    File_سطر* flp() ثابت { أرجع m_flp; }
};

صنف V3ControlResolver final {
    تعداد Profile_داده_نما : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profile_داده;  // Access to profile_data records
    uint8_t m_نما = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3_Control_Resolver_Hier_کارگر_مدخل> m_hierWorkers;
    File_سطر* m_profile_File_سطر = اشاره_تهی;  // Location profile_data was read from

    V3ControlResolver() = افتراضي;
    ~V3ControlResolver() = افتراضي;

عام:
    ساكن V3ControlResolver& s() {
        ساكن V3ControlResolver s_singleton;
        أرجع s_singleton;
    }
    V3ControlModuleResolver& modules() { أرجع m_modules; }
    V3ControlFileResolver& files() { أرجع m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { أرجع m_scopeTraces; }

    فراغ جمع_کن_Profile_داده(File_سطر* fl, ثابت string& hierDpi, uint64_t هزینه) {
        // Empty key for hierarchical DPI wrapper costs.
        جمع_کن_Profile_داده(fl, hierDpi, "", هزینه, HIER_DPI);
    }
    فراغ جمع_کن_Profile_داده(File_سطر* fl, ثابت string& model, ثابت string& کلید, uint64_t هزینه,
                        Profile_داده_نما نما = MTASK) {
        إذا (!m_profile_File_سطر) m_profile_File_سطر = fl;
        إذا (هزینه == 0) هزینه = 1;  // Cost 0 means delete (or no data)
        إذا (نما == MTASK) {
            m_profile_داده[model][کلید] += هزینه;
        } وإلا إذا (نما == HIER_DPI) {
            m_profile_داده[model][کلید] = std::max(m_profile_داده[model][کلید], هزینه);
        }
        m_نما |= نما;
    }
    منطقي شامل_است_MTask_Profile_داده() ثابت { أرجع m_نما & MTASK; }
    uint64_t get_Profile_داده(ثابت string& hierDpi) ثابت {
        // Empty key for hierarchical DPI wrapper costs.
        أرجع get_Profile_داده(hierDpi, "");
    }
    فراغ جمع_کن_Hier_Workers(File_سطر* flp, ثابت string& model, صحيح workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    صحيح getHierWorkers(ثابت string& model) ثابت {
        ثابت خودکار mit = m_hierWorkers.پیدا_کن(model);
        // Assign a single worker if no specified.
        أرجع mit != m_hierWorkers.cend() ? mit->دومی.workers() : 0;
    }
    File_سطر* get_Hier_Workers_File_سطر(ثابت string& model) ثابت {
        ثابت خودکار mit = m_hierWorkers.پیدا_کن(model);
        أرجع mit != m_hierWorkers.cend() ? mit->دومی.flp() : v3Global.rootp()->fileline();
    }
    uint64_t get_Profile_داده(ثابت string& model, ثابت string& کلید) ثابت {
        ثابت خودکار mit = m_profile_داده.پیدا_کن(model);
        إذا (mit == m_profile_داده.cend()) أرجع 0;
        ثابت خودکار it = mit->دومی.پیدا_کن(کلید);
        إذا (it == mit->دومی.cend()) أرجع 0;
        أرجع it->دومی;
    }
    File_سطر* get_Profile_داده_File_سطر() ثابت { أرجع m_profile_File_سطر; }  // Maybe null
    ساكن uint64_t get_Current_Hier_بلوک_هزینه() {
        إذا (uint64_t هزینه = V3Control::get_Profile_داده(v3Global.opt.پیشوند())) {
            UINFO(9, "Fetching cost from profile info: " << هزینه);
            أرجع هزینه;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* ثابت netlistp = v3Global.rootp();
        uint64_t هزینه = 0;
        لكل (صحيح i = 0; i < VEval::ENUM_انتها; ++i) {
            ثابت VEval eval{i};
            إذا (eval.slow()) استمر;
            هزینه += V3_Instr_شمار::شمار(netlistp->evalFuncp(eval), خطأ);
        }
        UINFO(9, "Evaluating cost: " << هزینه);
        أرجع هزینه;
    }
};

//######################################################################
// V3Control

فراغ V3Control::جمع_کن_Case_پر(ثابت string& filename, صحيح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.جمع_کن_سطر_Attribute(lineno, VPragmaType::پر_CASE);
}
فراغ V3Control::جمع_کن_Case_Parallel(ثابت string& filename, صحيح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.جمع_کن_سطر_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
فراغ V3Control::جمع_کن_Coverage_بلوک_Off(ثابت string& filename, صحيح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.جمع_کن_سطر_Attribute(lineno, VPragmaType::COVERAGE_بلوک_OFF);
}
فراغ V3Control::جمع_کن_Coverage_بلوک_Off(ثابت string& module, ثابت string& blockname) {
    V3ControlResolver::s().modules().at(module).جمع_کن_Coverage_بلوک_Off(blockname);
}

فراغ V3Control::جمع_کن_Hier_Workers(File_سطر* fl, ثابت string& model, صحيح workers) {
    V3ControlResolver::s().جمع_کن_Hier_Workers(fl, model, workers);
}

فراغ V3Control::جمع_کن_Fsm_ثبات_Wrapper(File_سطر* fl, ثابت string& module, ثابت string& d,
                                      ثابت string& q, ثابت string& clock, ثابت string& reset,
                                      ثابت string& reset_مقدار) {
    string missing;
    إذا (module.خالی()) missing += "-module";
    إذا (d.خالی()) missing = VString::dot(missing, ", ", "-d");
    إذا (q.خالی()) missing = VString::dot(missing, ", ", "-q");
    إذا (clock.خالی()) missing = VString::dot(missing, ", ", "-clock");
    إذا (!missing.خالی()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        أرجع;
    }

    ثابت Fsm_ثبات_Wrapper desc{module, d, q, clock, reset, reset_مقدار};
    V3ControlResolver::s().modules().at(module).مجموعه_Fsm_ثبات_Wrapper(fl, desc);
}

فراغ V3Control::جمع_کن_Ignore(V3_Error_کد کد, منطقي on, ثابت string& filename, صحيح min, صحيح max) {
    UINFO(9, "addIgnore " << کد << " " << min << "-" << max << " fn=" << filename);
    إذا (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_سطر::globalWarnOff(کد, !on);
    } وإلا {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).جمع_کن_Ignore(کد, min, max, on);
    }
}
فراغ V3Control::جمع_کن_Ignore_تطابق(V3_Error_کد کد, ثابت string& filename, ثابت string& contents,
                               ثابت string& تطابق) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).جمع_کن_Ignore_تطابق(کد, contents, تطابق);
}

فراغ V3Control::جمع_کن_Inline(File_سطر* fl, ثابت string& module, ثابت string& ftask, منطقي on) {
    إذا (ftask.خالی()) {
        V3ControlResolver::s().modules().at(module).مجموعه_Inline(on);
    } وإلا {
        إذا (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } وإلا {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).مجموعه_No_Inline(on);
        }
    }
}

فراغ V3Control::جمع_کن_Module_Pragma(ثابت string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).جمع_کن_Module_Pragma(pragma);
}

فراغ V3Control::جمع_کن_Profile_داده(File_سطر* fl, ثابت string& hierDpi, uint64_t هزینه) {
    V3ControlResolver::s().جمع_کن_Profile_داده(fl, hierDpi, هزینه);
}
فراغ V3Control::جمع_کن_Profile_داده(File_سطر* fl, ثابت string& model, ثابت string& کلید,
                               uint64_t هزینه) {
    V3ControlResolver::s().جمع_کن_Profile_داده(fl, model, کلید, هزینه);
}

فراغ V3Control::جمع_کن_Scope_Trace_On(منطقي on, ثابت string& scope, صحيح سطوح) {
    V3ControlResolver::s().scopeTraces().جمع_کن_Scope_Trace_On(on, scope, سطوح);
}

فراغ V3Control::جمع_کن_Var_Attr(File_سطر* fl, ثابت string& module, ثابت string& ftask,
                           Var_Spec_نوع نوع, ثابت string& الگو_2, VAttrType attr,
                           Ast_Sen_درخت* sensep) {
    إذا (sensep) {
        File_سطر* ثابت flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_درخت(), sensep);
        // Used to be only accepted on public_flat_rw
        إذا (attr != VAttrType::VAR_عمومی_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            أرجع;
        }
    }

    إذا (نوع != Var_Spec_نوع::VAR) {
        بدل (attr) {
        حالة VAttrType::VAR_عمومی_FLAT:
        حالة VAttrType::VAR_عمومی_FLAT_RD:
        حالة VAttrType::VAR_عمومی_FLAT_RW: اكسر;
        افتراضي:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            أرجع;
        }
    }

    // Semantics: Most of the attributes operate on signals
    إذا (الگو_2.خالی()) {
        إذا (attr == VAttrType::VAR_عمومی) {
            إذا (ftask.خالی()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).جمع_کن_Module_Pragma(
                    VPragmaType::عمومی_MODULE);
            } وإلا {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).مجموعه_عمومی(درست);
            }
        } وإلا {
            fl->v3error("missing -var");
        }
    } وإلا {
        إذا (attr == VAttrType::VAR_FORCEABLE) {
            إذا (module.خالی()) {
                fl->v3error("forceable missing -module");
            } وإلا إذا (!ftask.خالی()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } وإلا {
                V3ControlResolver::s().modules().at(module).vars().at(الگو_2).جمع_کن(attr);
            }
        } وإلا {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                إذا (ftask.خالی()) {
                    إذا (نوع == Var_Spec_نوع::PARAM) أرجع mod.params().at(الگو_2);
                    إذا (نوع == Var_Spec_نوع::پورت) أرجع mod.ports().at(الگو_2);
                    UASSERT_OBJ(نوع == Var_Spec_نوع::VAR, fl, "Unexpected VarSpecKind");
                    أرجع mod.vars().at(الگو_2);
                }
                إذا (نوع == Var_Spec_نوع::PARAM) أرجع mod.ftasks().at(ftask).params().at(الگو_2);
                إذا (نوع == Var_Spec_نوع::پورت) أرجع mod.ftasks().at(ftask).ports().at(الگو_2);
                UASSERT_OBJ(نوع == Var_Spec_نوع::VAR, fl, "Unexpected VarSpecKind");
                أرجع mod.ftasks().at(ftask).vars().at(الگو_2);
            }();
            controlVar.جمع_کن(attr);
        }
    }
}

فراغ V3Control::applyCase(AstCase* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->applyCase(nodep);
}

فراغ V3Control::apply_Coverage_بلوک(Ast_گره_Module* modulep, AstBegin* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->apply_بلوک(nodep);
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (modp) modp->apply_بلوک(nodep);
}
فراغ V3Control::apply_Coverage_بلوک(Ast_گره_Module* modulep, Ast_Gen_بلوک* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->apply_بلوک(nodep);
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (modp) modp->apply_بلوک(nodep);
}

فراغ V3Control::applyIgnores(File_سطر* filelinep) {
    ثابت string& filename = filelinep->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->applyIgnores(filelinep);
}

فراغ V3Control::applyModule(Ast_گره_Module* modulep) {
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (modp) modp->apply(modulep);
}

فراغ V3Control::applyFTask(Ast_گره_Module* modulep, Ast_گره_FTask* ftaskp) {
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (!modp) أرجع;
    ثابت V3ControlFTask* ثابت ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    إذا (ftp) ftp->apply(ftaskp);
}

قالب <نام_نوع T_Resolver>
ساكن فراغ resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    ثابت std::string name = varp->prettyDehashOrigOrName();
    إذا (ثابت V3ControlVar* ثابت vp = resolver.vars().resolve(name)) vp->apply(varp);
    إذا (varp->isParam()) {
        إذا (ثابت V3ControlVar* ثابت vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    إذا (varp->isIO()) {
        إذا (ثابت V3ControlVar* ثابت vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

فراغ V3Control::applyVarAttr(ثابت Ast_گره_Module* modulep, ثابت Ast_گره_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* ثابت modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    إذا (!modp) أرجع;
    إذا (ftaskp) {
        V3ControlFTask* ثابت ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        إذا (!ftp) أرجع;
        resolveThenApply(*ftp, varp);
    } وإلا {
        resolveThenApply(*modp, varp);
    }
}

صحيح V3Control::getHierWorkers(ثابت string& model) {
    أرجع V3ControlResolver::s().getHierWorkers(model);
}
File_سطر* V3Control::get_Hier_Workers_File_سطر(ثابت string& model) {
    أرجع V3ControlResolver::s().get_Hier_Workers_File_سطر(model);
}
ثابت V3Control::Fsm_ثبات_Wrapper* V3Control::get_Fsm_ثبات_Wrapper(ثابت string& module) {
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(module);
    أرجع modp ? modp->fsm_ثبات_Wrapperp() : اشاره_تهی;
}
uint64_t V3Control::get_Profile_داده(ثابت string& hierDpi) {
    أرجع V3ControlResolver::s().get_Profile_داده(hierDpi);
}
uint64_t V3Control::get_Profile_داده(ثابت string& model, ثابت string& کلید) {
    أرجع V3ControlResolver::s().get_Profile_داده(model, کلید);
}
File_سطر* V3Control::get_Profile_داده_File_سطر() {
    أرجع V3ControlResolver::s().get_Profile_داده_File_سطر();
}
منطقي V3Control::getScopeTraceOn(ثابت string& scope) {
    أرجع V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

فراغ V3Control::contents_افزودن_متن(ثابت string& متن) { WildcardContents::افزودن_متن(متن); }

منطقي V3Control::شامل_است_MTask_Profile_داده() {
    أرجع V3ControlResolver::s().شامل_است_MTask_Profile_داده();
}
uint64_t V3Control::get_Current_Hier_بلوک_هزینه() {
    أرجع V3ControlResolver::s().get_Current_Hier_بلوک_هزینه();
}

منطقي V3Control::waive(ثابت File_سطر* filelinep, V3_Error_کد کد, ثابت string& پیام) {
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    إذا (!filep) أرجع خطأ;
    أرجع filep->waive(کد, پیام);
}

فراغ V3Control::selfTest() { VInterval_درخت::selfTest(); }
