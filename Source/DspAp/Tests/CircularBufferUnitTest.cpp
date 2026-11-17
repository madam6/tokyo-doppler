#include "Misc/AutomationTest.h"
#include "../Audio/DelayLine/DelayLineMetasoundNode.h"

#if WITH_DEV_AUTOMATION_TESTS

// This test suite is separated into 5 specific domains crucial for circular buffer
// - Adding
// - Storing (basically complement to adding, more about data integrity)
// - Retrieving (ensuring that samples can be read correctly)
// - Wrap around logic (main trick of the data structure)
// - Safety (inputs that come from user-facing pins and could index out of bounds)
// Domains 1-4 are tested under 3 separate loads: basic case, huge amount of data, edge cases

// ----------------------------------------
// Domain 1 (Adding) tests
// ----------------------------------------


// Basic test of 1 element being added
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircularBufferAddBasic, "DspAp.Audio.CircularBuffer.1_Add.Basic", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCircularBufferAddBasic::RunTest(const FString& Args)
{
    Metasound::CircularBuffer cb(10.0f, 1.0f); // Capacity = 10
    cb.Add(27.1f);

    TestEqual(TEXT("Data is placed exactly at the initial head position 0"), cb.data[0], 27.1f);
    TestEqual(TEXT("Head correctly increments by 1 after adding"), cb.currentHead, 1);
    return true;
}

// Test with huge amount of data added to buffer
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircularBufferAddHuge, "DspAp.Audio.CircularBuffer.1_Add.HugeData", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCircularBufferAddHuge::RunTest(const FString& Args)
{
    Metasound::CircularBuffer cb(440000.0f, 10.0f); // Capacity = 4 400 000 (100 seconds of 44K sampleRate audio)

    // Add half a buffer's worth of data
    for (int i = 0; i < 2200000; i++)
    {
        cb.Add(static_cast<float>(i));
    }

    TestEqual(TEXT("Head index correctly handles millions of additions without breaking"), cb.currentHead, 2200000);
    return true;
}

// Tests when capacity is at absolute minimum = 1
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircularBufferAddEdge, "DspAp.Audio.CircularBuffer.1_Add.EdgeCase", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCircularBufferAddEdge::RunTest(const FString& Args)
{
    Metasound::CircularBuffer cb(1.0f, 1.0f);

    cb.Add(17.f);
    TestEqual(TEXT("Head immediately loops back to 0 when capacity is 1"), cb.currentHead, 0);

    cb.Add(34.f);
    TestEqual(TEXT("Buffer correctly overwrites its single memory slot"), cb.data[0], 34.f);
    return true;
}


// ----------------------------------------
// Domain 2 (Storing) tests
// ----------------------------------------

// Checking that elements correctly stored at respective indices
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircularBufferStoreBasic, "DspAp.Audio.CircularBuffer.2_Store.Basic", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCircularBufferStoreBasic::RunTest(const FString& Args)
{
    Metasound::CircularBuffer cb(10.0f, 1.0f);
    cb.Add(1.1f);
    cb.Add(2.2f);
    cb.Add(3.3f);

    // Check raw memory to ensure numbers aren't drifting or rounding
    TestEqual(TEXT("Index 0 stored accurately"), cb.data[0], 1.1f);
    TestEqual(TEXT("Index 1 stored accurately"), cb.data[1], 2.2f);
    TestEqual(TEXT("Index 2 stored accurately"), cb.data[2], 3.3f);
    return true;
}

// Same as previous, but on huge data
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircularBufferStoreHuge, "DspAp.Audio.CircularBuffer.2_Store.HugeData", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCircularBufferStoreHuge::RunTest(const FString& Args)
{
    Metasound::CircularBuffer cb(48000.0f, 2.0f); // Capacity = 96 000

    for (int i = 0; i < 96000; i++)
    {
        cb.Add(static_cast<float>(i) * 0.5f);
    }

    TestEqual(TEXT("Middle of huge buffer retains exact stored value"), cb.data[48000], 24000.0f);
    TestEqual(TEXT("End of huge buffer retains exact stored value"), cb.data[95999], 47999.5f);
    return true;
}

