#
# @file     TrackCaloDecorator.py
# @author   Millie McDonald <emcdonal@cern.ch> // Main author
# @author   Joakim Olsson <joakim.olsson@cern.ch> // Modifications and cross-checks
# @brief    A derivation for ATLAS Run II E/p analyses. Extrapolates all tracks to calorimeter and decorates them with cluster and cell energies.
#

# include("AthAnalysisBaseComps/SuppressLogging.py") # Optional include to suppress as much athena output as possible

#====================================================================
# SETUP TOOLS
#====================================================================
from AthenaCommon import CfgMgr
DerivationFrameworkJob = CfgMgr.AthSequencer("EoverPSeq")

# Set up stream auditor
from AthenaCommon.AppMgr import ServiceMgr as svcMgr
if not hasattr(svcMgr, 'DecisionSvc'):
    svcMgr += CfgMgr.DecisionSvc()
svcMgr.DecisionSvc.CalcStats = True

#svcMgr.MessageSvc.OutputLevel = DEBUG #Comment this out when submitting grid jobs
svcMgr.MessageSvc.defaultLimit = 9999999

#====================================================================
# SET UP STREAM
#====================================================================
from OutputStreamAthenaPool.MultipleStreamManager import MSMgr
from D2PDMaker.D2PDHelpers import buildFileName
from PrimaryDPDMaker.PrimaryDPDFlags import primDPD
streamName = primDPD.WriteDAOD_EOP.StreamName
fileName = buildFileName( primDPD.WriteDAOD_EOP )
EOPStream = MSMgr.NewPoolRootStream( streamName, fileName )

# Save cutflow histograms
doCutflow = True
if doCutflow:
    from GaudiSvc.GaudiSvcConf import THistSvc
    ServiceMgr += THistSvc()
    svcMgr.THistSvc.Output += ["CutflowStream DATAFILE='cutflow.root' OPT='RECREATE'"]

#====================================================================
# THE ATLAS EXTRAPOLATOR
#====================================================================
# Configure the extrapolator. Maybe we should configure how the tracks loose energy in the ATLAS calorimeters?
from TrackToCalo.TrackToCaloConf import Trk__ParticleCaloExtensionTool
from TrkExTools.AtlasExtrapolator import AtlasExtrapolator
EOPAtlasExtrapolator = AtlasExtrapolator(name = 'EOPExtrapolator')
ToolSvc += EOPAtlasExtrapolator


#====================================================================
# THE TRACK TO CALORIMETER EXTRAPOLATION TOOL
#====================================================================
EOPTrackToCaloExtensionTool = Trk__ParticleCaloExtensionTool(name="EOPParticleExtensionTool",
                                                             ParticleType = "pion", ##What difference does the choice between pion and muon make?
                                                             Extrapolator = EOPAtlasExtrapolator)
ToolSvc += EOPTrackToCaloExtensionTool

#====================================================================
# AUGMENTATION TOOL
#====================================================================
from DerivationFrameworkEoverP.DerivationFrameworkEoverPConf import DerivationFramework__TrackCaloDecorator
CaloDeco = DerivationFramework__TrackCaloDecorator(name = "TrackCaloDecorator",
                                                   TrackContainer = "InDetTrackParticles",
                                                   CaloClusterContainer = "CaloCalTopoClusters",
                                                   DecorationPrefix = "CALO",
                                                   TheTrackExtrapolatorTool = EOPTrackToCaloExtensionTool,
                                                   Extrapolator = EOPAtlasExtrapolator,
                                                   DoCutflow = doCutflow)
ToolSvc += CaloDeco

#--------------------------------------------------------------------
## 2/ setup JpsiFinder tool
##    These are general tools independent of DerivationFramework that do the 
##    actual vertex fitting and some pre-selection.

## 1/ setup vertexing tools and services
include("JpsiUpsilonTools/configureServices.py")

from JpsiUpsilonTools.JpsiUpsilonToolsConf import Analysis__JpsiFinder
EOPJpsiFinder = Analysis__JpsiFinder(name                         = "EOPJpsiFinder",
                                     OutputLevel                 = INFO,
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
                                     useCombinedMeasurement      = False, # Only takes effect if combOnly=True
                                     muonCollectionKey           = "Muons",
                                     TrackParticleCollection     = "InDetTrackParticles",
                                     V0VertexFitterTool          = TrkV0Fitter,             # V0 vertex fitter
                                     useV0Fitter                 = False,                # if False a TrkVertexFitterTool will be used
                                     TrkVertexFitterTool         = TrkVKalVrtFitter,        # VKalVrt vertex fitter
                                     TrackSelectorTool           = InDetTrackSelectorTool,
                                     ConversionFinderHelperTool  = InDetConversionHelper,
                                     VertexPointEstimator        = VtxPointEstimator,
                                     useMCPCuts                  = False,
                                     track1Mass                  = 938.272, # Not very important, only used to calculate inv. mass cut
                                     track2Mass                  = 139.57)
ToolSvc += EOPJpsiFinder
print      EOPJpsiFinder

#--------------------------------------------------------------------
## 3/ setup the vertex reconstruction "call" tool(s). They are part of the derivation framework.
##    These Augmentation tools add output vertex collection(s) into the StoreGate and add basic
##    decorations which do not depend on the vertex mass hypothesis (e.g. lxy, ptError, etc).
##    There should be one tool per topology, i.e. Jpsi and Psi(2S) do not need two instance of the
##    Reco tool is the JpsiFinder mass window is wide enough.

