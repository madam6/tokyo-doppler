#include "Misc/AutomationTest.h"
#include "../Audio/DopplerEffect/DopplerEffectMetasoundNode.h" 

#if WITH_DEV_AUTOMATION_TESTS

// This test suite is separated into 3 specific domains crucial for Doppler effect:
// - Standard Movement (One object moving, proving the base formula)
// - Mutual Movement (Both observer and source moving)
// - Limits & Safety (Handling supersonic speeds and division by zero)
// Each domain is tested under 3 separate loads: basic case, huge amount of data, edge cases

// ----------------------------------------
// Domain 1 (Standard Movement) tests
// ----------------------------------------

// Basic test: Stationary case
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDopplerStandardBasic, "DspAp.Audio.Doppler.1_Standard.Basic", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDopplerStandardBasic::RunTest(const FString& Args)
{
    using namespace Metasound;
    FOperatorSettings Settings(48000, 100);

    // Inputs:
    FFloatReadRef speedOfSound = TDataReadReference<float>::CreateNew(34300.0f);
    FFloatReadRef observerVelocity = TDataReadReference<float>::CreateNew(0.0f);
    FFloatReadRef sourceVelocity = TDataReadReference<float>::CreateNew(0.0f);

    // Object setup:
    DopplerCalculator Calculator(Settings, speedOfSound, observerVelocity, sourceVelocity);
    FOutputVertexInterfaceData OutputData;
    Calculator.BindOutputs(OutputData);
    TDataReadReference<float> result = OutputData.GetDataReadReference<float>(METASOUND_GET_PARAM_NAME(DopplerNode::PitchMultiplierName));

    Calculator.Execute();

    // Math: (34300 + 0) / (34300 - 0) = 1.0
    TestEqual(TEXT("Stationary setup results in pitch 1.0"), *result, 1.0f);
    return true;
}

// Test with significant speed
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDopplerStandardHuge, "DspAp.Audio.Doppler.1_Standard.HugeData", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDopplerStandardHuge::RunTest(const FString& Args)
{
    using namespace Metasound;
    FOperatorSettings Settings(48000, 100);

    // Inputs:
    FFloatReadRef speedOfSound = TDataReadReference<float>::CreateNew(34300.0f);
    FFloatReadRef observerVelocity = TDataReadReference<float>::CreateNew(0.0f);
    FFloatReadRef sourceVelocity = TDataReadReference<float>::CreateNew(20000.0f);

    // Object setup:
    DopplerCalculator Calculator(Settings, speedOfSound, observerVelocity, sourceVelocity);
    FOutputVertexInterfaceData OutputData;
    Calculator.BindOutputs(OutputData);
    TDataReadReference<float> result = OutputData.GetDataReadReference<float>(METASOUND_GET_PARAM_NAME(DopplerNode::PitchMultiplierName));

    Calculator.Execute();

    // Math: 34300 / (34300 - 20000) = 34300 / 14300 = 2.3986
    float expected = 34300.0f / 14300.0f;
    TestEqual(TEXT("High speed setup results in correct positive pitch shift"), *result, expected);
    return true;
}

// Test with receding source (Negative velocity)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDopplerStandardEdge, "DspAp.Audio.Doppler.1_Standard.EdgeCase", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDopplerStandardEdge::RunTest(const FString& Args)
{
    using namespace Metasound;
    FOperatorSettings Settings(48000, 100);

    // Inputs:
    FFloatReadRef speedOfSound = TDataReadReference<float>::CreateNew(34300.0f);
    FFloatReadRef observerVelocity = TDataReadReference<float>::CreateNew(0.0f);
    FFloatReadRef sourceVelocity = TDataReadReference<float>::CreateNew(-5000.0f);
    
    // Object setup:
    DopplerCalculator Calculator(Settings, speedOfSound, observerVelocity, sourceVelocity);
    FOutputVertexInterfaceData OutputData;
    Calculator.BindOutputs(OutputData);
    TDataReadReference<float> result = OutputData.GetDataReadReference<float>(METASOUND_GET_PARAM_NAME(DopplerNode::PitchMultiplierName));

    Calculator.Execute();

    // Math: 34300 / (34300 - (-5000)) = 34300 / 39300 = 0.8727
    float expected = 34300.0f / 39300.0f;
    TestEqual(TEXT("Receding source correctly lowers pitch below 1.0"), *result, expected);
    return true;
}

// ----------------------------------------
// Domain 2 (Mutual Movement) tests
// ----------------------------------------