// Same as previous, but not storing extreme floating point values (silence, negative, high)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircularBufferStoreEdge, "DspAp.Audio.CircularBuffer.2_Store.EdgeCase", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCircularBufferStoreEdge::RunTest(const FString& Args)
{
    Metasound::CircularBuffer cb(10.0f, 1.0f);

    cb.Add(0.0f);
    cb.Add(-1.0f);
    cb.Add(99999.9f);

    TestEqual(TEXT("Stores pure silence (0.0f) safely"), cb.data[0], 0.0f);
    TestEqual(TEXT("Stores negative values safely"), cb.data[1], -1.0f);
    TestEqual(TEXT("Stores high amplitudes safely"), cb.data[2], 99999.9f);
    return true;
}


// ----------------------------------------
// Domain 3 (Retrieving) tests
// ----------------------------------------

// Basic test for historical samples
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircularBufferRetrieveBasic, "DspAp.Audio.CircularBuffer.3_Retrieve.Basic", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCircularBufferRetrieveBasic::RunTest(const FString& Args)
{
    float sampleRate = 10.0f;
    Metasound::CircularBuffer cb(sampleRate, 1.0f); // Capacity = 10

    for (int i = 0; i < 5; i++) { cb.Add(static_cast<float>(i)); } // Adds 0, 1, 2, 3, 4. Head is now at 5

    // Read from 0.2 seconds ago (2 samples ago). Should read index 3, which is 3.0f
    float retrieved = cb.Read(0.2f, sampleRate);
    TestEqual(TEXT("Retrieves historical sample correctly based on delay time"), retrieved, 3.0f);
    return true;
}

// Retrieval with huge amount of data
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircularBufferRetrieveHuge, "DspAp.Audio.CircularBuffer.3_Retrieve.HugeData", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCircularBufferRetrieveHuge::RunTest(const FString& Args)
{
    float sampleRate = 44000.0f;
    Metasound::CircularBuffer cb(sampleRate, 10.0f); // 10 second buffer

    // Fill with 5 seconds of data (220 000 samples)
    for (int i = 0; i < 220000; i++) { cb.Add(static_cast<float>(i)); }

    // Retrieve from exactly 4.0 seconds ago 
    // Head is at 220,000. Target index is 44,000. Value should be 44000.0f.
    float retrieved = cb.Read(4.0f, sampleRate);
    TestEqual(TEXT("Retrieves historical sample accurately over massive time scales"), retrieved, 44000.0f);
    return true;
}

// Testing edge case with 0 delay. Should read the sample exactly at the current head
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircularBufferRetrieveEdge, "DspAp.Audio.CircularBuffer.3_Retrieve.EdgeCase", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCircularBufferRetrieveEdge::RunTest(const FString& Args)
{
    float sampleRate = 10.0f;
    Metasound::CircularBuffer cb(sampleRate, 1.0f);

    for (int i = 0; i < 5; i++) { cb.Add(static_cast<float>(i)); }

    // Note: Since Add() advances the head *after* placing, 0 delay reads the empty slot waiting for the next sample.
    // Because we initialize with new float[capacity](), it should be exactly 0.0f.
    float zeroDelayRetrieved = cb.Read(0.0f, sampleRate);
    TestEqual(TEXT("0 delay securely retrieves the current write-head slot"), zeroDelayRetrieved, 0.0f);
    return true;
}


// ----------------------------------------
// Domain 4 (Wrap around) tests
// ----------------------------------------

