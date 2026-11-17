#pragma once

#include "HAL/Platform.h"
#include "MetasoundAudioBuffer.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundFacade.h"
#include "MetasoundNodeInterface.h"
#include "MetasoundPrimitives.h"
#include <cmath>

namespace Metasound
{

    struct CircularBuffer
    {
        int capacity{ 0 };
        float* data{ nullptr };
        int currentHead{ 0 };
        int currentTail{ 0 };

        // Constructor
        // Initializes the circular buffer by calculating its total capacity and allocating memory
        // parameters:
        //     - sampleRate : The sample rate of the audio
        //     - length : The maximum length of the delay line in seconds
        // return:
        //     - none
        CircularBuffer(float sampleRate, float length)
            : capacity(static_cast<int>((sampleRate* length)))
            , data(new float[capacity]())
            , currentHead(0)
            , currentTail(0)
        {
        };

        // Destructor
        // Frees the heap allocated array from memory to prevent leaks
        // parameters:
        //     - none
        // return:
        //     - none
        ~CircularBuffer()
        {
            delete[] data;
        };

        // Add() function
        // Adds a new audio sample to the buffer and safely advances the write head (currentHead)
        // parameters:
        //     - sample : The single audio float value to store in the buffer.
        // return:
        //     - none
        void Add(float sample)
        {
            data[currentHead] = sample;
            // Safely increment the write head, by using modulo operator making sure that index wraps if capacity is achieved
            currentHead = (currentHead + 1) % capacity;
        };

        // Read() function
        // Retrieves a historical audio sample from the buffer based on the requested delay time
        // parameters:
        //     - delayInSeconds : How far back in time to read the sample
        //     - sampleRate : The current sample rate to convert time into array indices.
        // return:
        //     - none
        float Read(float delayInSeconds, float sampleRate)
        {
            // Convert the requested delay time from seconds into a discrete number of samples (array indices) to look backwards
            int difference = static_cast<int>((delayInSeconds * sampleRate));
            
            // Calculate the tail position by subtracting the difference from the current head
            currentTail = currentHead - difference;

            // Safely wrap the tail pointer around to the top of the array if it drops below zero
            currentTail < 0 ? currentTail += capacity : currentTail;
            return data[currentTail];
        }
    };


class DelayLine : public TExecutableOperator<DelayLine>
{
public:

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
    CircularBuffer m_CircularBuffer;

    // Engine's sample rate, used to convert seconds into sample when looking into circular buffer
    float m_SampleRate;

    // Delay time in seconds
    FFloatReadRef DelayValue;

    // Feedback: from 0 to 1
    // 0 = no feedback
    // 0.5 = delayed sound is multiplied by 0.5 and fed back, afterwards 0.25 then 0.125
    // 1.0 = infinte feedback
    FFloatReadRef FeedbackValue;

    FAudioBufferReadRef	 AudioInput;
    FAudioBufferWriteRef AudioOutput;
};

using FDelayLineNode = TNodeFacade<DelayLine>;

}
