# Copyright (C) 2002-2022 CERN for the benefit of the ATLAS collaboration

from AthenaConfiguration.ComponentFactory import CompFactory
from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator

# A fixed version of this config function
def BPHY_InDetDetailedTrackSelectorToolCfg(flags, name='BPHY_InDetDetailedTrackSelectorTool', **kwargs):
    acc = ComponentAccumulator()

    # Different from other InDetTrackSelectorToolCfg
    if "Extrapolator" not in kwargs:
        from TrkConfig.AtlasExtrapolatorConfig import InDetExtrapolatorCfg
        kwargs.setdefault("Extrapolator", acc.popToolsAndMerge(InDetExtrapolatorCfg(flags)))

    kwargs.setdefault("pTMin"                , 400.0)
    kwargs.setdefault("IPd0Max"              , 10000.0)
    kwargs.setdefault("IPz0Max"              , 10000.0)
    kwargs.setdefault("z0Max"                , 10000.0)
    kwargs.setdefault("sigIPd0Max"           , 10000.0)
    kwargs.setdefault("sigIPz0Max"           , 10000.0)
    kwargs.setdefault("d0significanceMax"    , -1.)
    kwargs.setdefault("z0significanceMax"    , -1.)
    kwargs.setdefault("etaMax"               , 9999.)
    kwargs.setdefault("useTrackSummaryInfo"  , True)
    kwargs.setdefault("nHitBLayer"           , 0)
    kwargs.setdefault("nHitPix"              , 1)
    kwargs.setdefault("nHitBLayerPlusPix"    , 1)
    kwargs.setdefault("nHitSct"              , 2)
    kwargs.setdefault("nHitSi"               , 3)
    kwargs.setdefault("nHitTrt"              , 0)
    kwargs.setdefault("nHitTrtHighEFractionMax", 10000.0)
    kwargs.setdefault("useSharedHitInfo"     , False)
    kwargs.setdefault("useTrackQualityInfo"  , True)
    kwargs.setdefault("fitChi2OnNdfMax"      , 10000.0)
    kwargs.setdefault("TrtMaxEtaAcceptance"  , 1.9)
# This line causes issues in this version of athena
    #kwargs.setdefault("UseEventInfoBS"       , True)
    kwargs.setdefault("TrackSummaryTool"     , None)

    from InDetConfig.InDetTrackSelectorToolConfig import InDetTrackSelectorToolCfg   
    acc.setPrivateTools(acc.popToolsAndMerge(InDetTrackSelectorToolCfg(flags, name, **kwargs)))
    return acc

# This does not exist in the athena release used
def JpsiFinderCfg(ConfigFlags,name="JpsiFinder", **kwargs):
    acc = ComponentAccumulator()
    kwargs.setdefault("useV0Fitter", False)
    kwargs.setdefault("V0VertexFitterTool", None)
    if "TrkVertexFitterTool" not in kwargs:
        from TrkConfig.TrkVKalVrtFitterConfig import BPHY_TrkVKalVrtFitterCfg
        kwargs.setdefault("TrkVertexFitterTool", acc.popToolsAndMerge(BPHY_TrkVKalVrtFitterCfg(ConfigFlags)))
    if "TrackSelectorTool" not in kwargs:
        from InDetConfig.InDetTrackSelectorToolConfig import BPHY_InDetDetailedTrackSelectorToolCfg
        kwargs.setdefault("TrackSelectorTool", acc.popToolsAndMerge(BPHY_InDetDetailedTrackSelectorToolCfg(ConfigFlags)))
    if "VertexPointEstimator" not in kwargs:
        from InDetConfig.InDetConversionFinderToolsConfig import BPHY_VertexPointEstimatorCfg
        kwargs.setdefault("VertexPointEstimator", acc.popToolsAndMerge(BPHY_VertexPointEstimatorCfg(ConfigFlags)))
    acc.addPublicTool(kwargs["TrkVertexFitterTool"])
    acc.addPublicTool(kwargs["TrackSelectorTool"])
    acc.addPublicTool(kwargs["VertexPointEstimator"])
    acc.setPrivateTools(CompFactory.Analysis.JpsiFinder(name, **kwargs))
    return acc

