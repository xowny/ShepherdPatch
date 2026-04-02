#include "execution_policy.h"

namespace shh
{
ModernExecutionPlan BuildModernExecutionPlan(bool disablePowerThrottling,
                                             bool enableGamesMmcssProfile)
{
    ModernExecutionPlan plan{};
    plan.applyGamesMmcssProfile = enableGamesMmcssProfile;

    if (disablePowerThrottling)
    {
        plan.applyProcessPowerThrottling = true;
        plan.processVersion = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
        plan.processControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
        plan.processStateMask = 0;

        plan.applyThreadPowerThrottling = true;
        plan.threadVersion = THREAD_POWER_THROTTLING_CURRENT_VERSION;
        plan.threadControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
        plan.threadStateMask = 0;
    }

    return plan;
}
} // namespace shh
