"""Build the Mod using a locally owned, exact-version game installation."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import pefile

TARGET = '2210ca88e410746ac3a898edd38a4520f6cda315f61380c0077edcae2f47ddaf'
ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-dir', type=Path, required=True)
    parser.add_argument('--vcvars', type=Path, help='Optional vcvarsall.bat path')
    args = parser.parse_args()
    game = args.game_dir.resolve()
    exe = game / 'unmetal.exe'
    if hashlib.sha256(exe.read_bytes()).hexdigest() != TARGET:
        raise SystemExit('Unsupported executable; no build performed.')
    original = pefile.PE(str(game / 'SDL2.dll'))
    source_exports = {(e.name, e.ordinal) for e in original.DIRECTORY_ENTRY_EXPORT.symbols}
    if any(e.forwarder for e in original.DIRECTORY_ENTRY_EXPORT.symbols):
        raise SystemExit('Use an unmodified game installation, not a proxy DLL.')
    build = ROOT / 'build'
    build.mkdir(exist_ok=True)
    pe = pefile.PE(str(exe))
    sites = [('d0',0x404a5d,6),('m0',0x42fa41,7),('e0',0x4792d0,5),
             ('s0',0x407be7,6),('r0',0x4071d8,6),('a0',0x48dae9,5)]
    # Private generated input: do not publish this header or the build directory.
    declarations = []
    for name, va, size in sites:
        data = pe.get_data(va - pe.OPTIONAL_HEADER.ImageBase, size)
        if len(data) != size: raise SystemExit('Missing hook site')
        declarations.append('const BYTE ' + name + '[] = {' + ','.join(str(b) for b in data) + '};')
    (build / 'target_signatures.inc').write_text('\n'.join(declarations)+'\n', encoding='ascii')
    if args.vcvars:
        vcvars = args.vcvars.resolve()
    else:
        vswhere = Path(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)')) / 'Microsoft Visual Studio/Installer/vswhere.exe'
        installation = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
            'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
        if not installation: raise SystemExit('Install Visual Studio C++ x86 build tools.')
        vcvars = Path(installation) / 'VC/Auxiliary/Build/vcvarsall.bat'
    # Paths are quoted; reject CMD metacharacters before writing a local helper.
    if any(c in str(vcvars) + str(ROOT) for c in '\r\n%!"&|<>^'):
        raise SystemExit('Build from a path without CMD metacharacters (parentheses are supported).')
    commands = ['@echo off', 'call "'+str(vcvars)+'" x86 >nul', 'if errorlevel 1 exit /b 1',
        'cl /nologo /EHsc /std:c++17 /LD /O2 /I. "../src/sdl2_proxy.cpp" "../src/forwarders.cpp" /link /OUT:SDL2.dll',
        'exit /b %errorlevel%']
    # Visual Studio normally lives under Program Files (x86); quoted paths are used.
    (build / 'compile.cmd').write_text('\n'.join(commands)+'\n', encoding='utf-8')
    subprocess.run(['cmd.exe','/d','/c','compile.cmd'], cwd=build, check=True)
    dll = pefile.PE(str(build / 'SDL2.dll'))
    if dll.FILE_HEADER.Machine != 0x14c: raise SystemExit('Not an x86 DLL')
    exports = {(e.name,e.ordinal) for e in dll.DIRECTORY_ENTRY_EXPORT.symbols}
    if exports != source_exports: raise SystemExit('Export mismatch')
    for e in dll.DIRECTORY_ENTRY_EXPORT.symbols:
        expected = None if e.name == b'SDL_Init' else b'SDL2_orig.' + e.name
        if e.forwarder != expected: raise SystemExit('Forwarder mismatch')
    print('PASS: exact target, x86 DLL and all 536 SDL export mappings')
    print('Built:', build / 'SDL2.dll')

if __name__ == '__main__':
    main()