# This does nto exist in the athena release used
def PrimaryVertexRefittingToolCfg(flags, **kwargs):
    acc = ComponentAccumulator()
    from TrkConfig.TrkVertexFitterUtilsConfig import TrackToVertexIPEstimatorCfg
    kwargs.setdefault( "TrackToVertexIPEstimator", acc.popToolsAndMerge( TrackToVertexIPEstimatorCfg(flags,**kwargs) ) )
    acc.setPrivateTools( CompFactory.Analysis.PrimaryVertexRefitter( **kwargs) )
    return acc

doCutflow = True

def EOPKernelCfg(flags, name='TrackCaloDecorator_KERN', **kwargs):
    """Configure the derivation framework driving algorithm (kernel) for EoverP"""
    # Get the ComponentAccumulator
    acc = ComponentAccumulator()

    # Extrapolator tool
    from TrkConfig.AtlasExtrapolatorConfig import AtlasExtrapolatorCfg
    extrapolator = acc.popToolsAndMerge(AtlasExtrapolatorCfg(flags, name="EOPExtrapolator"))
    acc.addPublicTool(extrapolator)

    from TrackToCalo.TrackToCaloConfig import ParticleCaloExtensionToolCfg
    caloExtensionTool = acc.popToolsAndMerge(ParticleCaloExtensionToolCfg(flags, name="EOPCaloExtensionTool"))
    acc.addPublicTool(caloExtensionTool)

    from MCTruthClassifier.MCTruthClassifierConfig import MCTruthClassifierCfg
    CommonTruthClassifier = acc.popToolsAndMerge(MCTruthClassifierCfg(flags, ParticleCaloExtensionTool=caloExtensionTool))
    acc.addPublicTool(CommonTruthClassifier)

    from InDetConfig.InDetConversionFinderToolsConfig import VertexPointEstimatorCfg
    VtxPointEstimator = acc.popToolsAndMerge(VertexPointEstimatorCfg(flags,
                                                    name = "VtxPointEstimator",
                                                    MaxChi2OfVtxEstimation = 2000.))
    acc.addPublicTool(VtxPointEstimator)

    from InDetConfig.InDetTrackHoleSearchConfig import InDetTrackHoleSearchToolCfg
    InDetHoleSearchTool = acc.popToolsAndMerge(InDetTrackHoleSearchToolCfg(flags,
                                                      name = "InDetHoleSearchTool",
                                                      Extrapolator = extrapolator,
                                                      CountDeadModulesAfterLastHit = False))
    acc.addPublicTool(InDetHoleSearchTool)

    from InDetConfig.InDetTrackSummaryHelperToolConfig import InDetTrackSummaryHelperToolCfg
    InDetTrackSummaryHelperTool = acc.popToolsAndMerge(InDetTrackSummaryHelperToolCfg(flags,
                                                                                      name = "InDetSummaryHelper",
                                                                                      HoleSearch = InDetHoleSearchTool))
    acc.addPublicTool(InDetTrackSummaryHelperTool)

    from TrkConfig.TrkTrackSummaryToolConfig import InDetTrackSummaryToolCfg
    InDetTrackSummaryTool = acc.popToolsAndMerge(InDetTrackSummaryToolCfg(flags,
                                                                          name = "InDetTrackSummaryTool",
                                              InDetSummaryHelperTool = InDetTrackSummaryHelperTool))
    acc.addPublicTool(InDetTrackSummaryTool)

    #from InDetConfig.InDetTrackSelectorToolConfig import BPHY_InDetDetailedTrackSelectorToolCfg
    InDetTrackSelectorTool = acc.popToolsAndMerge(BPHY_InDetDetailedTrackSelectorToolCfg(flags,
                                                                                       name = "InDetDetailedTrackSelectorTool",
                                                                                       TrackSummaryTool = InDetTrackSummaryTool,
                                                                                       Extrapolator = extrapolator))
    acc.addPublicTool(InDetTrackSelectorTool)

    from TrkConfig.TrkVKalVrtFitterConfig import BPHY_TrkVKalVrtFitterCfg
    TrkVKalVrtFitter = acc.popToolsAndMerge(BPHY_TrkVKalVrtFitterCfg(flags,
                                                                     name = "VKalVrtFitterName",
                                                                     Extrapolator = extrapolator,
                                                                     FirstMeasuredPoint = True,
                                                                     MakeExtendedVertex = True))
    acc.addPublicTool(TrkVKalVrtFitter)

    from TrkConfig.TrkV0FitterConfig import TrkV0VertexFitterCfg
    TrkV0Fitter = acc.popToolsAndMerge(TrkV0VertexFitterCfg(flags,
                                                            name = 'TrkV0FitterName',
                                                            Extrapolator = extrapolator))
    acc.addPublicTool(TrkV0Fitter)
