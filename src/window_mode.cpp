#include "window_mode.h"

#include <windows.h>

namespace shh
{
BorderlessWindowPlan BuildBorderlessWindowPlan(bool forceBorderless, const Rect& monitorRect)
{
    BorderlessWindowPlan plan;
    if (!forceBorderless)
    {
        return plan;
    }

    const long width = monitorRect.right - monitorRect.left;
    const long height = monitorRect.bottom - monitorRect.top;
    if (width <= 0 || height <= 0)
    {
        return plan;
    }

    plan.apply = true;
    plan.x = monitorRect.left;
    plan.y = monitorRect.top;
    plan.width = width;
    plan.height = height;
    plan.style = WS_POPUP;
    plan.exStyle = WS_EX_APPWINDOW;
    return plan;
}
} // namespace shh
