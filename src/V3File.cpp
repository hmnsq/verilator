// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: File stream wrapper that understands indentation
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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3File.h"

#അന്തര്ഭാവ "V3Os.h"
#അന്തര്ഭാവ "V3String.h"

#അന്തര്ഭാവ <cerrno>
#അന്തര്ഭാവ <cstdarg>
#അന്തര്ഭാവ <fcntl.h>
#അന്തര്ഭാവ <iomanip>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <memory>

#അന്തര്ഭാവ <sys/stat.h>
#അന്തര്ഭാവ <sys/types.h>

// clang-format off
#എങ്കിൽ defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#endif

#ifdef HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#അല്ലെങ്കിൽ
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#endif

#ifdef INFILTER_PIPE
# include <sys/wait.h>
#endif

#എങ്കിൽ defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr പൂർണ്ണസംഖ്യ INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr പൂർണ്ണസംഖ്യ INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_SIZE_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

വർഗ്ഗം V3FileDependImp final {
    // TYPES
    വർഗ്ഗം DependFile final {
        // A single file
        സ്ഥിരം തർക്കം m_target;  // True if write, else read
        തർക്കം m_exists = സത്യം;  // File exists
        സ്ഥിരം string m_filename;  // Filename
        ഘടന stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    പൊതു:
        DependFile(സ്ഥിരം string& filename, തർക്കം target)
            : m_target{target}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = സ്ഥിരസ്ഥിതി;
        സ്ഥിരം string& filename() സ്ഥിരം { തിരികെ m_filename; }
        തർക്കം target() സ്ഥിരം { തിരികെ m_target; }
        തർക്കം exists() സ്ഥിരം { തിരികെ m_exists; }
        off_t size() സ്ഥിരം { തിരികെ m_stat.st_size; }
        ino_t ino() സ്ഥിരം { തിരികെ m_stat.st_ino; }
        time_t cstime() സ്ഥിരം { തിരികെ m_stat.st_ctime; }  // Seconds
        time_t cnstime() സ്ഥിരം { തിരികെ VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        time_t mstime() സ്ഥിരം { തിരികെ m_stat.st_mtime; }  // Seconds
        time_t mnstime() സ്ഥിരം { തിരികെ VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_ചിഹ്നം() {
            സ്ഥിതം VHashSha512 s_emptyHash;
            തിരികെ m_hash.digestSymbol24() != s_emptyHash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        ശൂന്യം loadStats() {
            എങ്കിൽ (!m_stat.st_mtime) {
                സ്ഥിരം string fn = filename();
                സ്ഥിരം പൂർണ്ണസംഖ്യ err = stat(fn.c_str(), &m_stat);
                എങ്കിൽ (err != 0) {
                    std::memset(&m_stat, 0, വലുപ്പം(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = അസത്യം;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } അല്ലെങ്കിൽ {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    എങ്കിൽ (!target() && size() <= FILE_HASH_SIZE_MAX) m_hash.തിരുകുക_File(filename());
                }
            }
        }
        തർക്കം operator<(സ്ഥിരം DependFile& rhs) സ്ഥിരം { തിരികെ filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::set<string> m_filenameSet VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::set<DependFile> m_filenameList;  // Files sourced/generated

    സ്ഥിതം string stripQuotes(സ്ഥിരം string& in) {
        string pretty = in;
        string::size_type pos;
        ആകുമ്പോൾ ((pos = pretty.find('\"')) != string::npos) pretty.replace(pos, 1, "_");
        ആകുമ്പോൾ ((pos = pretty.find('\n')) != string::npos) pretty.replace(pos, 1, "_");
        തിരികെ pretty;
    }

പൊതു:
    // ACCESSOR METHODS
    ശൂന്യം addSrcDepend(സ്ഥിരം string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        സ്ഥിരം V3LockGuard lock{m_mutex};
        സ്ഥിരം auto itFoundPair = m_filenameSet.തിരുകുക(filename);
        എങ്കിൽ (itFoundPair.second) {
            DependFile df{filename, അസത്യം};
            df.loadStats();  // Get size now, in case changes during the run
            m_filenameList.തിരുകുക(df);
        }
    }
    ശൂന്യം addTgtDepend(സ്ഥിരം string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        സ്ഥിരം V3LockGuard lock{m_mutex};
        സ്ഥിരം auto itFoundPair = m_filenameSet.തിരുകുക(filename);
        എങ്കിൽ (itFoundPair.second) m_filenameList.തിരുകുക(DependFile{filename, സത്യം});
    }
    ശൂന്യം writeDepend(സ്ഥിരം string& filename);
    std::vector<string> getAllDeps() സ്ഥിരം;
    ശൂന്യം writeTimes(സ്ഥിരം string& filename, സ്ഥിരം string& cmdlineIn);
    തർക്കം checkTimes(സ്ഥിരം string& filename, സ്ഥിരം string& cmdlineIn);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

ശൂന്യം V3FileDependImp::writeDepend(സ്ഥിരം string& filename) {
    സ്ഥിരം std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    എങ്കിൽ (ofp->fail()) v3fatal("Can't write file: " << filename);

    വേണ്ടി (സ്ഥിരം DependFile& i : m_filenameList) {
        എങ്കിൽ (i.target()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    വേണ്ടി (സ്ഥിരം DependFile& i : m_filenameList) {
        എങ്കിൽ (!i.target()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    എങ്കിൽ (v3Global.opt.makePhony()) {
        *ofp << '\n';
        വേണ്ടി (സ്ഥിരം DependFile& i : m_filenameList) {
            എങ്കിൽ (!i.target()) *ofp << i.filename() << ":\n";
        }
    }
}

std::vector<string> V3FileDependImp::getAllDeps() സ്ഥിരം {
    std::vector<string> r;
    വേണ്ടി (സ്ഥിരം auto& itr : m_filenameList) {
        എങ്കിൽ (!itr.target() && itr.exists()) r.തള്ളുക_back(itr.filename());
    }
    തിരികെ r;
}

ശൂന്യം V3FileDependImp::writeTimes(സ്ഥിരം string& filename, സ്ഥിരം string& cmdlineIn) {
    സ്ഥിരം std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    എങ്കിൽ (ofp->fail()) v3fatal("Can't write file: " << filename);

    സ്ഥിരം string cmdline = stripQuotes(cmdlineIn);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    വേണ്ടി (auto& it : m_filenameList) {
        DependFile* സ്ഥിരം depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->loadStats();
        off_t showSize = depfilep->size();
        ino_t showIno = depfilep->ino();
        എങ്കിൽ (depfilep->filename() == filename) {
            showSize = 0;
            showIno = 0;  // We're writing it, so need to ignore it
        }

        *ofp << (depfilep->target() ? "T" : "S") << " ";
        *ofp << " " << std::setw(8) << showSize;
        *ofp << " " << std::setw(8) << showIno;
        *ofp << " " << std::setw(11) << depfilep->cstime();
        *ofp << " " << std::setw(11) << depfilep->cnstime();
        *ofp << " " << std::setw(11) << depfilep->mstime();
        *ofp << " " << std::setw(11) << depfilep->mnstime();
        *ofp << " \"" << depfilep->hash_Digest_ചിഹ്നം() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

തർക്കം V3FileDependImp::checkTimes(സ്ഥിരം string& filename, സ്ഥിരം string& cmdlineIn) {
    സ്ഥിരം std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    എങ്കിൽ (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        തിരികെ അസത്യം;
    }
    {
        സ്ഥിരം string ignore = V3Os::getline(*ifp);
        എങ്കിൽ (ignore.empty()) { /*used*/
        }
    }
    {
        അക്ഷരം chkDir;
        *ifp >> chkDir;
        അക്ഷരം quote;
        *ifp >> quote;
        സ്ഥിരം string chkCmdline = V3Os::getline(*ifp, '"');
        സ്ഥിരം string cmdline = stripQuotes(cmdlineIn);
        എങ്കിൽ (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            തിരികെ അസത്യം;
        }
    }

    സ്ഥിരം തർക്കം skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").empty();

    ആകുമ്പോൾ (!ifp->eof()) {
        അക്ഷരം chkDir;
        *ifp >> chkDir;
        off_t chkSize;
        *ifp >> chkSize;
        ino_t chkIno;
        *ifp >> chkIno;
        എങ്കിൽ (ifp->eof()) നിർത്തുക;  // Needed to read final whitespace before found eof
        time_t chkCstime;
        *ifp >> chkCstime;
        time_t chkCnstime;
        *ifp >> chkCnstime;
        time_t chkMstime;
        *ifp >> chkMstime;
        time_t chkMnstime;
        *ifp >> chkMnstime;
        അക്ഷരം quote;
        *ifp >> quote;
        സ്ഥിരം string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        സ്ഥിരം string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        ഘടന stat curStat;
        സ്ഥിരം പൂർണ്ണസംഖ്യ err = stat(chkFilename.c_str(), &curStat);
        എങ്കിൽ (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            തിരികെ അസത്യം;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        എങ്കിൽ (filename == chkFilename) തുടരുക;

        UINFO(9, " got d=" << chkDir << " s=" << chkSize << " ct=" << chkCstime << "."
                           << chkCnstime << " mt=" << chkMstime << "." << chkMnstime
                           << " h=" << chkHash << " fn= " << chkFilename);
        UINFO(9, " nowSt  s=" << curStat.st_size << " mt=" << curStat.st_mtime
                              << " ct=" << curStat.st_ctime);

        // We'd like this rule:
        // if (!(curStat.st_size == chkSize
        //      && curStat.st_mtime == chkMstime) {
        // However NFS messes us up, as there might be some data outstanding when
        // we determined the original size.  For safety, we know the creation time
        // must be within a few second window... call it 20 sec.
        എങ്കിൽ (!(curStat.st_size >= chkSize && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_size << "=?" << chkSize << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            എങ്കിൽ (skipHashing) തിരികെ അസത്യം;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            എങ്കിൽ (chkHash == "unhashed") തിരികെ അസത്യം;

            VHashSha512 curHash;
            curHash.തിരുകുക_File(chkFilename);
            എങ്കിൽ (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                തിരികെ അസത്യം;
            }
        }
    }
    തിരികെ സത്യം;
}

//######################################################################
// V3File

ശൂന്യം V3File::addSrcDepend(സ്ഥിരം string& filename) VL_MT_SAFE { dependImp.addSrcDepend(filename); }
ശൂന്യം V3File::addTgtDepend(സ്ഥിരം string& filename) VL_MT_SAFE { dependImp.addTgtDepend(filename); }
ശൂന്യം V3File::writeDepend(സ്ഥിരം string& filename) { dependImp.writeDepend(filename); }
std::vector<string> V3File::getAllDeps() { തിരികെ dependImp.getAllDeps(); }
ശൂന്യം V3File::writeTimes(സ്ഥിരം string& filename, സ്ഥിരം string& cmdlineIn) {
    dependImp.writeTimes(filename, cmdlineIn);
}
തർക്കം V3File::checkTimes(സ്ഥിരം string& filename, സ്ഥിരം string& cmdlineIn) {
    തിരികെ dependImp.checkTimes(filename, cmdlineIn);
}
ശൂന്യം V3File::createMakeDirFor(സ്ഥിരം string& filename) {
    എങ്കിൽ (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().length() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
ശൂന്യം V3File::createMakeDir() {
    സ്ഥിതം തർക്കം s_created = അസത്യം;
    എങ്കിൽ (!s_created) {
        s_created = സത്യം;
        V3Os::createDir(v3Global.opt.makeDir());
        എങ്കിൽ (v3Global.opt.hier_മുകൾ()) V3Os::createDir(v3Global.opt.hier_മുകൾ_Data_Dir());
    }
}

//######################################################################
// VInFilterImp

വർഗ്ഗം VInFilterImp final {
    using StrList = VInFilter::StrList;

    std::map<സ്ഥിരം std::string, std::string> m_contentsMap;  // Cache of file contents
    തർക്കം m_readEof = അസത്യം;  // Received EOF on read
#ifdef INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#അല്ലെങ്കിൽ
    പൂർണ്ണസംഖ്യ m_pid = 0;  // fork() process id - always zero as disabled
#endif
    തർക്കം m_pidExited = അസത്യം;  //  waitpid() reaped the filter process
    പൂർണ്ണസംഖ്യ m_pidStatus = 0;  // Exit status of the filter process
    പൂർണ്ണസംഖ്യ m_writeFd = 0;  // File descriptor TO filter
    പൂർണ്ണസംഖ്യ m_readFd = 0;  // File descriptor FROM filter

സ്വകാര്യ:
    // METHODS

    തർക്കം readContents(സ്ഥിരം string& filename, StrList& outl) {
        എങ്കിൽ (m_pid) {
            തിരികെ readContentsFilter(filename, outl);
        } അല്ലെങ്കിൽ {
            തിരികെ readContentsFile(filename, outl);
        }
    }
    തർക്കം readContentsFile(സ്ഥിരം string& filename, StrList& outl) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ fd = open(filename.c_str(), O_RDONLY);
        എങ്കിൽ (fd < 0) തിരികെ അസത്യം;
        m_readEof = അസത്യം;
        readBlocks(fd, -1, outl);
        close(fd);
        തിരികെ സത്യം;
    }
    // cppcheck-suppress constParameterReference
    തർക്കം readContentsFilter(സ്ഥിരം string& filename, StrList& outl) {
        (ശൂന്യം)filename;  // Prevent unused variable warning
        (ശൂന്യം)outl;  // Prevent unused variable warning
#ifdef INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        സ്ഥിരം string വരി = read_Filter_വരി();
        എങ്കിൽ (വരി.find("Content-Length") != string::npos) {
            പൂർണ്ണസംഖ്യ len = 0;
            sscanf(വരി.c_str(), "Content-Length: %d\n", &len);
            readBlocks(m_readFd, len, outl);
            തിരികെ സത്യം;
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (വരി != "") v3error("--pipe-filter protocol error, unexpected: " << വരി);
            തിരികെ അസത്യം;
        }
#അല്ലെങ്കിൽ
        v3fatalSrc("--pipe-filter not implemented on this platform");
        തിരികെ അസത്യം;
#endif
    }

    // cppcheck-suppress functionConst
    ശൂന്യം checkFilter(തർക്കം hang) {
#ifdef INFILTER_PIPE
        എങ്കിൽ (!m_pidExited && waitpid(m_pid, &m_pidStatus, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pidStatus << " exit=" << WEXITSTATUS(m_pidStatus) << " err"
                         << std::strerror(errno));
            m_readEof = സത്യം;
            m_pidExited = സത്യം;
        }
#endif
    }

    ശൂന്യം readBlocks(പൂർണ്ണസംഖ്യ fd, പൂർണ്ണസംഖ്യ size, StrList& outl) {
        അക്ഷരം buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        ആകുമ്പോൾ (!m_readEof && (size < 0 || size > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            എങ്കിൽ (size > 0 && size < todo) todo = size;
            errno = 0;
            സ്ഥിരം ssize_t got = പഠ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            എങ്കിൽ (got > 0) {
                outl.emplace_back(string(buf, got));
                sizegot += got;
            } അല്ലെങ്കിൽ എങ്കിൽ (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(അസത്യം);
                V3Os::u_sleep(1000);
                തുടരുക;
            } അല്ലെങ്കിൽ {
                m_readEof = സത്യം;
                നിർത്തുക;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_വരി() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string വരി;
        ആകുമ്പോൾ (!m_readEof) {
            StrList outl;
            readBlocks(m_readFd, 1, outl);
            സ്ഥിരം string onechar = listString(outl);
            വരി += onechar;
            എങ്കിൽ (onechar == "\n") {
                എങ്കിൽ (വരി == "\n") {
                    വരി = "";
                    തുടരുക;
                } അല്ലെങ്കിൽ {
                    നിർത്തുക;
                }
            }
        }
        UINFO(6, "filter-line-in: " << വരി);
        തിരികെ വരി;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    ശൂന്യം writeFilter(സ്ഥിരം string& out) {
        എങ്കിൽ (debug() >= 6) {
            UINFO(6, "filter-out: " << out);
            എങ്കിൽ (out[out.length() - 1] != '\n') cout << '\n';
        }
        എങ്കിൽ (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = സത്യം;
            stop();
        }
        unsigned offset = 0;
        ആകുമ്പോൾ (!m_readEof && out.length() > offset) {
            errno = 0;
            സ്ഥിരം പൂർണ്ണസംഖ്യ got = ലിഖ(m_writeFd, (out.c_str()) + offset, out.length() - offset);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            എങ്കിൽ (got > 0) {
                offset += got;
            } അല്ലെങ്കിൽ എങ്കിൽ (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(അസത്യം);
                V3Os::u_sleep(1000);
                തുടരുക;
            } അല്ലെങ്കിൽ {
                നിർത്തുക;
            }
        }
    }

    // Start the filter
    ശൂന്യം start(സ്ഥിരം string& command) {
        എങ്കിൽ (command == "") {
            m_pid = 0;  // Disabled
        } അല്ലെങ്കിൽ {
            startFilter(command);
        }
    }
    ശൂന്യം startFilter(സ്ഥിരം string& command) {
        (ശൂന്യം)command;  // Prevent Unused variable warning
#ifdef INFILTER_PIPE
        പൂർണ്ണസംഖ്യ fd_stdin[2];  // Can't use std::array
        പൂർണ്ണസംഖ്യ fd_stdout[2];  // Can't use std::array
        constexpr പൂർണ്ണസംഖ്യ P_RD = 0;
        constexpr പൂർണ്ണസംഖ്യ P_WR = 1;

        എങ്കിൽ (VL_UNCOVERABLE(പ്രണാല(fd_stdin) != 0 || പ്രണാല(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        എങ്കിൽ (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        സ്ഥിരം pid_t pid = fork();
        എങ്കിൽ (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        എങ്കിൽ (pid == 0) {  // Child
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<അക്ഷരം*>(nullptr));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } അല്ലെങ്കിൽ {  // Parent
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = അസത്യം;
            m_pidStatus = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = അസത്യം;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            പൂർണ്ണസംഖ്യ flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#അല്ലെങ്കിൽ
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

    ശൂന്യം stop() {
        എങ്കിൽ (m_pid) stopFilter();
    }
    ശൂന്യം stopFilter() {
        UINFO(6, "Stopping filter process");
#ifdef INFILTER_PIPE
        close(m_writeFd);
        checkFilter(സത്യം);
        എങ്കിൽ (!WIFEXITED(m_pidStatus) || WEXITSTATUS(m_pidStatus) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#അല്ലെങ്കിൽ
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

സംരക്ഷിത:
    friend വർഗ്ഗം VInFilter;
    // Read file contents and return it
    തർക്കം readWholefile(സ്ഥിരം string& filename, StrList& outl) {
        സ്ഥിരം auto it = m_contentsMap.find(filename);
        എങ്കിൽ (it != m_contentsMap.end()) {
            outl.തള്ളുക_back(it->second);
            തിരികെ സത്യം;
        }
        എങ്കിൽ (!readContents(filename, outl)) തിരികെ അസത്യം;
        എങ്കിൽ (listSize(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        തിരികെ സത്യം;
    }
    സ്ഥിതം size_t listSize(സ്ഥിരം StrList& sl) {
        size_t result = 0;
        വേണ്ടി (സ്ഥിരം string& i : sl) result += i.length();
        തിരികെ result;
    }
    സ്ഥിതം string listString(സ്ഥിരം StrList& sl) {
        string result;
        വേണ്ടി (സ്ഥിരം string& i : sl) result += i;
        തിരികെ result;
    }
    // CONSTRUCTORS
    explicit VInFilterImp(സ്ഥിരം string& command) { start(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(സ്ഥിരം string& command) { m_impp = പുതിയ VInFilterImp{command}; }
VInFilter::~VInFilter() {
    എങ്കിൽ (m_impp) VL_DO_CLEAR(delete m_impp, m_impp = nullptr);
}

തർക്കം VInFilter::readWholefile(സ്ഥിരം string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    തിരികെ m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3OutFormatter::V3OutFormatter(V3OutFormatter::Language lang)
    : m_lang{lang} {
    m_blockIndent = v3Global.opt.decoration() ? 4 : 1;
    m_commaWidth = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
സ്ഥിതം സ്ഥിരം std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> table;
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i <= MAXSPACE; ++i) table[i] = std::string(static_cast<size_t>(i), ' ');
    തിരികെ table;
}();

സ്ഥിരം std::string& V3OutFormatter::indentSpaces(പൂർണ്ണസംഖ്യ അക്കം) {
    // Indent the specified number of spaces.
    തിരികെ s_indentSpaces[std::max(0, std::min(അക്കം, MAXSPACE))];
}

തർക്കം V3OutFormatter::tokenMatch(സ്ഥിരം അക്ഷരം* cp, സ്ഥിരം അക്ഷരം* cmp) {
    ആകുമ്പോൾ (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    എങ്കിൽ (*cmp) തിരികെ അസത്യം;
    എങ്കിൽ (*cp && !std::isspace(*cp)) തിരികെ അസത്യം;
    തിരികെ സത്യം;
}

തർക്കം V3OutFormatter::tokenStart(സ്ഥിരം അക്ഷരം* cp) {
    തിരികെ (tokenMatch(cp, "begin") || tokenMatch(cp, "case") || tokenMatch(cp, "casex")
            || tokenMatch(cp, "casez") || tokenMatch(cp, "class") || tokenMatch(cp, "function")
            || tokenMatch(cp, "interface") || tokenMatch(cp, "module") || tokenMatch(cp, "package")
            || tokenMatch(cp, "task"));
}

തർക്കം V3OutFormatter::tokenEnd(സ്ഥിരം അക്ഷരം* cp) {
    തിരികെ (tokenMatch(cp, "end") || tokenMatch(cp, "endcase") || tokenMatch(cp, "endclass")
            || tokenMatch(cp, "endfunction") || tokenMatch(cp, "endinterface")
            || tokenMatch(cp, "endmodule") || tokenMatch(cp, "endpackage")
            || tokenMatch(cp, "endtask"));
}

തർക്കം V3OutFormatter::tokenNotStart(സ്ഥിരം അക്ഷരം* cp) {
    തിരികെ (tokenMatch(cp, "export") || tokenMatch(cp, "import"));
}

പൂർണ്ണസംഖ്യ V3OutFormatter::endLevels(സ്ഥിരം അക്ഷരം* strg) {
    പൂർണ്ണസംഖ്യ levels = m_indentLevel;
    {
        സ്ഥിരം അക്ഷരം* cp = strg;
        ആകുമ്പോൾ (std::isspace(*cp)) ++cp;
        തിരഞ്ഞെടുപ്പ് (*cp) {
        അവസ്ഥ '\n':  // Newlines.. No need for whitespace before it
            തിരികെ 0;
        അവസ്ഥ '#':  // Preproc directive
            എങ്കിൽ (m_lang == LA_C) തിരികെ 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            സ്ഥിരം അക്ഷരം* mp = cp;
            വേണ്ടി (; std::isalnum(*mp); ++mp) {}
            എങ്കിൽ (mp[0] == ':' && mp[1] != ':') തിരികെ (levels - m_blockIndent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    വേണ്ടി (സ്ഥിരം അക്ഷരം* cp = strg; *cp; ++cp) {
        തിരഞ്ഞെടുപ്പ് (*cp) {
        അവസ്ഥ '}':
        അവസ്ഥ ')': levels -= m_blockIndent; നിർത്തുക;
        അവസ്ഥ '<':
            എങ്കിൽ (m_lang == LA_XML) {
                എങ്കിൽ (cp[1] == '/') levels -= m_blockIndent;
            }
            നിർത്തുക;
        അവസ്ഥ 'e':
            എങ്കിൽ (m_lang == LA_VERILOG && tokenEnd(cp)) levels -= m_blockIndent;
            നിർത്തുക;
        അവസ്ഥ '\t':
        അവസ്ഥ ' ': നിർത്തുക;  // Continue
        സ്ഥിരസ്ഥിതി: തിരികെ levels;  // Letter
        }
    }
    തിരികെ levels;
}

ശൂന്യം V3OutFormatter::putns(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം അക്ഷരം* strg) {
    എങ്കിൽ (!v3Global.opt.decoration()) {
        putsOutput(strg);
        തിരികെ;
    }

    സ്ഥിരം തർക്കം put_കണ്ണി_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_sourceLastFilenameno != nodep->fileline()->filenameno()  //
            || m_sourceLastLineno != nodep->fileline()->firstLineno())  //
        && File_വരി::builtInFilename() != nodep->fileline()->filename();

    എങ്കിൽ (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_കണ്ണി_Decoration)) {
        putsNoTracking(indentSpaces(endLevels(strg)));
        m_prependIndent = അസത്യം;
    }

    എങ്കിൽ (put_കണ്ണി_Decoration) {
        സ്ഥിരം File_വരി* സ്ഥിരം flp = nodep->fileline();
        m_sourceLastLineno = flp->firstLineno();
        m_sourceLastFilenameno = flp->filenameno();
        സ്ഥിരം std::string lineno = std::to_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvtToHex(nodep) + "*/");
    }

    തർക്കം notstart = അസത്യം;
    തർക്കം wordstart = സത്യം;
    തർക്കം equalsForBracket = അസത്യം;  // Looking for "= {"
    വേണ്ടി (സ്ഥിരം അക്ഷരം* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        എങ്കിൽ (std::isalpha(*cp)) {
            എങ്കിൽ (wordstart && m_lang == LA_VERILOG && tokenNotStart(cp)) notstart = സത്യം;
            എങ്കിൽ (wordstart && m_lang == LA_VERILOG && !notstart && tokenStart(cp)) indentInc();
            എങ്കിൽ (wordstart && m_lang == LA_VERILOG && tokenEnd(cp)) indentDec();
        }
        സ്ഥിരം തർക്കം wasInBackslash = m_inBackslash;
        m_inBackslash = അസത്യം;
        തിരഞ്ഞെടുപ്പ് (*cp) {
        അവസ്ഥ '\n':
            ++m_lineno;
            wordstart = സത്യം;
            എങ്കിൽ (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = സത്യം;
            } അല്ലെങ്കിൽ {
                m_prependIndent = അസത്യം;
                putsNoTracking(indentSpaces(endLevels(cp + 1)));
            }
            നിർത്തുക;
        അവസ്ഥ ' ': wordstart = സത്യം; നിർത്തുക;
        അവസ്ഥ '\t': wordstart = സത്യം; നിർത്തുക;
        അവസ്ഥ '"':
            wordstart = അസത്യം;
            എങ്കിൽ (!wasInBackslash) m_inStringLiteral = !m_inStringLiteral;
            നിർത്തുക;
        അവസ്ഥ '\\':
            wordstart = അസത്യം;
            എങ്കിൽ (!wasInBackslash) m_inBackslash = സത്യം;
            നിർത്തുക;
        അവസ്ഥ '/':
            എങ്കിൽ (m_lang == LA_C || m_lang == LA_VERILOG) {
                എങ്കിൽ (cp > strg && cp[-1] == '/' && !m_inStringLiteral) {
                    // Output ignoring contents to EOL
                    ++cp;
                    ആകുമ്പോൾ (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    എങ്കിൽ (*cp) putcNoTracking(*cp);
                }
            }
            നിർത്തുക;
        അവസ്ഥ '{':
            എങ്കിൽ (!m_inStringLiteral) {
                എങ്കിൽ (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.തള്ളുക(m_indentLevel
                                    * m_blockIndent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            നിർത്തുക;
        അവസ്ഥ '}':
            എങ്കിൽ (!m_inStringLiteral) {
                എങ്കിൽ (m_bracketLevel > 0) {
                    m_parenVec.എടുക്കുക();
                    --m_bracketLevel;
                }
                indentDec();
            }
            നിർത്തുക;
        അവസ്ഥ '(':
            എങ്കിൽ (!m_inStringLiteral) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.തള്ളുക(m_column);
            }
            നിർത്തുക;
        അവസ്ഥ ')':
            എങ്കിൽ (!m_inStringLiteral) {
                എങ്കിൽ (!m_parenVec.empty()) m_parenVec.എടുക്കുക();
                indentDec();
            }
            നിർത്തുക;
        അവസ്ഥ '<':
            എങ്കിൽ (m_lang == LA_XML) {
                എങ്കിൽ (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } അല്ലെങ്കിൽ എങ്കിൽ (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } അല്ലെങ്കിൽ {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            നിർത്തുക;
        അവസ്ഥ '>':
            എങ്കിൽ (m_lang == LA_XML) {
                indentDec();
                എങ്കിൽ (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            നിർത്തുക;
        സ്ഥിരസ്ഥിതി: wordstart = അസത്യം; നിർത്തുക;
        }

        തിരഞ്ഞെടുപ്പ് (*cp) {
        അവസ്ഥ '=': equalsForBracket = സത്യം; നിർത്തുക;
        അവസ്ഥ ' ': നിർത്തുക;
        സ്ഥിരസ്ഥിതി: equalsForBracket = അസത്യം; നിർത്തുക;
        }
    }
}

ശൂന്യം V3OutFormatter::put_Break_വ്യഞ്ജകം() {
    എങ്കിൽ (!m_parenVec.empty()) putBreak();
}

// Add a line break if too wide
ശൂന്യം V3OutFormatter::putBreak() {
    എങ്കിൽ (!v3Global.opt.decoration()) തിരികെ;
    എങ്കിൽ (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        എങ്കിൽ (exceededWidth()) {
            putcNoTracking('\n');
            എങ്കിൽ (!m_parenVec.empty()) putsNoTracking(indentSpaces(m_parenVec.മുകൾ()));
        }
    }
}

ശൂന്യം V3OutFormatter::putsQuoted(സ്ഥിരം string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    സ്ഥിരം string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    വേണ്ടി (സ്ഥിരം അക്ഷരം c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    എങ്കിൽ (strg.find('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
ശൂന്യം V3OutFormatter::putsNoTracking(സ്ഥിരം string& strg) {
    എങ്കിൽ (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        തിരികെ;
    }
    // Don't track {}'s, probably because it's a $display format string
    വേണ്ടി (സ്ഥിരം അക്ഷരം c : strg) putcNoTracking(c);
}

ശൂന്യം V3OutFormatter::putcNoTracking(അക്ഷരം chr) {
    എങ്കിൽ (!v3Global.opt.decoration()) {
        putcOutput(chr);
        തിരികെ;
    }
    തിരഞ്ഞെടുപ്പ് (chr) {
    അവസ്ഥ '\n':
        ++m_lineno;
        m_column = 0;
        m_nobreak = സത്യം;
        നിർത്തുക;
    അവസ്ഥ '\t': m_column = ((m_column + 9) / 8) * 8; നിർത്തുക;
    അവസ്ഥ ' ':
    അവസ്ഥ '(':
    അവസ്ഥ '|':
    അവസ്ഥ '&': ++m_column; നിർത്തുക;
    സ്ഥിരസ്ഥിതി:
        ++m_column;
        m_nobreak = അസത്യം;
        നിർത്തുക;
    }
    putcOutput(chr);
}

string V3OutFormatter::quoteNameControls(സ്ഥിരം string& namein,
                                         V3OutFormatter::Language lang) VL_PURE {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string out;
    എങ്കിൽ (lang == LA_XML) {
        // Encode chars into XML string
        വേണ്ടി (സ്ഥിരം അക്ഷരം c : namein) {
            എങ്കിൽ (c == '"') {
                out += "&quot;"s;
            } അല്ലെങ്കിൽ എങ്കിൽ (c == '\'') {
                out += "&apos;"s;
            } അല്ലെങ്കിൽ എങ്കിൽ (c == '<') {
                out += "&lt;"s;
            } അല്ലെങ്കിൽ എങ്കിൽ (c == '>') {
                out += "&gt;"s;
            } അല്ലെങ്കിൽ എങ്കിൽ (c == '&') {
                out += "&amp;"s;
            } അല്ലെങ്കിൽ എങ്കിൽ (std::isprint(c)) {
                out += c;
            } അല്ലെങ്കിൽ {
                out += "&#"s + cvtToStr(static_cast<unsigned പൂർണ്ണസംഖ്യ>(c & 0xff)) + ";";
            }
        }
    } അല്ലെങ്കിൽ {
        // Encode control chars into C style escapes
        വേണ്ടി (സ്ഥിരം അക്ഷരം c : namein) {
            എങ്കിൽ (c == '\\' || c == '"') {
                out += "\\"s + c;
            } അല്ലെങ്കിൽ എങ്കിൽ (c == '\n') {
                out += "\\n";
            } അല്ലെങ്കിൽ എങ്കിൽ (c == '\r') {
                out += "\\r";
            } അല്ലെങ്കിൽ എങ്കിൽ (c == '\t') {
                out += "\\t";
            } അല്ലെങ്കിൽ എങ്കിൽ (std::isprint(c)) {
                out += c;
            } അല്ലെങ്കിൽ {
                // This will also cover \a etc
                സ്ഥിരം string octal
                    = "\\"s + cvtToStr((c >> 6) & 3) + cvtToStr((c >> 3) & 7) + cvtToStr(c & 7);
                out += octal;
            }
        }
    }
    തിരികെ out;
}

//----------------------------------------------------------------------
// Simple wrappers

ശൂന്യം V3OutFormatter::printf(സ്ഥിരം അക്ഷരം* fmt...) {
    constexpr size_t bufsize = 5000;
    അക്ഷരം sbuff[bufsize];
    va_list ap;
    va_start(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_end(ap);
    ഇത്->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3OutFile::V3OutFile(സ്ഥിരം string& filename, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_filename{filename}
    , m_bufferp{പുതിയ std::array<അക്ഷരം, WRITE_BUFFER_SIZE_BYTES>{}} {
    എങ്കിൽ ((m_fp = V3File::new_fopen_w(filename)) == nullptr) {
        v3fatal("Can't write file: " << filename);
    }
}

V3OutFile::~V3OutFile() {
    writeBlock();

    എങ്കിൽ (m_fp) fclose(m_fp);
    m_fp = nullptr;
}

ശൂന്യം V3OutFile::putsForceIncs() {
    സ്ഥിരം VStringList& forceIncs = v3Global.opt.forceIncs();
    വേണ്ടി (സ്ഥിരം string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

ശൂന്യം V3OutCFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = സത്യം;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    വേണ്ടി (അക്ഷരം& c : var) {
        എങ്കിൽ (!std::isalnum(c)) c = '_';
    }
    puts("\n#ifndef " + var + "\n");
    puts("#define " + var + "  // guard\n");
}

//######################################################################
// V3OutStream

V3OutStream::V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_ostream{ostream} {}

//######################################################################
// VIdProtect

വർഗ്ഗം VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<സ്ഥിരം std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_set<std::string> m_newIdSet VL_GUARDED_BY(m_mutex);  // Which new names exist
സംരക്ഷിത:
    // CONSTRUCTORS
    friend വർഗ്ഗം VIdProtect;
    സ്ഥിതം VIdProtectImp& singleton() VL_MT_SAFE {
        സ്ഥിതം VIdProtectImp s;
        തിരികെ s;
    }

പൊതു:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = സ്ഥിരസ്ഥിതി;
    // METHODS
    string passthru(സ്ഥിരം string& old) VL_MT_SAFE_EXCLUDES(m_mutex) {
        എങ്കിൽ (!v3Global.opt.protectIds()) തിരികെ old;
        സ്ഥിരം V3LockGuard lock{m_mutex};
        സ്ഥിരം auto it = m_nameMap.find(old);
        എങ്കിൽ (it != m_nameMap.end()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->second,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } അല്ലെങ്കിൽ {
            m_nameMap.emplace(old, old);
            m_newIdSet.തിരുകുക(old);
        }
        തിരികെ old;
    }
    string protectIf(സ്ഥിരം string& old, തർക്കം doIt) VL_MT_SAFE_EXCLUDES(m_mutex) {
        എങ്കിൽ (!v3Global.opt.protectIds() || old.empty() || !doIt) തിരികെ old;
        സ്ഥിരം V3LockGuard lock{m_mutex};
        സ്ഥിരം auto pair = m_nameMap.emplace(old, "");
        എങ്കിൽ (pair.second) {
            string out;
            എങ്കിൽ (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                out = "PS" + old;
            } അല്ലെങ്കിൽ {
                VHashSha512 digest{v3Global.opt.protect_താക്കോൽ_Defaulted()};
                digest.തിരുകുക(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                out = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                വേണ്ടി (size_t len = 6; len < out.size() - 3; len += 3) {
                    സ്ഥിരം string tryout = out.substr(0, len);
                    എങ്കിൽ (m_newIdSet.തിരുകുക(tryout).second) {
                        out = tryout;
                        നിർത്തുക;
                    }
                }
            }
            pair.first->second = out;
        }
        തിരികെ pair.first->second;
    }
    string protect_വാക്കുകൾ_If(സ്ഥിരം string& old, തർക്കം doIt) VL_MT_SAFE {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        എങ്കിൽ (!(doIt && v3Global.opt.protectIds())) തിരികെ old;
        string out;
        string::size_type start = 0;
        // space, ., ->
        ആകുമ്പോൾ (സത്യം) {
            // When C++11, use find_if and lambda
            string::size_type pos = string::npos;
            string separator;
            trySep(old, start, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ")", pos /*ref*/, separator /*ref*/);
            എങ്കിൽ (pos == string::npos) നിർത്തുക;
            out += protectIf(old.substr(start, pos - start), സത്യം) + separator;
            start = pos + separator.length();
        }
        out += protectIf(old.substr(start), സത്യം);
        തിരികെ out;
    }
    ശൂന്യം writeMapFile(സ്ഥിരം string& filename) സ്ഥിരം {
        V3OutXmlFile of{filename};
        of.putsHeader();
        of.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        of.puts("<verilator_id_map>\n");
        {
            വേണ്ടി (സ്ഥിരം auto& itr : m_nameMap) {
                of.puts("<map from=\"" + itr.second + "\" to=\"" + itr.first + "\"/>\n");
            }
        }
        of.puts("</verilator_id_map>\n");
    }

സ്വകാര്യ:
    ശൂന്യം trySep(സ്ഥിരം string& old, string::size_type start, സ്ഥിരം string& trySep,
                string::size_type& posr, string& separatorr) VL_PURE {
        സ്ഥിരം string::size_type trypos = old.find(trySep, start);
        എങ്കിൽ (trypos != string::npos) {
            എങ്കിൽ (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(സ്ഥിരം string& in) VL_MT_SAFE {
    തിരികെ v3Global.opt.protectIds() ? "" : in;
}
string VIdProtect::protectIf(സ്ഥിരം string& old, തർക്കം doIt) VL_MT_SAFE {
    തിരികെ VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_വാക്കുകൾ_If(സ്ഥിരം string& old, തർക്കം doIt) VL_MT_SAFE {
    തിരികെ VIdProtectImp::singleton().protect_വാക്കുകൾ_If(old, doIt);
}
ശൂന്യം VIdProtect::writeMapFile(സ്ഥിരം string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