from DerivationFrameworkEoverP.DerivationFrameworkEoverPConf import DerivationFramework__Reco_mumu
EOPRefitPV = False
EOPRecotrktrk = DerivationFramework__Reco_mumu(
    name                   = "EOPRecotrktrk",
    JpsiFinder             = EOPJpsiFinder,
    OutputVtxContainerName = "LambdaCandidates",
    PVContainerName        = "PrimaryVertices",
    RefPVContainerName     = "EOPLambdaRefittedPrimaryVertices",
    RefitPV = EOPRefitPV)
ToolSvc += EOPRecotrktrk

#--------------------------------------------------------------------
## 4/ setup the vertex selection and augmentation tool(s). These tools decorate the vertices with
##    variables that depend on the vertex mass hypothesis, e.g. invariant mass, proper decay time, etc.
##    Property HypothesisName is used as a prefix for these decorations.
##    They also perform tighter selection, flagging the vertecis that passed. The flag is a Char_t branch
##    named "passed_"+HypothesisName. It is used later by the "SelectEvent" and "Thin_vtxTrk" tools
##    to determine which events and candidates should be kept in the output stream.
##    Multiple instances of the Select_* tools can be used on a single input collection as long as they
##    use different "HypothesisName" flags.

from DerivationFrameworkEoverP.DerivationFrameworkEoverPConf import DerivationFramework__Select_onia2mumu
EOPSelectLambda2trktrk = DerivationFramework__Select_onia2mumu(
    name                  = "EOPSelectLambda2trktrk",
    HypothesisName        = "Lambda",
    InputVtxContainerName = EOPRecotrktrk.OutputVtxContainerName,
    TrkMasses             = [938.272, 139.57], # Proton, pion PDG mass
    VtxMassHypo           = 1115.0, # lambda PDG mass
    MassMin               = 1105.0,
    MassMax               = 1125.0,
    Chi2Max               = 15)
ToolSvc += EOPSelectLambda2trktrk

#====================================================================
# CREATE THE DERIVATION KERNEL ALGORITHM AND PASS THE ABOVE TOOLS
#====================================================================
from DerivationFrameworkCore.DerivationFrameworkCoreConf import DerivationFramework__DerivationKernel
DerivationFrameworkJob += CfgMgr.DerivationFramework__DerivationKernel("TrackCaloDecorator_KERN", AugmentationTools = [CaloDeco])
topSequence += DerivationFrameworkJob
EOPStream.AcceptAlgs(["TrackCaloDecorator_KERN"])

#====================================================================
# CONTENT DEFINITION
#====================================================================
ToolSvc += CfgMgr.xAODMaker__TriggerMenuMetaDataTool(
        "TriggerMenuMetaDataTool" )
svcMgr.MetaDataSvc.MetaDataTools += [ ToolSvc.TriggerMenuMetaDataTool ]

excludedAuxData='-caloExtension.-cellAssociation.-trackParameterCovarianceMatrices.-parameterX.-parameterY\
.-parameterZ.-parameterPX.-parameterPY.-parameterPZ.-parameterPosition'

# Add generic event information
EOPStream.AddItem("xAOD::EventInfo#*")
EOPStream.AddItem("xAOD::EventAuxInfo#*")

# Add event shape information
EOPStream.AddItem("xAOD::HIEventShapeContainer#*")
EOPStream.AddItem("xAOD::HIEventShapeAuxContainer#*")

# Add truth-related information
EOPStream.AddItem("xAOD::TruthParticleContainer#TruthParticles")
EOPStream.AddItem("xAOD::TruthParticleAuxContainer#TruthParticlesAux.")
EOPStream.AddItem("xAOD::TruthVertexContainer#*")
EOPStream.AddItem("xAOD::TruthVertexAuxContainer#*")
EOPStream.AddItem("xAOD::TruthEventContainer#*")
EOPStream.AddItem("xAOD::TruthEventAuxContainer#*")

# # Add cells and clusters ## Unnecessary - takes up way too much space
# EOPStream.AddItem( "xAOD::CaloClusterContainer#*" )
# EOPStream.AddItem( "xAOD::CaloClusterAuxContainer#*" )
# # EOPStream.AddItem( "xAOD::CaloClusterAuxContainer#CaloCalTopoCluster" )
# EOPStream.AddItem( "CaloCellContainer#*")
# EOPStream.AddItem( "CaloCompactCellContainer#*");

# Add track particles
EOPStream.AddItem("xAOD::TrackParticleContainer#InDetTrackParticles")
EOPStream.AddItem("xAOD::TrackParticleAuxContainer#InDetTrackParticlesAux."+excludedAuxData)

# Add vertices
EOPStream.AddItem("xAOD::VertexContainer#PrimaryVertices")
EOPStream.AddItem("xAOD::VertexAuxContainer#PrimaryVerticesAux.-vxTrackAtVertex")

# Add secondary vertices
#EOPStream.AddItem("xAOD::VertexContainer#LambdaCandidates")
#EOPStream.AddItem("xAOD::VertexAuxContainer#LambdaCandidatesAux.")
#EOPStream.AddItem("xAOD::VertexAuxContainer#LambdaCandidatesAux.-vxTrackAtVertex")

#EOPStream.AddItem("xAOD::VertexContainer#V0LambdaVertices")
#EOPStream.AddItem("xAOD::VertexAuxContainer#V0LambdaVerticesAux.")
#EOPStream.AddItem("xAOD::VertexAuxContainer#V0LambdaVerticesAux.-vxTrackAtVertex")

# Add trigger
EOPStream.AddMetaDataItem("xAOD::TriggerMenuContainer#TriggerMenu")
EOPStream.AddMetaDataItem("xAOD::TriggerMenuAuxContainer#TriggerMenuAux.")

EOPStream.AddItem("xAOD::TrigDecision#xTrigDecision")
EOPStream.AddItem("xAOD::TrigDecisionAuxInfo#xTrigDecisionAux.")
EOPStream.AddItem("xAOD::TrigConfKeys#TrigConfKeys")

