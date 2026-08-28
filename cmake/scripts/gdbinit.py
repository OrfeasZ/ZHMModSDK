#!/bin/env python3

# Copyright 2021 Rémi Bernon for CodeWeavers
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA

# Local changes against upstream:
#   - Rebase PE images on their ImageBase, not on file offsets. Wine does not map them by
#     file offset, so every section got the wrong address.
#   - Match section names by prefix. A PE limits a name to 8 characters, so DWARF arrives
#     truncated as '.debug_i'.
#   - Read ImageBase from the PE header, and drop the `file` and `readelf` calls. Upstream
#     used four subprocesses for each module. `objdump -p` alone prints 39 MB of import and
#     relocation tables to reach a field on line 20.
#   - Parse the maps file only after it changes, because this runs at every stop.
#   - Use GNU objdump. The section parsing needs its "2**" alignment column and ALLOC flag,
#     and llvm-objdump prints neither.

from __future__ import print_function

import os
import re
import struct
import subprocess

import gdb


def _is_gnu_objdump(path):
    try:
        return 'GNU objdump' in subprocess.check_output([path, '--version'],
                                                        stderr=subprocess.STDOUT).decode()
    except Exception:
        return False


def _find_objdump():
    """Find GNU objdump. _has_debug reads the 'DEBUGGING' flag and the '2**' alignment
    column, and llvm-objdump prints neither, so it reports every module as having no debug
    info.

    OBJDUMP is honoured only when it really is GNU. A nix shell exports OBJDUMP=objdump the
    same way it exports AR=ar, and a bare 'objdump' finds llvm-objdump whenever
    llvm-binutils comes before GNU binutils on PATH.
    """
    candidates = []
    if os.environ.get('OBJDUMP'):
        candidates.append(os.environ['OBJDUMP'])
    candidates += [os.path.join(d, 'objdump')
                   for d in os.environ.get('PATH', '').split(os.pathsep)]

    for p in candidates:
        if _is_gnu_objdump(p):
            return p

    print('gdb-wine: no GNU objdump on PATH -- PE modules will load without symbols')
    return 'objdump'


OBJDUMP = _find_objdump()


def _image_base(path):
    """('elf', None) or ('pe', imagebase) for a mapped file, or None if it is neither.

    ELF gives None because its load address needs the program headers. readelf supplies
    that, and only for the few modules that have debug info.
    """
    try:
        with open(path, 'rb') as f:
            head = f.read(0x40)
            if head[:4] == b'\x7fELF':
                return ('elf', None)
            if head[:2] != b'MZ' or len(head) < 0x40:
                return None
            f.seek(struct.unpack_from('<I', head, 0x3c)[0])
            opt = f.read(0x40)
            if opt[:4] != b'PE\0\0':
                return None
            magic = struct.unpack_from('<H', opt, 24)[0]
            if magic == 0x10b:      # PE32
                return ('pe', struct.unpack_from('<I', opt, 24 + 28)[0])
            if magic == 0x20b:      # PE32+
                return ('pe', struct.unpack_from('<Q', opt, 24 + 24)[0])
    except Exception:
        pass
    return None


def _has_debug(sections):
    for line in sections.split('\n'):
        if 'DEBUGGING' in line:
            return True
        if '2**' in line:
            name = line.split(None, 2)[1]
            # Match by prefix. A PE limits a section name to 8 characters, so DWARF can
            # arrive as '.debug_i', not '.debug_info'.
            if name == '.gnu_debuglink' or name.startswith('.debug'):
                return True
    return False


class LoadSymbolFiles(gdb.Command):
    'Command to load symbol files directly from /proc/<pid>/maps.'

    def __init__(self):
        sup = super(LoadSymbolFiles, self)
        sup.__init__('load-symbol-files', gdb.COMMAND_FILES, gdb.COMPLETE_NONE,
                     False)

        self.libs = {}
        self.maps = {}
        gdb.execute('alias -a lsf = load-symbol-files', True)

    def invoke(self, arg, from_tty):
        pid = gdb.selected_inferior().pid
        if not pid in self.libs: self.libs[pid] = {}

        def command(cmd, confirm=from_tty, to_string=not from_tty):
            gdb.execute(cmd, from_tty=confirm, to_string=to_string)

        def execute(cmd):
            return subprocess.check_output(cmd, stderr=subprocess.STDOUT) \
                .decode('utf-8')

        # If the file did not change, no new module is mapped. This runs at every stop,
        # which includes every step.
        with open('/proc/{}/maps'.format(pid), 'r') as maps:
            data = maps.read()
        if self.maps.get(pid) == data:
            return
        self.maps[pid] = data

        libs = {}
        for line in data.split('\n'):
            fields = line.split(None, 5)
            if len(fields) < 6: continue
            addr, node, path = fields[0], fields[4], fields[5].strip()
            if node == '0': continue
            if path in libs: continue
            libs[path] = int(addr.split('-')[0], 16)

        # unload symbol file if address changed
        for k in set(libs) & set(self.libs[pid]):
            if libs[k] != self.libs[pid][k]:
                try:
                    command('remove-symbol-file "{}"'.format(k), confirm=False)
                except:
                    print("warn: Failed to unload symbol file {}".format(k))
                finally:
                    del self.libs[pid][k]

        # load symbol file for new mappings
        for k in set(libs) - set(self.libs[pid]):
            if arg is not None and re.search(arg, k) is None: continue
            addr = self.libs[pid][k] = libs[k]

            kind = _image_base(k)
            if kind is None: continue

            try:
                sections = execute([OBJDUMP, '-h', k])
            except:
                continue

            if not _has_debug(sections):
                print('no debugging info found in {}'.format(k))
                continue

            base = kind[1]
            if base is None:
                try:
                    out = execute(['readelf', '-l', k])
                    base = next(int(l.split()[2], 16)
                                for l in out.split('\n') if 'LOAD' in l)
                except:
                    continue

            name, offs = None, None
            cmd = 'add-symbol-file "{}"'.format(k)
            for line in sections.split('\n'):
                if '2**' in line:
                    _, name, _, vma, _, off, _ = line.split(None, 6)
                    offs = int(vma, 16) - base
                if 'ALLOC' in line and offs is not None:
                    cmd += ' -s {} 0x{:x}'.format(name, addr + offs)

            print('loading symbols for {}'.format(k))
            command(cmd, confirm=False, to_string=True)


LoadSymbolFiles()
