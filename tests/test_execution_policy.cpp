#include "execution_policy.h"
#include "test_registry.h"

TEST_CASE(BuildModernExecutionPlanEnablesPowerAndMmcssPolicies)
{
    const shh::ModernExecutionPlan plan = shh::BuildModernExecutionPlan(true, true);

    CHECK_TRUE(plan.applyProcessPowerThrottling);
    CHECK_EQ(plan.processVersion, static_cast<ULONG>(PROCESS_POWER_THROTTLING_CURRENT_VERSION));
    CHECK_EQ(plan.processControlMask,
             static_cast<ULONG>(PROCESS_POWER_THROTTLING_EXECUTION_SPEED));
    CHECK_EQ(plan.processStateMask, 0u);

    CHECK_TRUE(plan.applyThreadPowerThrottling);
    CHECK_EQ(plan.threadVersion, static_cast<ULONG>(THREAD_POWER_THROTTLING_CURRENT_VERSION));
    CHECK_EQ(plan.threadControlMask,
             static_cast<ULONG>(THREAD_POWER_THROTTLING_EXECUTION_SPEED));
    CHECK_EQ(plan.threadStateMask, 0u);

    CHECK_TRUE(plan.applyGamesMmcssProfile);
}

TEST_CASE(BuildModernExecutionPlanAllowsMmcssWithoutPowerPolicy)
{
    const shh::ModernExecutionPlan plan = shh::BuildModernExecutionPlan(false, true);

    CHECK_TRUE(!plan.applyProcessPowerThrottling);
    CHECK_TRUE(!plan.applyThreadPowerThrottling);
    CHECK_TRUE(plan.applyGamesMmcssProfile);
}
