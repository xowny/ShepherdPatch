#include "d3d_proc_redirect.h"

#include <algorithm>
#include <string>

namespace shh
{
namespace
{
std::string ToLowerAscii(std::string_view text)
{
    std::string lowered(text);
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char ch) -> char {
                       if (ch >= 'A' && ch <= 'Z')
                       {
                           return static_cast<char>(ch - 'A' + 'a');
                       }

                       return static_cast<char>(ch);
                   });
    return lowered;
}

std::string_view Basename(std::string_view path)
{
    const std::size_t separator = path.find_last_of("\\/");
    if (separator == std::string_view::npos)
    {
        return path;
    }

    return path.substr(separator + 1);
}
} // namespace

D3DProcRedirectKind ClassifyD3DProcRedirect(std::string_view modulePath,
                                            std::string_view procName)
{
    const std::string moduleName = ToLowerAscii(Basename(modulePath));
    if (moduleName != "d3d9.dll")
    {
        return D3DProcRedirectKind::none;
    }

    if (procName == "Direct3DCreate9")
    {
        return D3DProcRedirectKind::direct3d_create9;
    }

    if (procName == "Direct3DCreate9Ex")
    {
        return D3DProcRedirectKind::direct3d_create9_ex;
    }

    return D3DProcRedirectKind::none;
}

const char* DescribeD3DProcRedirectKind(D3DProcRedirectKind kind)
{
    switch (kind)
    {
    case D3DProcRedirectKind::direct3d_create9:
        return "Direct3DCreate9";
    case D3DProcRedirectKind::direct3d_create9_ex:
        return "Direct3DCreate9Ex";
    case D3DProcRedirectKind::none:
    default:
        return "none";
    }
}
} // namespace shh
