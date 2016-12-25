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

#====================================================================
# SET UP STREAM
#====================================================================
from OutputStreamAthenaPool.MultipleStreamManager import MSMgr
from D2PDMaker.D2PDHelpers import buildFileName
from PrimaryDPDMaker.PrimaryDPDFlags import primDPD
streamName = primDPD.WriteDAOD_EOP.StreamName
fileName   = buildFileName( primDPD.WriteDAOD_EOP )
EOPStream = MSMgr.NewPoolRootStream( streamName, fileName )

# Save cutflow histograms
doCutflow = True
if doCutflow:
    from GaudiSvc.GaudiSvcConf import THistSvc
    ServiceMgr += THistSvc()
    svcMgr.THistSvc.Output += ["CutflowStream DATAFILE='cutflow.root' OPT='RECREATE'"]

#====================================================================
# AUGMENTATION TOOL
#====================================================================
from DerivationFrameworkEoverP.DerivationFrameworkEoverPConf import DerivationFramework__TrackCaloDecorator
CaloDeco = DerivationFramework__TrackCaloDecorator(name = "TrackCaloDecorator",
                                                   TrackContainer = "InDetTrackParticles",
                                                   CaloClusterContainer = "CaloCalTopoClusters",
                                                   DecorationPrefix = "CALO",
                                                   DoCutflow = doCutflow,
                                                   OutputLevel = DEBUG)
ToolSvc += CaloDeco

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

# Add trigger
EOPStream.AddMetaDataItem("xAOD::TriggerMenuContainer#TriggerMenu")
EOPStream.AddMetaDataItem("xAOD::TriggerMenuAuxContainer#TriggerMenuAux.")

EOPStream.AddItem("xAOD::TrigDecision#xTrigDecision")
EOPStream.AddItem("xAOD::TrigDecisionAuxInfo#xTrigDecisionAux.")
EOPStream.AddItem("xAOD::TrigConfKeys#TrigConfKeys")

