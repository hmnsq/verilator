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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3File.h"

#अन्तर्भाव "V3Os.h"
#अन्तर्भाव "V3String.h"

#अन्तर्भाव <cerrno>
#अन्तर्भाव <cstdarg>
#अन्तर्भाव <fcntl.h>
#अन्तर्भाव <iomanip>
#अन्तर्भाव <map>
#अन्तर्भाव <memory>

#अन्तर्भाव <sys/stat.h>
#अन्तर्भाव <sys/types.h>

// clang-format off
#जर defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#endif

#ifdef HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#अन्यथा
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#endif

#ifdef INFILTER_PIPE
# include <sys/wait.h>
#endif

#जर defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr पूर्णांक INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr पूर्णांक INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_SIZE_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

वर्ग V3FileDependImp final {
    // TYPES
    वर्ग DependFile final {
        // A single file
        स्थिरांक तार्किक m_target;  // True if write, else read
        तार्किक m_exists = सत्य;  // File exists
        स्थिरांक string m_filename;  // Filename
        संरचना stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    सार्वजनिक:
        DependFile(स्थिरांक string& filename, तार्किक target)
            : m_target{target}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = पूर्वनिर्धारित;
        स्थिरांक string& filename() स्थिरांक { परत m_filename; }
        तार्किक target() स्थिरांक { परत m_target; }
        तार्किक exists() स्थिरांक { परत m_exists; }
        off_t size() स्थिरांक { परत m_stat.st_size; }
        ino_t ino() स्थिरांक { परत m_stat.st_ino; }
        समय_प्रकार cstime() स्थिरांक { परत m_stat.st_ctime; }  // Seconds
        समय_प्रकार cnstime() स्थिरांक { परत VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        समय_प्रकार mstime() स्थिरांक { परत m_stat.st_mtime; }  // Seconds
        समय_प्रकार mnstime() स्थिरांक { परत VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_चिन्ह() {
            स्थिर VHashSha512 s_emptyHash;
            परत m_hash.digestSymbol24() != s_emptyHash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        शून्य loadStats() {
            जर (!m_stat.st_mtime) {
                स्थिरांक string fn = filename();
                स्थिरांक पूर्णांक err = stat(fn.c_str(), &m_stat);
                जर (err != 0) {
                    std::स_रखो(&m_stat, 0, आकार(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = असत्य;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } अन्यथा {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    जर (!target() && size() <= FILE_HASH_SIZE_MAX) m_hash.घाला_File(filename());
                }
            }
        }
        तार्किक चालक<(स्थिरांक DependFile& rhs) स्थिरांक { परत filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::set<string> m_filenameSet VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::set<DependFile> m_filenameList;  // Files sourced/generated

    स्थिर string stripQuotes(स्थिरांक string& in) {
        string pretty = in;
        string::size_type pos;
        पर्यंत ((pos = pretty.find('\"')) != string::npos) pretty.replace(pos, 1, "_");
        पर्यंत ((pos = pretty.find('\n')) != string::npos) pretty.replace(pos, 1, "_");
        परत pretty;
    }

सार्वजनिक:
    // ACCESSOR METHODS
    शून्य addSrcDepend(स्थिरांक string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        स्थिरांक V3LockGuard lock{m_mutex};
        स्थिरांक स्वतः itFoundPair = m_filenameSet.घाला(filename);
        जर (itFoundPair.second) {
            DependFile df{filename, असत्य};
            df.loadStats();  // Get size now, in case changes during the run
            m_filenameList.घाला(df);
        }
    }
    शून्य addTgtDepend(स्थिरांक string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        स्थिरांक V3LockGuard lock{m_mutex};
        स्थिरांक स्वतः itFoundPair = m_filenameSet.घाला(filename);
        जर (itFoundPair.second) m_filenameList.घाला(DependFile{filename, सत्य});
    }
    शून्य writeDepend(स्थिरांक string& filename);
    std::vector<string> getAllDeps() स्थिरांक;
    शून्य writeTimes(स्थिरांक string& filename, स्थिरांक string& cmdlineIn);
    तार्किक checkTimes(स्थिरांक string& filename, स्थिरांक string& cmdlineIn);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

शून्य V3FileDependImp::writeDepend(स्थिरांक string& filename) {
    स्थिरांक std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    जर (ofp->fail()) v3fatal("Can't write file: " << filename);

    साठी (स्थिरांक DependFile& i : m_filenameList) {
        जर (i.target()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    साठी (स्थिरांक DependFile& i : m_filenameList) {
        जर (!i.target()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    जर (v3Global.opt.makePhony()) {
        *ofp << '\n';
        साठी (स्थिरांक DependFile& i : m_filenameList) {
            जर (!i.target()) *ofp << i.filename() << ":\n";
        }
    }
}

std::vector<string> V3FileDependImp::getAllDeps() स्थिरांक {
    std::vector<string> r;
    साठी (स्थिरांक स्वतः& itr : m_filenameList) {
        जर (!itr.target() && itr.exists()) r.ढकला_back(itr.filename());
    }
    परत r;
}

शून्य V3FileDependImp::writeTimes(स्थिरांक string& filename, स्थिरांक string& cmdlineIn) {
    स्थिरांक std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    जर (ofp->fail()) v3fatal("Can't write file: " << filename);

    स्थिरांक string cmdline = stripQuotes(cmdlineIn);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    साठी (स्वतः& it : m_filenameList) {
        DependFile* स्थिरांक depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->loadStats();
        off_t showSize = depfilep->size();
        ino_t showIno = depfilep->ino();
        जर (depfilep->filename() == filename) {
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
        *ofp << " \"" << depfilep->hash_Digest_चिन्ह() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

तार्किक V3FileDependImp::checkTimes(स्थिरांक string& filename, स्थिरांक string& cmdlineIn) {
    स्थिरांक std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    जर (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        परत असत्य;
    }
    {
        स्थिरांक string ignore = V3Os::getline(*ifp);
        जर (ignore.empty()) { /*used*/
        }
    }
    {
        अक्षर chkDir;
        *ifp >> chkDir;
        अक्षर quote;
        *ifp >> quote;
        स्थिरांक string chkCmdline = V3Os::getline(*ifp, '"');
        स्थिरांक string cmdline = stripQuotes(cmdlineIn);
        जर (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            परत असत्य;
        }
    }

    स्थिरांक तार्किक skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").empty();

    पर्यंत (!ifp->eof()) {
        अक्षर chkDir;
        *ifp >> chkDir;
        off_t chkSize;
        *ifp >> chkSize;
        ino_t chkIno;
        *ifp >> chkIno;
        जर (ifp->eof()) थांबा;  // Needed to read final whitespace before found eof
        समय_प्रकार chkCstime;
        *ifp >> chkCstime;
        समय_प्रकार chkCnstime;
        *ifp >> chkCnstime;
        समय_प्रकार chkMstime;
        *ifp >> chkMstime;
        समय_प्रकार chkMnstime;
        *ifp >> chkMnstime;
        अक्षर quote;
        *ifp >> quote;
        स्थिरांक string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        स्थिरांक string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        संरचना stat curStat;
        स्थिरांक पूर्णांक err = stat(chkFilename.c_str(), &curStat);
        जर (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            परत असत्य;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        जर (filename == chkFilename) सुरूठेवा;

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
        जर (!(curStat.st_size >= chkSize && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_size << "=?" << chkSize << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            जर (skipHashing) परत असत्य;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            जर (chkHash == "unhashed") परत असत्य;

            VHashSha512 curHash;
            curHash.घाला_File(chkFilename);
            जर (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                परत असत्य;
            }
        }
    }
    परत सत्य;
}

//######################################################################
// V3File

शून्य V3File::addSrcDepend(स्थिरांक string& filename) VL_MT_SAFE { dependImp.addSrcDepend(filename); }
शून्य V3File::addTgtDepend(स्थिरांक string& filename) VL_MT_SAFE { dependImp.addTgtDepend(filename); }
शून्य V3File::writeDepend(स्थिरांक string& filename) { dependImp.writeDepend(filename); }
std::vector<string> V3File::getAllDeps() { परत dependImp.getAllDeps(); }
शून्य V3File::writeTimes(स्थिरांक string& filename, स्थिरांक string& cmdlineIn) {
    dependImp.writeTimes(filename, cmdlineIn);
}
तार्किक V3File::checkTimes(स्थिरांक string& filename, स्थिरांक string& cmdlineIn) {
    परत dependImp.checkTimes(filename, cmdlineIn);
}
शून्य V3File::createMakeDirFor(स्थिरांक string& filename) {
    जर (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().length() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
शून्य V3File::createMakeDir() {
    स्थिर तार्किक s_created = असत्य;
    जर (!s_created) {
        s_created = सत्य;
        V3Os::createDir(v3Global.opt.makeDir());
        जर (v3Global.opt.hier_शीर्ष()) V3Os::createDir(v3Global.opt.hier_शीर्ष_Data_Dir());
    }
}

//######################################################################
// VInFilterImp

वर्ग VInFilterImp final {
    using StrList = VInFilter::StrList;

    std::map<स्थिरांक std::string, std::string> m_contentsMap;  // Cache of file contents
    तार्किक m_readEof = असत्य;  // Received EOF on read
#ifdef INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#अन्यथा
    पूर्णांक m_pid = 0;  // fork() process id - always zero as disabled
#endif
    तार्किक m_pidExited = असत्य;  //  waitpid() reaped the filter process
    पूर्णांक m_pidStatus = 0;  // Exit status of the filter process
    पूर्णांक m_writeFd = 0;  // File descriptor TO filter
    पूर्णांक m_readFd = 0;  // File descriptor FROM filter

खाजगी:
    // METHODS

    तार्किक readContents(स्थिरांक string& filename, StrList& outl) {
        जर (m_pid) {
            परत readContentsFilter(filename, outl);
        } अन्यथा {
            परत readContentsFile(filename, outl);
        }
    }
    तार्किक readContentsFile(स्थिरांक string& filename, StrList& outl) {
        स्थिरांक पूर्णांक fd = खोलो(filename.c_str(), O_RDONLY);
        जर (fd < 0) परत असत्य;
        m_readEof = असत्य;
        readBlocks(fd, -1, outl);
        बंद(fd);
        परत सत्य;
    }
    // cppcheck-suppress constParameterReference
    तार्किक readContentsFilter(स्थिरांक string& filename, StrList& outl) {
        (शून्य)filename;  // Prevent unused variable warning
        (शून्य)outl;  // Prevent unused variable warning
#ifdef INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        स्थिरांक string ओळ = read_Filter_ओळ();
        जर (ओळ.find("Content-Length") != string::npos) {
            पूर्णांक len = 0;
            माला_पूछो(ओळ.c_str(), "Content-Length: %d\n", &len);
            readBlocks(m_readFd, len, outl);
            परत सत्य;
        } अन्यथा {
            जर (ओळ != "") v3error("--pipe-filter protocol error, unexpected: " << ओळ);
            परत असत्य;
        }
#अन्यथा
        v3fatalSrc("--pipe-filter not implemented on this platform");
        परत असत्य;
#endif
    }

    // cppcheck-suppress functionConst
    शून्य checkFilter(तार्किक hang) {
#ifdef INFILTER_PIPE
        जर (!m_pidExited && waitpid(m_pid, &m_pidStatus, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pidStatus << " exit=" << WEXITSTATUS(m_pidStatus) << " err"
                         << std::म_त्रुटि(त्रुटि_सं));
            m_readEof = सत्य;
            m_pidExited = सत्य;
        }
#endif
    }

    शून्य readBlocks(पूर्णांक fd, पूर्णांक size, StrList& outl) {
        अक्षर buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        पर्यंत (!m_readEof && (size < 0 || size > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            जर (size > 0 && size < todo) todo = size;
            त्रुटि_सं = 0;
            स्थिरांक ssize_t got = पठ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            जर (got > 0) {
                outl.emplace_back(string(buf, got));
                sizegot += got;
            } अन्यथा जर (त्रुटि_सं == EINTR || त्रुटि_सं == EAGAIN
#ifdef EWOULDBLOCK
                       || त्रुटि_सं == EWOULDBLOCK
#endif
            ) {
                checkFilter(असत्य);
                V3Os::u_sleep(1000);
                सुरूठेवा;
            } अन्यथा {
                m_readEof = सत्य;
                थांबा;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_ओळ() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string ओळ;
        पर्यंत (!m_readEof) {
            StrList outl;
            readBlocks(m_readFd, 1, outl);
            स्थिरांक string onechar = listString(outl);
            ओळ += onechar;
            जर (onechar == "\n") {
                जर (ओळ == "\n") {
                    ओळ = "";
                    सुरूठेवा;
                } अन्यथा {
                    थांबा;
                }
            }
        }
        UINFO(6, "filter-line-in: " << ओळ);
        परत ओळ;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    शून्य writeFilter(स्थिरांक string& out) {
        जर (debug() >= 6) {
            UINFO(6, "filter-out: " << out);
            जर (out[out.length() - 1] != '\n') cout << '\n';
        }
        जर (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = सत्य;
            stop();
        }
        अचिन्हित offset = 0;
        पर्यंत (!m_readEof && out.length() > offset) {
            त्रुटि_सं = 0;
            स्थिरांक पूर्णांक got = लिख(m_writeFd, (out.c_str()) + offset, out.length() - offset);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            जर (got > 0) {
                offset += got;
            } अन्यथा जर (त्रुटि_सं == EINTR || त्रुटि_सं == EAGAIN
#ifdef EWOULDBLOCK
                       || त्रुटि_सं == EWOULDBLOCK
#endif
            ) {
                checkFilter(असत्य);
                V3Os::u_sleep(1000);
                सुरूठेवा;
            } अन्यथा {
                थांबा;
            }
        }
    }

    // Start the filter
    शून्य start(स्थिरांक string& command) {
        जर (command == "") {
            m_pid = 0;  // Disabled
        } अन्यथा {
            startFilter(command);
        }
    }
    शून्य startFilter(स्थिरांक string& command) {
        (शून्य)command;  // Prevent Unused variable warning
#ifdef INFILTER_PIPE
        पूर्णांक fd_stdin[2];  // Can't use std::array
        पूर्णांक fd_stdout[2];  // Can't use std::array
        constexpr पूर्णांक P_RD = 0;
        constexpr पूर्णांक P_WR = 1;

        जर (VL_UNCOVERABLE(प्रणाल(fd_stdin) != 0 || प्रणाल(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::म_त्रुटि(त्रुटि_सं));  // LCOV_EXCL_LINE
        जर (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        स्थिरांक pid_t pid = विभाजन();
        जर (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::म_त्रुटि(त्रुटि_सं));  // LCOV_EXCL_LINE
        जर (pid == 0) {  // Child
            UINFO(6, "In child");
            बंद(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            बंद(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<अक्षर*>(nullptr));
            // Don't use v3fatal, we don't share the common structures any more
            ख_लिखो(मानक_त्रुटि, "--pipe-filter: exec failed: %s\n", std::म_त्रुटि(त्रुटि_सं));
            _exit(1);
        } अन्यथा {  // Parent
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = असत्य;
            m_pidStatus = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = असत्य;

            बंद(fd_stdin[P_RD]);
            बंद(fd_stdout[P_WR]);

            पूर्णांक flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#अन्यथा
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

    शून्य stop() {
        जर (m_pid) stopFilter();
    }
    शून्य stopFilter() {
        UINFO(6, "Stopping filter process");
#ifdef INFILTER_PIPE
        बंद(m_writeFd);
        checkFilter(सत्य);
        जर (!WIFEXITED(m_pidStatus) || WEXITSTATUS(m_pidStatus) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        बंद(m_readFd);
        UINFO(6, "Closed");
#अन्यथा
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

संरक्षित:
    मित्र वर्ग VInFilter;
    // Read file contents and return it
    तार्किक readWholefile(स्थिरांक string& filename, StrList& outl) {
        स्थिरांक स्वतः it = m_contentsMap.find(filename);
        जर (it != m_contentsMap.end()) {
            outl.ढकला_back(it->second);
            परत सत्य;
        }
        जर (!readContents(filename, outl)) परत असत्य;
        जर (listSize(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        परत सत्य;
    }
    स्थिर माप_प्रकार listSize(स्थिरांक StrList& sl) {
        माप_प्रकार result = 0;
        साठी (स्थिरांक string& i : sl) result += i.length();
        परत result;
    }
    स्थिर string listString(स्थिरांक StrList& sl) {
        string result;
        साठी (स्थिरांक string& i : sl) result += i;
        परत result;
    }
    // CONSTRUCTORS
    explicit VInFilterImp(स्थिरांक string& command) { start(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(स्थिरांक string& command) { m_impp = नवीन VInFilterImp{command}; }
VInFilter::~VInFilter() {
    जर (m_impp) VL_DO_CLEAR(delete m_impp, m_impp = nullptr);
}

तार्किक VInFilter::readWholefile(स्थिरांक string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    परत m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3OutFormatter::V3OutFormatter(V3OutFormatter::Language lang)
    : m_lang{lang} {
    m_blockIndent = v3Global.opt.decoration() ? 4 : 1;
    m_commaWidth = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

स्थिर constexpr पूर्णांक MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
स्थिर स्थिरांक std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> table;
    साठी (पूर्णांक i = 0; i <= MAXSPACE; ++i) table[i] = std::string(static_cast<माप_प्रकार>(i), ' ');
    परत table;
}();

स्थिरांक std::string& V3OutFormatter::indentSpaces(पूर्णांक अंक) {
    // Indent the specified number of spaces.
    परत s_indentSpaces[std::max(0, std::min(अंक, MAXSPACE))];
}

तार्किक V3OutFormatter::tokenMatch(स्थिरांक अक्षर* cp, स्थिरांक अक्षर* cmp) {
    पर्यंत (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    जर (*cmp) परत असत्य;
    जर (*cp && !std::है_खाली(*cp)) परत असत्य;
    परत सत्य;
}

तार्किक V3OutFormatter::tokenStart(स्थिरांक अक्षर* cp) {
    परत (tokenMatch(cp, "begin") || tokenMatch(cp, "case") || tokenMatch(cp, "casex")
            || tokenMatch(cp, "casez") || tokenMatch(cp, "class") || tokenMatch(cp, "function")
            || tokenMatch(cp, "interface") || tokenMatch(cp, "module") || tokenMatch(cp, "package")
            || tokenMatch(cp, "task"));
}

तार्किक V3OutFormatter::tokenEnd(स्थिरांक अक्षर* cp) {
    परत (tokenMatch(cp, "end") || tokenMatch(cp, "endcase") || tokenMatch(cp, "endclass")
            || tokenMatch(cp, "endfunction") || tokenMatch(cp, "endinterface")
            || tokenMatch(cp, "endmodule") || tokenMatch(cp, "endpackage")
            || tokenMatch(cp, "endtask"));
}

तार्किक V3OutFormatter::tokenNotStart(स्थिरांक अक्षर* cp) {
    परत (tokenMatch(cp, "export") || tokenMatch(cp, "import"));
}

पूर्णांक V3OutFormatter::endLevels(स्थिरांक अक्षर* strg) {
    पूर्णांक levels = m_indentLevel;
    {
        स्थिरांक अक्षर* cp = strg;
        पर्यंत (std::है_खाली(*cp)) ++cp;
        निवड (*cp) {
        स्थिती '\n':  // Newlines.. No need for whitespace before it
            परत 0;
        स्थिती '#':  // Preproc directive
            जर (m_lang == LA_C) परत 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            स्थिरांक अक्षर* mp = cp;
            साठी (; std::है_अक्षर_अंक(*mp); ++mp) {}
            जर (mp[0] == ':' && mp[1] != ':') परत (levels - m_blockIndent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    साठी (स्थिरांक अक्षर* cp = strg; *cp; ++cp) {
        निवड (*cp) {
        स्थिती '}':
        स्थिती ')': levels -= m_blockIndent; थांबा;
        स्थिती '<':
            जर (m_lang == LA_XML) {
                जर (cp[1] == '/') levels -= m_blockIndent;
            }
            थांबा;
        स्थिती 'e':
            जर (m_lang == LA_VERILOG && tokenEnd(cp)) levels -= m_blockIndent;
            थांबा;
        स्थिती '\t':
        स्थिती ' ': थांबा;  // Continue
        पूर्वनिर्धारित: परत levels;  // Letter
        }
    }
    परत levels;
}

शून्य V3OutFormatter::putns(स्थिरांक Ast_गाठ* nodep, स्थिरांक अक्षर* strg) {
    जर (!v3Global.opt.decoration()) {
        putsOutput(strg);
        परत;
    }

    स्थिरांक तार्किक put_गाठ_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_sourceLastFilenameno != nodep->fileline()->filenameno()  //
            || m_sourceLastLineno != nodep->fileline()->firstLineno())  //
        && File_ओळ::builtInFilename() != nodep->fileline()->filename();

    जर (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_गाठ_Decoration)) {
        putsNoTracking(indentSpaces(endLevels(strg)));
        m_prependIndent = असत्य;
    }

    जर (put_गाठ_Decoration) {
        स्थिरांक File_ओळ* स्थिरांक flp = nodep->fileline();
        m_sourceLastLineno = flp->firstLineno();
        m_sourceLastFilenameno = flp->filenameno();
        स्थिरांक std::string lineno = std::to_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvtToHex(nodep) + "*/");
    }

    तार्किक notstart = असत्य;
    तार्किक wordstart = सत्य;
    तार्किक equalsForBracket = असत्य;  // Looking for "= {"
    साठी (स्थिरांक अक्षर* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        जर (std::है_अक्षर(*cp)) {
            जर (wordstart && m_lang == LA_VERILOG && tokenNotStart(cp)) notstart = सत्य;
            जर (wordstart && m_lang == LA_VERILOG && !notstart && tokenStart(cp)) indentInc();
            जर (wordstart && m_lang == LA_VERILOG && tokenEnd(cp)) indentDec();
        }
        स्थिरांक तार्किक wasInBackslash = m_inBackslash;
        m_inBackslash = असत्य;
        निवड (*cp) {
        स्थिती '\n':
            ++m_lineno;
            wordstart = सत्य;
            जर (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = सत्य;
            } अन्यथा {
                m_prependIndent = असत्य;
                putsNoTracking(indentSpaces(endLevels(cp + 1)));
            }
            थांबा;
        स्थिती ' ': wordstart = सत्य; थांबा;
        स्थिती '\t': wordstart = सत्य; थांबा;
        स्थिती '"':
            wordstart = असत्य;
            जर (!wasInBackslash) m_inStringLiteral = !m_inStringLiteral;
            थांबा;
        स्थिती '\\':
            wordstart = असत्य;
            जर (!wasInBackslash) m_inBackslash = सत्य;
            थांबा;
        स्थिती '/':
            जर (m_lang == LA_C || m_lang == LA_VERILOG) {
                जर (cp > strg && cp[-1] == '/' && !m_inStringLiteral) {
                    // Output ignoring contents to EOL
                    ++cp;
                    पर्यंत (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    जर (*cp) putcNoTracking(*cp);
                }
            }
            थांबा;
        स्थिती '{':
            जर (!m_inStringLiteral) {
                जर (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.ढकला(m_indentLevel
                                    * m_blockIndent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            थांबा;
        स्थिती '}':
            जर (!m_inStringLiteral) {
                जर (m_bracketLevel > 0) {
                    m_parenVec.काढा();
                    --m_bracketLevel;
                }
                indentDec();
            }
            थांबा;
        स्थिती '(':
            जर (!m_inStringLiteral) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.ढकला(m_column);
            }
            थांबा;
        स्थिती ')':
            जर (!m_inStringLiteral) {
                जर (!m_parenVec.empty()) m_parenVec.काढा();
                indentDec();
            }
            थांबा;
        स्थिती '<':
            जर (m_lang == LA_XML) {
                जर (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } अन्यथा जर (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } अन्यथा {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            थांबा;
        स्थिती '>':
            जर (m_lang == LA_XML) {
                indentDec();
                जर (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            थांबा;
        पूर्वनिर्धारित: wordstart = असत्य; थांबा;
        }

        निवड (*cp) {
        स्थिती '=': equalsForBracket = सत्य; थांबा;
        स्थिती ' ': थांबा;
        पूर्वनिर्धारित: equalsForBracket = असत्य; थांबा;
        }
    }
}

शून्य V3OutFormatter::put_Break_अभिव्यक्ती() {
    जर (!m_parenVec.empty()) putBreak();
}

// Add a line break if too wide
शून्य V3OutFormatter::putBreak() {
    जर (!v3Global.opt.decoration()) परत;
    जर (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        जर (exceededWidth()) {
            putcNoTracking('\n');
            जर (!m_parenVec.empty()) putsNoTracking(indentSpaces(m_parenVec.शीर्ष()));
        }
    }
}

शून्य V3OutFormatter::putsQuoted(स्थिरांक string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    स्थिरांक string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    साठी (स्थिरांक अक्षर c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    जर (strg.find('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
शून्य V3OutFormatter::putsNoTracking(स्थिरांक string& strg) {
    जर (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        परत;
    }
    // Don't track {}'s, probably because it's a $display format string
    साठी (स्थिरांक अक्षर c : strg) putcNoTracking(c);
}

शून्य V3OutFormatter::putcNoTracking(अक्षर chr) {
    जर (!v3Global.opt.decoration()) {
        putcOutput(chr);
        परत;
    }
    निवड (chr) {
    स्थिती '\n':
        ++m_lineno;
        m_column = 0;
        m_nobreak = सत्य;
        थांबा;
    स्थिती '\t': m_column = ((m_column + 9) / 8) * 8; थांबा;
    स्थिती ' ':
    स्थिती '(':
    स्थिती '|':
    स्थिती '&': ++m_column; थांबा;
    पूर्वनिर्धारित:
        ++m_column;
        m_nobreak = असत्य;
        थांबा;
    }
    putcOutput(chr);
}

string V3OutFormatter::quoteNameControls(स्थिरांक string& namein,
                                         V3OutFormatter::Language lang) VL_PURE {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string out;
    जर (lang == LA_XML) {
        // Encode chars into XML string
        साठी (स्थिरांक अक्षर c : namein) {
            जर (c == '"') {
                out += "&quot;"s;
            } अन्यथा जर (c == '\'') {
                out += "&apos;"s;
            } अन्यथा जर (c == '<') {
                out += "&lt;"s;
            } अन्यथा जर (c == '>') {
                out += "&gt;"s;
            } अन्यथा जर (c == '&') {
                out += "&amp;"s;
            } अन्यथा जर (std::है_छाप(c)) {
                out += c;
            } अन्यथा {
                out += "&#"s + cvtToStr(static_cast<अचिन्हित पूर्णांक>(c & 0xff)) + ";";
            }
        }
    } अन्यथा {
        // Encode control chars into C style escapes
        साठी (स्थिरांक अक्षर c : namein) {
            जर (c == '\\' || c == '"') {
                out += "\\"s + c;
            } अन्यथा जर (c == '\n') {
                out += "\\n";
            } अन्यथा जर (c == '\r') {
                out += "\\r";
            } अन्यथा जर (c == '\t') {
                out += "\\t";
            } अन्यथा जर (std::है_छाप(c)) {
                out += c;
            } अन्यथा {
                // This will also cover \a etc
                स्थिरांक string octal
                    = "\\"s + cvtToStr((c >> 6) & 3) + cvtToStr((c >> 3) & 7) + cvtToStr(c & 7);
                out += octal;
            }
        }
    }
    परत out;
}

//----------------------------------------------------------------------
// Simple wrappers

शून्य V3OutFormatter::म_लिखो(स्थिरांक अक्षर* fmt...) {
    constexpr माप_प्रकार bufsize = 5000;
    अक्षर sbuff[bufsize];
    बहु_सूची ap;
    बहु_शुरू(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    बहु_पूर्ण(ap);
    हे->माला_दो(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3OutFile::V3OutFile(स्थिरांक string& filename, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_filename{filename}
    , m_bufferp{नवीन std::array<अक्षर, WRITE_BUFFER_SIZE_BYTES>{}} {
    जर ((m_fp = V3File::new_fopen_w(filename)) == nullptr) {
        v3fatal("Can't write file: " << filename);
    }
}

V3OutFile::~V3OutFile() {
    writeBlock();

    जर (m_fp) ख_बंद(m_fp);
    m_fp = nullptr;
}

शून्य V3OutFile::putsForceIncs() {
    स्थिरांक VStringList& forceIncs = v3Global.opt.forceIncs();
    साठी (स्थिरांक string& i : forceIncs) माला_दो("#include \"" + i + "\"\n");
}

शून्य V3OutCFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = सत्य;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    साठी (अक्षर& c : var) {
        जर (!std::है_अक्षर_अंक(c)) c = '_';
    }
    माला_दो("\n#ifndef " + var + "\n");
    माला_दो("#define " + var + "  // guard\n");
}

//######################################################################
// V3OutStream

V3OutStream::V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_ostream{ostream} {}

//######################################################################
// VIdProtect

वर्ग VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<स्थिरांक std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_set<std::string> m_newIdSet VL_GUARDED_BY(m_mutex);  // Which new names exist
संरक्षित:
    // CONSTRUCTORS
    मित्र वर्ग VIdProtect;
    स्थिर VIdProtectImp& singleton() VL_MT_SAFE {
        स्थिर VIdProtectImp s;
        परत s;
    }

सार्वजनिक:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = पूर्वनिर्धारित;
    // METHODS
    string passthru(स्थिरांक string& old) VL_MT_SAFE_EXCLUDES(m_mutex) {
        जर (!v3Global.opt.protectIds()) परत old;
        स्थिरांक V3LockGuard lock{m_mutex};
        स्थिरांक स्वतः it = m_nameMap.find(old);
        जर (it != m_nameMap.end()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->second,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } अन्यथा {
            m_nameMap.emplace(old, old);
            m_newIdSet.घाला(old);
        }
        परत old;
    }
    string protectIf(स्थिरांक string& old, तार्किक doIt) VL_MT_SAFE_EXCLUDES(m_mutex) {
        जर (!v3Global.opt.protectIds() || old.empty() || !doIt) परत old;
        स्थिरांक V3LockGuard lock{m_mutex};
        स्थिरांक स्वतः pair = m_nameMap.emplace(old, "");
        जर (pair.second) {
            string out;
            जर (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                out = "PS" + old;
            } अन्यथा {
                VHashSha512 digest{v3Global.opt.protect_किल्ली_Defaulted()};
                digest.घाला(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                out = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                साठी (माप_प्रकार len = 6; len < out.size() - 3; len += 3) {
                    स्थिरांक string tryout = out.substr(0, len);
                    जर (m_newIdSet.घाला(tryout).second) {
                        out = tryout;
                        थांबा;
                    }
                }
            }
            pair.first->second = out;
        }
        परत pair.first->second;
    }
    string protect_शब्द_If(स्थिरांक string& old, तार्किक doIt) VL_MT_SAFE {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        जर (!(doIt && v3Global.opt.protectIds())) परत old;
        string out;
        string::size_type start = 0;
        // space, ., ->
        पर्यंत (सत्य) {
            // When C++11, use find_if and lambda
            string::size_type pos = string::npos;
            string separator;
            trySep(old, start, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ")", pos /*ref*/, separator /*ref*/);
            जर (pos == string::npos) थांबा;
            out += protectIf(old.substr(start, pos - start), सत्य) + separator;
            start = pos + separator.length();
        }
        out += protectIf(old.substr(start), सत्य);
        परत out;
    }
    शून्य writeMapFile(स्थिरांक string& filename) स्थिरांक {
        V3OutXmlFile of{filename};
        of.putsHeader();
        of.माला_दो("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        of.माला_दो("<verilator_id_map>\n");
        {
            साठी (स्थिरांक स्वतः& itr : m_nameMap) {
                of.माला_दो("<map from=\"" + itr.second + "\" to=\"" + itr.first + "\"/>\n");
            }
        }
        of.माला_दो("</verilator_id_map>\n");
    }

खाजगी:
    शून्य trySep(स्थिरांक string& old, string::size_type start, स्थिरांक string& trySep,
                string::size_type& posr, string& separatorr) VL_PURE {
        स्थिरांक string::size_type trypos = old.find(trySep, start);
        जर (trypos != string::npos) {
            जर (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(स्थिरांक string& in) VL_MT_SAFE {
    परत v3Global.opt.protectIds() ? "" : in;
}
string VIdProtect::protectIf(स्थिरांक string& old, तार्किक doIt) VL_MT_SAFE {
    परत VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_शब्द_If(स्थिरांक string& old, तार्किक doIt) VL_MT_SAFE {
    परत VIdProtectImp::singleton().protect_शब्द_If(old, doIt);
}
शून्य VIdProtect::writeMapFile(स्थिरांक string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
