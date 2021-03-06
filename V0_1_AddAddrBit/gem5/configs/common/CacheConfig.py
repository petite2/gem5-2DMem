# Copyright (c) 2012-2013, 2015-2016 ARM Limited
# All rights reserved
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2010 Advanced Micro Devices, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Lisa Hsu

# Configure the M5 cache hierarchy config in one place
#

import m5
from m5.objects import *
from Caches import *

def config_cache(options, system):
    if options.external_memory_system and (options.caches or options.l2cache):
        print "External caches and internal caches are exclusive options.\n"
        sys.exit(1)

    if options.external_memory_system:
        ExternalCache = ExternalCacheFactory(options.external_memory_system)

    if options.cpu_type == "arm_detailed":
        try:
            from O3_ARM_v7a import *
        except:
            print "arm_detailed is unavailable. Did you compile the O3 model?"
            sys.exit(1)

        dcache_class, icache_class, l2_cache_class, walk_cache_class = \
            O3_ARM_v7a_DCache, O3_ARM_v7a_ICache, O3_ARM_v7aL2, \
            O3_ARM_v7aWalkCache
    else:
        dcache_class, icache_class, l2_cache_class, walk_cache_class = \
            L1_DCache, L1_ICache, L2Cache, None

        if buildEnv['TARGET_ISA'] == 'x86':
            walk_cache_class = PageTableWalkerCache

    # Set the cache line size of the system
    system.cache_line_size = options.cacheline_size

    # If elastic trace generation is enabled, make sure the memory system is
    # minimal so that compute delays do not include memory access latencies.
    # Configure the compulsory L1 caches for the O3CPU, do not configure
    # any more caches.
    if options.l2cache and options.elastic_trace_en:
        fatal("When elastic trace is enabled, do not configure L2 caches.")

    if options.l2cache:
        # Provide a clock for the L2 and the L1-to-L2 bus here as they
        # are not connected using addTwoLevelCacheHierarchy. Use the
        # same clock as the CPUs.
        # MJL_Begin
        l2_tag_latency = 6
        l2_data_latency = 9
        if options.l2_size in ["2MB"]:
            l2_tag_latency = 8
            l2_data_latency = 12
        if options.MJL_L2sameSetMapping:
            MJL_ignore_extra_tag_check_latecy = options.MJL_L2sameSetMapping
        else:
            MJL_ignore_extra_tag_check_latecy = options.MJL_noLatOverhead
        # MJL_End 
        system.l2 = l2_cache_class(clk_domain=system.cpu_clk_domain,
                                   size=options.l2_size,
                                   assoc=options.l2_assoc\
                                   # MJL_Begin
                                   , tag_latency=l2_tag_latency
                                   , data_latency=l2_data_latency
                                   , MJL_2D_Cache=options.MJL_2DL2Cache\
                                   , MJL_timeStep=options.MJL_timeStep\
                                   , MJL_2D_Transfer_Type=options.MJL_2DL2TransferType\
                                   , MJL_extra2DWrite_latency=options.MJL_extra2DWrite_latency\
                                   , MJL_has2DLLC=options.MJL_2DL2Cache\
                                   , sequential_access=True\
                                   , MJL_sameSetMapping=options.MJL_L2sameSetMapping\
                                   , MJL_oracleProxy=options.MJL_oracleProxy\
                                   , MJL_oracleProxyReplay=options.MJL_oracleProxyReplay\
                                   , MJL_ignoreExtraTagCheckLatency=MJL_ignore_extra_tag_check_latecy\
                                   , MJL_bloomFilterSize=options.MJL_l2_bloomFilterSize\
                                   , MJL_bloomFilterHashFuncId=options.MJL_l2_bloomFilterHashFuncId\
                                   # MJL_End
                                   )

        system.tol2bus = L2XBar(clk_domain = system.cpu_clk_domain)
        system.l2.cpu_side = system.tol2bus.master
        system.l2.mem_side = system.membus.slave
        # MJL_Begin
        if options.l3cache or options.l3cacheWithPrivateL2s:
            fatal("Having l3 implies having l2, shouldn't have both flags set at the same time")
        if options.MJL_2DL2Cache and options.MJL_L2sameSetMapping:
            fatal("Physically 2D caches option does not coexist with same set mapping option")
        if options.MJL_Prefetcher:
            system.l2.prefetcher = L2StridePrefetcher(MJL_colPf = options.MJL_colPf, MJL_pfBasedPredictDir = options.MJL_pfBasedPredictDir) 
        # MJL_End
    # MJL_Begin
    if options.l3cache:
        # Provide a clock for the L2 and the L1-to-L2 bus here as they
        # are not connected using addTwoLevelCacheHierarchy. Use the
        # same clock as the CPUs.
        if options.MJL_L2sameSetMapping:
            MJL_l2_ignore_extra_tag_check_latecy = options.MJL_L2sameSetMapping
        else:
            MJL_l2_ignore_extra_tag_check_latecy = options.MJL_noLatOverhead

        system.l2 = l2_cache_class(clk_domain=system.cpu_clk_domain,
                                   size=options.l2_size,
                                   assoc=options.l2_assoc\
                                   # MJL_Begin
                                   , tag_latency=6
                                   , data_latency=9
                                   , MJL_timeStep=options.MJL_timeStep\
                                   , MJL_has2DLLC=options.MJL_2DL2Cache\
                                   , sequential_access=True\
                                   , MJL_sameSetMapping=options.MJL_L2sameSetMapping\
                                   , MJL_oracleProxy=options.MJL_oracleProxy\
                                   , MJL_oracleProxyReplay=options.MJL_oracleProxyReplay\
                                   , MJL_ignoreExtraTagCheckLatency=MJL_l2_ignore_extra_tag_check_latecy\
                                   , MJL_bloomFilterSize=options.MJL_l2_bloomFilterSize\
                                   , MJL_bloomFilterHashFuncId=options.MJL_l2_bloomFilterHashFuncId\
                                   # MJL_End
                                   )

        if options.MJL_L3sameSetMapping:
            MJL_l3_ignore_extra_tag_check_latecy = options.MJL_L3sameSetMapping
        else:
            MJL_l3_ignore_extra_tag_check_latecy = options.MJL_noLatOverhead

        system.l3 = l2_cache_class(clk_domain=system.cpu_clk_domain,
                                   size=options.l3_size,
                                   assoc=options.l3_assoc\
                                   # MJL_Begin
                                   , tag_latency=8
                                   , data_latency=12
                                   , response_latency=20
                                   , mshrs=20
                                   , tgts_per_mshr=12
                                   , write_buffers=8
                                   , MJL_2D_Cache=options.MJL_2DL2Cache\
                                   , MJL_timeStep=options.MJL_timeStep\
                                   , MJL_2D_Transfer_Type=options.MJL_2DL2TransferType\
                                   , MJL_extra2DWrite_latency=options.MJL_extra2DWrite_latency\
                                   , MJL_has2DLLC=options.MJL_2DL2Cache\
                                   , sequential_access=True\
                                   , MJL_sameSetMapping=options.MJL_L3sameSetMapping\
                                   , MJL_oracleProxy=options.MJL_oracleProxy\
                                   , MJL_oracleProxyReplay=options.MJL_oracleProxyReplay\
                                   , MJL_ignoreExtraTagCheckLatency=MJL_l3_ignore_extra_tag_check_latecy\
                                   , MJL_bloomFilterSize=options.MJL_l3_bloomFilterSize\
                                   , MJL_bloomFilterHashFuncId=options.MJL_l3_bloomFilterHashFuncId\
                                   # MJL_End
                                   )

        system.tol2bus = L2XBar(clk_domain = system.cpu_clk_domain)
        system.tol3bus = L2XBar(clk_domain = system.cpu_clk_domain, width=32)
        
        system.l2.cpu_side = system.tol2bus.master
        system.l2.mem_side = system.tol3bus.slave

        system.l3.cpu_side = system.tol3bus.master
        system.l3.mem_side = system.membus.slave

        if options.l2cache or options.l3cacheWithPrivateL2s:
            fatal("Having l3 implies having l2, shouldn't have both flags set at the same time")
        if options.MJL_2DL2Cache and options.MJL_L3sameSetMapping:
            fatal("Physically 2D caches option does not coexist with same set mapping option")
        if options.MJL_Prefetcher:
            system.l3.prefetcher = L2StridePrefetcher(MJL_colPf = options.MJL_colPf, MJL_pfBasedPredictDir = options.MJL_pfBasedPredictDir) 

    if options.l3cacheWithPrivateL2s:
        if options.MJL_L3sameSetMapping:
            MJL_l3_ignore_extra_tag_check_latecy = options.MJL_L3sameSetMapping
        else:
            MJL_l3_ignore_extra_tag_check_latecy = options.MJL_noLatOverhead
        system.l3 = l2_cache_class(clk_domain=system.cpu_clk_domain,
                                   size=options.l3_size,
                                   assoc=options.l3_assoc\
                                   # MJL_Begin
                                   , tag_latency=8
                                   , data_latency=12
                                   , response_latency=20
                                   , mshrs=20
                                   , tgts_per_mshr=12
                                   , write_buffers=8
                                   , MJL_2D_Cache=options.MJL_2DL2Cache\
                                   , MJL_timeStep=options.MJL_timeStep\
                                   , MJL_2D_Transfer_Type=options.MJL_2DL2TransferType\
                                   , MJL_extra2DWrite_latency=options.MJL_extra2DWrite_latency\
                                   , MJL_has2DLLC=options.MJL_2DL2Cache\
                                   , sequential_access=True\
                                   , MJL_sameSetMapping=options.MJL_L3sameSetMapping\
                                   , MJL_oracleProxy=options.MJL_oracleProxy\
                                   , MJL_oracleProxyReplay=options.MJL_oracleProxyReplay\
                                   , MJL_ignoreExtraTagCheckLatency=MJL_l3_ignore_extra_tag_check_latecy\
                                   , MJL_bloomFilterSize=options.MJL_l3_bloomFilterSize\
                                   , MJL_bloomFilterHashFuncId=options.MJL_l3_bloomFilterHashFuncId\
                                   # MJL_End
                                   )
        system.tol3bus = L2XBar(clk_domain = system.cpu_clk_domain, width=32)
        system.l3.cpu_side = system.tol3bus.master
        system.l3.mem_side = system.membus.slave
        if options.l2cache or options.l3cache:
            fatal("Having l3 implies having l2, shouldn't have both flags set at the same time")
        if options.MJL_2DL2Cache and options.MJL_L3sameSetMapping:
            fatal("Physically 2D caches option does not coexist with same set mapping option")
        if options.MJL_Prefetcher:
            system.l3.prefetcher = L2StridePrefetcher(MJL_colPf = options.MJL_colPf, MJL_pfBasedPredictDir = options.MJL_pfBasedPredictDir) 
    # MJL_End

    if options.memchecker:
        system.memchecker = MemChecker()

    for i in xrange(options.num_cpus):
        if options.caches:
            icache = icache_class(size=options.l1i_size,
                                  assoc=options.l1i_assoc\
                                  # MJL_Begin
                                  , MJL_timeStep=options.MJL_timeStep\
                                  # MJL_End
                                  )
            # MJL_Begin
            if options.MJL_L1sameSetMapping:
                MJL_ignore_extra_tag_check_latecy = options.MJL_L1sameSetMapping
            else:
                MJL_ignore_extra_tag_check_latecy = options.MJL_noLatOverhead
            # MJL_End
            dcache = dcache_class(size=options.l1d_size,
                                  assoc=options.l1d_assoc\
                                  # MJL_Begin
                                  , MJL_PC2DirFile=options.MJL_PC2DirFile\
                                  , MJL_VecListFile=options.MJL_VecListFile\
                                  , MJL_timeStep=options.MJL_timeStep\
                                  , MJL_has2DLLC=options.MJL_2DL2Cache\
                                  , MJL_predictDir=options.MJL_predictDir\
                                  , MJL_mshrPredictDir=options.MJL_mshrPredictDir\
                                  , MJL_pfBasedPredictDir=options.MJL_pfBasedPredictDir\
                                  , MJL_sameSetMapping=options.MJL_L1sameSetMapping\
                                  , MJL_oracleProxy=options.MJL_oracleProxy\
                                  , MJL_oracleProxyReplay=options.MJL_oracleProxyReplay\
                                  , MJL_ignoreExtraTagCheckLatency=MJL_ignore_extra_tag_check_latecy\
                                  , MJL_bloomFilterSize=options.MJL_l1d_bloomFilterSize\
                                  , MJL_bloomFilterHashFuncId=options.MJL_l1d_bloomFilterHashFuncId\
                                  # MJL_End
                                  )
            # MJL_Begin
            if options.MJL_mshrPredictDir and not options.MJL_predictDir:
                fatal("Cannot use mshr scheme for prediction when prediction is not enabled")
            if options.MJL_pfBasedPredictDir and not options.MJL_predictDir:
                fatal("Cannot use prefetch scheme for prediction when prediction is not enabled")
            if options.MJL_pfBasedPredictDir and not options.MJL_Prefetcher:
                fatal("Cannot use prefetch scheme for prediction when prefetcher is not enabled")
            if options.MJL_mshrPredictDir and options.MJL_pfBasedPredictDir:
                fatal("Cannot use prefetch and mshr scheme for prediction at the same time")
            # MJL_End

            # If we have a walker cache specified, instantiate two
            # instances here
            if walk_cache_class:
                iwalkcache = walk_cache_class()
                dwalkcache = walk_cache_class()
            else:
                iwalkcache = None
                dwalkcache = None

            if options.memchecker:
                dcache_mon = MemCheckerMonitor(warn_only=True)
                dcache_real = dcache

                # Do not pass the memchecker into the constructor of
                # MemCheckerMonitor, as it would create a copy; we require
                # exactly one MemChecker instance.
                dcache_mon.memchecker = system.memchecker

                # Connect monitor
                dcache_mon.mem_side = dcache.cpu_side

                # Let CPU connect to monitors
                dcache = dcache_mon

            # MJL_Begin
            if options.l3cacheWithPrivateL2s:
                if options.MJL_L2sameSetMapping:
                    MJL_l2_ignore_extra_tag_check_latecy = options.MJL_L2sameSetMapping
                else:
                    MJL_l2_ignore_extra_tag_check_latecy = options.MJL_noLatOverhead
                l2cache = l2_cache_class(clk_domain=system.cpu_clk_domain,
                                   size=options.l2_size,
                                   assoc=options.l2_assoc\
                                   # MJL_Begin
                                   , tag_latency=6
                                   , data_latency=9
                                   , MJL_timeStep=options.MJL_timeStep\
                                   , MJL_has2DLLC=options.MJL_2DL2Cache\
                                   , sequential_access=True\
                                   , MJL_sameSetMapping=options.MJL_L2sameSetMapping\
                                   , MJL_oracleProxy=options.MJL_oracleProxy\
                                   , MJL_oracleProxyReplay=options.MJL_oracleProxyReplay\
                                   , MJL_ignoreExtraTagCheckLatency=MJL_l2_ignore_extra_tag_check_latecy\
                                   , MJL_bloomFilterSize=options.MJL_l2_bloomFilterSize\
                                   , MJL_bloomFilterHashFuncId=options.MJL_l2_bloomFilterHashFuncId\
                                   # MJL_End
                                   )
                system.cpu[i].addTwoLevelCacheHierarchy(icache, dcache, l2cache, iwalkcache, dwalkcache)
            else:
            # MJL_End
            # When connecting the caches, the clock is also inherited
            # from the CPU in question
                system.cpu[i].addPrivateSplitL1Caches(icache, dcache,
                                                  iwalkcache, dwalkcache)

            if options.memchecker:
                # The mem_side ports of the caches haven't been connected yet.
                # Make sure connectAllPorts connects the right objects.
                system.cpu[i].dcache = dcache_real
                system.cpu[i].dcache_mon = dcache_mon

        elif options.external_memory_system:
            # These port names are presented to whatever 'external' system
            # gem5 is connecting to.  Its configuration will likely depend
            # on these names.  For simplicity, we would advise configuring
            # it to use this naming scheme; if this isn't possible, change
            # the names below.
            if buildEnv['TARGET_ISA'] in ['x86', 'arm']:
                system.cpu[i].addPrivateSplitL1Caches(
                        ExternalCache("cpu%d.icache" % i),
                        ExternalCache("cpu%d.dcache" % i),
                        ExternalCache("cpu%d.itb_walker_cache" % i),
                        ExternalCache("cpu%d.dtb_walker_cache" % i))
            else:
                system.cpu[i].addPrivateSplitL1Caches(
                        ExternalCache("cpu%d.icache" % i),
                        ExternalCache("cpu%d.dcache" % i))

        system.cpu[i].createInterruptController()
        if options.l2cache:
            system.cpu[i].connectAllPorts(system.tol2bus, system.membus)
        # MJL_Begin
        elif options.l3cache:
            system.cpu[i].connectAllPorts(system.tol2bus, system.membus)
        elif options.l3cacheWithPrivateL2s:
            system.cpu[i].connectAllPorts(system.tol3bus, system.membus)
        # MJL_End
        elif options.external_memory_system:
            system.cpu[i].connectUncachedPorts(system.membus)
        else:
            system.cpu[i].connectAllPorts(system.membus)

    return system

# ExternalSlave provides a "port", but when that port connects to a cache,
# the connecting CPU SimObject wants to refer to its "cpu_side".
# The 'ExternalCache' class provides this adaptation by rewriting the name,
# eliminating distracting changes elsewhere in the config code.
class ExternalCache(ExternalSlave):
    def __getattr__(cls, attr):
        if (attr == "cpu_side"):
            attr = "port"
        return super(ExternalSlave, cls).__getattr__(attr)

    def __setattr__(cls, attr, value):
        if (attr == "cpu_side"):
            attr = "port"
        return super(ExternalSlave, cls).__setattr__(attr, value)

def ExternalCacheFactory(port_type):
    def make(name):
        return ExternalCache(port_data=name, port_type=port_type,
                             addr_ranges=[AllMemory])
    return make
