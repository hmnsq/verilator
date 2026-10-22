# pylint: disable=line-too-long,invalid-name,multiple-statements,missing-function-docstring,missing-class-docstring,missing-module-docstring,no-else-return,too-few-public-methods,unused-argument
# DESCRIPTION: Verilator: GDB startup file with useful defines
# SPDX-FileCopyrightText: 2023 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
######################################################################

ਆਯਾਤ tempfile
ਆਯਾਤ gdb  # pylint: disable=import-error


ਪਰਿਭਾਸ਼ਾ _vltgdb_get_dump(ਗੰਢ):
    gdb.execute(f'set $_gdb_dump_json_str = AstNode::dumpTreeJsonGdb({node})')
    dump = gdb.execute('printf "%s", $_gdb_dump_json_str', ਮੰਜ਼ਿਲ_string=ਸੱਚ)
    gdb.execute('call (void)free($_gdb_dump_json_str)')
    ਵਾਪਸ dump


ਪਰਿਭਾਸ਼ਾ _vltgdb_tmpfile():
    ਵਾਪਸ tempfile.NamedTemporaryFile(ਬਹੁਲਕ="wt")  # write, text mode


ਪਰਿਭਾਸ਼ਾ _vltgdb_fwrite(file, s):
    """Write to file and flush buffer before passing the file to astsee"""
    file.ਲਿਖ(s)
    file.flush()


ਵਰਗ AstseeCmd(gdb.Command):
    """Verilator: Pretty print or diff nodes using `astsee`. A node can be:
    * an pointer identifier,
    * an address literal,
    * a GDB value (like `$1`) that stores a dump previously done by the `jstash` command.
    Apart from not taking input from a file, it works exactly like `verilator_astsee`:
    * passing one node gives you a pretty print,
    * passing two nodes gives you a diff,
    * for more options see `astsee` readme/help.
    For examples see internals.rst
    """

    ਪਰਿਭਾਸ਼ਾ __init__(self):
        ਪਰਮ().__init__("jtree", gdb.COMMAND_USER, gdb.COMPLETE_ਵਿਅੰਜਕ)

    ਪਰਿਭਾਸ਼ਾ _null_check(self, old, ਨਵਾਂ):
        err = ""
        ਜੇ old == "<nullptr>\n":
            err += "old == <nullptr>\n"
        ਜੇ ਨਵਾਂ == "<nullptr>\n":
            err += "new == <nullptr>"
        ਜੇ err:
            ਉਠਾਓ gdb.GdbError(err.strip("\n"))

    ਪਰਿਭਾਸ਼ਾ invoke(self, arg_str, from_tty):
        ਤਃ astsee ਆਯਾਤ verilator_cli ਇਵ astsee  # pylint: disable=import-error,import-outside-toplevel
        # we import it here so "no module X" error would occur on typing `jtree` rather than on every gdb start

        # We hack `astsee_verilator`'s arg parser to find arguments with nodes
        # After finding them, we replace them with proper files
        astsee_ਦਲੀਲਾਂ = astsee.parser.parse_ਦਲੀਲਾਂ(gdb.string_ਮੰਜ਼ਿਲ_argv(arg_str))
        ਨਾਲ _vltgdb_tmpfile() ਇਵ oldfile, _vltgdb_tmpfile() ਇਵ newfile, _vltgdb_tmpfile(
        ) ਇਵ metafile:
            ਜੇ astsee_ਦਲੀਲਾਂ.file:
                _vltgdb_fwrite(oldfile, _vltgdb_get_dump(astsee_ਦਲੀਲਾਂ.file))
                astsee_ਦਲੀਲਾਂ.file = oldfile.name
            ਜੇ astsee_ਦਲੀਲਾਂ.newfile:
                _vltgdb_fwrite(newfile, _vltgdb_get_dump(astsee_ਦਲੀਲਾਂ.newfile))
                astsee_ਦਲੀਲਾਂ.newfile = newfile.name
            ਜੇ astsee_ਦਲੀਲਾਂ.meta ਹੈ ਕੁਝ_ਨਹੀਂ:
                # pass
                gdb.execute(f'call AstNode::dumpJsonMetaFileGdb("{metafile.name}")')
                astsee_ਦਲੀਲਾਂ.meta = metafile.name
            ਕੋਸ਼ਿਸ਼:
                astsee.ਮੁੱਖ(astsee_ਦਲੀਲਾਂ)
            ਅਪਵਾਦ SystemExit ਇਵ e:  # astsee prints nice errmsgs on exit(), so rethrow it as GdbError to suppress cryptic python trace
                ਉਠਾਓ gdb.GdbError("astsee exited with non-zero code") ਤਃ e


AstseeCmd()
