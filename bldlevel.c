// 20100808 AB entry for decoding in bldlevel
//

#if (defined __DEBUG_PMPRINTF__) || (defined __DEBUG_PMPRINTF_LEVEL2__) || (defined __DEBUG__)
    const char BldDatTim[] = "\n\rBuildDateTime "__DATE__" "__TIME__" DEBUG_BUILD";
#else
    const char BldDatTim[] = "\n\rBuildDateTime "__DATE__" "__TIME__;
#endif


