/* cpu_x86.cpp
 *
 * Author           : Alexander J. Yee
 * Date Created     : 04/12/2014
 * Last Modified    : 04/12/2014
 *
 */

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//  Dependencies
#include <iostream>
#include <cstring>
#include "cpu_x86.h"

#if _MSC_VER
 #include <Windows.h>
 #include <intrin.h>
#else
 #include <cpuid.h>
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace FeatureDetector{
    using std::cout;
    using std::endl;
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void cpu_x86::print(const char* label, bool yes){
    cout << label;
    cout << (yes ? "Yes" : "No") << endl;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#if _MSC_VER
    void cpu_x86::cpuid(int32_t out[4], int32_t x){
        __cpuidex(out, x, 0);
    }
    uint64_t xgetbv(unsigned int x){
        return _xgetbv(x);
    }
#else
    void cpu_x86::cpuid(int32_t out[4], int32_t x){
        __cpuid_count(x, 0, out[0], out[1], out[2], out[3]);
    }
    uint64_t xgetbv(unsigned int index){
        uint32_t eax, edx;
        __asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(index));
        return ((uint64_t)edx << 32) | eax;
    }
    #define _XCR_XFEATURE_ENABLED_MASK  0
#endif

bool cpu_x86::detect_OS_x64(){
    return sizeof(void*)==8;
}

cpu_x86::cpu_x86(){
    memset(this, 0, sizeof(*this));
}
bool cpu_x86::detect_OS_AVX(){
    //  Copied from: http://stackoverflow.com/a/22521619/922184

    bool avxSupported = false;

    int cpuInfo[4];
    cpuid(cpuInfo, 1);

    bool osUsesXSAVE_XRSTORE = (cpuInfo[2] & (1 << 27)) != 0;
    bool cpuAVXSuport = (cpuInfo[2] & (1 << 28)) != 0;

    if (osUsesXSAVE_XRSTORE && cpuAVXSuport)
    {
        uint64_t xcrFeatureMask = xgetbv(_XCR_XFEATURE_ENABLED_MASK);
        avxSupported = (xcrFeatureMask & 0x6) == 0x6;
    }

    return avxSupported;
}
bool cpu_x86::detect_OS_AVX512(){
    if (!detect_OS_AVX())
        return false;

    uint64_t xcrFeatureMask = xgetbv(_XCR_XFEATURE_ENABLED_MASK);
    return (xcrFeatureMask & 0xe6) == 0xe6;
}
std::string cpu_x86::get_vendor_string(){
    int32_t CPUInfo[4];
    char name[13];

    cpuid(CPUInfo, 0);
    memcpy(name + 0, &CPUInfo[1], 4);
    memcpy(name + 4, &CPUInfo[3], 4);
    memcpy(name + 8, &CPUInfo[2], 4);
    name[12] = '\0';

    return name;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void cpu_x86::detect_host(){
    //  OS Features
    OS_x64 = detect_OS_x64();
    OS_AVX = detect_OS_AVX();
    OS_AVX512 = detect_OS_AVX512();

    //  Vendor
    std::string vendor(get_vendor_string());
    if (vendor == "GenuineIntel"){
        Vendor_Intel = true;
    }else if (vendor == "AuthenticAMD"){
        Vendor_AMD = true;
    }

    int info[4];
    cpuid(info, 0);
    int nIds = info[0];

    cpuid(info, 0x80000000);
    uint32_t nExIds = info[0];

    //  Detect Features
    if (nIds >= 0x00000001){
        cpuid(info, 0x00000001);
        HW_MMX    = (info[3] & ((int)1 << 23)) != 0;
        HW_SSE    = (info[3] & ((int)1 << 25)) != 0;
        HW_SSE2   = (info[3] & ((int)1 << 26)) != 0;
        HW_SSE3   = (info[2] & ((int)1 <<  0)) != 0;

        HW_SSSE3  = (info[2] & ((int)1 <<  9)) != 0;
        HW_SSE41  = (info[2] & ((int)1 << 19)) != 0;
        HW_SSE42  = (info[2] & ((int)1 << 20)) != 0;
        HW_AES    = (info[2] & ((int)1 << 25)) != 0;

        HW_AVX    = (info[2] & ((int)1 << 28)) != 0;
        HW_FMA3   = (info[2] & ((int)1 << 12)) != 0;

        HW_RDRAND = (info[2] & ((int)1 << 30)) != 0;
    }
    if (nIds >= 0x00000007){
        cpuid(info, 0x00000007);
        HW_AVX2         = (info[1] & ((int)1 <<  5)) != 0;

        HW_BMI1         = (info[1] & ((int)1 <<  3)) != 0;
        HW_BMI2         = (info[1] & ((int)1 <<  8)) != 0;
        HW_ADX          = (info[1] & ((int)1 << 19)) != 0;
        HW_MPX          = (info[1] & ((int)1 << 14)) != 0;
        HW_SHA          = (info[1] & ((int)1 << 29)) != 0;
        HW_PREFETCHWT1  = (info[2] & ((int)1 <<  0)) != 0;

        HW_AVX512_F     = (info[1] & ((int)1 << 16)) != 0;
        HW_AVX512_CD    = (info[1] & ((int)1 << 28)) != 0;
        HW_AVX512_PF    = (info[1] & ((int)1 << 26)) != 0;
        HW_AVX512_ER    = (info[1] & ((int)1 << 27)) != 0;
        HW_AVX512_VL    = (info[1] & ((int)1 << 31)) != 0;
        HW_AVX512_BW    = (info[1] & ((int)1 << 30)) != 0;
        HW_AVX512_DQ    = (info[1] & ((int)1 << 17)) != 0;
        HW_AVX512_IFMA  = (info[1] & ((int)1 << 21)) != 0;
        HW_AVX512_VBMI  = (info[2] & ((int)1 <<  1)) != 0;
    }
    if (nExIds >= 0x80000001){
        cpuid(info, 0x80000001);
        HW_x64   = (info[3] & ((int)1 << 29)) != 0;
        HW_ABM   = (info[2] & ((int)1 <<  5)) != 0;
        HW_SSE4a = (info[2] & ((int)1 <<  6)) != 0;
        HW_FMA4  = (info[2] & ((int)1 << 16)) != 0;
        HW_XOP   = (info[2] & ((int)1 << 11)) != 0;
    }
}
void cpu_x86::print() const{
    cout << "CPU Vendor:" << endl;
    print("    AMD         = ", Vendor_AMD);
    print("    Intel       = ", Vendor_Intel);
    cout << endl;

    cout << "OS Features:" << endl;
#ifdef _WIN32
    print("    64-bit      = ", OS_x64);
#endif
    print("    OS AVX      = ", OS_AVX);
    print("    OS AVX512   = ", OS_AVX512);
    cout << endl;

    cout << "Hardware Features:" << endl;
    print("    MMX         = ", HW_MMX);
    print("    x64         = ", HW_x64);
    print("    ABM         = ", HW_ABM);
    print("    RDRAND      = ", HW_RDRAND);
    print("    BMI1        = ", HW_BMI1);
    print("    BMI2        = ", HW_BMI2);
    print("    ADX         = ", HW_ADX);
    print("    MPX         = ", HW_MPX);
    print("    PREFETCHWT1 = ", HW_PREFETCHWT1);
    cout << endl;

    cout << "SIMD: 128-bit" << endl;
    print("    SSE         = ", HW_SSE);
    print("    SSE2        = ", HW_SSE2);
    print("    SSE3        = ", HW_SSE3);
    print("    SSSE3       = ", HW_SSSE3);
    print("    SSE4a       = ", HW_SSE4a);
    print("    SSE4.1      = ", HW_SSE41);
    print("    SSE4.2      = ", HW_SSE42);
    print("    AES-NI      = ", HW_AES);
    print("    SHA         = ", HW_SHA);
    cout << endl;

    cout << "SIMD: 256-bit" << endl;
    print("    AVX         = ", HW_AVX);
    print("    XOP         = ", HW_XOP);
    print("    FMA3        = ", HW_FMA3);
    print("    FMA4        = ", HW_FMA4);
    print("    AVX2        = ", HW_AVX2);
    cout << endl;

    cout << "SIMD: 512-bit" << endl;
    print("    AVX512-F    = ", HW_AVX512_F);
    print("    AVX512-CD   = ", HW_AVX512_CD);
    print("    AVX512-PF   = ", HW_AVX512_PF);
    print("    AVX512-ER   = ", HW_AVX512_ER);
    print("    AVX512-VL   = ", HW_AVX512_VL);
    print("    AVX512-BW   = ", HW_AVX512_BW);
    print("    AVX512-DQ   = ", HW_AVX512_DQ);
    print("    AVX512-IFMA = ", HW_AVX512_IFMA);
    print("    AVX512-VBMI = ", HW_AVX512_VBMI);
    cout << endl;

    cout << "Summary:" << endl;
    print("    Safe to use AVX:     ", HW_AVX && OS_AVX);
    print("    Safe to use AVX512:  ", HW_AVX512_F && OS_AVX512);
    cout << endl;
}
void cpu_x86::print_host(){
    cpu_x86 features;
    features.detect_host();
    features.print();
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
}