// Basic Mutual: Both moving toward each other
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDopplerMutualBasic, "DspAp.Audio.Doppler.2_Mutual.Basic", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDopplerMutualBasic::RunTest(const FString& Args)
{
    using namespace Metasound;
    FOperatorSettings Settings(48000, 100);

    // Inputs:
    FFloatReadRef speedOfSound = TDataReadReference<float>::CreateNew(34300.0f);
    FFloatReadRef observerVelocity = TDataReadReference<float>::CreateNew(1000.0f);
    FFloatReadRef sourceVelocity = TDataReadReference<float>::CreateNew(1000.0f);

    DopplerCalculator Calculator(Settings, speedOfSound, observerVelocity, sourceVelocity);
    FOutputVertexInterfaceData OutputData;
    Calculator.BindOutputs(OutputData);
    TDataReadReference<float> result = OutputData.GetDataReadReference<float>(METASOUND_GET_PARAM_NAME(DopplerNode::PitchMultiplierName));

    Calculator.Execute();

    // Math: (34300 + 1000) / (34300 - 1000) = 35300 / 33300 = 1.06006
    float expected = 35300.0f / 33300.0f;
    TestEqual(TEXT("Mutual approach changes pitch shift correctly."), *result, expected);
    return true;
}

// Huge Mutual: High speed chase (Both moving same direction)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDopplerMutualHuge, "DspAp.Audio.Doppler.2_Mutual.HugeData", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDopplerMutualHuge::RunTest(const FString& Args)
{
    using namespace Metasound;
    FOperatorSettings Settings(48000, 100);

    // Scenario: Observer (v=5000) chasing Source (v=5000). Relative speed is 0.
    // In my node moving away from source is negative moving toward is positive.
    // Inputs:
    FFloatReadRef speedOfSound = TDataReadReference<float>::CreateNew(34300.0f);
    FFloatReadRef observerVelocity = TDataReadReference<float>::CreateNew(5000.0f);
    FFloatReadRef sourceVelocity = TDataReadReference<float>::CreateNew(-5000.0f);

    // Object setup:
    DopplerCalculator Calculator(Settings, speedOfSound, observerVelocity, sourceVelocity);
    FOutputVertexInterfaceData OutputData;
    Calculator.BindOutputs(OutputData);
    TDataReadReference<float> result = OutputData.GetDataReadReference<float>(METASOUND_GET_PARAM_NAME(DopplerNode::PitchMultiplierName));

    Calculator.Execute();

    // Math: (34300 + 5000) / (34300 - (-5000)) = 39300 / 39300 = 1.0
    TestEqual(TEXT("High speed chase with 0 relative velocity keeps 1.0 pitch"), *result, 1.0f);
    return true;
}

// ----------------------------------------
// Domain 3 (Limits & Safety) tests
// ----------------------------------------

// Edge Case: Sonic speed (Division by zero)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDopplerLimitMach1, "DspAp.Audio.Doppler.3_Limits.Sonic", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDopplerLimitMach1::RunTest(const FString& Args)
{
    using namespace Metasound;
    FOperatorSettings Settings(48000, 100);

    // Inputs:
    FFloatReadRef speedOfSound = TDataReadReference<float>::CreateNew(34300.0f);
    FFloatReadRef observerVelocity = TDataReadReference<float>::CreateNew(0.0f);
    FFloatReadRef sourceVelocity = TDataReadReference<float>::CreateNew(34300.0f);

    // Object setup:
    DopplerCalculator Calculator(Settings, speedOfSound, observerVelocity, sourceVelocity);
    FOutputVertexInterfaceData OutputData;
    Calculator.BindOutputs(OutputData);
    TDataReadReference<float> result = OutputData.GetDataReadReference<float>(METASOUND_GET_PARAM_NAME(DopplerNode::PitchMultiplierName));

    Calculator.Execute();

    // Math: 34300 - 34300 = 0. Code logic should catch the 0.0001 threshold and return 1.0
    TestEqual(TEXT("Mach 1 source handles potential division by zero safely"), *result, 1.0f);
    return true;
}

// Edge Case: Supersonic
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDopplerLimitSupersonic, "DspAp.Audio.Doppler.3_Limits.Supersonic", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FDopplerLimitSupersonic::RunTest(const FString& Args)
{
    using namespace Metasound;
    FOperatorSettings Settings(48000, 100);

    // Inputs:
    FFloatReadRef speedOfSound = TDataReadReference<float>::CreateNew(34300.0f);
    FFloatReadRef observerVelocity = TDataReadReference<float>::CreateNew(0.0f);
    FFloatReadRef sourceVelocity = TDataReadReference<float>::CreateNew(40000.0f);

    DopplerCalculator Calculator(Settings, speedOfSound, observerVelocity, sourceVelocity);
    FOutputVertexInterfaceData OutputData;
    Calculator.BindOutputs(OutputData);
    TDataReadReference<float> result = OutputData.GetDataReadReference<float>(METASOUND_GET_PARAM_NAME(DopplerNode::PitchMultiplierName));

    Calculator.Execute();

    // Math: 34300 / (34300 - 40000) = 34300 / -5700 = -6.01.
    // Logic should clamp negative pitch results to 0.0 to avoid bugs
    TestEqual(TEXT("Supersonic travel is safely clamped to 0.0 to aviud crushes"), *result, 0.0f);
    return true;
}

#endif
