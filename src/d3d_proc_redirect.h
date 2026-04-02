#pragma once

#include <string_view>

namespace shh
{
enum class D3DProcRedirectKind
{
    none = 0,
    direct3d_create9,
    direct3d_create9_ex,
};

D3DProcRedirectKind ClassifyD3DProcRedirect(std::string_view modulePath,
                                            std::string_view procName);
const char* DescribeD3DProcRedirectKind(D3DProcRedirectKind kind);
} // namespace shh
