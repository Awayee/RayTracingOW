#include "Test/TestCase.h"
#include "Math/MathUtil.h"
#include "Core/Log.h"

void EstimitatePI(){
    // Surpose r = 1
    int NumInsideCircle = 0;
    constexpr int N = 100000;
    for(int i=0; i<N; ++i){
        float x = Math::Random(-1.0f, 1.0f);
        float y = Math::Random(-1.0f, 1.0f);
        if(x*x + y*y < 1.0f){
            ++NumInsideCircle;
        }
    }
    float EstimitateOfPI = 4.0f * (float)NumInsideCircle / (float)N;
    LOG_INFO("Estimitate of PI = %f", EstimitateOfPI);
}

void EmisitatePILoop(){
    constexpr int N = 100000;
    int NumRuns = 0;
    int NumInsideCircle = 0;
    while (true){
        ++NumRuns;
        float x = Math::Random(-1.0f, 1.0f);
        float y = Math::Random(-1.0f, 1.0f);
        if(x*x + y*y < 1.0f){
            ++NumInsideCircle;
        }
        if(NumRuns % N == 0){
            float EstimitateOfPI = 4.0f * (float)NumInsideCircle / (float)NumRuns;
            LOG_INFO("Estimitate of PI = %f", EstimitateOfPI);
        }
    }
}

void EstimitatePIStratified(){
    // Surpose r = 1
    int NumInsideCircle = 0;
    int NumInsideCircleStratified = 0;
    constexpr int N = 100000;
    constexpr int SqrtN = 100;
    for(int j=0; j<SqrtN; ++j){
        for(int i=0; i<SqrtN; ++i){
            float x = Math::Random(-1.0f, 1.0f);
            float y = Math::Random(-1.0f, 1.0f);
            if(x*x + y*y < 1.0f){
                ++NumInsideCircle;
            }

            x = (j + Math::Random01()) / (float)SqrtN * 2.0f - 1.0f;
            y = (i + Math::Random01()) / (float)SqrtN * 2.0f - 1.0f;
            if(x*x + y*y < 1.0f){
                ++NumInsideCircleStratified;
            }
        }
    }
    float EstimitateOfPI = 4.0f * (float)NumInsideCircle / (float)(SqrtN * SqrtN);
    float EstimitateOfPIStratified =  4.0f * (float)NumInsideCircleStratified / (float)(SqrtN * SqrtN);
    LOG_INFO("Estimitate of PI = %f", EstimitateOfPI);
    LOG_INFO("Stratified Estimitate of PI = %f", EstimitateOfPIStratified);
}

TestCaseSet::TestCaseSet() {
}

TestCaseSet::~TestCaseSet() {
}

void TestCaseSet::Run() {
    EstimitatePIStratified();
}
