/* ========================================================================

   (C) Copyright 2023 by Molly Rocket, Inc., All Rights Reserved.
   
   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any damages
   arising from the use of this software.
   
   Please see https://computerenhance.com for more information
   
   ======================================================================== */

/* ========================================================================
   LISTING 91
   ======================================================================== */

#include "listing_0074_platform_metrics.cpp"

#ifndef PROFILER
#define PROFILER 0
#endif

#if PROFILER

struct profile_anchor
{
    u64 TSCElapsedExclusive; // NOTE(casey): Does NOT include children
    u64 TSCElapsedInclusive; // NOTE(casey): DOES include children
    u64 HitCount;
    char const *Label;
};
static profile_anchor GlobalProfilerAnchors[4096];
static u32 GlobalProfilerParent;

struct profile_block
{
    profile_block(char const *Label_, u32 AnchorIndex_)
    {
        ParentIndex = GlobalProfilerParent;
        
        AnchorIndex = AnchorIndex_;
        Label = Label_;

        profile_anchor *Anchor = GlobalProfilerAnchors + AnchorIndex;
        OldTSCElapsedInclusive = Anchor->TSCElapsedInclusive;
        
        GlobalProfilerParent = AnchorIndex;
        StartTSC = ReadCPUTimer();
    }
    
    ~profile_block(void)
    {
        u64 Elapsed = ReadCPUTimer() - StartTSC;
        GlobalProfilerParent = ParentIndex;
    
        profile_anchor *Parent = GlobalProfilerAnchors + ParentIndex;
        profile_anchor *Anchor = GlobalProfilerAnchors + AnchorIndex;
        
        Parent->TSCElapsedExclusive -= Elapsed;
        Anchor->TSCElapsedExclusive += Elapsed;
        Anchor->TSCElapsedInclusive = OldTSCElapsedInclusive + Elapsed;
        ++Anchor->HitCount;
        
        /* NOTE(casey): This write happens every time solely because there is no
           straightforward way in C++ to have the same ease-of-use. In a better programming
           language, it would be simple to have the anchor points gathered and labeled at compile
           time, and this repetative write would be eliminated. */
        Anchor->Label = Label;
    }
    
    char const *Label;
    u64 OldTSCElapsedInclusive;
    u64 StartTSC;
    u32 ParentIndex;
    u32 AnchorIndex;
};

#define NameConcat2(A, B) A##B
#define NameConcat(A, B) NameConcat2(A, B)
#define TimeBlock(Name) profile_block NameConcat(Block, __LINE__)(Name, __COUNTER__ + 1);
#define ProfilerEndOfCompilationUnit static_assert(__COUNTER__ < ArrayCount(GlobalProfilerAnchors), "Number of profile points exceeds size of profiler::Anchors array")

static void PrintTimeElapsed(u64 TotalTSCElapsed, profile_anchor *Anchor)
{
    f64 Percent = 100.0 * ((f64)Anchor->TSCElapsedExclusive / (f64)TotalTSCElapsed);
    printf("  %s[%llu]: %llu (%.2f%%", Anchor->Label, Anchor->HitCount, Anchor->TSCElapsedExclusive, Percent);
    if(Anchor->TSCElapsedInclusive != Anchor->TSCElapsedExclusive)
    {
        f64 PercentWithChildren = 100.0 * ((f64)Anchor->TSCElapsedInclusive / (f64)TotalTSCElapsed);
        printf(", %.2f%% w/children", PercentWithChildren);
    }
    printf(")\n");
}

static void PrintAnchorData(u64 TotalCPUElapsed)
{
    for(u32 AnchorIndex = 0; AnchorIndex < ArrayCount(GlobalProfilerAnchors); ++AnchorIndex)
    {
        profile_anchor *Anchor = GlobalProfilerAnchors + AnchorIndex;
        if(Anchor->TSCElapsedInclusive)
        {
            PrintTimeElapsed(TotalCPUElapsed, Anchor);
        }
    }
}

#else

#define TimeBlock(...)
#define PrintAnchorData(...)
#define ProfilerEndOfCompilationUnit

#endif

struct profiler
{
    u64 StartTSC;
    u64 EndTSC;
};
static profiler GlobalProfiler;

#define TimeFunction TimeBlock(__func__)

static void BeginProfile(void)
{
    GlobalProfiler.StartTSC = ReadCPUTimer();
}

static void EndAndPrintProfile()
{
    GlobalProfiler.EndTSC = ReadCPUTimer();
    u64 CPUFreq = EstimateCPUTimerFreq();
    
    u64 TotalCPUElapsed = GlobalProfiler.EndTSC - GlobalProfiler.StartTSC;
    
    if(CPUFreq)
    {
        printf("\nTotal time: %0.4fms (CPU freq %llu)\n", 1000.0 * (f64)TotalCPUElapsed / (f64)CPUFreq, CPUFreq);
    }
    
    PrintAnchorData(TotalCPUElapsed);
}
