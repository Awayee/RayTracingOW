#include "Test/TestCase.h"
#include "Math/MathUtil.h"
#include "Core/Log.h"
#include <algorithm>
#include <vector>

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

float MontcaloIntegrator(float (*Func)(float), float A, float B){
    constexpr int N = 100000;
    float Sum = 0.0f;
    for(int i=0; i<N; ++i){
        float X = Math::Random(A, B);
        Sum += Func(X);
    }
    float E = Sum / N;
    float I = (B - A) * E;
    return I;
}

void IntegrateXPower2(){
    const float I = MontcaloIntegrator([](float X){return X * X;}, 0.0f, 2.0f);
    printf("Integration of x^2 = %f", I);
}

void IntegrateSinX(){
    const float I = MontcaloIntegrator([](float X){return Math::Log(Math::Sin(X));}, 0.0f, 2.0f);
    printf("Integration of ln(sin(x)) = %f", I);
}

// Find the half point of PDF in [0, 2PI]
void TestFindHalfPointOfPDF(){
    struct FSample{
        float X;
        float PX;
    };
    const int32 N = 100000;
    std::vector<FSample> Samples;
    Samples.resize(N);
    float Sum = 0.0f;
    
    for(int32 i=0; i<N; ++i){
        // Calc area.
        float x = Math::Random(0.0f, 2.0f * Math::PI);
        float SinX = Math::Sin(x);
        float px = Math::Exp(-x / (2.0f * Math::PI)) * SinX * SinX;
        Sum += px;

        // cache samples
        Samples[i] = FSample{x, px};
    }

    // Sort samples by x.
    std::sort(Samples.begin(), Samples.end(), [](const FSample& L, const FSample& R){return L.X < R.X;});

    float HalfSum = Sum * 0.5f;
    float HalfwayPoint = 0.0f;
    float Accum = 0.0f;
    for(int32 i=0; i<N; ++i){
        Accum += Samples[i].PX;
        if(Accum >= HalfSum){
            HalfwayPoint = Samples[i].X;
            break;
        }
    }

    printf("Averange = %f\n", Sum / N);
    printf("Area under curve = %f\n", Sum / N * 2.0f * Math::PI);
    printf("Halfway = %f\n", HalfwayPoint);
}

// Importance sampling

inline float ICD(float d){
    return 2.0f * d;
}
inline float PDF(float x){
    return 0.5f;
}
inline float ICDLinear(float d){
    return Math::Sqrt(4.0f * d);
}
inline float PDFLinear(float x){
    return 0.5f * x;
}
inline float ICDQuadratic(float d){
    return 8.0f * Math::Pow(d, 1.0f/3.0f);
}
inline float PDFQuadratic(float x){
    return 3.0f / 8.0f * x * x;
}

void TestPDFXPower2(){
    const int32 N = 1;
    float Sum = 0.0f;
    for (int i=0; i<N; ++i){
        float Rand = Math::Random01();
        if(Rand == 0.0f){
            continue;
        }
        float x = ICDQuadratic(Rand);
        float pdf = PDFQuadratic(x);
        Sum += x * x / pdf;
    }
    printf("I = %f\n", Sum / N);
}

// Test for RandomCosineDirection
void TestForRandomCosineDirection(){
    auto f = [](const Math::FVector3& D){
        float CosTheta = D.Z;
        return CosTheta * CosTheta * CosTheta;
    };

    auto pdf = [](const Math::FVector3& D){
        return D.Z / Math::PI;
    };

    constexpr int32 N = 100000;
    float Sum = 0.0f;
    for(int32 i=0; i<N; ++i){
        Math::FVector3 D = Math::RandomCosineDirection();
        Sum += f(D) / pdf(D);
    }
    
    printf("PI/2=%f, Estimate=%f\n", Math::PI / 2.0f, Sum / N);
}

TestCaseSet::TestCaseSet() {
}

TestCaseSet::~TestCaseSet() {
}

void TestCaseSet::Run() {
    TestForRandomCosineDirection();
}
