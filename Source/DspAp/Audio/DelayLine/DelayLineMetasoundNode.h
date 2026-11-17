#pragma once

#include "CoreMinimal.h"
#include "MetasoundAudioBuffer.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundFacade.h"
#include "MetasoundNodeInterface.h"
#include "MetasoundPrimitives.h"

namespace Metasound
{

    // Fixed-size ring buffer that stores the last `capacity` samples.
    // The write head always points at the slot the *next* sample will go into,
    // so reading `n` samples behind the head returns the sample written n writes ago.
    struct CircularBuffer
    {
        int capacity{ 0 };
        TArray<float> data;
        int currentHead{ 0 };
        int currentTail{ 0 };

        // Constructor
        // Initializes the circular buffer by calculating its total capacity and allocating zeroed memory
        // parameters:
        //     - sampleRate : The sample rate of the audio
        //     - length : The maximum length of the delay line in seconds
        CircularBuffer(float sampleRate, float length)
            : capacity(FMath::Max(1, static_cast<int>(sampleRate * length)))
        {
            // Allocate once up front: nothing on the audio thread ever reallocates this buffer
            data.SetNumZeroed(capacity);
        }

        // Add() function
        // Adds a new audio sample to the buffer and advances the write head (currentHead)
        // parameters:
        //     - sample : The single audio float value to store in the buffer.
        void Add(float sample)
        {
            data[currentHead] = sample;
            // Increment the write head, using modulo so the index wraps once capacity is reached
            currentHead = (currentHead + 1) % capacity;
        }

        // Read() function
        // Retrieves a historical audio sample from the buffer based on the requested delay time
        // parameters:
        //     - delayInSeconds : How far back in time to read the sample
        //     - sampleRate : The current sample rate to convert time into array indices.
        // return:
        //     - the sample written `delayInSeconds * sampleRate` writes ago
        float Read(float delayInSeconds, float sampleRate)
        {
            // Convert the requested delay time from seconds into a discrete number of samples to look backwards
            int difference = static_cast<int>(delayInSeconds * sampleRate);

            // The delay comes straight from a user-facing pin, so it can be negative or longer than the buffer.
            // Clamp it so the read index can never leave the array.
            difference = FMath::Clamp(difference, 0, capacity - 1);

            // Calculate the tail position by subtracting the difference from the current head
            currentTail = currentHead - difference;

            // Wrap the tail pointer around to the top of the array if it drops below zero
            if (currentTail < 0)
            {
                currentTail += capacity;
            }
            return data[currentTail];
        }
    };


class DelayLine : public TExecutableOperator<DelayLine>
{
public:

    // Longest delay the node can produce, in seconds. Sets the size of the circular buffer.
    static constexpr float MaxDelaySeconds = 2.0f;

    // Feedback is clamped just below 1 so the loop always decays instead of growing without bound.
    static constexpr float MaxFeedback = 0.999f;

    static const FNodeClassMetadata& GetNodeInfo();
    static const FVertexInterface& DeclareVertexInterface();

    static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);

    DelayLine(const FOperatorSettings& InSettings,
        const FFloatReadRef& Delay,
        const FFloatReadRef& Feedback,
        const FAudioBufferReadRef& Input);

    virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
    virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;

    // Execute() function
    // The core DSP loop. Runs every audio block to process incoming samples, mix delayed audio, and apply feedback
    void Execute();

private:
    // Delay time in seconds
    FFloatReadRef DelayValue;

    // Feedback: from 0 to 1
    // 0 = no feedback (single echo)
    // 0.5 = delayed sound is multiplied by 0.5 and fed back, afterwards 0.25 then 0.125
    // values close to 1 = very long, slowly decaying tail
    FFloatReadRef FeedbackValue;

    FAudioBufferReadRef  AudioInput;
    FAudioBufferWriteRef AudioOutput;

    // Engine's sample rate, used to convert seconds into samples when looking into the circular buffer
    float m_SampleRate;

    CircularBuffer m_CircularBuffer;
};

using FDelayLineNode = TNodeFacade<DelayLine>;

}