#    #from JpsiUpsilonTools.JpsiUpsilonToolsConf import Analysis__JpsiFinder
    #from JpsiUpsilonTools.JpsiUpsilonToolsConfig import JpsiFinderCfg
    EOPLambdaFinder = acc.popToolsAndMerge(JpsiFinderCfg(flags,
                                           name                      = "EOPLambdaFinder",
                                           muAndMu                     = False,
                                           muAndTrack                  = False,
                                           TrackAndTrack               = True,
                                           assumeDiMuons               = False, 
                                           invMassUpper                = 1125.0,
                                           invMassLower                = 1105.0,
                                           Chi2Cut                     = 15.,
                                           oppChargesOnly              = True,
                                           combOnly            	 = False,
                                           atLeastOneComb              = False,
                                           useCombinedMeasurement      = False, 
                                           muonCollectionKey           = "Muons",
                                           TrackParticleCollection     = "InDetTrackParticles",
                                           V0VertexFitterTool          = TrkV0Fitter,             # V0 vertex fitter
                                           useV0Fitter                 = False,                # if False a TrkVertexFitterTool will be used
                                           TrkVertexFitterTool         = TrkVKalVrtFitter,        # VKalVrt vertex fitter
                                           TrackSelectorTool           = InDetTrackSelectorTool,
                                           VertexPointEstimator        = VtxPointEstimator,
                                           useMCPCuts                  = False,
                                           track1Mass                  = 938.272, # Not very important, only used to calculate inv. mass cut
                                           track2Mass                  = 139.57))
    acc.addPublicTool(EOPLambdaFinder)

    EOPKsFinder = acc.popToolsAndMerge(JpsiFinderCfg(flags,
                                           name                      = "EOPKsFinder",
                                           muAndMu                     = False,
                                           muAndTrack                  = False,
                                           TrackAndTrack               = True,
                                           assumeDiMuons               = False, 
                                           invMassUpper                = 520.0,
                                           invMassLower                = 480.0,
                                           Chi2Cut                     = 15.,
                                           oppChargesOnly              = True,
                                           combOnly            	       = False,
                                           atLeastOneComb              = False,
                                           useCombinedMeasurement      = False, 
                                           muonCollectionKey           = "Muons",
                                           TrackParticleCollection     = "InDetTrackParticles",
                                           V0VertexFitterTool          = TrkV0Fitter,             # V0 vertex fitter
                                           useV0Fitter                 = False,                # if False a TrkVertexFitterTool will be used
                                           TrkVertexFitterTool         = TrkVKalVrtFitter,        # VKalVrt vertex fitter
                                           TrackSelectorTool           = InDetTrackSelectorTool,
                                           VertexPointEstimator        = VtxPointEstimator,
                                           useMCPCuts                  = False,
                                           track1Mass                  = 139.57,
                                           track2Mass                  = 139.57))
    acc.addPublicTool(EOPKsFinder)

    EOPPhiFinder = acc.popToolsAndMerge(JpsiFinderCfg(flags,
                                           name                      = "EOPPhiFinder",
                                           muAndMu                     = False,
                                           muAndTrack                  = False,
                                           TrackAndTrack               = True,
                                           assumeDiMuons               = False, 
                                           invMassUpper                = 1200.0,
                                           invMassLower                = 987.354, # taken from HIGG2D5
                                           Chi2Cut                     = 15.,
                                           oppChargesOnly              = True,
                                           combOnly            	       = False,
                                           atLeastOneComb              = False,
                                           useCombinedMeasurement      = False, 
                                           muonCollectionKey           = "Muons",
                                           TrackParticleCollection     = "InDetTrackParticles",
                                           V0VertexFitterTool          = TrkV0Fitter,             # V0 vertex fitter
                                           useV0Fitter                 = False,                # if False a TrkVertexFitterTool will be used
                                           TrkVertexFitterTool         = TrkVKalVrtFitter,        # VKalVrt vertex fitter
                                           TrackSelectorTool           = InDetTrackSelectorTool,
                                           VertexPointEstimator        = VtxPointEstimator,
                                           useMCPCuts                  = False,
                                           track1Mass                  = 493.677, # Not very important, only used to calculate inv. mass cut
                                           track2Mass                  = 493.677))
    acc.addPublicTool(EOPPhiFinder)

    from TrkConfig.TrkVertexAnalysisUtilsConfig import V0ToolsCfg
    V0Tools = acc.popToolsAndMerge(V0ToolsCfg(flags, name = "V0Tools", Extrapolator=extrapolator))
    #V0Tools = CompFactory.Trk.V0Tools("ThisIsV0Tools", Extrapolator=extrapolator)
    acc.addPublicTool(V0Tools)

    PvRefitter = acc.popToolsAndMerge(PrimaryVertexRefittingToolCfg(flags, name="PvRefitter"))
    acc.addPublicTool(PvRefitter)

    EOPRefitPV = False
    EOPLambdaRecotrktrk = CompFactory.DerivationFramework.Reco_mumu(
        name                   = "EOPLambdaRecotrktrk",
        JpsiFinder             = EOPLambdaFinder,
        V0Tools = V0Tools,
        PVRefitter = PvRefitter,
        OutputVtxContainerName = "LambdaCandidates",
        PVContainerName        = "PrimaryVertices",
        RefPVContainerName     = "EOPLambdaRefittedPrimaryVertices",
        RefitPV = EOPRefitPV)
    acc.addPublicTool(EOPLambdaRecotrktrk)

    EOPKsRecotrktrk = CompFactory.DerivationFramework.Reco_mumu(
        name                   = "EOPKsRecotrktrk",
        JpsiFinder             = EOPKsFinder,
        V0Tools = V0Tools,
        PVRefitter = PvRefitter,
        OutputVtxContainerName = "KsCandidates",
        PVContainerName        = "PrimaryVertices",
        RefPVContainerName     = "EOPKsRefittedPrimaryVertices",
        RefitPV = EOPRefitPV)
    acc.addPublicTool(EOPKsRecotrktrk)

    EOPPhiRecotrktrk = CompFactory.DerivationFramework.Reco_mumu(
        name                   = "EOPPhiRecotrktrk",
        JpsiFinder             = EOPPhiFinder,
        V0Tools = V0Tools,
        PVRefitter = PvRefitter,
        OutputVtxContainerName = "PhiCandidates",
        PVContainerName        = "PrimaryVertices",
        RefPVContainerName     = "EOPPhiRefittedPrimaryVertices",
        RefitPV = EOPRefitPV)
    acc.addPublicTool(EOPPhiRecotrktrk)

    EOPSelectLambda2trktrk = CompFactory.DerivationFramework.Select_onia2mumu(
        name                  = "EOPSelectLambda2trktrk",
        HypothesisName        = "Lambda",
        InputVtxContainerName = EOPLambdaRecotrktrk.OutputVtxContainerName,
        TrkMasses             = [938.272, 139.57], # Proton, pion PDG mass
        VtxMassHypo           = 1115.0, # lambda PDG mass
        MassMin               = 1105.0,
        MassMax               = 1125.0,
        Chi2Max               = 15,
        V0Tool = V0Tools)
    acc.addPublicTool(EOPSelectLambda2trktrk)

    EOPSelectKs2trktrk = CompFactory.DerivationFramework.Select_onia2mumu(
        name                  = "EOPSelectKs2trktrk",
        HypothesisName        = "Ks",
        InputVtxContainerName = EOPKsRecotrktrk.OutputVtxContainerName,
        TrkMasses             = [139.57, 139.57],
        VtxMassHypo           = 497.611, # lambda k_s mass
        MassMin               = 480.0,
        MassMax               = 520.0,
        Chi2Max               = 15,
        V0Tool = V0Tools)
    acc.addPublicTool(EOPSelectKs2trktrk)

    EOPSelectPhi2trktrk = CompFactory.DerivationFramework.Select_onia2mumu(
        name                  = "EOPSelectPhi2trktrk",
        HypothesisName        = "Phi",
        #InputVtxContainerName = "StoreGateSvc+"+EOPLambdaRecotrktrk.OutputVtxContainerName,
        InputVtxContainerName = EOPPhiRecotrktrk.OutputVtxContainerName,
        TrkMasses             = [493.677, 493.677], # K+/- PDG mass
        VtxMassHypo           = 1019.461, # phi PDG mass
        MassMin               = 987.354,
        MassMax               = 1200.0,
        Chi2Max               = 15,
        V0Tool = V0Tools)
    acc.addPublicTool(EOPSelectPhi2trktrk)


    CaloDeco = CompFactory.DerivationFramework.TrackCaloDecorator(name = "TrackCaloDecorator",
                                                                  TrackContainer = "InDetTrackParticles",
                                                                  CaloClusterContainer = "CaloCalTopoClusters",
                                                                  DecorationPrefix = "CALO",
                                                                  TheTrackExtrapolatorTool = caloExtensionTool,
                                                                  Extrapolator = extrapolator,
                                                                  MCTruthClassifier = CommonTruthClassifier,
                                                                  DoCutflow = doCutflow)
    acc.addPublicTool(CaloDeco)

    #augmentationTools = [extrapolator, caloExtensionTool, CommonTruthClassifier, CaloDeco]
    #augmentationTools = [extrapolator, caloExtensionTool, CaloDeco]
    #augmentationTools = [caloExtensionTool, CaloDeco]
    augmentationTools = [CaloDeco, EOPLambdaRecotrktrk, EOPSelectLambda2trktrk, EOPKsRecotrktrk, EOPSelectKs2trktrk, EOPPhiRecotrktrk, EOPSelectPhi2trktrk]
    DerivationKernel = CompFactory.DerivationFramework.DerivationKernel
    acc.addEventAlgo(DerivationKernel(name, AugmentationTools = augmentationTools))
    #acc.setPrivateTools(caloExtensionTool)

    return acc

