#include "test_registry.h"

#include "thread_diagnostics.h"

TEST_CASE(FormatModuleRelativeAddressIncludesModuleOffsetWhenAvailable)
{
    const std::string formatted =
        shh::FormatModuleRelativeAddress(0x10ABCDEFu, 0x10000000u, "g_SilentHill");

    CHECK_TRUE(formatted.find("0x10ABCDEF") != std::string::npos);
    CHECK_TRUE(formatted.find("g_SilentHill+0xabcdef") != std::string::npos);
}

TEST_CASE(FormatModuleRelativeAddressLeavesPlainHexWithoutModuleBase)
{
    const std::string formatted =
        shh::FormatModuleRelativeAddress(0x10ABCDEFu, 0u, "g_SilentHill");

    CHECK_EQ(formatted, std::string("0x10ABCDEF"));
}

TEST_CASE(DescribeLegacyThreadWrapperSnapshotIncludesPayloadFields)
{
    const shh::LegacyThreadWrapperSnapshot snapshot{
        .threadHandleValue = 0x12345678u,
        .entryPoint = 0x1024E780u,
        .userParameter = 0x0F908F80u,
        .tag = 0x10D90A90u,
        .stateByte = 1u,
    };

    const std::string description =
        shh::DescribeLegacyThreadWrapperSnapshot(snapshot, 0x10000000u);

    CHECK_TRUE(description.find("payload.thread=0x12345678") != std::string::npos);
    CHECK_TRUE(description.find("payload.entry=0x1024E780 (g_SilentHill+0x24e780)") !=
               std::string::npos);
    CHECK_TRUE(description.find("payload.parameter=0x0F908F80") != std::string::npos);
    CHECK_TRUE(description.find("payload.tag=0x10D90A90") != std::string::npos);
    CHECK_TRUE(description.find("payload.state=0x1") != std::string::npos);
}

TEST_CASE(DescribeLegacyThreadWrapperObjectGraphSnapshotIncludesKeyPointers)
{
    const shh::LegacyThreadWrapperObjectGraphSnapshot snapshot{
        .rootObject = 0x2011F140u,
        .rootVtable = 0x10ABC000u,
        .object1C = 0x20123000u,
        .object1CVtable = 0x10ABD000u,
        .object58 = 0x20124000u,
        .object58Vtable = 0x10ABE000u,
        .nestedB4 = 0x20125000u,
        .nestedB4Vtable = 0x10ABF000u,
    };

    const std::string description =
        shh::DescribeLegacyThreadWrapperObjectGraphSnapshot(snapshot, 0x10000000u);

    CHECK_TRUE(description.find("payload.object=0x2011F140") != std::string::npos);
    CHECK_TRUE(description.find("payload.object.vtable=0x10ABC000 (g_SilentHill+0xabc000)") !=
               std::string::npos);
    CHECK_TRUE(description.find("payload.object+0x1c=0x20123000") != std::string::npos);
    CHECK_TRUE(description.find("payload.object+0x58=0x20124000") != std::string::npos);
    CHECK_TRUE(description.find("payload.object+0x1c+0xb4=0x20125000") != std::string::npos);
}

TEST_CASE(ClassifyLegacyThreadTagRecognizesKnownWorkerKinds)
{
    CHECK_EQ(shh::ClassifyLegacyThreadTag("LoadingScreenThread"),
             shh::LegacyThreadRole::LoadingScreen);
    CHECK_EQ(shh::ClassifyLegacyThreadTag("StdAudio Engine"),
             shh::LegacyThreadRole::StdAudioEngine);
    CHECK_EQ(shh::ClassifyLegacyThreadTag("SomethingElse"), shh::LegacyThreadRole::Unknown);
}

TEST_CASE(DescribeLegacyThreadRoleReturnsStableLogLabels)
{
    CHECK_EQ(shh::DescribeLegacyThreadRole(shh::LegacyThreadRole::LoadingScreen),
             std::string_view("LoadingScreen"));
    CHECK_EQ(shh::DescribeLegacyThreadRole(shh::LegacyThreadRole::StdAudioEngine),
             std::string_view("StdAudioEngine"));
    CHECK_EQ(shh::DescribeLegacyThreadRole(shh::LegacyThreadRole::Unknown),
             std::string_view("Unknown"));
}
