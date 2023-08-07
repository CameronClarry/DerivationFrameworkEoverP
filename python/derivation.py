# Copyright (C) 2002-2022 CERN for the benefit of the ATLAS collaboration

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

    #from InDetConversionFinderTools.InDetConversionFinderToolsConf import InDet__VertexPointEstimator
    #from InDetConversionFinderTools.InDetConversionFinderToolsConfig import VertexPointEstimatorCfg
    from InDetConfig.InDetConversionFinderToolsConfig import VertexPointEstimatorCfg
    VtxPointEstimator = acc.popToolsAndMerge(VertexPointEstimatorCfg(flags,
                                                    name = "VtxPointEstimator",
                                                    MaxChi2OfVtxEstimation = 2000.))
    acc.addPublicTool(VtxPointEstimator)
    print(flags.InDet.SecVertex.VtxPt.MinDeltaR)
    print(flags.InDet.SecVertex.VtxPt.MaxDeltaR)
    print(flags.InDet.SecVertex.VtxPt.MaxPhi)

#    from InDetRecExample.InDetKeys import InDetKeys
#
#    from InDetAssociationTools.InDetAssociationToolsConf import InDet__InDetPRD_AssociationToolGangedPixels
#    InDetPrdAssociationTool = InDet__InDetPRD_AssociationToolGangedPixels(name                           = "InDetPrdAssociationTool",
#                                                                          PixelClusterAmbiguitiesMapName = InDetKeys.GangedPixelMap())
#    ToolSvc += InDetPrdAssociationTool
#
#    from RecExConfig.RecFlags import rec
#    CountDeadModulesAfterLastHit=False
##rec.Commissioning=False
#
#    from InDetRecExample.InDetJobProperties import InDetFlags
#
#    from InDetTrackHoleSearch.InDetTrackHoleSearchConf import InDet__InDetTrackHoleSearchTool
#    InDetHoleSearchTool = InDet__InDetTrackHoleSearchTool(name = "InDetHoleSearchTool",
#                                                          Extrapolator = InDetExtrapolator,
#                                                          #Commissioning = rec.Commissioning())
#                                                          CountDeadModulesAfterLastHit = CountDeadModulesAfterLastHit)
#    ToolSvc += InDetHoleSearchTool
#
#    from AthenaCommon.DetFlags import DetFlags
#
#    from InDetTrackSummaryHelperTool.InDetTrackSummaryHelperToolConf import InDet__InDetTrackSummaryHelperTool
#    InDetTrackSummaryHelperTool = InDet__InDetTrackSummaryHelperTool(name         = "InDetSummaryHelper",
#                                                                     AssoTool     = InDetPrdAssociationTool,
#                                                                     DoSharedHits = False,
#                                                                     HoleSearch   = InDetHoleSearchTool,
#                                                                     usePixel      = DetFlags.haveRIO.pixel_on(),
#                                                                     useSCT        = DetFlags.haveRIO.SCT_on(),
#                                                                     useTRT        = DetFlags.haveRIO.TRT_on())
#    ToolSvc += InDetTrackSummaryHelperTool
#
#    from InDetTrackSelectorTool.InDetTrackSelectorToolConf import InDet__InDetDetailedTrackSelectorTool
#    InDetTrackSelectorTool = InDet__InDetDetailedTrackSelectorTool(name = "InDetDetailedTrackSelectorTool",
#                                                                   pTMin                = 400.0,
#                                                                   IPd0Max              = 10000.0,
#                                                                   IPz0Max              = 10000.0,
#                                                                   z0Max                = 10000.0,
#                                                                   sigIPd0Max           = 10000.0,
#                                                                   sigIPz0Max           = 10000.0,
#                                                                   d0significanceMax    = -1.,
#                                                                   z0significanceMax    = -1.,
#                                                                   etaMax               = 9999.,
#                                                                   useTrackSummaryInfo  = True,
#                                                                   nHitBLayer           = 0,
#                                                                   nHitPix              = 1,
#                                                                   nHitBLayerPlusPix    = 1,
#                                                                   nHitSct              = 2,
#                                                                   nHitSi               = 3,
#                                                                   nHitTrt              = 0,
#                                                                   nHitTrtHighEFractionMax = 10000.0,
#                                                                   useSharedHitInfo     = False,
#                                                                   useTrackQualityInfo  = True,
#                                                                   fitChi2OnNdfMax      = 10000.0,
#                                                                   TrtMaxEtaAcceptance  = 1.9,
#                                                                   TrackSummaryTool     = InDetTrackSummaryTool,
#                                                                   Extrapolator         = extrapolator
#                                                                  )
#
#    ToolSvc+=InDetTrackSelectorTool
#
#    from TrkVKalVrtFitter.TrkVKalVrtFitterConf import Trk__TrkVKalVrtFitter
#    TrkVKalVrtFitter = Trk__TrkVKalVrtFitter(
#                                             name                = "VKalVrtFitterName",
#                                             Extrapolator        = InDetExtrapolator,
##                                         MagFieldSvc         = InDetMagField,
#                                             FirstMeasuredPoint  = True,
#                                             #FirstMeasuredPointLimit = True,
#                                             MakeExtendedVertex  = True)
#    ToolSvc += TrkVKalVrtFitter
#
#    from TrkV0Fitter.TrkV0FitterConf import Trk__TrkV0VertexFitter
#    TrkV0Fitter = Trk__TrkV0VertexFitter(name              = 'TrkV0FitterName',
#                                         MaxIterations     = 10,
#                                         Use_deltaR        = False,
#                                         Extrapolator      = InDetExtrapolator,
##                                     MagneticFieldTool = InDetMagField
#                                         )
#    ToolSvc += TrkV0Fitter
#
#    #from JpsiUpsilonTools.JpsiUpsilonToolsConf import Analysis__JpsiFinder
#    from JpsiUpsilonTools.JpsiUpsilonToolsConfig import JpsiFinderCfg
#    EOPLambdaFinder = JpsiFinderCfg(name                      = "EOPLambdaFinder",
#                                         muAndMu                     = False,
#                                         muAndTrack                  = False,
#                                         TrackAndTrack               = True,
#                                         assumeDiMuons               = False, 
#                                         invMassUpper                = 1125.0,
#                                         invMassLower                = 1105.0,
#                                         Chi2Cut                     = 15.,
#                                         oppChargesOnly              = True,
#                                         combOnly            	 = False,
#                                         atLeastOneComb              = False,
#                                         useCombinedMeasurement      = False, 
#                                         muonCollectionKey           = "Muons",
#                                         TrackParticleCollection     = "InDetTrackParticles",
#                                         V0VertexFitterTool          = TrkV0Fitter,             # V0 vertex fitter
#                                         useV0Fitter                 = False,                # if False a TrkVertexFitterTool will be used
#                                         TrkVertexFitterTool         = TrkVKalVrtFitter,        # VKalVrt vertex fitter
#                                         TrackSelectorTool           = InDetTrackSelectorTool,
#                                         VertexPointEstimator        = VtxPointEstimator,
#                                         useMCPCuts                  = False,
#                                         track1Mass                  = 938.272, # Not very important, only used to calculate inv. mass cut
#                                         track2Mass                  = 139.57)
#    ToolSvc += EOPLambdaFinder
#
#    from DerivationFrameworkEoverP.DerivationFrameworkEoverPConf import DerivationFramework__Reco_mumu
#    EOPRefitPV = False
#    EOPLambdaRecotrktrk = DerivationFramework__Reco_mumu(
#        name                   = "EOPLambdaRecotrktrk",
#        JpsiFinder             = EOPLambdaFinder,
#        OutputVtxContainerName = "LambdaCandidates",
#        PVContainerName        = "PrimaryVertices",
#        RefPVContainerName     = "EOPLambdaRefittedPrimaryVertices",
#        RefitPV = EOPRefitPV)
#    ToolSvc += EOPLambdaRecotrktrk
#
#    from DerivationFrameworkEoverP.DerivationFrameworkEoverPConf import DerivationFramework__Select_onia2mumu
#    EOPSelectLambda2trktrk = DerivationFramework__Select_onia2mumu(
#        name                  = "EOPSelectLambda2trktrk",
#        HypothesisName        = "Lambda",
#        InputVtxContainerName = EOPLambdaRecotrktrk.OutputVtxContainerName,
#        TrkMasses             = [938.272, 139.57], # Proton, pion PDG mass
#        VtxMassHypo           = 1115.0, # lambda PDG mass
#        MassMin               = 1105.0,
#        MassMax               = 1125.0,
#        Chi2Max               = 15)
#    ToolSvc += EOPSelectLambda2trktrk



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
    augmentationTools = [CaloDeco]#, EOPSelectLambda2trktrk, EOPLambdaRecotrktrk]
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
    EOPSlimmingHelper.AllVariables += ["VertexContainer", "VertexAuxContainer"]
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
