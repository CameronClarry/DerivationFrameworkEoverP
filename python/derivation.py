# Copyright (C) 2002-2022 CERN for the benefit of the ATLAS collaboration

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

    EOPSelectLambda2trktrk = CompFactory.DerivationFramework.Select_onia2mumu(
        name                  = "EOPSelectLambda2trktrk",
        HypothesisName        = "Lambda",
        InputVtxContainerName = "StoreGateSvc+"+EOPLambdaRecotrktrk.OutputVtxContainerName,
        TrkMasses             = [938.272, 139.57], # Proton, pion PDG mass
        VtxMassHypo           = 1115.0, # lambda PDG mass
        MassMin               = 1105.0,
        MassMax               = 1125.0,
        Chi2Max               = 15,
        V0Tool = V0Tools)
    acc.addPublicTool(EOPSelectLambda2trktrk)



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
    augmentationTools = [CaloDeco, EOPLambdaRecotrktrk, EOPSelectLambda2trktrk]
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

    # InDetTrackParticlesAuxDyn.CALO_trkEta_TileBar2
    EOPSlimmingHelper.ExtraVariables += ["InDetTrackParticles.numberOfTRTHits.CALO_trkEta_TileBar2"]
    EOPSlimmingHelper.AllVariables += ["InDetTrackParticles"]
    EOPSlimmingHelper.AllVariables += ["VertexContainer", "VertexAuxContainer", "LambdaCandidates", "LambdaCandidatesAuxDyn", "OniaCandidates"]
    EOPItemList = EOPSlimmingHelper.GetItemList()
    acc.merge(OutputStreamCfg(flags, "DAOD_EOP", ItemList=EOPItemList, AcceptAlgs=["TrackCaloDecorator_KERN"]))

    return acc

if __name__=="__main__":

    from AthenaCommon.GlobalFlags  import globalflags
    globalflags.DataSource.set_Value_and_Lock('data')

    from AthenaConfiguration.AllConfigFlags import ConfigFlags as cfgFlags

    cfgFlags.Concurrency.NumThreads=8
    cfgFlags.Input.isMC=False
    #cfgFlags.Input.Files= ["/cvmfs/atlas-nightlies.cern.ch/repo/data/data-art/RecExRecoTest/mc20e_13TeV/valid1.410000.PowhegPythiaEvtGen_P2012_ttbar_hdamp172p5_nonallhad.ESD.e4993_s3227_r12689/myESD.pool.root"]
    cfgFlags.Input.Files= ["/eos/user/c/caclarry/data18_13TeV/ESD.30341268._033971.pool.root.1"]
    #cfgFlags.Input.Files= ["/eos/user/c/caclarry/mc20_13TeV/ESD.31450936._000024.pool.root.1"]
    #cfgFlags.Output.AODFileName="output_AOD.root"
    #cfgFlags.Output.doWriteAOD=True
    cfgFlags.lock()

    from AthenaConfiguration.MainServicesConfig import MainServicesCfg
    cfg=MainServicesCfg(cfgFlags)

    from AthenaPoolCnvSvc.PoolReadConfig import PoolReadCfg
    cfg.merge(PoolReadCfg(cfgFlags))
    from eflowRec.PFRun3Config import PFFullCfg
    cfg.merge(PFFullCfg(cfgFlags))
    
    from eflowRec.PFRun3Remaps import ListRemaps

    list_remaps=ListRemaps()
    for mapping in list_remaps:
        cfg.merge(mapping)    

    #Given we rebuild topoclusters from the ESD, we should also redo the matching between topoclusters and muon clusters.
    #The resulting links are used to create the global GPF muon-FE links.
    from AthenaConfiguration.ComponentFactory import CompFactory
    from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
    
    cfg.merge(EOPCfg(cfgFlags))
    result = ComponentAccumulator()    
    #result.addEventAlgo(CompFactory.ClusterMatching.CaloClusterMatchLinkAlg("MuonTCLinks", ClustersToDecorate="MuonClusterCollection"))
    cfg.merge(result)

    cfg.run(maxEvents=10)