def EOPCfg(flags):
    # Get the ComponentAccumulator
    acc = ComponentAccumulator()

    # Create and merge the kernel
    acc.merge(EOPKernelCfg(flags, name='TrackCaloDecorator_KERN', StreamName = "OutputStreamDOAD_EOP"))

    from OutputStreamAthenaPool.OutputStreamConfig import OutputStreamCfg
    #from xAODMetaDataCnv.InfileMetaDataConfig import SetupMetaDataForStreamCfg
    from DerivationFrameworkCore.SlimmingHelper import SlimmingHelper

    EOPSlimmingHelper = SlimmingHelper("EOPSlimmingHelper", NamesAndTypes = flags.Input.TypedCollections, ConfigFlags = flags)
    EOPSlimmingHelper.SmartCollections = ["EventInfo","InDetTrackParticles","PrimaryVertices"]

    EOPSlimmingHelper.AllVariables += ["InDetTrackParticles"]

    # Add secondary vertices
    EOPSlimmingHelper.StaticContent += ["xAOD::VertexContainer#LambdaCandidates","xAOD::VertexAuxContainer#LambdaCandidatesAux.","xAOD::VertexAuxContainer#LambdaCandidatesAux.-vxTrackAtVertex"]
    EOPSlimmingHelper.StaticContent += ["xAOD::VertexContainer#KsCandidates","xAOD::VertexAuxContainer#KsCandidatesAux.","xAOD::VertexAuxContainer#KsCandidatesAux.-vxTrackAtVertex"]
    EOPSlimmingHelper.StaticContent += ["xAOD::VertexContainer#PhiCandidates","xAOD::VertexAuxContainer#PhiCandidatesAux.","xAOD::VertexAuxContainer#PhiCandidatesAux.-vxTrackAtVertex"]

    # Add truth information
    if flags.Input.isMC:
        EOPSlimmingHelper.AllVariables += ["TruthParticles"]

    EOPItemList = EOPSlimmingHelper.GetItemList()
    acc.merge(OutputStreamCfg(flags, "DAOD_EOP", ItemList=EOPItemList, AcceptAlgs=["TrackCaloDecorator_KERN"]))

    return acc

