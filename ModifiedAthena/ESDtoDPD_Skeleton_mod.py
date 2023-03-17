# Copyright (C) 2002-2022 CERN for the benefit of the ATLAS collaboration

from PyJobTransforms.CommonRunArgsToFlags import commonRunArgsToFlags
from PyJobTransforms.TransformUtils import processPreExec, processPreInclude, processPostExec, processPostInclude


def fromRunArgs(runArgs):
    from AthenaCommon.Logging import logging
    log = logging.getLogger('ESDtoDPD')
    log.info('****************** STARTING Reconstruction (ESDtoDPD) *****************')

    log.info('**** Transformation run arguments')
    log.info(str(runArgs))

    import time
    timeStart = time.time()

    log.info('**** executing ROOT6Setup')
    from PyUtils.Helpers import ROOT6Setup
    ROOT6Setup(batch=True)

    log.info('**** Setting-up configuration flags')
    from AthenaConfiguration.AllConfigFlags import ConfigFlags
    commonRunArgsToFlags(runArgs, ConfigFlags)
    from RecJobTransforms.RecoConfigFlags import recoRunArgsToFlags
    recoRunArgsToFlags(runArgs, ConfigFlags)

    # Autoconfigure enabled subdetectors
    if hasattr(runArgs, 'detectors'):
        detectors = runArgs.detectors
    else:
        detectors = None

    # TODO: event service?

    ## Inputs
    # BS
    inputsDRAW = [prop for prop in dir(runArgs) if prop.startswith('inputESD') and prop.endswith('File')]
    if len(inputsDRAW) == 1:
        ConfigFlags.Input.Files = getattr(runArgs, inputsDRAW[0])
    elif len(inputsDRAW) > 1:
        raise RuntimeError('Impossible to run RAWtoALL with multiple input DRAW files (viz.: {0})'.format(inputsDRAW))

    ## Outputs
    if hasattr(runArgs, 'outputDAOD_EOPFile'):
        flagString = 'Output.DAOD_EOPFileName'
        ConfigFlags.addFlag(flagString, runArgs.outputDAOD_EOPFile)
        ConfigFlags.Output.doWriteDAOD = True
        log.info("---------- Configured DAOD_EOP output")

    #from AthenaConfiguration.Enums import ProductionStep
    #ConfigFlags.Common.ProductionStep=ProductionStep.Reconstruction

    # Setup detector flags
    from AthenaConfiguration.DetectorConfigFlags import setupDetectorFlags
    setupDetectorFlags(ConfigFlags, detectors, use_metadata=True, toggle_geometry=True, keep_beampipe=True)
    # Print reco domain status
    from RecJobTransforms.RecoConfigFlags import printRecoFlags
    printRecoFlags(ConfigFlags)

    # Setup perfmon flags from runargs
    from PerfMonComps.PerfMonConfigHelpers import setPerfmonFlagsFromRunArgs
    setPerfmonFlagsFromRunArgs(ConfigFlags, runArgs)

    # Pre-include
    processPreInclude(runArgs, ConfigFlags)

    # Pre-exec
    processPreExec(runArgs, ConfigFlags)

    # Lock flags
    ConfigFlags.lock()

    log.info("Configuring according to flag values listed below")
    ConfigFlags.dump()

    # Main reconstruction steering
    from RecJobTransforms.RecoSteering import RecoSteering
    cfg = RecoSteering(ConfigFlags)

    # Performance DPDs 
    cfg.flagPerfmonDomain('PerfDPD')
    # IDTIDE
    for flag in [key for key in ConfigFlags._flagdict.keys() if ("Output.DAOD_IDTIDEFileName" in key)]:
        from DerivationFrameworkInDet.IDTIDE import IDTIDECfg
        cfg.merge(IDTIDECfg(ConfigFlags))
        log.info("---------- Configured IDTIDE perfDPD")

    # Special message service configuration
    from Digitization.DigitizationSteering import DigitizationMessageSvcCfg
    cfg.merge(DigitizationMessageSvcCfg(ConfigFlags))

    # Post-include
    processPostInclude(runArgs, ConfigFlags, cfg)

    # Post-exec
    processPostExec(runArgs, ConfigFlags, cfg)

    timeConfig = time.time()
    log.info("configured in %d seconds", timeConfig - timeStart)

    # Print sum information about AccumulatorCache performance
    from AthenaConfiguration.AccumulatorCache import AccumulatorDecorator
    AccumulatorDecorator.printStats() 

    # Run the final accumulator
    sc = cfg.run()
    timeFinal = time.time()
    log.info("Run RAWtoALL_skeleton in %d seconds (running %d seconds)", timeFinal - timeStart, timeFinal - timeConfig)

    import sys
    sys.exit(not sc.isSuccess())
