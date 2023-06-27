# Copyright (C) 2002-2022 CERN for the benefit of the ATLAS collaboration

doCutflow = True

def EOPKernelCfg(flags, name='TrackCaloDecorator_KERN', **kwargs):
    """Configure the derivation framework driving algorithm (kernel) for EoverP"""
    # Get the ComponentAccumulator
    acc = ComponentAccumulator()

    # Extrapolator tool 1
    #extrapolator = CompFactory.Trk.Extrapolator(name = "EOPExtrapolator")
    # Extrapolator tool 2
    from TrkConfig.AtlasExtrapolatorConfig import AtlasExtrapolatorCfg
    extrapolator = acc.popToolsAndMerge(AtlasExtrapolatorCfg(flags, name="EOPExtrapolator"))
    acc.addPublicTool(extrapolator)
    #caloExtensionTool = CompFactory.Trk.ParticleCaloExtensionTool(name="EOPParticleCaloExtensionTool",
                                                                  #ParticleType = "pion", ##What difference does the choice between pion and muon make?
                                                                  #Extrapolator = extrapolator)

    from TrackToCalo.TrackToCaloConfig import ParticleCaloExtensionToolCfg
    caloExtensionTool = acc.popToolsAndMerge(ParticleCaloExtensionToolCfg(flags, name="EOPCaloExtensionTool"))
    acc.addPublicTool(caloExtensionTool)

    #CommonTruthClassifier = CompFactory.MCTruthClassifier(name = "CommonTruthClassifier",
                                                      #ParticleCaloExtensionTool = caloExtensionTool)
    from MCTruthClassifier.MCTruthClassifierConfig import MCTruthClassifierCfg
    CommonTruthClassifier = acc.popToolsAndMerge(MCTruthClassifierCfg(flags, ParticleCaloExtensionTool=caloExtensionTool))
    acc.addPublicTool(CommonTruthClassifier)


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
    augmentationTools = [CaloDeco]
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

    #EOPSlimmingHelper.ExtraVariables += ["TrigConfKeys.TrigConfKeys"]
    #EOPSlimmingHelper.AllVariables += ["TrigConfKeys"]
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
    cfgFlags.Input.Files= ["/eos/user/c/caclarry/data18_13TeV.00354175.physics_MinBias.merge.ESD.r13575_p5088/ESD.29797580._000004.pool.root.1"]
    #cfgFlags.Input.Files= ["/eos/user/c/caclarry/mc20_13TeV/ESD.31450936._000024.pool.root.1"]
    cfgFlags.Output.AODFileName="output_AOD.root"
    cfgFlags.Output.doWriteAOD=True
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

    cfg.run(maxEvents=100)