// Basic wrap around case, testing head position + retrieved value
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircularBufferWrapBasic, "DspAp.Audio.CircularBuffer.4_WrapAround.Basic", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCircularBufferWrapBasic::RunTest(const FString& Args)
{
    float sampleRate = 10.0f;
    Metasound::CircularBuffer cb(sampleRate, 1.0f); // Capacity = 10

    // Add 12 items. Buffer should contain [10, 11, 2, 3, 4, 5, 6, 7, 8, 9]
    // Head is at index 2.
    for (int i = 0; i < 12; i++) { cb.Add(static_cast<float>(i)); }

    TestEqual(TEXT("Write head wrapped around boundary safely"), cb.currentHead, 2);

    // Read 0.4s ago (4 samples ago). Head is 2. 2 - 4 = -2. Wraps around to capacity-2 = 8
    // Index 8 contains 8.0f
    float retrieved = cb.Read(0.4f, sampleRate);
    TestEqual(TEXT("Read head wrapped backward mathematically correctly"), retrieved, 8.0f);
    return true;
}

// Wrap around logic after millions of additions
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircularBufferWrapHuge, "DspAp.Audio.CircularBuffer.4_WrapAround.HugeData", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCircularBufferWrapHuge::RunTest(const FString& Args)
{
    Metasound::CircularBuffer cb(10.0f, 1.0f); // Capacity = 10

    // Buffer wraps around 100 000 times
    for (int i = 0; i < 1000000; i++)
    {
        cb.Add(1.0f);
    }

    // 1 000 000 % 10 = 0
    TestEqual(TEXT("Modulo math remains stable after 100 000 wraps"), cb.currentHead, 0);
    return true;
}

// Wrap around from 0 to negative
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircularBufferWrapEdge, "DspAp.Audio.CircularBuffer.4_WrapAround.EdgeCase", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCircularBufferWrapEdge::RunTest(const FString& Args)
{
    float sampleRate = 10.0f;
    Metasound::CircularBuffer cb(sampleRate, 1.0f); // Capacity = 10

    for (int i = 0; i < 10; i++) { cb.Add(static_cast<float>(i)); } // Fills 0 through 9

    // Head is exactly at 0
    // Ask for 0.1s delay. Math: 0 - 1 = -1. Should wrap exactly to index 9
    // Index 9 holds 9.0f
    float retrieved = cb.Read(0.1f, sampleRate);
    TestEqual(TEXT("Safely calculates wrap exactly on the -1 array boundary edge"), retrieved, 9.0f);
    return true;
}

// ----------------------------------------
// Domain 5 (Safety) tests
// ----------------------------------------

// Delay times outside [0, buffer length) are clamped instead of reading outside the array
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircularBufferSafetyOutOfRange, "DspAp.Audio.CircularBuffer.5_Safety.OutOfRangeDelay", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCircularBufferSafetyOutOfRange::RunTest(const FString& Args)
{
    float sampleRate = 10.0f;
    Metasound::CircularBuffer cb(sampleRate, 1.0f); // Capacity = 10

    for (int i = 0; i < 10; i++) { cb.Add(static_cast<float>(i)); } // Fills 0 through 9, head back at 0

    // 5 seconds = 50 samples, far longer than the buffer. Clamped to capacity - 1 = 9 samples ago -> value 1
    TestEqual(TEXT("Delay longer than the buffer clamps to the oldest readable sample"), cb.Read(5.0f, sampleRate), 1.0f);

    // Negative delay clamps to 0 samples -> slot under the write head
    TestEqual(TEXT("Negative delay clamps to the write-head slot"), cb.Read(-1.0f, sampleRate), 0.0f);
    return true;
}

// A zero-length buffer would make the modulo in Add() divide by zero
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircularBufferSafetyZeroCapacity, "DspAp.Audio.CircularBuffer.5_Safety.ZeroCapacity", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FCircularBufferSafetyZeroCapacity::RunTest(const FString& Args)
{
    Metasound::CircularBuffer cb(0.0f, 1.0f);

    TestEqual(TEXT("Capacity never drops below one sample"), cb.capacity, 1);
    cb.Add(5.0f);
    TestEqual(TEXT("Single slot buffer still stores samples"), cb.Read(0.0f, 0.0f), 5.0f);
    return true;
}

#endif
