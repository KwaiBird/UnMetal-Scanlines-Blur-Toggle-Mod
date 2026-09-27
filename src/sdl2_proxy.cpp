#define WIN32_LEAN_AND_MEAN
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
#include <windows.h>
#include <wincrypt.h>
#include <gl/GL.h>
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")
// UnMetal 1.0.13, Steam build 12471095. Native calls are tied to target_hash.
static HMODULE self;
static volatile LONG scanlines = 1;
static volatile LONG softening = 1;
extern "C" uintptr_t draw_yes = 0, draw_no = 0, menu_end = 0;
extern "C" uintptr_t soften_yes = 0x407bed, soften_no = 0x407d82;
static void *event_original;
static const BYTE target_hash[32] = {
    0x22, 0x10, 0xca, 0x88, 0xe4, 0x10, 0x74, 0x6a, 0xc3, 0xa8, 0x98,
    0xed, 0xd3, 0x8a, 0x45, 0x20, 0xf6, 0xcd, 0xa3, 0x15, 0xf6, 0x13,
    0x80, 0xc0, 0x07, 0x7e, 0xdc, 0xae, 0x2f, 0x47, 0xdd, 0xaf};
template <class T> static T at(uintptr_t x) { return reinterpret_cast<T>(x); }
static int &fld(void *p, int o) {
  return *reinterpret_cast<int *>(reinterpret_cast<char *>(p) + o);
}
#include "resize.inc"
static wchar_t config[MAX_PATH];
static bool path() {
  DWORD n = GetModuleFileNameW(self, config, MAX_PATH);
  if (!n || n >= MAX_PATH - 25)
    return false;
  wchar_t *s = wcsrchr(config, L'\\');
  if (!s)
    return false;
  wcscpy_s(s + 1, MAX_PATH - (s + 1 - config), L"unmetal_scanlines.ini");
  return true;
}
static void load() {
  if (!path())
    return;
  HANDLE f = CreateFileW(config, GENERIC_READ, FILE_SHARE_READ, 0,
                         OPEN_EXISTING, 0, 0);
  if (f == INVALID_HANDLE_VALUE)
    return;
  char b[256] = {};
  DWORD n = 0;
  BOOL ok = ReadFile(f, b, sizeof(b) - 1, &n, 0);
  CloseHandle(f);
  if (!ok || n == sizeof(b)-1) return;
  char* context = nullptr;
  for (char* line = strtok_s(b,"\r\n",&context); line; line = strtok_s(nullptr,"\r\n",&context)) {
    if (!strcmp(line,"scanlines=0")) scanlines = 0;
    else if (!strcmp(line,"scanlines=1")) scanlines = 1;
    else if (!strcmp(line,"softening=0")) softening = 0;
    else if (!strcmp(line,"softening=1")) softening = 1;
  }
}
static void save() {
  if (!path())
    return;
  wchar_t temp[MAX_PATH];
  wcscpy_s(temp, config);
  wcscat_s(temp, L".tmp");
  HANDLE f = CreateFileW(temp, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
  if (f == INVALID_HANDLE_VALUE)
    return;
  char b[] = "scanlines=1\nsoftening=1\n";
  b[10] = scanlines ? '1' : '0';
  b[22] = softening ? '1' : '0';
  DWORD n = 0;
  BOOL ok = WriteFile(f, b, sizeof(b)-1, &n, 0) && n == sizeof(b)-1 && FlushFileBuffers(f);
  CloseHandle(f);
  if (ok)
    MoveFileExW(temp, config,
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
  else
    DeleteFileW(temp);
}
static const wchar_t *label() {
  static const wchar_t *l[] = {
      L"SCANLINES",
      L"L\u00cdNEAS DE BARRIDO",
      L"LINEE DI SCANSIONE",
      L"\u0421\u0422\u0420\u041e\u041a\u0418 "
      L"\u0420\u0410\u0417\u0412\u0401\u0420\u0422\u041a\u0418",
      L"BILDSCHIRMLINIEN",
      L"LIGNES DE BALAYAGE",
      L"LINHAS DE VARREDURA",
      L"\u8d70\u67fb\u7dda",
      L"\u626b\u63cf\u7ebf",
      L"\uc8fc\uc0ac\uc120",
      L"\u4117\u4313\u4300\u4203\u420a \u4022\u4302\u4322\u421f"};
  int i = *at<int *>(0xdf8850);
  return l[i >= 0 && i < 11 ? i : 0];
}
static const wchar_t *softening_label() {
  static const wchar_t *l[] = {
    L"SCREEN BLUR", L"SUAVIZADO", L"SFUMATURA",
    L"\u0420\u0410\u0417\u041c\u042b\u0422\u0418\u0415", L"WEICHZEICHNUNG",
    L"FLOU", L"SUAVIZA\u00c7\u00c3O", L"\u753b\u9762\u307c\u304b\u3057",
    L"\u753b\u9762\u6a21\u7cca", L"\ud654\uba74 \ud750\ub9bc",
    L"\u4105\u4201\u4302\u4300\u420e"
  };
  int i = *at<int *>(0xdf8850);
  return l[i >= 0 && i < 11 ? i : 0];
}
// Preview is separate from the engine's applied mode; arrows never apply it.
static int preview_mode=0;
static void* preview_owner=nullptr;
static bool retain_preview=false;
static volatile LONG mode_applied=1;
static wchar_t mode_text[512];
struct Mode {
  int value, id;
};
static int modes(Mode *out) {
  int n = 0, full = *at<int *>(0xde6fbc);
  if (full < 1)
    full = 1;
  if (full > 4)
    full = 4;
  for (int i = 1; i <= full; ++i)
    out[n++] = {i, full == 1 ? 0x2743 : 0x2776 + i};
  const Mode w[] = {{0x25b6, 0x2745},
                    {0x3248, 0x2746},
                    {0x3eda, 0x2747},
                    {0x2ee6c, 0x2748},
                    {0x3e890, 0x274b}};
  typedef int(__cdecl * Fits)(int);
  for (int i = 0; i < 5; ++i)
    if (at<Fits>(0x4773b0)(w[i].value))
      out[n++] = w[i];
  return n;
}
static void __cdecl build(void *menu) {
  Mode list[9];
  int n=modes(list), index=0;
  if (!retain_preview || preview_owner!=menu) preview_mode=*at<int*>(0xde6fb0);
  preview_owner=menu;
  for(int i=0;i<n;++i) if(list[i].value==preview_mode) {index=i;break;}
  preview_mode=list[index].value;
  int id=list[index].id;
  typedef const wchar_t *(__thiscall * Dict)(void *, int);
  typedef void *(__thiscall * Add)(void *, const wchar_t *, int);
  typedef void(__thiscall * Bind)(void *, void *, int);
  typedef void(__thiscall * Check)(void *, int, void *);
  typedef void(__thiscall * AddId)(void *, int);
  const wchar_t *text = at<Dict>(0x47c3e0)(at<void *>(0xde7018), id - 0x2710);
  // ASCII angle brackets exist in every shipped font; native Bind draws the box.
  _snwprintf_s(mode_text,sizeof(mode_text)/sizeof(mode_text[0]),_TRUNCATE,
              L"<< %ls",text);
  void *mode_item=at<Add>(0x478e70)(menu, mode_text, 0x3001);
  mode_applied=(preview_mode==*at<int*>(0xde6fb0));
  at<Bind>(0x48dbf0)(mode_item, const_cast<LONG*>(&mode_applied), 1);
  fld(mode_item,0x438) |= 1;
  void *item = at<Add>(0x478e70)(menu, label(), 0x3002);
  at<Bind>(0x48dbf0)(item, const_cast<LONG *>(&scanlines), 1);
  fld(item, 0x438) |= 1;
  item = at<Add>(0x478e70)(menu, softening_label(), 0x3003);
  at<Bind>(0x48dbf0)(item, const_cast<LONG *>(&softening), 1);
  fld(item, 0x438) |= 1;
  at<Check>(0x479cb0)(menu, 0x2760, at<void *>(0x5e1de4));
  // Native EBP is 0xb4e67c here (assigned at 0x42f546), not zero.
  fld(menu, 0x45bc) = 0xb4e67c;
  at<AddId>(0x479c50)(menu, 0x2718);
  fld(menu, 0x45bc) = 0xb4d3d4;
}
#include "selector_draw.inc"
static bool modal_active(void* menu) {
  void* popup=reinterpret_cast<void*>(fld(menu,0x4628));
  return popup && fld(popup,0x11720)!=0;
}
static void refresh_mode_row(void* menu, bool keep) {
  typedef void(__thiscall *Build)(void*,int);
  typedef void(__thiscall *Focus)(void*,int,int);
  retain_preview=keep;
  at<Build>(0x42f050)(menu,5);
  retain_preview=false;
  at<Focus>(0x48dd90)(static_cast<char*>(menu)+0x24,0x3001,1);
}
static bool browse_mode(void* menu, int delta) {
  Mode list[9]; int n=modes(list),index=0;
  for(int i=0;i<n;++i) if(list[i].value==preview_mode) {index=i;break;}
  int next=index+delta;
  if(next<0) next=n-1;
  if(next>=n) next=0;
  preview_mode=list[next].value;
  refresh_mode_row(menu,true);
  return true;
}
extern "C" __declspec(naked) void draw_gate() {
  __asm {
 jnz skip
 cmp dword ptr[scanlines],0
 je disabled
 jmp dword ptr[draw_yes]
 disabled:
 skip:
 jmp dword ptr[draw_no]
  }
}
// Keep the central image (0x407be2), skip only displaced translucent copies.
extern "C" __declspec(naked) void softening_gate() {
  __asm {
    cmp dword ptr[softening],0
    je skip
    // Replay the complete six-byte stolen FLD, preserving the x87 stack.
    fld dword ptr ds:[0x4c5744]
    jmp dword ptr[soften_yes]
  skip:
    jmp dword ptr[soften_no]
  }
}
extern "C" __declspec(naked) void menu_gate() {
  __asm {
 pushad
 push esi
 call build
 add esp,4
 popad
 jmp dword ptr[menu_end]
  }
}
static int __fastcall event_gate(void *menu, void *, int *output, int flag) {
  typedef int(__thiscall * Event)(void *, int *, int);
  typedef int(__thiscall * Selected)(void *);
  Event original = reinterpret_cast<Event>(event_original);
  if (fld(menu, 0x4714) != 5)
    return original(menu, output, flag);
  if (modal_active(menu)) return original(menu,output,flag);
  typedef void*(__thiscall *Focused)(void*);
  void* focus=at<Focused>(0x48dd10)(static_cast<char*>(menu)+0x24);
  // Same normalized right/left actions used by the native brightness slider.
  int right=*at<int*>(0x74374c), left=*at<int*>(0x743748);
  if (!flag && focus && fld(focus,0)==0x3001 && right!=left) {
    browse_mode(menu,right?1:-1);
    return 6;
  }
  int id = at<Selected>(0x479190)(menu);
  if (id == 0x3002 || id == 0x3003) {
    fld(menu, 0x45c0) = 0x2760;
    int r = original(menu, output, flag);
    if (r == 6)
      save();
    return r;
  }
  if (id == 0x3001) {
    if (preview_mode==*at<int*>(0xde6fb0)) {
      fld(menu,0x45c0)=0;
      return 6; // Enter on the applied mode does not open a restart dialog.
    }
    Mode list[9]; int n=modes(list),selected=-1;
    for(int i=0;i<n;++i) if(list[i].value==preview_mode) {selected=i;break;}
    if(selected<0) {refresh_mode_row(menu,false);return 6;}
    fld(menu,0x45c0)=list[selected].id;
    int r=original(menu,output,flag);
    if(r==6 || r==7) refresh_mode_row(menu,false);
    return r;
  }
  return original(menu, output, flag);
}
static bool valid_exe() {
  wchar_t path[MAX_PATH];
  if (!GetModuleFileNameW(0, path, MAX_PATH))
    return false;
  HANDLE f =
      CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if (f == INVALID_HANDLE_VALUE)
    return false;
  HCRYPTPROV provider = 0;
  HCRYPTHASH hash = 0;
  bool ok = false;
  if (CryptAcquireContextW(&provider, 0, 0, PROV_RSA_AES,
                           CRYPT_VERIFYCONTEXT) &&
      CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash)) {
    BYTE b[16384], digest[32];
    DWORD n = 0, size = 32;
    BOOL read;
    do {
      read = ReadFile(f, b, sizeof b, &n, 0);
      if (read && n)
        read = CryptHashData(hash, b, n, 0);
    } while (read && n);
    ok = read && n == 0 &&
         CryptGetHashParam(hash, HP_HASHVAL, digest, &size, 0) && size == 32 &&
         !memcmp(digest, target_hash, 32);
  }
  if (hash)
    CryptDestroyHash(hash);
  if (provider)
    CryptReleaseContext(provider, 0);
  CloseHandle(f);
  return ok;
}
static void jump(BYTE *b, void *from, void *to) {
  b[0] = 0xe9;
  *reinterpret_cast<int32_t *>(b + 1) = static_cast<int32_t>(
      reinterpret_cast<uintptr_t>(to) - reinterpret_cast<uintptr_t>(from) - 5);
}
static bool patch(BYTE *to, const BYTE *expected, int n,
                  const BYTE *replacement) {
  if (memcmp(to, expected, n))
    return false;
  DWORD old = 0, unused = 0;
  if (!VirtualProtect(to, n, PAGE_EXECUTE_READWRITE, &old))
    return false;
  memcpy(to, replacement, n);
  FlushInstructionCache(GetCurrentProcess(), to, n);
  VirtualProtect(to, n, old, &unused);
  return true;
}
static void install() {
  if (reinterpret_cast<uintptr_t>(GetModuleHandleW(0)) != 0x400000 ||
      !valid_exe())
    return;
  BYTE *draw = at<BYTE *>(0x404a5d), *menu = at<BYTE *>(0x42fa41),
       *event = at<BYTE *>(0x4792d0), *soft = at<BYTE *>(0x407be7),
       *resize = at<BYTE *>(0x4071d8), *selector = at<BYTE *>(0x48dae9);
#include "target_signatures.inc"
  if (memcmp(draw, d0, 6) || memcmp(menu, m0, 7) || memcmp(event, e0, 5) || memcmp(soft, s0, 6) || memcmp(resize, r0, 6) || memcmp(selector, a0, 5))
    return;
  draw_yes = 0x404a63;
  draw_no = 0x404b6d;
  menu_end = 0x42fba4;
  BYTE *trampoline = static_cast<BYTE *>(
      VirtualAlloc(0, 16, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
  if (!trampoline)
    return;
  memcpy(trampoline, e0, 5);
  jump(trampoline + 5, trampoline + 5, event + 5);
  event_original = trampoline;
  load();
  BYTE d[6] = {}, m[7] = {}, e[5] = {}, soften[6] = {}, resizing[6] = {}, arrows[5] = {};
  jump(d, draw, draw_gate);
  d[5] = 0x90;
  jump(m, menu, menu_gate);
  m[5] = m[6] = 0x90;
  jump(e, event, event_gate);
  jump(soften, soft, softening_gate); soften[5]=0x90;
  jump(resizing,resize,resize_gate); resizing[5]=0x90;
  patch(resize,r0,6,resizing);
  jump(arrows,selector,selector_gate); arrows[0]=0xe8;
  patch(selector,a0,5,arrows);
  patch(soft, s0, 6, soften);
  patch(draw, d0, 6, d);
  patch(menu, m0, 7, m);
  patch(event, e0, 5, e);
}
extern "C" int __cdecl SDL_Init(uint32_t flags) {
  HMODULE original = LoadLibraryW(L"SDL2_orig.dll");
  if (!original) return -1;
  typedef int(__cdecl * Init)(uint32_t);
  Init init = reinterpret_cast<Init>(GetProcAddress(original, "SDL_Init"));
  if (!init) return -1;
  int result = init(flags);
  static volatile LONG installed = 0;
  if (result == 0 && InterlockedCompareExchange(&installed, 1, 0) == 0)
    install();
  return result;
}
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    self = instance;
    DisableThreadLibraryCalls(instance);
  }
  return TRUE;
}
