// Runs at the native once-only FBO initialization check, before any FBO binds.
static const uintptr_t screen_targets[]={0x50b1f0,0x50b180,0x50b324,0x50ac6c};
extern "C" uintptr_t resize_resume=0x4071de;
static void __cdecl sync_screen_targets() {
  int width=*at<int*>(0xde2164),height=*at<int*>(0xde2168);
  if(width<=0 || height<=0 || !*at<int*>(0x5e4b18)) return;
  for(int i=0;i<4;++i) {
    if(i==3 && !*at<int*>(0x50c630)) continue;
    void* target=at<void*>(screen_targets[i]);
    void* img=reinterpret_cast<void*>(fld(target,0));
    if(!img || (fld(img,0xc)==width && fld(img,0x10)==height)) continue;
    GLint bound=0,format=0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&bound);
    glBindTexture(GL_TEXTURE_2D,static_cast<GLuint>(fld(img,0)));
    glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_INTERNAL_FORMAT,&format);
    if(format!=GL_RGB8 && format!=GL_RGBA8) {
      glBindTexture(GL_TEXTURE_2D,bound);
      continue;
    }
    typedef void*(__cdecl *Alloc)(size_t,const char*,int);
    typedef void(__cdecl *Free)(void*);
    size_t bytes=static_cast<size_t>(width)*height*4;
    void* pixels=at<Alloc>(0x4807c0)(bytes,"Mod resized framebuffer",0);
    memset(pixels,0x80,bytes);
    glTexImage2D(GL_TEXTURE_2D,0,format,width,height,0,
                 format==GL_RGBA8?GL_RGBA:GL_RGB,GL_UNSIGNED_BYTE,pixels);
    GLint actual_w=0,actual_h=0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&actual_w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&actual_h);
    glBindTexture(GL_TEXTURE_2D,bound);
    if(actual_w!=width || actual_h!=height) {
      at<Free>(0x4809b0)(pixels);
      continue;
    }
    void* old=reinterpret_cast<void*>(fld(img,8));
    fld(img,8)=reinterpret_cast<int>(pixels);
    fld(img,0xc)=width; fld(img,0x10)=height;
    fld(target,0x1c)=width; fld(target,0x20)=height;
    at<Free>(0x4809b0)(old);
  }
}
extern "C" __declspec(naked) void resize_gate() {
  __asm {
    pushfd
    pushad
    call sync_screen_targets
    popad
    popfd
    // Replay the original six-byte CMP, including its flags for the native JNE.
    cmp dword ptr ds:[0x5e4b18],ebx
    jmp dword ptr[resize_resume]
  }
}
