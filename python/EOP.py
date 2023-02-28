from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory

doCutflow = True
if doCutflow:
    from GaudiSvc.GaudiSvcConf import THistSvc
    #ServiceMgr += THistSvc()
    #svcMgr.THistSvc.Output += ["CutflowStream DATAFILE='cutflow.root' OPT='RECREATE'"]


def EOPKernelCfg(ConfigFlags, name='TrackCaloDecorator_KERN', **kwargs):
    """Configure the derivation framework driving algorithm (kernel) for EoverP"""
    # Get the ComponentAccumulator
    acc = ComponentAccumulator()

    EOPAtlasExtrapolator = acc.getPrimaryAndMerge(CompFactory.Trk.Extrapolator(name = "EOPExtrapolator"))
    #EOPAtlasExtrapolator = AtlasExtrapolator(name = 'EOPExtrapolator')
    
    EOPTrackToCaloExtensionTool = acc.getPrimaryAndMerge(CompFactory.Trk.ParticleCaloExtensionTool(name = "EOPParticleExtensionTool",
                                                                            ParticleType = "pion"))
    #EOPTrackToCaloExtensionTool = Trk__ParticleCaloExtensionTool(name="EOPParticleExtensionTool",
                                                             #ParticleType = "pion", ##What difference does the choice between pion and muon make?
                                                             #Extrapolator = EOPAtlasExtrapolator)

    CommonTruthClassifier = acc.getPrimaryAndMerge(CompFactory.MCTruthClassifier(name = "CommonTruthClassifier",
                                                      ParticleCaloExtensionTool = EOPTrackToCaloExtensionTool))
    #CommonTruthClassifier = MCTruthClassifier(name = "CommonTruthClassifier",
                                            #ParticleCaloExtensionTool=EOPTrackToCaloExtensionTool) 

    CaloDeco = acc.getPrimaryAndMerge(CompFactory.DerivationFramework.TrackCaloDecorator(name = "TrackCaloDecorator",
                                                                  TrackContainer = "InDetTrackParticles",
                                                                  CaloClusterContainer = "CaloCalTopoClusters",
                                                                  DecorationPrefix = "CALO",
                                                                  TheTrackExtrapolatorTool = EOPTrackToCaloExtensionTool,
                                                                  #Extrapolator = EOPAtlasExtrapolator, # how to instantiate this with ca?
                                                                  MCTruthClassifier = CommonTruthClassifier,
                                                                  DoCutflow = doCutflow))

    #CaloDeco = DerivationFramework__TrackCaloDecorator(name = "TrackCaloDecorator",
                                                   #TrackContainer = "InDetTrackParticles",
                                                   #CaloClusterContainer = "CaloCalTopoClusters",
                                                   #DecorationPrefix = "CALO",
                                                   #TheTrackExtrapolatorTool = EOPTrackToCaloExtensionTool,
                                                   #Extrapolator = EOPAtlasExtrapolator,
                                                   #MCTruthClassifier = CommonTruthClassifier,
                                                   #DoCutflow = doCutflow)
    #acc.merge(CaloDeco)

    augmentationTools = [EOPAtlasExtrapolator, EOPTrackToCaloExtensionTool, CommonTruthClassifier, CaloDeco]
    DerivationKernel = CompFactory.DerivationFramework.DerivationKernel
    acc.addEventAlgo(DerivationKernel(name, AugmentataionTools = augmentationTools))

    return acc

def EOPCfg(ConfigFlags):
    # Get the ComponentAccumulator
    acc = ComponentAccumulator()

    # Create and merge the kernel
    acc.merge(EOPKernelCfg(ConfigFlags, name='TrackCaloDecorator_KERN'))

    # Output stream
    EOPItemList = []
    acc.merge(OutputStreamCfg(ConfigFlags, "DAOD_EOP", ItemList=EOPItemList, AcceptAlgs=["TrackCaloDecorator_KERN"]))

    return acc