if __name__=="__main__":

    import argparse
    import os
    parser = argparse.ArgumentParser(description='Submit plotting batch jobs for the EoverPAnalysis plotting')
    parser.add_argument('--input_files', '-i', dest="input_files", type=str, required=True, help='comma-separated list of files to run on (or a text file containing paths)')
    parser.add_argument('--useFileList', action='store_true', help='whether to parse the input_files as a text file containing the file paths')
    parser.add_argument('--isData', action=argparse.BooleanOptionalAction, help='whether the samples to be run over are data')
    parser.add_argument('--nthreads', dest="nthreads", type=int, default=8, help='number of threads to use')
    parser.add_argument('--maxEvents', dest="max_events", type=int, default=None, help='maximum number of events to process')
    parser.add_argument('--athenaThreads', dest="athena_threads", action=argparse.BooleanOptionalAction, help='use the environment variable ATHENA_PROC_NUMBER for the number of threads')
    args = parser.parse_args()
    
    # Set config flags
    from AthenaConfiguration.AllConfigFlags import ConfigFlags as cfgFlags
    if args.athena_threads:
        print("ATHENA_PROC_NUMBER:", os.environ['ATHENA_PROC_NUMBER'])
        cfgFlags.Concurrency.NumThreads=int(os.environ['ATHENA_PROC_NUMBER'])
    else:
        cfgFlags.Concurrency.NumThreads=args.nthreads
    if args.isData:
        cfgFlags.Input.isMC=False
    if args.useFileList:
        with open(args.input_files, 'r') as f:
            cfgFlags.Input.Files = [line.strip() for line in f.readlines()]
    else:
        cfgFlags.Input.Files = args.input_files.split(",")
    cfgFlags.lock()

    from AthenaConfiguration.MainServicesConfig import MainServicesCfg
    cfg=MainServicesCfg(cfgFlags)

    from AthenaPoolCnvSvc.PoolReadConfig import PoolReadCfg
    cfg.merge(PoolReadCfg(cfgFlags))

    cfg.merge(EOPCfg(cfgFlags))

    cfg.run(maxEvents=args.max_events)
