#include "d3d_proc_redirect.h"
#include "test_registry.h"

TEST_CASE(ClassifyD3DProcRedirectAcceptsDirect3DCreate9ForD3D9Module)
{
    CHECK_EQ(shh::ClassifyD3DProcRedirect(R"(C:\Windows\SysWOW64\d3d9.dll)",
                                          "Direct3DCreate9"),
             shh::D3DProcRedirectKind::direct3d_create9);
}

TEST_CASE(ClassifyD3DProcRedirectAcceptsDirect3DCreate9ExForCaseInsensitiveBasename)
{
    CHECK_EQ(shh::ClassifyD3DProcRedirect("D3D9.DLL", "Direct3DCreate9Ex"),
             shh::D3DProcRedirectKind::direct3d_create9_ex);
}

TEST_CASE(ClassifyD3DProcRedirectRejectsNonD3D9Modules)
{
    CHECK_EQ(shh::ClassifyD3DProcRedirect(R"(C:\Windows\SysWOW64\dinput8.dll)",
                                          "Direct3DCreate9"),
             shh::D3DProcRedirectKind::none);
}

TEST_CASE(ClassifyD3DProcRedirectRejectsUnrelatedExports)
{
    CHECK_EQ(shh::ClassifyD3DProcRedirect(R"(C:\Windows\SysWOW64\d3d9.dll)",
                                          "Direct3DShaderValidatorCreate9"),
             shh::D3DProcRedirectKind::none);
}
